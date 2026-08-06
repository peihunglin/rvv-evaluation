/* A100 IME FP16-input, FP32-accumulate column-range kernel. */

#include <par-res-kern_general.h>

#define TILE 8
#define A_arr(i, j) A[(i) * order + (j)]
#define B_arr(i, j) B[(i) * order + (j)]
#define C_arr(i, j) C[(i) * order + (j)]

typedef _Float16 fp16_t;

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

void gemm_hetero_ime_a100(const fp16_t * RESTRICT A,
                          const fp16_t * RESTRICT B,
                          float * RESTRICT C,
                          long order,
                          long col_begin,
                          long col_end,
                          int workers,
                          int worker_id)
{
  for (long col = col_begin + worker_id * TILE; col < col_end; col += workers * TILE) {
    long cols = MIN((long)TILE, col_end - col);
    for (long row = 0; row < order; row += TILE) {
      long rows = MIN((long)TILE, order - row);
      if (rows == TILE && cols == TILE)
        ime_tile(A, B, C, order, row, col);
      else
        scalar_tile(A, B, C, order, row, col, rows, cols);
    }
  }
}
