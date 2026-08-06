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

#define TILE 8
#define A_arr(i, j) A[(i) * order + (j)]
#define B_arr(i, j) B[(i) * order + (j)]
#define C_arr(i, j) C[(i) * order + (j)]

typedef _Float16 fp16_t;

static inline fp16_t init_a(long k)
{
  return (fp16_t)((int)(k % 13) - 6);
}

static inline fp16_t init_b(long k)
{
  return (fp16_t)((int)(k % 7) - 3);
}

static double reference_checksum(long order, int iterations)
{
  double dot = 0.0;

  for (long k = 0; k < order; k++)
    dot += (double)init_a(k) * (double)init_b(k);

  return dot * (double)order * (double)order * (double)(iterations + 1);
}

static inline void scalar_macc_8x8x8(const fp16_t * RESTRICT a,
                                    const fp16_t * RESTRICT b,
                                    float * RESTRICT c)
{
  for (int kk = 0; kk < TILE; kk++)
    for (int jj = 0; jj < TILE; jj++)
      for (int ii = 0; ii < TILE; ii++)
        c[ii * TILE + jj] += (float)a[ii * TILE + kk] *
                             (float)b[jj * TILE + kk];
}

static inline void ime_macc_8x8x8(const fp16_t * RESTRICT a,
                                  const fp16_t * RESTRICT b,
                                  float * RESTRICT c)
{
#if defined(USE_IME_ASM) && defined(__riscv)
  __asm__ volatile(
      "vsetvli        t0, zero, e32, m2        \n\t"
      "vle32.v        v16, (%[c])              \n\t"
      "vsetvli        t0, zero, e16, m1        \n\t"
      "vle16.v        v8,  (%[a])              \n\t"
      "vle16.v        v10, (%[b])              \n\t"
      "smt.vfwmadot   v16, v8, v10             \n\t"
      "vsetvli        t0, zero, e32, m2        \n\t"
      "vse32.v        v16, (%[c])              \n\t"
      :
      : [a] "r"(a), [b] "r"(b), [c] "r"(c)
      : "memory", "t0");
#else
  scalar_macc_8x8x8(a, b, c);
#endif
}

static void ime_tile(const fp16_t * RESTRICT A,
                     const fp16_t * RESTRICT B,
                     float * RESTRICT C,
                     long order,
                     long row,
                     long col)
{
  fp16_t a_tile[TILE * TILE] __attribute__((aligned(64)));
  fp16_t b_tile[TILE * TILE] __attribute__((aligned(64)));
  float c_tile[TILE * TILE] __attribute__((aligned(64)));
  long k;

  for (int jj = 0; jj < TILE; jj++)
    for (int ii = 0; ii < TILE; ii++)
      c_tile[ii * TILE + jj] = C_arr(row + ii, col + jj);

  for (k = 0; k + TILE <= order; k += TILE) {
    for (int kk = 0; kk < TILE; kk++)
      for (int ii = 0; ii < TILE; ii++)
        a_tile[ii * TILE + kk] = A_arr(row + ii, k + kk);

    for (int jj = 0; jj < TILE; jj++)
      for (int kk = 0; kk < TILE; kk++)
        b_tile[jj * TILE + kk] = B_arr(k + kk, col + jj);

    ime_macc_8x8x8(a_tile, b_tile, c_tile);
  }

  for (; k < order; k++)
    for (int jj = 0; jj < TILE; jj++)
      for (int ii = 0; ii < TILE; ii++)
        c_tile[ii * TILE + jj] += (float)A_arr(row + ii, k) *
                                  (float)B_arr(k, col + jj);

  for (int jj = 0; jj < TILE; jj++)
    for (int ii = 0; ii < TILE; ii++)
      C_arr(row + ii, col + jj) = c_tile[ii * TILE + jj];
}

static void scalar_tile(const fp16_t * RESTRICT A,
                        const fp16_t * RESTRICT B,
                        float * RESTRICT C,
                        long order,
                        long row,
                        long col,
                        long rows,
                        long cols)
{
  for (long k = 0; k < order; k++)
    for (long jj = 0; jj < cols; jj++)
      for (long ii = 0; ii < rows; ii++)
        C_arr(row + ii, col + jj) += (float)A_arr(row + ii, k) *
                                     (float)B_arr(k, col + jj);
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
  double gemm_time;
  double avgtime;
  double checksum = 0.0;
  double ref_checksum;
  double checksum_error;
  double epsilon = 1.e-3;
  fp16_t * RESTRICT A;
  fp16_t * RESTRICT B;
  float * RESTRICT C;

  printf("Parallel Research Kernels version %s\n", PRKVERSION);
  printf("OpenMP FP16 IME dense matrix-matrix multiplication\n");

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

  if (argc == 5) block = atoi(*++argv);
  else block = TILE;
  if (block != TILE)
    printf("WARNING: IME kernel uses fixed %dx%d tiles; ignoring tile size %d\n",
           TILE, TILE, block);

  A = (fp16_t *)prk_malloc(order * order * sizeof(fp16_t));
  B = (fp16_t *)prk_malloc(order * order * sizeof(fp16_t));
  C = (float *)prk_malloc(order * order * sizeof(float));
  if (!A || !B || !C) {
    printf("ERROR: Could not allocate space for matrices\n");
    exit(EXIT_FAILURE);
  }

  ref_checksum = reference_checksum(order, iterations);

  #pragma omp parallel for
  for (long i = 0; i < order; i++) {
    for (long j = 0; j < order; j++) {
      A_arr(i, j) = init_a(j);
      B_arr(i, j) = init_b(i);
      C_arr(i, j) = 0.0f;
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
        printf("IME tile size         = %d\n", TILE);
        printf("Number of iterations  = %d\n", iterations);
      }
    }
    bail_out(num_error);
  }

  if (shortcut) exit(EXIT_SUCCESS);

  for (int iter = 0; iter <= iterations; iter++) {
    if (iter == 1) gemm_time = wtime();

    #pragma omp parallel for collapse(2) schedule(static)
    for (long col = 0; col < order; col += TILE) {
      for (long row = 0; row < order; row += TILE) {
        long rows = MIN((long)TILE, order - row);
        long cols = MIN((long)TILE, order - col);

        if (rows == TILE && cols == TILE)
          ime_tile(A, B, C, order, row, col);
        else
          scalar_tile(A, B, C, order, row, col, rows, cols);
      }
    }
  }
  gemm_time = wtime() - gemm_time;

  for (long j = 0; j < order; j++)
    for (long i = 0; i < order; i++)
      checksum += C_arr(i, j);

  checksum_error = ABS(checksum - ref_checksum);
  if ((ref_checksum == 0.0 && checksum_error > epsilon) ||
      (ref_checksum != 0.0 && checksum_error / ABS(ref_checksum) > epsilon)) {
    printf("ERROR: Checksum = %lf, Reference checksum = %lf\n",
           checksum, ref_checksum);
    exit(EXIT_FAILURE);
  }

  printf("Solution validates\n");

  double nflops = 2.0 * (double)order * (double)order * (double)order;
  avgtime = gemm_time / iterations;
  printf("Rate (MFlops/s): %lf  Avg time (s): %lf\n",
         1.0E-06 * nflops / avgtime, avgtime);

  prk_free(A);
  prk_free(B);
  prk_free(C);

  exit(EXIT_SUCCESS);
}
