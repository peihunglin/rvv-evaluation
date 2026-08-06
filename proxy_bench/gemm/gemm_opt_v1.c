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
 * Optimized portable-C DGEMM for the PRK GEMM benchmark.
 *
 * Changes relative to gemm.c:
 *   - direct accumulation into C (no AA/BB/CC temporary tiles)
 *   - 4x4 register microkernel
 *   - K blocking; the optional "tile size" argument is used as KC
 *   - OpenMP partitions output-column groups, avoiding write sharing
 *
 * Matrices remain column-major and the PRK initialization/checksum
 * semantics are preserved.
 */
#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#ifndef MTYPE
#define MTYPE double
#endif

#ifndef GEMM_OPT_KC
#define GEMM_OPT_KC 256
#endif

#define MR 4
#define NR 4
#define A_IDX(i,j,n) ((i) + (n)*(j))
#define B_IDX(i,j,n) ((i) + (n)*(j))
#define C_IDX(i,j,n) ((i) + (n)*(j))

static inline void dgemm_4x4_kc(const MTYPE * RESTRICT A,
                                const MTYPE * RESTRICT B,
                                MTYPE * RESTRICT C,
                                long n, long i, long j,
                                long k0, long k1)
{
  MTYPE c00=C[A_IDX(i+0,j+0,n)], c01=C[C_IDX(i+0,j+1,n)];
  MTYPE c02=C[A_IDX(i+0,j+2,n)], c03=C[A_IDX(i+0,j+3,n)];
  MTYPE c10=C[A_IDX(i+1,j+0,n)], c11=C[A_IDX(i+1,j+1,n)];
  MTYPE c12=C[A_IDX(i+1,j+2,n)], c13=C[A_IDX(i+1,j+3,n)];
  MTYPE c20=C[A_IDX(i+2,j+0,n)], c21=C[A_IDX(i+2,j+1,n)];
  MTYPE c22=C[A_IDX(i+2,j+2,n)], c23=C[A_IDX(i+2,j+3,n)];
  MTYPE c30=C[A_IDX(i+3,j+0,n)], c31=C[A_IDX(i+3,j+1,n)];
  MTYPE c32=C[A_IDX(i+3,j+2,n)], c33=C[A_IDX(i+3,j+3,n)];

  for (long k=k0; k<k1; ++k) {
    const MTYPE a0=A[A_IDX(i+0,k,n)], a1=A[A_IDX(i+1,k,n)];
    const MTYPE a2=A[A_IDX(i+2,k,n)], a3=A[A_IDX(i+3,k,n)];
    const MTYPE b0=B[B_IDX(k,j+0,n)], b1=B[B_IDX(k,j+1,n)];
    const MTYPE b2=B[B_IDX(k,j+2,n)], b3=B[B_IDX(k,j+3,n)];

    c00 += a0*b0; c01 += a0*b1; c02 += a0*b2; c03 += a0*b3;
    c10 += a1*b0; c11 += a1*b1; c12 += a1*b2; c13 += a1*b3;
    c20 += a2*b0; c21 += a2*b1; c22 += a2*b2; c23 += a2*b3;
    c30 += a3*b0; c31 += a3*b1; c32 += a3*b2; c33 += a3*b3;
  }

  C[C_IDX(i+0,j+0,n)]=c00; C[C_IDX(i+0,j+1,n)]=c01;
  C[C_IDX(i+0,j+2,n)]=c02; C[C_IDX(i+0,j+3,n)]=c03;
  C[C_IDX(i+1,j+0,n)]=c10; C[C_IDX(i+1,j+1,n)]=c11;
  C[C_IDX(i+1,j+2,n)]=c12; C[C_IDX(i+1,j+3,n)]=c13;
  C[C_IDX(i+2,j+0,n)]=c20; C[C_IDX(i+2,j+1,n)]=c21;
  C[C_IDX(i+2,j+2,n)]=c22; C[C_IDX(i+2,j+3,n)]=c23;
  C[C_IDX(i+3,j+0,n)]=c30; C[C_IDX(i+3,j+1,n)]=c31;
  C[C_IDX(i+3,j+2,n)]=c32; C[C_IDX(i+3,j+3,n)]=c33;
}

static inline void dgemm_edge(const MTYPE * RESTRICT A,
                              const MTYPE * RESTRICT B,
                              MTYPE * RESTRICT C,
                              long n, long i0, long i1,
                              long j0, long j1,
                              long k0, long k1)
{
  for (long j=j0; j<j1; ++j)
    for (long i=i0; i<i1; ++i) {
      MTYPE cij=C[C_IDX(i,j,n)];
      for (long k=k0; k<k1; ++k)
        cij += A[A_IDX(i,k,n)] * B[B_IDX(k,j,n)];
      C[C_IDX(i,j,n)] = cij;
    }
}

int main(int argc, char **argv)
{
  int iterations, nthread_input, nthread=0, shortcut, num_error=0;
  long order;
  int kc;
  double gemm_time=0.0, avgtime, checksum=0.0, ref_checksum;
#if defined(MTYPE_IS_FLOAT)
  const double epsilon=1.e-4;
#else
  const double epsilon=1.e-8;
#endif
  MTYPE * RESTRICT A, * RESTRICT B, * RESTRICT C;

  printf("Parallel Research Kernels version %s\n", PRKVERSION);
  printf("OpenMP optimized C dense matrix-matrix multiplication\n");
  if (argc != 4 && argc != 5) {
    printf("Usage: %s <# threads> <# iterations> <matrix order> [KC]\n", *argv);
    exit(EXIT_FAILURE);
  }

  nthread_input=atoi(*++argv);
  if (nthread_input<1 || nthread_input>MAX_THREADS) {
    printf("ERROR: Invalid number of threads: %d\n", nthread_input);
    exit(EXIT_FAILURE);
  }
  omp_set_num_threads(nthread_input);

  iterations=atoi(*++argv);
  if (iterations<1) { printf("ERROR: Iterations must be positive\n"); exit(EXIT_FAILURE); }

  order=atol(*++argv);
  shortcut=(order<0);
  if (shortcut) order=-order;
  if (order<1) { printf("ERROR: Matrix order must be positive\n"); exit(EXIT_FAILURE); }

  kc=(argc==5) ? atoi(*++argv) : GEMM_OPT_KC;
  if (kc<=0) kc=GEMM_OPT_KC;

  A=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  B=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  C=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  if (!A || !B || !C) { printf("ERROR: matrix allocation failed\n"); exit(EXIT_FAILURE); }

  const double fn=(double)order;
  ref_checksum=0.25*fn*fn*fn*(fn-1.0)*(fn-1.0);

  #pragma omp parallel for schedule(static)
  for (long j=0; j<order; ++j)
    for (long i=0; i<order; ++i) {
      A[A_IDX(i,j,order)]=(MTYPE)j;
      B[B_IDX(i,j,order)]=(MTYPE)j;
      C[C_IDX(i,j,order)]=(MTYPE)0.0;
    }

  #pragma omp parallel
  {
    #pragma omp master
    {
      nthread=omp_get_num_threads();
      if (nthread!=nthread_input) num_error=1;
      printf("Matrix order          = %ld\n", order);
      printf("Number of threads     = %d\n", nthread_input);
#if defined(MTYPE_IS_FLOAT)
      printf("Precision             = float\n");
#else
      printf("Precision             = double\n");
#endif
      printf("Register tile         = %dx%d\n", MR, NR);
      printf("K blocking (KC)       = %d\n", kc);
      printf("Number of iterations  = %d\n", iterations);
    }
  }
  bail_out(num_error);
  if (shortcut) exit(EXIT_SUCCESS);

  for (int iter=0; iter<=iterations; ++iter) {
    if (iter==1) gemm_time=wtime();

    #pragma omp parallel for schedule(static)
    for (long j=0; j<order; j+=NR) {
      const long j1=MIN(j+(long)NR, order);
      for (long kk=0; kk<order; kk+=kc) {
        const long k1=MIN(kk+(long)kc, order);
        long i=0;
        if (j1-j==NR) {
          for (; i+MR<=order; i+=MR)
            dgemm_4x4_kc(A,B,C,order,i,j,kk,k1);
        }
        if (i<order || j1-j!=NR)
          dgemm_edge(A,B,C,order,i,order,j,j1,kk,k1);
      }
    }
  }
  gemm_time=wtime()-gemm_time;

  for (long j=0; j<order; ++j)
    for (long i=0; i<order; ++i)
      checksum += C[C_IDX(i,j,order)];

  ref_checksum *= (iterations+1);
  const double err=ABS(checksum-ref_checksum);
  if ((ref_checksum==0.0 && err>epsilon) ||
      (ref_checksum!=0.0 && err/ABS(ref_checksum)>epsilon)) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n", checksum, ref_checksum);
    exit(EXIT_FAILURE);
  }
  printf("Solution validates\n");
  const double nflops=2.0*fn*fn*fn;
  avgtime=gemm_time/iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
         1.e-6*nflops/avgtime, avgtime);

  prk_free(A); prk_free(B); prk_free(C);
  return 0;
}
