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

#include <par-res-kern_general.h>
#include <par-res-kern_omp.h>

#if defined(__riscv)
#include <riscv_vector.h>
#endif

#ifndef MTYPE
#define MTYPE double
#endif

#ifndef RVV_LMUL_VALUE
#define RVV_LMUL_VALUE 1
#endif

#define A_arr(i, j) A[(i) + order * (j)]
#define B_arr(i, j) B[(i) + order * (j)]
#define C_arr(i, j) C[(i) + order * (j)]

static inline void rvv_macc_column(const MTYPE * RESTRICT A,
                                   const MTYPE * RESTRICT B,
                                   MTYPE * RESTRICT C,
                                   long order,
                                   long j,
                                   long k)
{
  long i = 0;
  MTYPE b = B_arr(k, j);

#if defined(__riscv) && defined(MTYPE_IS_FLOAT)
#if RVV_LMUL_VALUE == 1
  while (i < order) {
    size_t vl = __riscv_vsetvl_e32m1((size_t)(order - i));
    vfloat32m1_t c_vec = __riscv_vle32_v_f32m1(&C_arr(i, j), vl);
    vfloat32m1_t a_vec = __riscv_vle32_v_f32m1(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f32m1(c_vec, b, a_vec, vl);
    __riscv_vse32_v_f32m1(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#elif RVV_LMUL_VALUE == 2
  while (i < order) {
    size_t vl = __riscv_vsetvl_e32m2((size_t)(order - i));
    vfloat32m2_t c_vec = __riscv_vle32_v_f32m2(&C_arr(i, j), vl);
    vfloat32m2_t a_vec = __riscv_vle32_v_f32m2(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f32m2(c_vec, b, a_vec, vl);
    __riscv_vse32_v_f32m2(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#elif RVV_LMUL_VALUE == 4
  while (i < order) {
    size_t vl = __riscv_vsetvl_e32m4((size_t)(order - i));
    vfloat32m4_t c_vec = __riscv_vle32_v_f32m4(&C_arr(i, j), vl);
    vfloat32m4_t a_vec = __riscv_vle32_v_f32m4(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f32m4(c_vec, b, a_vec, vl);
    __riscv_vse32_v_f32m4(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#elif RVV_LMUL_VALUE == 8
  while (i < order) {
    size_t vl = __riscv_vsetvl_e32m8((size_t)(order - i));
    vfloat32m8_t c_vec = __riscv_vle32_v_f32m8(&C_arr(i, j), vl);
    vfloat32m8_t a_vec = __riscv_vle32_v_f32m8(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f32m8(c_vec, b, a_vec, vl);
    __riscv_vse32_v_f32m8(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#else
#error "RVV_LMUL_VALUE must be 1, 2, 4, or 8"
#endif
#elif defined(__riscv) && defined(MTYPE_IS_DOUBLE)
#if RVV_LMUL_VALUE == 1
  while (i < order) {
    size_t vl = __riscv_vsetvl_e64m1((size_t)(order - i));
    vfloat64m1_t c_vec = __riscv_vle64_v_f64m1(&C_arr(i, j), vl);
    vfloat64m1_t a_vec = __riscv_vle64_v_f64m1(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f64m1(c_vec, b, a_vec, vl);
    __riscv_vse64_v_f64m1(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#elif RVV_LMUL_VALUE == 2
  while (i < order) {
    size_t vl = __riscv_vsetvl_e64m2((size_t)(order - i));
    vfloat64m2_t c_vec = __riscv_vle64_v_f64m2(&C_arr(i, j), vl);
    vfloat64m2_t a_vec = __riscv_vle64_v_f64m2(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f64m2(c_vec, b, a_vec, vl);
    __riscv_vse64_v_f64m2(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#elif RVV_LMUL_VALUE == 4
  while (i < order) {
    size_t vl = __riscv_vsetvl_e64m4((size_t)(order - i));
    vfloat64m4_t c_vec = __riscv_vle64_v_f64m4(&C_arr(i, j), vl);
    vfloat64m4_t a_vec = __riscv_vle64_v_f64m4(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f64m4(c_vec, b, a_vec, vl);
    __riscv_vse64_v_f64m4(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#elif RVV_LMUL_VALUE == 8
  while (i < order) {
    size_t vl = __riscv_vsetvl_e64m8((size_t)(order - i));
    vfloat64m8_t c_vec = __riscv_vle64_v_f64m8(&C_arr(i, j), vl);
    vfloat64m8_t a_vec = __riscv_vle64_v_f64m8(&A_arr(i, k), vl);
    c_vec = __riscv_vfmacc_vf_f64m8(c_vec, b, a_vec, vl);
    __riscv_vse64_v_f64m8(&C_arr(i, j), c_vec, vl);
    i += (long)vl;
  }
#else
#error "RVV_LMUL_VALUE must be 1, 2, 4, or 8"
#endif
#else
  for (; i < order; i++)
    C_arr(i, j) += A_arr(i, k) * b;
#endif
}

int main(int argc, char **argv)
{
  int iterations;
  int nthread_input;
  int nthread;
  int block;
  int shortcut;
  int num_error = 0;
  long order;
  double forder;
  double gemm_time;
  double avgtime;
  double checksum = 0.0;
  double ref_checksum;
  double checksum_error;
#if defined(MTYPE_IS_FLOAT)
  double epsilon = 1.e-4;
#else
  double epsilon = 1.e-8;
#endif
  MTYPE * RESTRICT A;
  MTYPE * RESTRICT B;
  MTYPE * RESTRICT C;

  printf("Parallel Research Kernels version %s\n", PRKVERSION);
  printf("OpenMP RVV intrinsic dense matrix-matrix multiplication\n");

  if (argc != 4 && argc != 5) {
    printf("Usage: %s <# threads> <# iterations> <matrix order> [tile size]\n", *argv);
    exit(EXIT_FAILURE);
  }

  nthread_input = atoi(*++argv);
  if ((nthread_input < 1) || (nthread_input > MAX_THREADS)) {
    printf("ERROR: Invalid number of threads: %d\n", nthread_input);
    exit(EXIT_FAILURE);
  }
  omp_set_num_threads(nthread_input);

  iterations = atoi(*++argv);
  if (iterations < 1) {
    printf("ERROR: Iterations must be positive: %d\n", iterations);
    exit(EXIT_FAILURE);
  }

  order = atol(*++argv);
  if (order < 0) {
    shortcut = 1;
    order = -order;
  }
  else shortcut = 0;
  if (order < 1) {
    printf("ERROR: Matrix order must be positive: %ld\n", order);
    exit(EXIT_FAILURE);
  }
  forder = 1.0 * order;

  if (argc == 5) block = atoi(*++argv);
  else block = DEFAULTBLOCK;

  A = (MTYPE *)prk_malloc(order * order * sizeof(MTYPE));
  B = (MTYPE *)prk_malloc(order * order * sizeof(MTYPE));
  C = (MTYPE *)prk_malloc(order * order * sizeof(MTYPE));
  if (!A || !B || !C) {
    printf("ERROR: Could not allocate space for global matrices\n");
    exit(EXIT_FAILURE);
  }

  ref_checksum = 0.25 * forder * forder * forder *
                 (forder - 1.0) * (forder - 1.0);

  #pragma omp parallel for
  for (long j = 0; j < order; j++) {
    for (long i = 0; i < order; i++) {
      A_arr(i, j) = B_arr(i, j) = (MTYPE)j;
      C_arr(i, j) = (MTYPE)0.0;
    }
  }

  #pragma omp parallel
  {
    #pragma omp master
    {
      nthread = omp_get_num_threads();
      if (nthread != nthread_input) {
        num_error = 1;
        printf("ERROR: number of requested threads %d does not equal ", nthread_input);
        printf("number of spawned threads %d\n", nthread);
      }
      else {
        printf("Matrix order          = %ld\n", order);
        if (shortcut) printf("Only doing initialization\n");
        printf("Number of threads     = %d\n", nthread_input);
        printf("Blocking factor       = %d\n", block);
        printf("Number of iterations  = %d\n", iterations);
#if defined(MTYPE_IS_FLOAT)
        printf("Precision             = float\n");
#else
        printf("Precision             = double\n");
#endif
        printf("RVV LMUL              = m%d\n", RVV_LMUL_VALUE);
      }
    }
    bail_out(num_error);
  }

  if (shortcut) exit(EXIT_SUCCESS);

  for (int iter = 0; iter <= iterations; iter++) {
    if (iter == 1) gemm_time = wtime();

    #pragma omp parallel for schedule(static)
    for (long j = 0; j < order; j++)
      for (long k = 0; k < order; k++)
        rvv_macc_column(A, B, C, order, j, k);
  }
  gemm_time = wtime() - gemm_time;

  for (long j = 0; j < order; j++)
    for (long i = 0; i < order; i++)
      checksum += C_arr(i, j);

  ref_checksum *= (iterations + 1);
  checksum_error = ABS(checksum - ref_checksum);
  if ((ref_checksum == 0.0 && checksum_error > epsilon) ||
      (ref_checksum != 0.0 && checksum_error / ABS(ref_checksum) > epsilon)) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n",
           checksum, ref_checksum);
    exit(EXIT_FAILURE);
  }

  printf("Solution validates\n");

  double nflops = 2.0 * forder * forder * forder;
  avgtime = gemm_time / iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
         1.0E-06 * nflops / avgtime, avgtime);

  prk_free(A);
  prk_free(B);
  prk_free(C);

  exit(EXIT_SUCCESS);
}
