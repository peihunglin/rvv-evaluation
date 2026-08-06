/*
 * gemm_opt.c
 *
 * Plain-C packed GEMM designed to give Clang/GCC a structure close to the
 * optimized RVV kernel, without using RVV intrinsics.
 *
 * Main ideas:
 *   - Separate MC / NC / KC blocking.
 *   - Pack A as Ap[k*mc + i].
 *   - Pack B as Bp[k*nc + j].
 *   - Process several output columns together (NR, default 4).
 *   - Keep a small strip of C in local arrays across the full KC loop.
 *   - Vectorize the small ii loops with OpenMP SIMD.
 *
 * The intention is to encourage the compiler to produce multiple independent
 * RVV accumulator vectors while preserving portable C source.
 *
 * Suggested X100 FP64 starting point:
 *      MC=32 NC=64 KC=64 NR=4 VBLOCK=8
 *
 * Suggested X100 FP32 starting point:
 *      MC=32 NC=64 KC=128 NR=4 VBLOCK=16
 *
 * Compile-time knobs:
 *      -DCOPT_MC=32
 *      -DCOPT_NC=64
 *      -DCOPT_NR=4
 *      -DCOPT_VBLOCK=8
 *
 * KC remains the optional command-line argument.
 */

#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#ifndef MTYPE
#define MTYPE double
#endif

#ifndef COPT_MC
#define COPT_MC 32
#endif

#ifndef COPT_NC
#define COPT_NC 64
#endif

#ifndef COPT_NR
#define COPT_NR 4
#endif

#ifndef COPT_KC
#if defined(MTYPE_IS_FLOAT)
#define COPT_KC 128
#else
#define COPT_KC 64
#endif
#endif

#ifndef COPT_VBLOCK
#if defined(MTYPE_IS_FLOAT)
#define COPT_VBLOCK 16
#else
#define COPT_VBLOCK 8
#endif
#endif

#if (COPT_NR != 2) && (COPT_NR != 4) && (COPT_NR != 6) && (COPT_NR != 8)
#error "COPT_NR must be 2, 4, 6, or 8"
#endif

#define MC COPT_MC
#define NC COPT_NC
#define NR COPT_NR
#define VB COPT_VBLOCK

#define A_IDX(i,j,n) ((i) + (n)*(j))
#define B_IDX(i,j,n) ((i) + (n)*(j))
#define C_IDX(i,j,n) ((i) + (n)*(j))

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

    for (int j=0; j<nc; ++j)
      dst[j] = B[B_IDX(pc+k,jc+j,n)];
  }
}

/*
 * Plain-C microkernel.
 *
 * Each VB-row strip is loaded into local C arrays once, updated for every k
 * in the KC panel, then stored once.  The ii loops are independent and marked
 * SIMD so a compiler can represent c0/c1/... as RVV vectors.
 */
static inline void
c_kernel_nr(const MTYPE * RESTRICT Ap,
            const MTYPE * RESTRICT Bp,
            MTYPE * RESTRICT C,
            long n, long ic, long jc,
            int jr, int mc, int nc, int kc)
{
  for (int ib=0; ib<mc; ib+=VB) {
    const int nv = MIN(VB, mc-ib);

    MTYPE c0[VB];
    MTYPE c1[VB];
#if NR >= 4
    MTYPE c2[VB];
    MTYPE c3[VB];
#endif
#if NR >= 6
    MTYPE c4[VB];
    MTYPE c5[VB];
#endif
#if NR >= 8
    MTYPE c6[VB];
    MTYPE c7[VB];
#endif

    #pragma omp simd
    for (int ii=0; ii<nv; ++ii) {
      const long gi = ic + ib + ii;
      c0[ii] = C[C_IDX(gi,jc+jr+0,n)];
      c1[ii] = C[C_IDX(gi,jc+jr+1,n)];
#if NR >= 4
      c2[ii] = C[C_IDX(gi,jc+jr+2,n)];
      c3[ii] = C[C_IDX(gi,jc+jr+3,n)];
#endif
#if NR >= 6
      c4[ii] = C[C_IDX(gi,jc+jr+4,n)];
      c5[ii] = C[C_IDX(gi,jc+jr+5,n)];
#endif
#if NR >= 8
      c6[ii] = C[C_IDX(gi,jc+jr+6,n)];
      c7[ii] = C[C_IDX(gi,jc+jr+7,n)];
#endif
    }

    int k=0;
    for (; k+1<kc; k+=2) {
      const MTYPE * RESTRICT bp0 = &Bp[(size_t)k*(size_t)nc + jr];
      const MTYPE * RESTRICT bp1 = bp0 + nc;
      const MTYPE * RESTRICT ap0 = &Ap[(size_t)k*(size_t)mc + ib];
      const MTYPE * RESTRICT ap1 = &Ap[(size_t)(k+1)*(size_t)mc + ib];

      const MTYPE b00=bp0[0], b01=bp0[1];
      const MTYPE b10=bp1[0], b11=bp1[1];
#if NR >= 4
      const MTYPE b02=bp0[2], b03=bp0[3];
      const MTYPE b12=bp1[2], b13=bp1[3];
#endif
#if NR >= 6
      const MTYPE b04=bp0[4], b05=bp0[5];
      const MTYPE b14=bp1[4], b15=bp1[5];
#endif
#if NR >= 8
      const MTYPE b06=bp0[6], b07=bp0[7];
      const MTYPE b16=bp1[6], b17=bp1[7];
#endif

      #pragma omp simd
      for (int ii=0; ii<nv; ++ii) {
        const MTYPE a0=ap0[ii];
        const MTYPE a1=ap1[ii];

        c0[ii] += a0*b00 + a1*b10;
        c1[ii] += a0*b01 + a1*b11;
#if NR >= 4
        c2[ii] += a0*b02 + a1*b12;
        c3[ii] += a0*b03 + a1*b13;
#endif
#if NR >= 6
        c4[ii] += a0*b04 + a1*b14;
        c5[ii] += a0*b05 + a1*b15;
#endif
#if NR >= 8
        c6[ii] += a0*b06 + a1*b16;
        c7[ii] += a0*b07 + a1*b17;
#endif
      }
    }

    if (k<kc) {
      const MTYPE * RESTRICT bp = &Bp[(size_t)k*(size_t)nc + jr];
      const MTYPE * RESTRICT ap = &Ap[(size_t)k*(size_t)mc + ib];

      const MTYPE b0=bp[0], b1=bp[1];
#if NR >= 4
      const MTYPE b2=bp[2], b3=bp[3];
#endif
#if NR >= 6
      const MTYPE b4=bp[4], b5=bp[5];
#endif
#if NR >= 8
      const MTYPE b6=bp[6], b7=bp[7];
#endif

      #pragma omp simd
      for (int ii=0; ii<nv; ++ii) {
        const MTYPE a=ap[ii];
        c0[ii] += a*b0;
        c1[ii] += a*b1;
#if NR >= 4
        c2[ii] += a*b2;
        c3[ii] += a*b3;
#endif
#if NR >= 6
        c4[ii] += a*b4;
        c5[ii] += a*b5;
#endif
#if NR >= 8
        c6[ii] += a*b6;
        c7[ii] += a*b7;
#endif
      }
    }

    #pragma omp simd
    for (int ii=0; ii<nv; ++ii) {
      const long gi = ic + ib + ii;
      C[C_IDX(gi,jc+jr+0,n)] = c0[ii];
      C[C_IDX(gi,jc+jr+1,n)] = c1[ii];
#if NR >= 4
      C[C_IDX(gi,jc+jr+2,n)] = c2[ii];
      C[C_IDX(gi,jc+jr+3,n)] = c3[ii];
#endif
#if NR >= 6
      C[C_IDX(gi,jc+jr+4,n)] = c4[ii];
      C[C_IDX(gi,jc+jr+5,n)] = c5[ii];
#endif
#if NR >= 8
      C[C_IDX(gi,jc+jr+6,n)] = c6[ii];
      C[C_IDX(gi,jc+jr+7,n)] = c7[ii];
#endif
    }
  }
}

static inline void
c_kernel_tail(const MTYPE * RESTRICT Ap,
              const MTYPE * RESTRICT Bp,
              MTYPE * RESTRICT C,
              long n,long ic,long jc,
              int jr,int nr,int mc,int nc,int kc)
{
  for (int r=0; r<nr; ++r) {
    for (int ib=0; ib<mc; ib+=VB) {
      const int nv=MIN(VB,mc-ib);
      MTYPE cv[VB];

      #pragma omp simd
      for (int ii=0; ii<nv; ++ii)
        cv[ii]=C[C_IDX(ic+ib+ii,jc+jr+r,n)];

      for (int k=0; k<kc; ++k) {
        const MTYPE b=Bp[(size_t)k*(size_t)nc+jr+r];
        const MTYPE * RESTRICT ap=&Ap[(size_t)k*(size_t)mc+ib];

        #pragma omp simd
        for (int ii=0; ii<nv; ++ii)
          cv[ii] += ap[ii]*b;
      }

      #pragma omp simd
      for (int ii=0; ii<nv; ++ii)
        C[C_IDX(ic+ib+ii,jc+jr+r,n)] = cv[ii];
    }
  }
}

static inline void
compute_panel(const MTYPE * RESTRICT Ap,
              const MTYPE * RESTRICT Bp,
              MTYPE * RESTRICT C,
              long n,long ic,long jc,
              int mc,int nc,int kc)
{
  int jr=0;
  for (; jr+NR<=nc; jr+=NR)
    c_kernel_nr(Ap,Bp,C,n,ic,jc,jr,mc,nc,kc);

  if (jr<nc)
    c_kernel_tail(Ap,Bp,C,n,ic,jc,jr,nc-jr,mc,nc,kc);
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
  printf("OpenMP packed C GEMM - compiler-vectorized\n");

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

  kc_block=(argc==5)?atoi(*++argv):COPT_KC;
  if(kc_block<=0) kc_block=COPT_KC;

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
#else
      printf("Precision             = double\n");
#endif
      printf("MC / NC / KC          = %d / %d / %d\n",MC,NC,kc_block);
      printf("C microkernel         = VB x NR = %d x %d\n",VB,NR);
      printf("K unroll              = 2\n");
      printf("A/B packing           = enabled\n");
      printf("RVV intrinsics        = none\n");
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
    MTYPE * RESTRICT Ap=
      (MTYPE*)prk_malloc((size_t)MC*(size_t)kc_block*sizeof(MTYPE));
    MTYPE * RESTRICT Bp=
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

  {
    const double nflops=2.0*fn*fn*fn;
    avgtime=gemm_time/iterations;
    printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
           1.e-6*nflops/avgtime,avgtime);
  }

  prk_free(A); prk_free(B); prk_free(C);
  return 0;
}
