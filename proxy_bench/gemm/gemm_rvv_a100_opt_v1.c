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
 * Optimized RVV FP32/FP64 GEMM for SpaceMiT A100.
 *
 * VLA kernel: e32/e64,m1, NR=8.  C vectors stay live for a KC panel and
 * each A vector is reused by eight independent vf-macc chains.
 * The same source remains vector-length agnostic; the target-specific
 * file mainly provides a conservative default KC and benchmark label.
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
#define RVV_OPT_KC 256
#endif
#define NR 8
#define A_IDX(i,j,n) ((i)+(n)*(j))
#define B_IDX(i,j,n) ((i)+(n)*(j))
#define C_IDX(i,j,n) ((i)+(n)*(j))

#if defined(__riscv)
#if defined(MTYPE_IS_FLOAT)
static inline void rvv8_kc(const MTYPE * RESTRICT A,
                           const MTYPE * RESTRICT B,
                           MTYPE * RESTRICT C,
                           long n, long j, long k0, long k1)
{
  for (long i=0; i<n; ) {
    size_t vl=__riscv_vsetvl_e32m1((size_t)(n-i));
    vfloat32m1_t c0=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+0,n)],vl);
    vfloat32m1_t c1=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+1,n)],vl);
    vfloat32m1_t c2=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+2,n)],vl);
    vfloat32m1_t c3=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+3,n)],vl);
    vfloat32m1_t c4=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+4,n)],vl);
    vfloat32m1_t c5=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+5,n)],vl);
    vfloat32m1_t c6=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+6,n)],vl);
    vfloat32m1_t c7=__riscv_vle32_v_f32m1(&C[C_IDX(i,j+7,n)],vl);

    for (long k=k0; k<k1; ++k) {
      vfloat32m1_t a=__riscv_vle32_v_f32m1(&A[A_IDX(i,k,n)],vl);
      c0=__riscv_vfmacc_vf_f32m1(c0,B[B_IDX(k,j+0,n)],a,vl);
      c1=__riscv_vfmacc_vf_f32m1(c1,B[B_IDX(k,j+1,n)],a,vl);
      c2=__riscv_vfmacc_vf_f32m1(c2,B[B_IDX(k,j+2,n)],a,vl);
      c3=__riscv_vfmacc_vf_f32m1(c3,B[B_IDX(k,j+3,n)],a,vl);
      c4=__riscv_vfmacc_vf_f32m1(c4,B[B_IDX(k,j+4,n)],a,vl);
      c5=__riscv_vfmacc_vf_f32m1(c5,B[B_IDX(k,j+5,n)],a,vl);
      c6=__riscv_vfmacc_vf_f32m1(c6,B[B_IDX(k,j+6,n)],a,vl);
      c7=__riscv_vfmacc_vf_f32m1(c7,B[B_IDX(k,j+7,n)],a,vl);
    }

    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+0,n)],c0,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+1,n)],c1,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+2,n)],c2,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+3,n)],c3,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+4,n)],c4,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+5,n)],c5,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+6,n)],c6,vl);
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j+7,n)],c7,vl);
    i += (long)vl;
  }
}

static inline void rvv1_kc(const MTYPE * RESTRICT A,
                           const MTYPE * RESTRICT B,
                           MTYPE * RESTRICT C,
                           long n, long j, long k0, long k1)
{
  for (long i=0; i<n; ) {
    size_t vl=__riscv_vsetvl_e32m1((size_t)(n-i));
    vfloat32m1_t c=__riscv_vle32_v_f32m1(&C[C_IDX(i,j,n)],vl);
    for (long k=k0; k<k1; ++k) {
      vfloat32m1_t a=__riscv_vle32_v_f32m1(&A[A_IDX(i,k,n)],vl);
      c=__riscv_vfmacc_vf_f32m1(c,B[B_IDX(k,j,n)],a,vl);
    }
    __riscv_vse32_v_f32m1(&C[C_IDX(i,j,n)],c,vl);
    i += (long)vl;
  }
}
#elif defined(MTYPE_IS_DOUBLE)
static inline void rvv8_kc(const MTYPE * RESTRICT A,
                           const MTYPE * RESTRICT B,
                           MTYPE * RESTRICT C,
                           long n, long j, long k0, long k1)
{
  for (long i=0; i<n; ) {
    size_t vl=__riscv_vsetvl_e64m1((size_t)(n-i));
    vfloat64m1_t c0=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+0,n)],vl);
    vfloat64m1_t c1=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+1,n)],vl);
    vfloat64m1_t c2=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+2,n)],vl);
    vfloat64m1_t c3=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+3,n)],vl);
    vfloat64m1_t c4=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+4,n)],vl);
    vfloat64m1_t c5=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+5,n)],vl);
    vfloat64m1_t c6=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+6,n)],vl);
    vfloat64m1_t c7=__riscv_vle64_v_f64m1(&C[C_IDX(i,j+7,n)],vl);

    for (long k=k0; k<k1; ++k) {
      vfloat64m1_t a=__riscv_vle64_v_f64m1(&A[A_IDX(i,k,n)],vl);
      c0=__riscv_vfmacc_vf_f64m1(c0,B[B_IDX(k,j+0,n)],a,vl);
      c1=__riscv_vfmacc_vf_f64m1(c1,B[B_IDX(k,j+1,n)],a,vl);
      c2=__riscv_vfmacc_vf_f64m1(c2,B[B_IDX(k,j+2,n)],a,vl);
      c3=__riscv_vfmacc_vf_f64m1(c3,B[B_IDX(k,j+3,n)],a,vl);
      c4=__riscv_vfmacc_vf_f64m1(c4,B[B_IDX(k,j+4,n)],a,vl);
      c5=__riscv_vfmacc_vf_f64m1(c5,B[B_IDX(k,j+5,n)],a,vl);
      c6=__riscv_vfmacc_vf_f64m1(c6,B[B_IDX(k,j+6,n)],a,vl);
      c7=__riscv_vfmacc_vf_f64m1(c7,B[B_IDX(k,j+7,n)],a,vl);
    }

    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+0,n)],c0,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+1,n)],c1,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+2,n)],c2,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+3,n)],c3,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+4,n)],c4,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+5,n)],c5,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+6,n)],c6,vl);
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j+7,n)],c7,vl);
    i += (long)vl;
  }
}

static inline void rvv1_kc(const MTYPE * RESTRICT A,
                           const MTYPE * RESTRICT B,
                           MTYPE * RESTRICT C,
                           long n, long j, long k0, long k1)
{
  for (long i=0; i<n; ) {
    size_t vl=__riscv_vsetvl_e64m1((size_t)(n-i));
    vfloat64m1_t c=__riscv_vle64_v_f64m1(&C[C_IDX(i,j,n)],vl);
    for (long k=k0; k<k1; ++k) {
      vfloat64m1_t a=__riscv_vle64_v_f64m1(&A[A_IDX(i,k,n)],vl);
      c=__riscv_vfmacc_vf_f64m1(c,B[B_IDX(k,j,n)],a,vl);
    }
    __riscv_vse64_v_f64m1(&C[C_IDX(i,j,n)],c,vl);
    i += (long)vl;
  }
}
#else
#error "Define MTYPE_IS_FLOAT or MTYPE_IS_DOUBLE"
#endif
#else
static inline void rvv8_kc(const MTYPE * RESTRICT A,const MTYPE * RESTRICT B,
                           MTYPE * RESTRICT C,long n,long j,long k0,long k1)
{
  for(long jj=0;jj<8;++jj) for(long i=0;i<n;++i) {
    MTYPE c=C[C_IDX(i,j+jj,n)];
    for(long k=k0;k<k1;++k) c+=A[A_IDX(i,k,n)]*B[B_IDX(k,j+jj,n)];
    C[C_IDX(i,j+jj,n)]=c;
  }
}
static inline void rvv1_kc(const MTYPE * RESTRICT A,const MTYPE * RESTRICT B,
                           MTYPE * RESTRICT C,long n,long j,long k0,long k1)
{
  for(long i=0;i<n;++i) {
    MTYPE c=C[C_IDX(i,j,n)];
    for(long k=k0;k<k1;++k) c+=A[A_IDX(i,k,n)]*B[B_IDX(k,j,n)];
    C[C_IDX(i,j,n)]=c;
  }
}
#endif

int main(int argc,char **argv)
{
  int iterations,nthread_input,nthread=0,shortcut,num_error=0;
  long order;
  int kc;
  double gemm_time=0.0,avgtime,checksum=0.0,ref_checksum;
#if defined(MTYPE_IS_FLOAT)
  const double epsilon=1.e-4;
#else
  const double epsilon=1.e-8;
#endif
  MTYPE * RESTRICT A,* RESTRICT B,* RESTRICT C;

  printf("Parallel Research Kernels version %s\n",PRKVERSION);
  printf("OpenMP optimized RVV GEMM - SpaceMiT A100\n");
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
  shortcut=(order<0); if(shortcut) order=-order;
  if(order<1) exit(EXIT_FAILURE);
  kc=(argc==5)?atoi(*++argv):RVV_OPT_KC;
  if(kc<=0) kc=RVV_OPT_KC;

  A=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  B=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  C=(MTYPE*)prk_malloc(order*order*sizeof(MTYPE));
  if(!A||!B||!C) exit(EXIT_FAILURE);

  const double fn=(double)order;
  ref_checksum=0.25*fn*fn*fn*(fn-1.0)*(fn-1.0);
  #pragma omp parallel for schedule(static)
  for(long j=0;j<order;++j) for(long i=0;i<order;++i) {
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
      printf("K blocking (KC)       = %d\n",kc);
      printf("Number of iterations  = %d\n",iterations);
    }
  }
  bail_out(num_error);
  if(shortcut) exit(EXIT_SUCCESS);

  for(int iter=0;iter<=iterations;++iter) {
    if(iter==1) gemm_time=wtime();
    #pragma omp parallel for schedule(static)
    for(long j=0;j<order;j+=NR) {
      const long nr=MIN((long)NR,order-j);
      for(long kk=0;kk<order;kk+=kc) {
        const long k1=MIN(kk+(long)kc,order);
        if(nr==NR) rvv8_kc(A,B,C,order,j,kk,k1);
        else for(long jj=0;jj<nr;++jj) rvv1_kc(A,B,C,order,j+jj,kk,k1);
      }
    }
  }
  gemm_time=wtime()-gemm_time;

  for(long j=0;j<order;++j) for(long i=0;i<order;++i)
    checksum+=C[C_IDX(i,j,order)];
  ref_checksum*=(iterations+1);
  const double err=ABS(checksum-ref_checksum);
  if((ref_checksum==0.0 && err>epsilon) ||
     (ref_checksum!=0.0 && err/ABS(ref_checksum)>epsilon)) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n",checksum,ref_checksum);
    exit(EXIT_FAILURE);
  }
  printf("Solution validates\n");
  const double nflops=2.0*fn*fn*fn;
  avgtime=gemm_time/iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",1.e-6*nflops/avgtime,avgtime);
  prk_free(A);prk_free(B);prk_free(C);
  return 0;
}
