/*
 * gemm_hetero_rvv_kernel.c
 *
 * Packed/register-blocked RVV worker for K3 heterogeneous RVV+RVV GEMM.
 * This source is compiled twice:
 *
 *   X100: MC=32 NC=64 NR=8, e{32,64}m1
 *   A100: MC=64 NC=64 NR=4, e{32,64}m1
 *
 * KC is passed at runtime (recommended FP64 KC=64).
 *
 * Each worker owns disjoint NC column panels in [col_begin,col_end),
 * with private packed A/B buffers. C stays resident in NR vector
 * accumulators for an entire KC panel. K is unrolled by 2.
 */

/*
 * Safe architecture-specific defaults.
 *
 * The A100 object should be compiled with -DHETERO_RVV_A100=1.
 * The X100 object uses the default path.
 *
 * Any individual value can still be overridden explicitly with
 * -DHETERO_RVV_MC=..., -DHETERO_RVV_NC=..., -DHETERO_RVV_NR=...
 */
#ifndef HETERO_RVV_MC
  #ifdef HETERO_RVV_A100
    #define HETERO_RVV_MC 64
  #else
    #define HETERO_RVV_MC 32
  #endif
#endif

#ifndef HETERO_RVV_NC
  #define HETERO_RVV_NC 64
#endif

#ifndef HETERO_RVV_NR
  #ifdef HETERO_RVV_A100
    #define HETERO_RVV_NR 4
  #else
    #define HETERO_RVV_NR 8
  #endif
#endif

#if (HETERO_RVV_NR != 4) && (HETERO_RVV_NR != 6) && (HETERO_RVV_NR != 8)
#error "HETERO_RVV_NR must be 4, 6, or 8"
#endif

#ifndef KERNEL_NAME
#error "KERNEL_NAME must be defined"
#endif

#define MC HETERO_RVV_MC
#define NC HETERO_RVV_NC
#define NR HETERO_RVV_NR

#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#if defined(__riscv)
#include <riscv_vector.h>
#endif

#ifndef MTYPE
#define MTYPE double
#endif







#define A_IDX(i,j,n) ((i) + (n)*(j))
#define B_IDX(i,j,n) ((i) + (n)*(j))
#define C_IDX(i,j,n) ((i) + (n)*(j))

/* Ap layout: Ap[k*mc + i], i is contiguous.
 * Bp layout: Bp[k*nc + j], j is contiguous.
 */
static inline void
pack_a_panel(const MTYPE * RESTRICT A,
             MTYPE * RESTRICT Ap,
             long n, long ic, long pc,
             int mc, int kc)
{
  for (int k=0; k<kc; ++k) {
    const MTYPE * RESTRICT src = &A[A_IDX(ic,pc+k,n)];
    MTYPE * RESTRICT dst = &Ap[(size_t)k*(size_t)mc];
    #pragma omp simd
    for (int i=0; i<mc; ++i)
      dst[i] = src[i];
  }
}

static inline void
pack_b_panel(const MTYPE * RESTRICT B,
             MTYPE * RESTRICT Bp,
             long n, long jc, long pc,
             int nc, int kc)
{
  for (int k=0; k<kc; ++k) {
    MTYPE * RESTRICT dst = &Bp[(size_t)k*(size_t)nc];
    /*
     * B is column-major, so B(pc+k,jc+j) is strided over j.
     * Packing pays that cost once; the microkernel then sees contiguous
     * scalar operands bp[0..NR-1] for every K step.
     */
    for (int j=0; j<nc; ++j)
      dst[j] = B[B_IDX(pc+k,jc+j,n)];
  }
}

#if defined(__riscv) && defined(MTYPE_IS_DOUBLE)

/* Full NR microkernel, FP64, e64,m1, K-unroll=2. */
static inline void
rvv_kernel_nr_f64(const MTYPE * RESTRICT Ap,
                  const MTYPE * RESTRICT Bp,
                  MTYPE * RESTRICT C,
                  long n, long ic, long jc,
                  int ir, int jr,
                  int mc, int nc, int kc)
{
  int i = ir;
  while (i < mc) {
    size_t vl = __riscv_vsetvl_e64m1((size_t)(mc-i));

    vfloat64m1_t c0 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+0,n)],vl);
    vfloat64m1_t c1 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+1,n)],vl);
    vfloat64m1_t c2 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+2,n)],vl);
    vfloat64m1_t c3 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+3,n)],vl);
#if NR >= 6
    vfloat64m1_t c4 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+4,n)],vl);
    vfloat64m1_t c5 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+5,n)],vl);
#endif
#if NR >= 8
    vfloat64m1_t c6 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+6,n)],vl);
    vfloat64m1_t c7 = __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+7,n)],vl);
#endif

    int k=0;
    for (; k+1<kc; k+=2) {
      const MTYPE * RESTRICT bp0 = &Bp[(size_t)k*(size_t)nc + jr];
      const MTYPE * RESTRICT bp1 = bp0 + nc;

      vfloat64m1_t a0 =
        __riscv_vle64_v_f64m1(&Ap[(size_t)k*(size_t)mc + i],vl);
      vfloat64m1_t a1 =
        __riscv_vle64_v_f64m1(&Ap[(size_t)(k+1)*(size_t)mc + i],vl);

      c0=__riscv_vfmacc_vf_f64m1(c0,bp0[0],a0,vl);
      c1=__riscv_vfmacc_vf_f64m1(c1,bp0[1],a0,vl);
      c2=__riscv_vfmacc_vf_f64m1(c2,bp0[2],a0,vl);
      c3=__riscv_vfmacc_vf_f64m1(c3,bp0[3],a0,vl);
#if NR >= 6
      c4=__riscv_vfmacc_vf_f64m1(c4,bp0[4],a0,vl);
      c5=__riscv_vfmacc_vf_f64m1(c5,bp0[5],a0,vl);
#endif
#if NR >= 8
      c6=__riscv_vfmacc_vf_f64m1(c6,bp0[6],a0,vl);
      c7=__riscv_vfmacc_vf_f64m1(c7,bp0[7],a0,vl);
#endif

      c0=__riscv_vfmacc_vf_f64m1(c0,bp1[0],a1,vl);
      c1=__riscv_vfmacc_vf_f64m1(c1,bp1[1],a1,vl);
      c2=__riscv_vfmacc_vf_f64m1(c2,bp1[2],a1,vl);
      c3=__riscv_vfmacc_vf_f64m1(c3,bp1[3],a1,vl);
#if NR >= 6
      c4=__riscv_vfmacc_vf_f64m1(c4,bp1[4],a1,vl);
      c5=__riscv_vfmacc_vf_f64m1(c5,bp1[5],a1,vl);
#endif
#if NR >= 8
      c6=__riscv_vfmacc_vf_f64m1(c6,bp1[6],a1,vl);
      c7=__riscv_vfmacc_vf_f64m1(c7,bp1[7],a1,vl);
#endif
    }

    if (k<kc) {
      const MTYPE * RESTRICT bp = &Bp[(size_t)k*(size_t)nc + jr];
      vfloat64m1_t a =
        __riscv_vle64_v_f64m1(&Ap[(size_t)k*(size_t)mc + i],vl);
      c0=__riscv_vfmacc_vf_f64m1(c0,bp[0],a,vl);
      c1=__riscv_vfmacc_vf_f64m1(c1,bp[1],a,vl);
      c2=__riscv_vfmacc_vf_f64m1(c2,bp[2],a,vl);
      c3=__riscv_vfmacc_vf_f64m1(c3,bp[3],a,vl);
#if NR >= 6
      c4=__riscv_vfmacc_vf_f64m1(c4,bp[4],a,vl);
      c5=__riscv_vfmacc_vf_f64m1(c5,bp[5],a,vl);
#endif
#if NR >= 8
      c6=__riscv_vfmacc_vf_f64m1(c6,bp[6],a,vl);
      c7=__riscv_vfmacc_vf_f64m1(c7,bp[7],a,vl);
#endif
    }

    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+0,n)],c0,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+1,n)],c1,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+2,n)],c2,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+3,n)],c3,vl);
#if NR >= 6
    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+4,n)],c4,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+5,n)],c5,vl);
#endif
#if NR >= 8
    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+6,n)],c6,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+7,n)],c7,vl);
#endif

    i += (int)vl;
  }
}

/* Tail columns, still vectorized over i. */
static inline void
rvv_kernel_tail_f64(const MTYPE * RESTRICT Ap,
                    const MTYPE * RESTRICT Bp,
                    MTYPE * RESTRICT C,
                    long n, long ic, long jc,
                    int jr, int nr,
                    int mc, int nc, int kc)
{
  for (int r=0; r<nr; ++r) {
    for (int i=0; i<mc; ) {
      size_t vl=__riscv_vsetvl_e64m1((size_t)(mc-i));
      vfloat64m1_t c=
        __riscv_vle64_v_f64m1(&C[C_IDX(ic+i,jc+jr+r,n)],vl);

      int k=0;
      for (; k+1<kc; k+=2) {
        vfloat64m1_t a0=
          __riscv_vle64_v_f64m1(&Ap[(size_t)k*(size_t)mc+i],vl);
        vfloat64m1_t a1=
          __riscv_vle64_v_f64m1(&Ap[(size_t)(k+1)*(size_t)mc+i],vl);
        c=__riscv_vfmacc_vf_f64m1(
          c,Bp[(size_t)k*(size_t)nc+jr+r],a0,vl);
        c=__riscv_vfmacc_vf_f64m1(
          c,Bp[(size_t)(k+1)*(size_t)nc+jr+r],a1,vl);
      }
      if (k<kc) {
        vfloat64m1_t a=
          __riscv_vle64_v_f64m1(&Ap[(size_t)k*(size_t)mc+i],vl);
        c=__riscv_vfmacc_vf_f64m1(
          c,Bp[(size_t)k*(size_t)nc+jr+r],a,vl);
      }
      __riscv_vse64_v_f64m1(&C[C_IDX(ic+i,jc+jr+r,n)],c,vl);
      i += (int)vl;
    }
  }
}

#elif defined(__riscv) && defined(MTYPE_IS_FLOAT)

/* FP32 equivalent, e32,m1. */
static inline void
rvv_kernel_nr_f32(const MTYPE * RESTRICT Ap,
                  const MTYPE * RESTRICT Bp,
                  MTYPE * RESTRICT C,
                  long n, long ic, long jc,
                  int ir, int jr,
                  int mc, int nc, int kc)
{
  int i=ir;
  while (i<mc) {
    size_t vl=__riscv_vsetvl_e32m1((size_t)(mc-i));
    vfloat32m1_t c0=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+0,n)],vl);
    vfloat32m1_t c1=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+1,n)],vl);
    vfloat32m1_t c2=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+2,n)],vl);
    vfloat32m1_t c3=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+3,n)],vl);
#if NR >= 6
    vfloat32m1_t c4=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+4,n)],vl);
    vfloat32m1_t c5=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+5,n)],vl);
#endif
#if NR >= 8
    vfloat32m1_t c6=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+6,n)],vl);
    vfloat32m1_t c7=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+7,n)],vl);
#endif

    int k=0;
    for (; k+1<kc; k+=2) {
      const MTYPE *bp0=&Bp[(size_t)k*(size_t)nc+jr];
      const MTYPE *bp1=bp0+nc;
      vfloat32m1_t a0=__riscv_vle32_v_f32m1(&Ap[(size_t)k*(size_t)mc+i],vl);
      vfloat32m1_t a1=__riscv_vle32_v_f32m1(&Ap[(size_t)(k+1)*(size_t)mc+i],vl);

      c0=__riscv_vfmacc_vf_f32m1(c0,bp0[0],a0,vl);
      c1=__riscv_vfmacc_vf_f32m1(c1,bp0[1],a0,vl);
      c2=__riscv_vfmacc_vf_f32m1(c2,bp0[2],a0,vl);
      c3=__riscv_vfmacc_vf_f32m1(c3,bp0[3],a0,vl);
#if NR >= 6
      c4=__riscv_vfmacc_vf_f32m1(c4,bp0[4],a0,vl);
      c5=__riscv_vfmacc_vf_f32m1(c5,bp0[5],a0,vl);
#endif
#if NR >= 8
      c6=__riscv_vfmacc_vf_f32m1(c6,bp0[6],a0,vl);
      c7=__riscv_vfmacc_vf_f32m1(c7,bp0[7],a0,vl);
#endif

      c0=__riscv_vfmacc_vf_f32m1(c0,bp1[0],a1,vl);
      c1=__riscv_vfmacc_vf_f32m1(c1,bp1[1],a1,vl);
      c2=__riscv_vfmacc_vf_f32m1(c2,bp1[2],a1,vl);
      c3=__riscv_vfmacc_vf_f32m1(c3,bp1[3],a1,vl);
#if NR >= 6
      c4=__riscv_vfmacc_vf_f32m1(c4,bp1[4],a1,vl);
      c5=__riscv_vfmacc_vf_f32m1(c5,bp1[5],a1,vl);
#endif
#if NR >= 8
      c6=__riscv_vfmacc_vf_f32m1(c6,bp1[6],a1,vl);
      c7=__riscv_vfmacc_vf_f32m1(c7,bp1[7],a1,vl);
#endif
    }
    if (k<kc) {
      const MTYPE *bp=&Bp[(size_t)k*(size_t)nc+jr];
      vfloat32m1_t a=__riscv_vle32_v_f32m1(&Ap[(size_t)k*(size_t)mc+i],vl);
      c0=__riscv_vfmacc_vf_f32m1(c0,bp[0],a,vl);
      c1=__riscv_vfmacc_vf_f32m1(c1,bp[1],a,vl);
      c2=__riscv_vfmacc_vf_f32m1(c2,bp[2],a,vl);
      c3=__riscv_vfmacc_vf_f32m1(c3,bp[3],a,vl);
#if NR >= 6
      c4=__riscv_vfmacc_vf_f32m1(c4,bp[4],a,vl);
      c5=__riscv_vfmacc_vf_f32m1(c5,bp[5],a,vl);
#endif
#if NR >= 8
      c6=__riscv_vfmacc_vf_f32m1(c6,bp[6],a,vl);
      c7=__riscv_vfmacc_vf_f32m1(c7,bp[7],a,vl);
#endif
    }

    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+0,n)],c0,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+1,n)],c1,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+2,n)],c2,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+3,n)],c3,vl);
#if NR >= 6
    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+4,n)],c4,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+5,n)],c5,vl);
#endif
#if NR >= 8
    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+6,n)],c6,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+7,n)],c7,vl);
#endif
    i+=(int)vl;
  }
}

static inline void
rvv_kernel_tail_f32(const MTYPE * RESTRICT Ap,
                    const MTYPE * RESTRICT Bp,
                    MTYPE * RESTRICT C,
                    long n,long ic,long jc,
                    int jr,int nr,int mc,int nc,int kc)
{
  for(int r=0;r<nr;++r)
    for(int i=0;i<mc;) {
      size_t vl=__riscv_vsetvl_e32m1((size_t)(mc-i));
      vfloat32m1_t c=__riscv_vle32_v_f32m1(&C[C_IDX(ic+i,jc+jr+r,n)],vl);
      for(int k=0;k<kc;++k) {
        vfloat32m1_t a=__riscv_vle32_v_f32m1(&Ap[(size_t)k*(size_t)mc+i],vl);
        c=__riscv_vfmacc_vf_f32m1(c,Bp[(size_t)k*(size_t)nc+jr+r],a,vl);
      }
      __riscv_vse32_v_f32m1(&C[C_IDX(ic+i,jc+jr+r,n)],c,vl);
      i+=(int)vl;
    }
}

#else

/* Host/scalar fallback for build sanity checks. */
static inline void
scalar_kernel(const MTYPE * RESTRICT Ap,
              const MTYPE * RESTRICT Bp,
              MTYPE * RESTRICT C,
              long n,long ic,long jc,
              int jr,int nr,int mc,int nc,int kc)
{
  for(int r=0;r<nr;++r)
    for(int i=0;i<mc;++i) {
      MTYPE c=C[C_IDX(ic+i,jc+jr+r,n)];
      for(int k=0;k<kc;++k)
        c += Ap[(size_t)k*(size_t)mc+i] *
             Bp[(size_t)k*(size_t)nc+jr+r];
      C[C_IDX(ic+i,jc+jr+r,n)]=c;
    }
}
#endif

static inline void
compute_panel(const MTYPE * RESTRICT Ap,
              const MTYPE * RESTRICT Bp,
              MTYPE * RESTRICT C,
              long n,long ic,long jc,
              int mc,int nc,int kc)
{
  int jr=0;
  for (; jr+NR<=nc; jr+=NR) {
#if defined(__riscv) && defined(MTYPE_IS_DOUBLE)
    rvv_kernel_nr_f64(Ap,Bp,C,n,ic,jc,0,jr,mc,nc,kc);
#elif defined(__riscv) && defined(MTYPE_IS_FLOAT)
    rvv_kernel_nr_f32(Ap,Bp,C,n,ic,jc,0,jr,mc,nc,kc);
#else
    scalar_kernel(Ap,Bp,C,n,ic,jc,jr,NR,mc,nc,kc);
#endif
  }
  if (jr<nc) {
#if defined(__riscv) && defined(MTYPE_IS_DOUBLE)
    rvv_kernel_tail_f64(Ap,Bp,C,n,ic,jc,jr,nc-jr,mc,nc,kc);
#elif defined(__riscv) && defined(MTYPE_IS_FLOAT)
    rvv_kernel_tail_f32(Ap,Bp,C,n,ic,jc,jr,nc-jr,mc,nc,kc);
#else
    scalar_kernel(Ap,Bp,C,n,ic,jc,jr,nc-jr,mc,nc,kc);
#endif
  }
}


typedef struct {
  MTYPE *ap;
  MTYPE *bp;
  size_t ap_elems;
  size_t bp_elems;
} hetero_rvv_tls_t;

static _Thread_local hetero_rvv_tls_t tls_buf = {0,0,0,0};

static inline void ensure_tls_buffers(int kc)
{
  const size_t need_a = (size_t)MC * (size_t)kc;
  const size_t need_b = (size_t)NC * (size_t)kc;

  if (tls_buf.ap_elems < need_a) {
    if (tls_buf.ap) prk_free(tls_buf.ap);
    tls_buf.ap = (MTYPE*)prk_malloc(need_a * sizeof(MTYPE));
    tls_buf.ap_elems = need_a;
  }

  if (tls_buf.bp_elems < need_b) {
    if (tls_buf.bp) prk_free(tls_buf.bp);
    tls_buf.bp = (MTYPE*)prk_malloc(need_b * sizeof(MTYPE));
    tls_buf.bp_elems = need_b;
  }

  if (!tls_buf.ap || !tls_buf.bp) {
    fprintf(stderr, "ERROR: hetero RVV packing allocation failed\n");
    abort();
  }
}

void KERNEL_NAME(const MTYPE * RESTRICT A,
                 const MTYPE * RESTRICT B,
                 MTYPE * RESTRICT C,
                 long order,
                 long col_begin,
                 long col_end,
                 int workers,
                 int worker_id,
                 int kc_block)
{
  if (col_begin >= col_end || workers <= 0) return;
  if (kc_block <= 0) kc_block = 64;

  ensure_tls_buffers(kc_block);

  const long n_panels = (col_end - col_begin + NC - 1) / NC;

  /* Static cyclic ownership of complete NC macro-panels. */
  for (long p = worker_id; p < n_panels; p += workers) {
    const long jc = col_begin + p * (long)NC;
    const int nc = (int)MIN((long)NC, col_end - jc);

    for (long pc = 0; pc < order; pc += kc_block) {
      const int kc = (int)MIN((long)kc_block, order - pc);

      pack_b_panel(B, tls_buf.bp, order, jc, pc, nc, kc);

      for (long ic = 0; ic < order; ic += MC) {
        const int mc = (int)MIN((long)MC, order - ic);

        pack_a_panel(A, tls_buf.ap, order, ic, pc, mc, kc);
        compute_panel(tls_buf.ap, tls_buf.bp, C,
                      order, ic, jc, mc, nc, kc);
      }
    }
  }
}
