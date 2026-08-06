/*
Copyright (c) 2013, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
* Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
* Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * gemm_rvv_x100_opt.c
 *
 * A100-oriented packed RVV GEMM.
 *
 * Compared with the previous VLx8 direct-global-memory version:
 *
 *   1. B is packed in exactly the order consumed by the microkernel:
 *          Bp[k*nc + j]
 *      so the NR scalar B operands for a K step are adjacent.
 *
 *   2. A is packed with i contiguous:
 *          Ap[k*mc + i]
 *      so successive K steps consume a compact packed panel instead of
 *      jumping by the full matrix leading dimension.
 *
 *   3. C stays in NR vector accumulators for an entire KC panel.
 *
 *   4. The K loop is unrolled by 2, exposing two A-vector loads and
 *      2*NR FMAs to the A100 vector pipelines.
 *
 *   5. Separate MC/NC/KC blocking:
 *          default FP64 MC=64, NC=64, KC=64; FP32 KC defaults to 128
 *      KC remains the optional command-line argument so it is easy to sweep.
 *
 *   6. One persistent OpenMP parallel region.  Each thread owns complete
 *      NC column panels and has private A/B packing buffers.
 *
 * Suggested A100 sweeps:
 *
 *   FP64: KC=32,48,64,80,96,128; NR=4,6,8; MC=32,64,96
 *   FP32: KC=64,96,128,160,192,256; NR=4,6,8; MC=32,64,96
 *
 * Build-time knobs:
 *   -DRVV_OPT_MC=64
 *   -DRVV_OPT_NC=64
 *   -DRVV_OPT_NR=4   // suggested FP64 starting point
 */

#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#if defined(__riscv)
#include <riscv_vector.h>
#endif

#ifndef MTYPE
#define MTYPE double
#endif

#ifndef RVV_OPT_KC
  #if defined(MTYPE_IS_FLOAT)
    #define RVV_OPT_KC 128
  #else
    #define RVV_OPT_KC 64
  #endif
#endif

#ifndef RVV_OPT_MC
#define RVV_OPT_MC 64
#endif

#ifndef RVV_OPT_NC
#define RVV_OPT_NC 64
#endif

#ifndef RVV_OPT_NR
  #if defined(MTYPE_IS_FLOAT)
    #define RVV_OPT_NR 8
  #else
    #define RVV_OPT_NR 4
  #endif
#endif

#if (RVV_OPT_NR != 4) && (RVV_OPT_NR != 6) && (RVV_OPT_NR != 8)
#error "RVV_OPT_NR must be 4, 6, or 8"
#endif

#define NR RVV_OPT_NR
#define MC RVV_OPT_MC
#define NC RVV_OPT_NC

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

int main(int argc,char **argv)
{
  int iterations,nthread_input,nthread=0,shortcut,num_error=0;
  long order;
  int kc_block;
  double gemm_time=0.0,avgtime,checksum=0.0,ref_checksum;

#if defined(MTYPE_IS_FLOAT)
  const double epsilon=1.e-4;
#else
  const double epsilon=1.e-8;
#endif

  MTYPE * RESTRICT A,* RESTRICT B,* RESTRICT C;

  printf("Parallel Research Kernels version %s\n",PRKVERSION);
  printf("OpenMP packed RVV GEMM - SpaceMiT A100\n");

  if(argc!=4 && argc!=5) {
    printf("Usage: %s <# threads> <# iterations> <matrix order> [KC]\n",*argv);
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

  kc_block=(argc==5)?atoi(*++argv):RVV_OPT_KC;
  if(kc_block<=0) kc_block=RVV_OPT_KC;

  A=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  B=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  C=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  if(!A||!B||!C) exit(EXIT_FAILURE);

  const double fn=(double)order;
  ref_checksum=0.25*fn*fn*fn*(fn-1.0)*(fn-1.0);

  #pragma omp parallel for schedule(static)
  for(long j=0;j<order;++j)
    for(long i=0;i<order;++i) {
      A[A_IDX(i,j,order)]=(MTYPE)j;
      B[B_IDX(i,j,order)]=(MTYPE)j;
      C[C_IDX(i,j,order)]=(MTYPE)0.0;
    }

  #pragma omp parallel
  {
    #pragma omp master
    {
      nthread=omp_get_num_threads();
      if(nthread!=nthread_input) num_error=1;
      printf("Matrix order          = %ld\n",order);
      printf("Number of threads     = %d\n",nthread_input);
#if defined(MTYPE_IS_FLOAT)
      printf("Precision             = float\n");
      printf("RVV register tile     = VL x %d, e32,m1\n",NR);
#else
      printf("Precision             = double\n");
      printf("RVV register tile     = VL x %d, e64,m1\n",NR);
#endif
      printf("MC / NC / KC          = %d / %d / %d\n",MC,NC,kc_block);
      printf("K unroll              = 2\n");
      printf("A/B packing           = enabled\n");
      printf("Number of iterations  = %d\n",iterations);
    }
  }

  bail_out(num_error);
  if(shortcut) {
    prk_free(A);prk_free(B);prk_free(C);
    return 0;
  }

  /*
   * Persistent team so each thread allocates its packing buffers once.
   * The omp-for distributes independent NC-column panels.
   */
  #pragma omp parallel
  {
    MTYPE * RESTRICT Ap =
      (MTYPE*)prk_malloc((size_t)MC*(size_t)kc_block*sizeof(MTYPE));
    MTYPE * RESTRICT Bp =
      (MTYPE*)prk_malloc((size_t)NC*(size_t)kc_block*sizeof(MTYPE));

    if(!Ap || !Bp) {
      #pragma omp critical
      num_error=1;
    }

    #pragma omp barrier

    if(!num_error) {
      for(int iter=0;iter<=iterations;++iter) {

        if(iter==1) {
          #pragma omp barrier
          #pragma omp master
          gemm_time=wtime();
        }

        #pragma omp for schedule(static)
        for(long jc=0;jc<order;jc+=NC) {
          const int nc=(int)MIN((long)NC,order-jc);

          for(long pc=0;pc<order;pc+=kc_block) {
            const int kc=(int)MIN((long)kc_block,order-pc);

            /* One B pack reused by every MC block in this column panel. */
            pack_b_panel(B,Bp,order,jc,pc,nc,kc);

            for(long ic=0;ic<order;ic+=MC) {
              const int mc=(int)MIN((long)MC,order-ic);

              pack_a_panel(A,Ap,order,ic,pc,mc,kc);
              compute_panel(Ap,Bp,C,order,ic,jc,mc,nc,kc);
            }
          }
        }
      }

      #pragma omp barrier
      #pragma omp master
      gemm_time=wtime()-gemm_time;
    }

    if(Ap) prk_free(Ap);
    if(Bp) prk_free(Bp);
  }

  bail_out(num_error);

  for(long j=0;j<order;++j)
    for(long i=0;i<order;++i)
      checksum+=C[C_IDX(i,j,order)];

  ref_checksum*=(iterations+1);
  const double err=ABS(checksum-ref_checksum);

  if((ref_checksum==0.0 && err>epsilon) ||
     (ref_checksum!=0.0 && err/ABS(ref_checksum)>epsilon)) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n",
           checksum,ref_checksum);
    exit(EXIT_FAILURE);
  }

  printf("Solution validates\n");

  const double nflops=2.0*fn*fn*fn;
  avgtime=gemm_time/iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
         1.e-6*nflops/avgtime,avgtime);

  prk_free(A);prk_free(B);prk_free(C);
  return 0;
}
