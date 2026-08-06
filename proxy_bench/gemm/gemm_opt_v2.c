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
 * gemm_opt.c
 *
 * Optimized portable-C PRK GEMM for RVV-capable CPUs.
 *
 * Key ideas:
 *   - retain the original PRK A/B packing
 *   - preserve contiguous i as the compiler-vectorized dimension
 *   - reuse each packed A element/vector across multiple C columns
 *   - keep accumulation in thread-private packed CC
 *   - explicit OpenMP SIMD on contiguous loops
 *
 * Tune GEMM_J_UNROLL at compile time: 1, 2, 4, or 8.
 */
#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#ifndef MTYPE
#define MTYPE double
#endif

#ifndef GEMM_J_UNROLL
#define GEMM_J_UNROLL 4
#endif

#if (GEMM_J_UNROLL != 1) && (GEMM_J_UNROLL != 2) && \
    (GEMM_J_UNROLL != 4) && (GEMM_J_UNROLL != 8)
#error "GEMM_J_UNROLL must be 1, 2, 4, or 8"
#endif

#define A_arr(i,j) A[(i) + order*(j)]
#define B_arr(i,j) B[(i) + order*(j)]
#define C_arr(i,j) C[(i) + order*(j)]

#define AA_arr(i,k) AA[(i) + ldp*(k)]
#define BB_arr(j,k) BB[(j) + ldp*(k)]
#define CC_arr(i,j) CC[(i) + ldp*(j)]

static inline void
compute_tile(const MTYPE * RESTRICT AA,
             const MTYPE * RESTRICT BB,
             MTYPE       * RESTRICT CC,
             const int mc,
             const int nc,
             const int kc,
             const int ldp)
{
  int i, j, k;

#if GEMM_J_UNROLL == 8
  for (k=0; k<kc; ++k) {
    for (j=0; j+7<nc; j+=8) {
      const MTYPE b0=BB_arr(j+0,k), b1=BB_arr(j+1,k);
      const MTYPE b2=BB_arr(j+2,k), b3=BB_arr(j+3,k);
      const MTYPE b4=BB_arr(j+4,k), b5=BB_arr(j+5,k);
      const MTYPE b6=BB_arr(j+6,k), b7=BB_arr(j+7,k);
      #pragma omp simd
      for (i=0; i<mc; ++i) {
        const MTYPE a=AA_arr(i,k);
        CC_arr(i,j+0)+=a*b0; CC_arr(i,j+1)+=a*b1;
        CC_arr(i,j+2)+=a*b2; CC_arr(i,j+3)+=a*b3;
        CC_arr(i,j+4)+=a*b4; CC_arr(i,j+5)+=a*b5;
        CC_arr(i,j+6)+=a*b6; CC_arr(i,j+7)+=a*b7;
      }
    }
    for (; j<nc; ++j) {
      const MTYPE b=BB_arr(j,k);
      #pragma omp simd
      for (i=0; i<mc; ++i) CC_arr(i,j)+=AA_arr(i,k)*b;
    }
  }

#elif GEMM_J_UNROLL == 4
  for (k=0; k<kc; ++k) {
    for (j=0; j+3<nc; j+=4) {
      const MTYPE b0=BB_arr(j+0,k), b1=BB_arr(j+1,k);
      const MTYPE b2=BB_arr(j+2,k), b3=BB_arr(j+3,k);
      #pragma omp simd
      for (i=0; i<mc; ++i) {
        const MTYPE a=AA_arr(i,k);
        CC_arr(i,j+0)+=a*b0;
        CC_arr(i,j+1)+=a*b1;
        CC_arr(i,j+2)+=a*b2;
        CC_arr(i,j+3)+=a*b3;
      }
    }
    for (; j<nc; ++j) {
      const MTYPE b=BB_arr(j,k);
      #pragma omp simd
      for (i=0; i<mc; ++i) CC_arr(i,j)+=AA_arr(i,k)*b;
    }
  }

#elif GEMM_J_UNROLL == 2
  for (k=0; k<kc; ++k) {
    for (j=0; j+1<nc; j+=2) {
      const MTYPE b0=BB_arr(j+0,k), b1=BB_arr(j+1,k);
      #pragma omp simd
      for (i=0; i<mc; ++i) {
        const MTYPE a=AA_arr(i,k);
        CC_arr(i,j+0)+=a*b0;
        CC_arr(i,j+1)+=a*b1;
      }
    }
    for (; j<nc; ++j) {
      const MTYPE b=BB_arr(j,k);
      #pragma omp simd
      for (i=0; i<mc; ++i) CC_arr(i,j)+=AA_arr(i,k)*b;
    }
  }

#else
  for (k=0; k<kc; ++k)
    for (j=0; j<nc; ++j) {
      const MTYPE b=BB_arr(j,k);
      #pragma omp simd
      for (i=0; i<mc; ++i) CC_arr(i,j)+=AA_arr(i,k)*b;
    }
#endif
}

int main(int argc, char **argv)
{
  int iter, i, ii, j, jj, k, kk, ig, jg, kg;
  int iterations;
  double dgemm_time=0.0, avgtime;
  double checksum=0.0, ref_checksum;
#if defined(MTYPE_IS_FLOAT)
  const double epsilon=1.e-4;
#else
  const double epsilon=1.e-5;
#endif
  int nthread_input, nthread=0;
  int num_error=0;
  static MTYPE * RESTRICT A, * RESTRICT B, * RESTRICT C;
  long order;
  int block;
  int shortcut;

  printf("Parallel Research Kernels version %s\n", PRKVERSION);
  printf("OpenMP optimized packed C dense matrix-matrix multiplication\n");

  if (argc != 4 && argc != 5) {
    printf("Usage: %s <# threads> <# iterations> <matrix order> [tile size]\n", *argv);
    exit(EXIT_FAILURE);
  }

  nthread_input=atoi(*++argv);
  if (nthread_input<1 || nthread_input>MAX_THREADS) {
    printf("ERROR: Invalid number of threads: %d\n", nthread_input);
    exit(EXIT_FAILURE);
  }
  omp_set_num_threads(nthread_input);

  iterations=atoi(*++argv);
  if (iterations<1) {
    printf("ERROR: Iterations must be positive: %d\n", iterations);
    exit(EXIT_FAILURE);
  }

  order=atol(*++argv);
  if (order<0) { shortcut=1; order=-order; }
  else shortcut=0;
  if (order<1) {
    printf("ERROR: Matrix order must be positive: %ld\n", order);
    exit(EXIT_FAILURE);
  }

  block=(argc==5) ? atoi(*++argv) : DEFAULTBLOCK;
  if (block<=0) {
    printf("ERROR: gemm_opt requires a positive tile size\n");
    exit(EXIT_FAILURE);
  }

  A=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  B=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  C=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  if (!A || !B || !C) {
    printf("ERROR: Could not allocate space for global matrices\n");
    exit(EXIT_FAILURE);
  }

  const double forder=(double)order;
  ref_checksum=0.25*forder*forder*forder*(forder-1.0)*(forder-1.0);

  #pragma omp parallel for schedule(static)
  for (j=0; j<order; ++j)
    for (i=0; i<order; ++i) {
      A_arr(i,j)=B_arr(i,j)=(MTYPE)j;
      C_arr(i,j)=(MTYPE)0.0;
    }

  #pragma omp parallel private(i,j,k,ii,jj,kk,ig,jg,kg,iter)
  {
    const int ldp=block+BOFFSET;
    const size_t tile_elems=(size_t)block*(size_t)ldp;
    MTYPE * RESTRICT AA=(MTYPE*)prk_malloc(3*tile_elems*sizeof(MTYPE));
    MTYPE * RESTRICT BB=NULL;
    MTYPE * RESTRICT CC=NULL;

    if (!AA) {
      #pragma omp critical
      {
        num_error=1;
        printf("Could not allocate matrix tiles on thread %d\n", omp_get_thread_num());
      }
    }
    bail_out(num_error);

    BB=AA+tile_elems;
    CC=BB+tile_elems;

    #pragma omp master
    {
      nthread=omp_get_num_threads();
      if (nthread!=nthread_input) {
        num_error=1;
        printf("ERROR: requested %d threads but spawned %d\n", nthread_input, nthread);
      } else {
        printf("Matrix order          = %ld\n", order);
        if (shortcut) printf("Only doing initialization\n");
        printf("Number of threads     = %d\n", nthread_input);
#if defined(MTYPE_IS_FLOAT)
        printf("Precision             = float\n");
#else
        printf("Precision             = double\n");
#endif
        printf("Blocking factor       = %d\n", block);
        printf("Block offset          = %d\n", BOFFSET);
        printf("J unroll              = %d\n", GEMM_J_UNROLL);
        printf("Vector dimension      = contiguous i loop\n");
        printf("Number of iterations  = %d\n", iterations);
      }
    }

    bail_out(num_error);

    if (!shortcut) {
      for (iter=0; iter<=iterations; ++iter) {
        if (iter==1) {
          #pragma omp barrier
          #pragma omp master
          dgemm_time=wtime();
        }

        #pragma omp for schedule(static)
        for (jj=0; jj<order; jj+=block) {
          const int nc=(int)MIN((long)block, order-(long)jj);

          for (kk=0; kk<order; kk+=block) {
            const int kc=(int)MIN((long)block, order-(long)kk);

            /* Pack/transposed B once per (jj,kk), reused over all ii. */
            for (k=0; k<kc; ++k) {
              kg=kk+k;
              #pragma omp simd
              for (j=0; j<nc; ++j) {
                jg=jj+j;
                BB_arr(j,k)=B_arr(kg,jg);
              }
            }

            for (ii=0; ii<order; ii+=block) {
              const int mc=(int)MIN((long)block, order-(long)ii);

              /* Pack A into unit-stride i vectors. */
              for (k=0; k<kc; ++k) {
                kg=kk+k;
                #pragma omp simd
                for (i=0; i<mc; ++i) {
                  ig=ii+i;
                  AA_arr(i,k)=A_arr(ig,kg);
                }
              }

              /* Initialize cache-local output tile. */
              for (j=0; j<nc; ++j) {
                #pragma omp simd
                for (i=0; i<mc; ++i)
                  CC_arr(i,j)=(MTYPE)0.0;
              }

              compute_tile(AA,BB,CC,mc,nc,kc,ldp);

              /* Add completed local tile to global C. */
              for (j=0; j<nc; ++j) {
                jg=jj+j;
                #pragma omp simd
                for (i=0; i<mc; ++i) {
                  ig=ii+i;
                  C_arr(ig,jg)+=CC_arr(i,j);
                }
              }
            }
          }
        }
      }

      #pragma omp barrier
      #pragma omp master
      dgemm_time=wtime()-dgemm_time;
    }

    prk_free(AA);
  }

  bail_out(num_error);

  if (shortcut) {
    prk_free(A); prk_free(B); prk_free(C);
    return 0;
  }

  for (j=0; j<order; ++j)
    for (i=0; i<order; ++i)
      checksum+=C_arr(i,j);

  ref_checksum*=(iterations+1);

  if (ABS((checksum-ref_checksum)/ref_checksum)>epsilon) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n",
           checksum, ref_checksum);
  } else {
    printf("Solution validates\n");
#if VERBOSE
    printf("Reference checksum = %lf, checksum = %lf\n",
           ref_checksum, checksum);
#endif
  }

  {
    const double nflops=2.0*forder*forder*forder;
    avgtime=dgemm_time/iterations;
    printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
           1.0E-06*nflops/avgtime, avgtime);
  }

  prk_free(A); prk_free(B); prk_free(C);
  return 0;
}
