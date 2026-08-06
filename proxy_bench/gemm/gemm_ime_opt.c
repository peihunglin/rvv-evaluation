/*
 * gemm_ime_opt.c
 *
 * SpaceMiT A100 IME FP16xFP16->FP32 GEMM.
 *
 * Optimization direction:
 *   - derived from the faster tiny-tile baseline, not the full-K packed version
 *   - 16x32 output register block (rm=2, rn=4):
 *       two A tiles x four B tiles -> eight 8x8 C accumulators
 *   - small K blocking (IME_KBLOCK_TILES, default 8)
 *   - A/B are packed into small, immediately-consumed stack buffers
 *   - eight FP32 C accumulators remain in IME/RVV registers across each K block
 *   - persistent OpenMP parallel region
 */

#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#define TILE 8

#ifndef IME_KBLOCK_TILES
#define IME_KBLOCK_TILES 8
#endif

#if IME_KBLOCK_TILES < 1
#error "IME_KBLOCK_TILES must be positive"
#endif

#define A_IDX(i,j,n) ((i)*(n)+(j))
#define B_IDX(i,j,n) ((i)*(n)+(j))
#define C_IDX(i,j,n) ((i)*(n)+(j))

typedef _Float16 fp16_t;

static inline fp16_t init_a(long k)
{
  return (fp16_t)((int)(k % 13) - 6);
}

static inline fp16_t init_b(long k)
{
  return (fp16_t)((int)(k % 7) - 3);
}

static double reference_checksum(long n, int iterations)
{
  double dot = 0.0;
  for (long k=0; k<n; ++k)
    dot += (double)init_a(k) * (double)init_b(k);
  return dot * (double)n * (double)n * (double)(iterations + 1);
}

static inline void pack_a_kblock(const fp16_t * RESTRICT A,
                                 fp16_t * RESTRICT ap,
                                 long n, long row, long k0, int nt)
{
  for (int t=0; t<nt; ++t) {
    const long kb = k0 + (long)t*TILE;
    for (int ii=0; ii<TILE; ++ii)
      for (int kk=0; kk<TILE; ++kk)
        ap[(size_t)t*TILE*TILE + ii*TILE + kk] =
          A[A_IDX(row+ii, kb+kk, n)];
  }
}

static inline void pack_b_kblock(const fp16_t * RESTRICT B,
                                 fp16_t * RESTRICT bp,
                                 long n, long col, long k0, int nt)
{
  for (int t=0; t<nt; ++t) {
    const long kb = k0 + (long)t*TILE;
    for (int jj=0; jj<TILE; ++jj)
      for (int kk=0; kk<TILE; ++kk)
        bp[(size_t)t*TILE*TILE + jj*TILE + kk] =
          B[B_IDX(kb+kk, col+jj, n)];
  }
}

static inline void load_c_8x8(const float * RESTRICT C,
                               float * RESTRICT ct,
                               long n, long row, long col)
{
  for (int ii=0; ii<TILE; ++ii)
    for (int jj=0; jj<TILE; ++jj)
      ct[ii*TILE+jj] = C[C_IDX(row+ii,col+jj,n)];
}

static inline void store_c_8x8(float * RESTRICT C,
                                const float * RESTRICT ct,
                                long n, long row, long col)
{
  for (int ii=0; ii<TILE; ++ii)
    for (int jj=0; jj<TILE; ++jj)
      C[C_IDX(row+ii,col+jj,n)] = ct[ii*TILE+jj];
}

static inline void scalar_macc_8x16_kblock(const fp16_t * RESTRICT ap,
                                           const fp16_t * RESTRICT bp0,
                                           const fp16_t * RESTRICT bp1,
                                           float * RESTRICT c0,
                                           float * RESTRICT c1,
                                           int nt)
{
  for (int t=0; t<nt; ++t) {
    const fp16_t *a  = ap  + (size_t)t*TILE*TILE;
    const fp16_t *b0 = bp0 + (size_t)t*TILE*TILE;
    const fp16_t *b1 = bp1 + (size_t)t*TILE*TILE;
    for (int kk=0; kk<TILE; ++kk)
      for (int jj=0; jj<TILE; ++jj)
        for (int ii=0; ii<TILE; ++ii) {
          const float av = (float)a[ii*TILE+kk];
          c0[ii*TILE+jj] += av * (float)b0[jj*TILE+kk];
          c1[ii*TILE+jj] += av * (float)b1[jj*TILE+kk];
        }
  }
}

static inline void ime_macc_8x16_kblock(const fp16_t * RESTRICT ap,
                                        const fp16_t * RESTRICT bp0,
                                        const fp16_t * RESTRICT bp1,
                                        float * RESTRICT c0,
                                        float * RESTRICT c1,
                                        int nt)
{
#if defined(USE_IME_ASM) && defined(__riscv)
  if (nt <= 0) return;
  __asm__ volatile(
      "vsetvli        t0, zero, e32, m2        \n\t"
      "vle32.v        v16, (%[c0])             \n\t"
      "vle32.v        v18, (%[c1])             \n\t"
      "mv             t1, %[nt]                \n\t"
      "mv             t2, %[ap]                \n\t"
      "mv             t3, %[bp0]               \n\t"
      "mv             t4, %[bp1]               \n\t"
      "vsetvli        t0, zero, e16, m1        \n\t"
      "1:                                       \n\t"
      "vle16.v        v8,  (t2)                \n\t"
      "vle16.v        v10, (t3)                \n\t"
      "vle16.v        v12, (t4)                \n\t"
      "smt.vfwmadot   v16, v8, v10             \n\t"
      "smt.vfwmadot   v18, v8, v12             \n\t"
      "addi           t2, t2, 128              \n\t"
      "addi           t3, t3, 128              \n\t"
      "addi           t4, t4, 128              \n\t"
      "addi           t1, t1, -1               \n\t"
      "bnez           t1, 1b                   \n\t"
      "vsetvli        t0, zero, e32, m2        \n\t"
      "vse32.v        v16, (%[c0])             \n\t"
      "vse32.v        v18, (%[c1])             \n\t"
      :
      : [ap] "r"(ap), [bp0] "r"(bp0), [bp1] "r"(bp1),
        [c0] "r"(c0), [c1] "r"(c1), [nt] "r"((long)nt)
      : "memory", "t0", "t1", "t2", "t3", "t4",
        "v8", "v10", "v12", "v16", "v17", "v18", "v19");
#else
  scalar_macc_8x16_kblock(ap,bp0,bp1,c0,c1,nt);
#endif
}

static inline void scalar_macc_8x8_kblock(const fp16_t * RESTRICT ap,
                                          const fp16_t * RESTRICT bp,
                                          float * RESTRICT c,
                                          int nt)
{
  for (int t=0; t<nt; ++t) {
    const fp16_t *a = ap + (size_t)t*TILE*TILE;
    const fp16_t *b = bp + (size_t)t*TILE*TILE;
    for (int kk=0; kk<TILE; ++kk)
      for (int jj=0; jj<TILE; ++jj)
        for (int ii=0; ii<TILE; ++ii)
          c[ii*TILE+jj] += (float)a[ii*TILE+kk] * (float)b[jj*TILE+kk];
  }
}

static inline void ime_macc_8x8_kblock(const fp16_t * RESTRICT ap,
                                       const fp16_t * RESTRICT bp,
                                       float * RESTRICT c,
                                       int nt)
{
#if defined(USE_IME_ASM) && defined(__riscv)
  if (nt <= 0) return;
  __asm__ volatile(
      "vsetvli        t0, zero, e32, m2        \n\t"
      "vle32.v        v16, (%[c])              \n\t"
      "mv             t1, %[nt]                \n\t"
      "mv             t2, %[ap]                \n\t"
      "mv             t3, %[bp]                \n\t"
      "vsetvli        t0, zero, e16, m1        \n\t"
      "1:                                       \n\t"
      "vle16.v        v8,  (t2)                \n\t"
      "vle16.v        v10, (t3)                \n\t"
      "smt.vfwmadot   v16, v8, v10             \n\t"
      "addi           t2, t2, 128              \n\t"
      "addi           t3, t3, 128              \n\t"
      "addi           t1, t1, -1               \n\t"
      "bnez           t1, 1b                   \n\t"
      "vsetvli        t0, zero, e32, m2        \n\t"
      "vse32.v        v16, (%[c])              \n\t"
      :
      : [ap] "r"(ap), [bp] "r"(bp), [c] "r"(c), [nt] "r"((long)nt)
      : "memory", "t0", "t1", "t2", "t3", "v8", "v10", "v16", "v17");
#else
  scalar_macc_8x8_kblock(ap,bp,c,nt);
#endif
}

static inline void k_scalar_tail_8x16(const fp16_t * RESTRICT A,
                                      const fp16_t * RESTRICT B,
                                      float * RESTRICT c0,
                                      float * RESTRICT c1,
                                      long n, long row, long col, long k0)
{
  for (long k=k0; k<n; ++k) {
    for (int ii=0; ii<TILE; ++ii) {
      const float a = (float)A[A_IDX(row+ii,k,n)];
      for (int jj=0; jj<TILE; ++jj) {
        c0[ii*TILE+jj] += a * (float)B[B_IDX(k,col+jj,n)];
        c1[ii*TILE+jj] += a * (float)B[B_IDX(k,col+TILE+jj,n)];
      }
    }
  }
}

static inline void k_scalar_tail_8x8(const fp16_t * RESTRICT A,
                                     const fp16_t * RESTRICT B,
                                     float * RESTRICT c,
                                     long n, long row, long col, long k0)
{
  for (long k=k0; k<n; ++k)
    for (int ii=0; ii<TILE; ++ii) {
      const float a = (float)A[A_IDX(row+ii,k,n)];
      for (int jj=0; jj<TILE; ++jj)
        c[ii*TILE+jj] += a * (float)B[B_IDX(k,col+jj,n)];
    }
}


/*
 * K3 IME 2.0 register-blocked microkernel: rm=2, rn=4.
 *
 * C register groups (e32,m2):
 *   row block 0: v16, v18, v20, v22
 *   row block 1: v24, v26, v28, v30
 *
 * Sources (e16,m1):
 *   A0=v8, A1=v9, B0=v10, B1=v11, B2=v12, B3=v13
 *
 * For each K tile we perform 8 smt.vfwmadot operations from only
 * 2 A loads + 4 B loads.  This is the reuse shape described by SpaceMiT
 * for high IME MAC utilization.
 */
static inline void
ime_macc_16x32_kblock(const fp16_t * RESTRICT ap0,
                      const fp16_t * RESTRICT ap1,
                      const fp16_t * RESTRICT bp0,
                      const fp16_t * RESTRICT bp1,
                      const fp16_t * RESTRICT bp2,
                      const fp16_t * RESTRICT bp3,
                      float * RESTRICT c00,
                      float * RESTRICT c01,
                      float * RESTRICT c02,
                      float * RESTRICT c03,
                      float * RESTRICT c10,
                      float * RESTRICT c11,
                      float * RESTRICT c12,
                      float * RESTRICT c13,
                      int nt)
{
#if defined(USE_IME_ASM) && defined(__riscv)
  if (nt <= 0) return;

  __asm__ volatile(
      "vsetvli        t0, zero, e32, m2        \n\t"
      "vle32.v        v16, (%[c00])            \n\t"
      "vle32.v        v18, (%[c01])            \n\t"
      "vle32.v        v20, (%[c02])            \n\t"
      "vle32.v        v22, (%[c03])            \n\t"
      "vle32.v        v24, (%[c10])            \n\t"
      "vle32.v        v26, (%[c11])            \n\t"
      "vle32.v        v28, (%[c12])            \n\t"
      "vle32.v        v30, (%[c13])            \n\t"

      "mv             t1, %[nt]                \n\t"
      "mv             t2, %[ap0]               \n\t"
      "mv             t3, %[ap1]               \n\t"
      "mv             t4, %[bp0]               \n\t"
      "mv             t5, %[bp1]               \n\t"
      "mv             t6, %[bp2]               \n\t"
      "mv             a0, %[bp3]               \n\t"

      "vsetvli        t0, zero, e16, m1        \n\t"
      "1:                                       \n\t"
      "vle16.v        v8,  (t2)                \n\t"
      "vle16.v        v9,  (t3)                \n\t"
      "vle16.v        v10, (t4)                \n\t"
      "vle16.v        v11, (t5)                \n\t"
      "vle16.v        v12, (t6)                \n\t"
      "vle16.v        v13, (a0)                \n\t"

      "smt.vfwmadot   v16, v8,  v10            \n\t"
      "smt.vfwmadot   v18, v8,  v11            \n\t"
      "smt.vfwmadot   v20, v8,  v12            \n\t"
      "smt.vfwmadot   v22, v8,  v13            \n\t"
      "smt.vfwmadot   v24, v9,  v10            \n\t"
      "smt.vfwmadot   v26, v9,  v11            \n\t"
      "smt.vfwmadot   v28, v9,  v12            \n\t"
      "smt.vfwmadot   v30, v9,  v13            \n\t"

      "addi           t2, t2, 128              \n\t"
      "addi           t3, t3, 128              \n\t"
      "addi           t4, t4, 128              \n\t"
      "addi           t5, t5, 128              \n\t"
      "addi           t6, t6, 128              \n\t"
      "addi           a0, a0, 128              \n\t"
      "addi           t1, t1, -1               \n\t"
      "bnez           t1, 1b                   \n\t"

      "vsetvli        t0, zero, e32, m2        \n\t"
      "vse32.v        v16, (%[c00])            \n\t"
      "vse32.v        v18, (%[c01])            \n\t"
      "vse32.v        v20, (%[c02])            \n\t"
      "vse32.v        v22, (%[c03])            \n\t"
      "vse32.v        v24, (%[c10])            \n\t"
      "vse32.v        v26, (%[c11])            \n\t"
      "vse32.v        v28, (%[c12])            \n\t"
      "vse32.v        v30, (%[c13])            \n\t"
      :
      : [ap0] "r"(ap0), [ap1] "r"(ap1),
        [bp0] "r"(bp0), [bp1] "r"(bp1), [bp2] "r"(bp2), [bp3] "r"(bp3),
        [c00] "r"(c00), [c01] "r"(c01), [c02] "r"(c02), [c03] "r"(c03),
        [c10] "r"(c10), [c11] "r"(c11), [c12] "r"(c12), [c13] "r"(c13),
        [nt] "r"((long)nt)
      : "memory", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0",
        "v8", "v9", "v10", "v11", "v12", "v13",
        "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
        "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");
#else
  float *cs[2][4]={{c00,c01,c02,c03},{c10,c11,c12,c13}};
  const fp16_t *as[2]={ap0,ap1};
  const fp16_t *bs[4]={bp0,bp1,bp2,bp3};
  for(int t=0;t<nt;++t)
    for(int rb=0;rb<2;++rb)
      for(int cb=0;cb<4;++cb) {
        const fp16_t *a=as[rb]+(size_t)t*TILE*TILE;
        const fp16_t *b=bs[cb]+(size_t)t*TILE*TILE;
        float *c=cs[rb][cb];
        for(int kk=0;kk<TILE;++kk)
          for(int jj=0;jj<TILE;++jj)
            for(int ii=0;ii<TILE;++ii)
              c[ii*TILE+jj]+=(float)a[ii*TILE+kk]*(float)b[jj*TILE+kk];
      }
#endif
}

static void
ime_tile_16x32(const fp16_t * RESTRICT A,
               const fp16_t * RESTRICT B,
               float * RESTRICT C,
               long n,long row,long col)
{
  fp16_t ap0[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t ap1[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t bp0[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t bp1[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t bp2[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t bp3[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));

  float c00[64] __attribute__((aligned(64))), c01[64] __attribute__((aligned(64)));
  float c02[64] __attribute__((aligned(64))), c03[64] __attribute__((aligned(64)));
  float c10[64] __attribute__((aligned(64))), c11[64] __attribute__((aligned(64)));
  float c12[64] __attribute__((aligned(64))), c13[64] __attribute__((aligned(64)));

  load_c_8x8(C,c00,n,row,       col);
  load_c_8x8(C,c01,n,row,       col+8);
  load_c_8x8(C,c02,n,row,       col+16);
  load_c_8x8(C,c03,n,row,       col+24);
  load_c_8x8(C,c10,n,row+8,     col);
  load_c_8x8(C,c11,n,row+8,     col+8);
  load_c_8x8(C,c12,n,row+8,     col+16);
  load_c_8x8(C,c13,n,row+8,     col+24);

  const long kfull=(n/TILE)*TILE;
  long k0=0;
  while(k0<kfull) {
    const long remain_tiles=(kfull-k0)/TILE;
    const int nt=(int)MIN((long)IME_KBLOCK_TILES,remain_tiles);

    pack_a_kblock(A,ap0,n,row,   k0,nt);
    pack_a_kblock(A,ap1,n,row+8, k0,nt);
    pack_b_kblock(B,bp0,n,col,    k0,nt);
    pack_b_kblock(B,bp1,n,col+8,  k0,nt);
    pack_b_kblock(B,bp2,n,col+16, k0,nt);
    pack_b_kblock(B,bp3,n,col+24, k0,nt);

    ime_macc_16x32_kblock(ap0,ap1,bp0,bp1,bp2,bp3,
                          c00,c01,c02,c03,c10,c11,c12,c13,nt);
    k0 += (long)nt*TILE;
  }

  if(kfull<n) {
    float *cs[2][4]={{c00,c01,c02,c03},{c10,c11,c12,c13}};
    for(long k=kfull;k<n;++k)
      for(int rb=0;rb<2;++rb)
        for(int ii=0;ii<TILE;++ii) {
          const float av=(float)A[A_IDX(row+rb*TILE+ii,k,n)];
          for(int cb=0;cb<4;++cb)
            for(int jj=0;jj<TILE;++jj)
              cs[rb][cb][ii*TILE+jj] += av*(float)B[B_IDX(k,col+cb*TILE+jj,n)];
        }
  }

  store_c_8x8(C,c00,n,row,   col);
  store_c_8x8(C,c01,n,row,   col+8);
  store_c_8x8(C,c02,n,row,   col+16);
  store_c_8x8(C,c03,n,row,   col+24);
  store_c_8x8(C,c10,n,row+8, col);
  store_c_8x8(C,c11,n,row+8, col+8);
  store_c_8x8(C,c12,n,row+8, col+16);
  store_c_8x8(C,c13,n,row+8, col+24);
}

static void ime_tile_8x16(const fp16_t * RESTRICT A,
                          const fp16_t * RESTRICT B,
                          float * RESTRICT C,
                          long n, long row, long col)
{
  fp16_t ap [IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t bp0[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t bp1[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  float c0[TILE*TILE] __attribute__((aligned(64)));
  float c1[TILE*TILE] __attribute__((aligned(64)));

  load_c_8x8(C,c0,n,row,col);
  load_c_8x8(C,c1,n,row,col+TILE);

  const long kfull = (n/TILE)*TILE;
  long k0 = 0;
  while (k0 < kfull) {
    const long remain_tiles = (kfull-k0)/TILE;
    const int nt = (int)MIN((long)IME_KBLOCK_TILES,remain_tiles);
    pack_a_kblock(A,ap,n,row,k0,nt);
    pack_b_kblock(B,bp0,n,col,k0,nt);
    pack_b_kblock(B,bp1,n,col+TILE,k0,nt);
    ime_macc_8x16_kblock(ap,bp0,bp1,c0,c1,nt);
    k0 += (long)nt*TILE;
  }

  if (kfull<n)
    k_scalar_tail_8x16(A,B,c0,c1,n,row,col,kfull);

  store_c_8x8(C,c0,n,row,col);
  store_c_8x8(C,c1,n,row,col+TILE);
}

static void ime_tile_8x8(const fp16_t * RESTRICT A,
                         const fp16_t * RESTRICT B,
                         float * RESTRICT C,
                         long n, long row, long col)
{
  fp16_t ap[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  fp16_t bp[IME_KBLOCK_TILES*TILE*TILE] __attribute__((aligned(64)));
  float ct[TILE*TILE] __attribute__((aligned(64)));

  load_c_8x8(C,ct,n,row,col);
  const long kfull=(n/TILE)*TILE;
  long k0=0;
  while(k0<kfull) {
    const long remain_tiles=(kfull-k0)/TILE;
    const int nt=(int)MIN((long)IME_KBLOCK_TILES,remain_tiles);
    pack_a_kblock(A,ap,n,row,k0,nt);
    pack_b_kblock(B,bp,n,col,k0,nt);
    ime_macc_8x8_kblock(ap,bp,ct,nt);
    k0 += (long)nt*TILE;
  }
  if(kfull<n)
    k_scalar_tail_8x8(A,B,ct,n,row,col,kfull);
  store_c_8x8(C,ct,n,row,col);
}

static void scalar_edge(const fp16_t * RESTRICT A,
                        const fp16_t * RESTRICT B,
                        float * RESTRICT C,
                        long n,long row,long col,long rows,long cols)
{
  for(long k=0;k<n;++k)
    for(long i=0;i<rows;++i) {
      const float a=(float)A[A_IDX(row+i,k,n)];
      for(long j=0;j<cols;++j)
        C[C_IDX(row+i,col+j,n)] += a*(float)B[B_IDX(k,col+j,n)];
    }
}

int main(int argc,char **argv)
{
  int iterations,nthread_input,nthread=0,shortcut,num_error=0;
  long order;
  double gemm_time=0.0,avgtime,checksum=0.0,ref_checksum;
  const double epsilon=1.e-3;
  fp16_t * RESTRICT A,* RESTRICT B;
  float * RESTRICT C;

  printf("Parallel Research Kernels version %s\n",PRKVERSION);
  printf("OpenMP optimized FP16 IME GEMM - SpaceMiT A100\n");

  if(argc!=4 && argc!=5) {
    printf("Usage: %s <# threads> <# iterations> <matrix order> [tile size]\n",*argv);
    exit(EXIT_FAILURE);
  }

  nthread_input=atoi(*++argv);
  if(nthread_input<1 || nthread_input>MAX_THREADS) exit(EXIT_FAILURE);
  omp_set_num_threads(nthread_input);

  iterations=atoi(*++argv);
  if(iterations<1) exit(EXIT_FAILURE);

  order=atol(*++argv);
  shortcut=(order<0);
  if(shortcut) order=-order;
  if(order<1) exit(EXIT_FAILURE);

  if(argc==5) {
    const int ignored=atoi(*++argv);
    if(ignored!=TILE)
      printf("WARNING: IME microtile is fixed at 8; argument %d ignored\n",ignored);
  }

  A=(fp16_t*)prk_malloc(order*order*sizeof(fp16_t));
  B=(fp16_t*)prk_malloc(order*order*sizeof(fp16_t));
  C=(float*)prk_malloc(order*order*sizeof(float));
  if(!A||!B||!C) exit(EXIT_FAILURE);

  ref_checksum=reference_checksum(order,iterations);

  #pragma omp parallel for schedule(static)
  for(long i=0;i<order;++i)
    for(long j=0;j<order;++j) {
      A[A_IDX(i,j,order)]=init_a(j);
      B[B_IDX(i,j,order)]=init_b(i);
      C[C_IDX(i,j,order)]=0.0f;
    }

  #pragma omp parallel
  {
    #pragma omp master
    {
      nthread=omp_get_num_threads();
      if(nthread!=nthread_input) num_error=1;
      printf("Matrix order          = %ld\n",order);
      printf("Number of threads     = %d\n",nthread_input);
      printf("IME microkernel       = 16x32 output (rm=2,rn=4), 8x8x8 primitive\n");
      printf("IME K-block tiles     = %d\n",IME_KBLOCK_TILES);
      printf("Register blocking     = 2 A tiles x 4 B tiles x 8 C accumulators\n");
      printf("Packed working set    = small immediate-use K block\n");
      printf("Number of iterations  = %d\n",iterations);
    }
  }

  bail_out(num_error);
  if(shortcut) {
    prk_free(A); prk_free(B); prk_free(C);
    return 0;
  }

  #pragma omp parallel
  {
    for(int iter=0;iter<=iterations;++iter) {
      if(iter==1) {
        #pragma omp barrier
        #pragma omp master
        gemm_time=wtime();
      }

      #pragma omp for collapse(2) schedule(static)
      for(long col=0;col<order;col+=4*TILE) {
        for(long row=0;row<order;row+=2*TILE) {
          const long rows=MIN((long)(2*TILE),order-row);
          const long cols=MIN((long)(4*TILE),order-col);

          if(rows==2*TILE && cols==4*TILE) {
            ime_tile_16x32(A,B,C,order,row,col);
          } else {
            /* Edge path reuses the proven 8x16 / 8x8 kernels. */
            for(long rr=0; rr<rows; rr+=TILE) {
              const long rrem=MIN((long)TILE,rows-rr);
              for(long cc=0; cc<cols; ) {
                const long crem=cols-cc;
                if(rrem==TILE && crem>=2*TILE) {
                  ime_tile_8x16(A,B,C,order,row+rr,col+cc);
                  cc += 2*TILE;
                } else if(rrem==TILE && crem>=TILE) {
                  ime_tile_8x8(A,B,C,order,row+rr,col+cc);
                  cc += TILE;
                } else {
                  const long cnow=MIN((long)TILE,crem);
                  scalar_edge(A,B,C,order,row+rr,col+cc,rrem,cnow);
                  cc += cnow;
                }
              }
            }
          }
        }
      }
    }

    #pragma omp barrier
    #pragma omp master
    gemm_time=wtime()-gemm_time;
  }

  for(long i=0;i<order;++i)
    for(long j=0;j<order;++j)
      checksum += C[C_IDX(i,j,order)];

  const double err=ABS(checksum-ref_checksum);
  if((ref_checksum==0.0 && err>epsilon) ||
     (ref_checksum!=0.0 && err/ABS(ref_checksum)>epsilon)) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n",checksum,ref_checksum);
    exit(EXIT_FAILURE);
  }

  printf("Solution validates\n");
  const double nflops=2.0*(double)order*(double)order*(double)order;
  avgtime=gemm_time/iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",1.e-6*nflops/avgtime,avgtime);

  prk_free(A); prk_free(B); prk_free(C);
  return 0;
}
