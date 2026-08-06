/* X100 RVV FP16-input, FP32-accumulate kernel for hetero RVV+IME mode. */

#include <par-res-kern_general.h>

#if defined(__riscv)
#include <riscv_vector.h>
#endif

#define TILE 8
#define A_arr(i, j) A[(i) * order + (j)]
#define B_arr(i, j) B[(i) * order + (j)]
#define C_arr(i, j) C[(i) * order + (j)]

typedef _Float16 fp16_t;

#ifndef RVV_LMUL_VALUE
#define RVV_LMUL_VALUE 1
#endif

static inline void rvv_fp16_macc_row_cols(const fp16_t * RESTRICT A,
                                          const fp16_t * RESTRICT B,
                                          float * RESTRICT C,
                                          long order,
                                          long row,
                                          long k,
                                          long col_begin,
                                          long col_end)
{
  long col = col_begin;
  float a = (float)A_arr(row, k);

#if defined(__riscv)
#if RVV_LMUL_VALUE == 1
  while (col < col_end) {
    size_t vl = __riscv_vsetvl_e32m1((size_t)(col_end - col));
    vfloat16mf2_t b16 = __riscv_vle16_v_f16mf2(&B_arr(k, col), vl);
    vfloat32m1_t c = __riscv_vle32_v_f32m1(&C_arr(row, col), vl);
    vfloat32m1_t b = __riscv_vfwcvt_f_f_v_f32m1(b16, vl);
    c = __riscv_vfmacc_vf_f32m1(c, a, b, vl);
    __riscv_vse32_v_f32m1(&C_arr(row, col), c, vl);
    col += (long)vl;
  }
#elif RVV_LMUL_VALUE == 2
  while (col < col_end) {
    size_t vl = __riscv_vsetvl_e32m2((size_t)(col_end - col));
    vfloat16m1_t b16 = __riscv_vle16_v_f16m1(&B_arr(k, col), vl);
    vfloat32m2_t c = __riscv_vle32_v_f32m2(&C_arr(row, col), vl);
    vfloat32m2_t b = __riscv_vfwcvt_f_f_v_f32m2(b16, vl);
    c = __riscv_vfmacc_vf_f32m2(c, a, b, vl);
    __riscv_vse32_v_f32m2(&C_arr(row, col), c, vl);
    col += (long)vl;
  }
#elif RVV_LMUL_VALUE == 4
  while (col < col_end) {
    size_t vl = __riscv_vsetvl_e32m4((size_t)(col_end - col));
    vfloat16m2_t b16 = __riscv_vle16_v_f16m2(&B_arr(k, col), vl);
    vfloat32m4_t c = __riscv_vle32_v_f32m4(&C_arr(row, col), vl);
    vfloat32m4_t b = __riscv_vfwcvt_f_f_v_f32m4(b16, vl);
    c = __riscv_vfmacc_vf_f32m4(c, a, b, vl);
    __riscv_vse32_v_f32m4(&C_arr(row, col), c, vl);
    col += (long)vl;
  }
#elif RVV_LMUL_VALUE == 8
  while (col < col_end) {
    size_t vl = __riscv_vsetvl_e32m8((size_t)(col_end - col));
    vfloat16m4_t b16 = __riscv_vle16_v_f16m4(&B_arr(k, col), vl);
    vfloat32m8_t c = __riscv_vle32_v_f32m8(&C_arr(row, col), vl);
    vfloat32m8_t b = __riscv_vfwcvt_f_f_v_f32m8(b16, vl);
    c = __riscv_vfmacc_vf_f32m8(c, a, b, vl);
    __riscv_vse32_v_f32m8(&C_arr(row, col), c, vl);
    col += (long)vl;
  }
#else
#error "RVV_LMUL_VALUE must be 1, 2, 4, or 8"
#endif
#else
  for (; col < col_end; col++)
    C_arr(row, col) += a * (float)B_arr(k, col);
#endif
}

void gemm_hetero_fp16_x100(const fp16_t * RESTRICT A,
                           const fp16_t * RESTRICT B,
                           float * RESTRICT C,
                           long order,
                           long col_begin,
                           long col_end,
                           int workers,
                           int worker_id)
{
  long total_cols = col_end - col_begin;
  long begin = col_begin + (total_cols * worker_id) / workers;
  long end = col_begin + (total_cols * (worker_id + 1)) / workers;

  for (long row = 0; row < order; row++)
    for (long k = 0; k < order; k++)
      rvv_fp16_macc_row_cols(A, B, C, order, row, k, begin, end);
}
