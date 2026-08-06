/*
 * gemm_hetero_fp16_x100_kernel.c
 *
 * Optimized X100 worker for heterogeneous RVV+IME GEMM.
 *
 * Data layout matches the IME path:
 *   A, B : row-major FP16
 *   C    : row-major FP32
 *
 * Microkernel:
 *   MR rows x VL columns, default MR=8
 *   - C vectors are loaded once and held across the entire K dimension.
 *   - one contiguous FP16 B vector is widened to FP32 for each K.
 *   - that B vector is reused by MR independent FP32 accumulators.
 *   - C vectors are stored once after all K.
 *
 * This removes the baseline hetero kernel's load-C/store-C on every K.
 */

#include <par-res-kern_general.h>

#if defined(__riscv)
#include <riscv_vector.h>
#endif

typedef _Float16 fp16_t;

#ifndef HETERO_X100_MR
#define HETERO_X100_MR 8
#endif

#if HETERO_X100_MR != 8
#error "This first optimized hetero FP16 X100 kernel currently uses MR=8"
#endif

#define A_IDX(i,j,n) ((i)*(n)+(j))
#define B_IDX(i,j,n) ((i)*(n)+(j))
#define C_IDX(i,j,n) ((i)*(n)+(j))

static inline void scalar_rows_cols(const fp16_t * RESTRICT A,
                                    const fp16_t * RESTRICT B,
                                    float * RESTRICT C,
                                    long n,
                                    long row,
                                    int mr,
                                    long col,
                                    long cols)
{
  for (int r=0; r<mr; ++r) {
    float *c = &C[C_IDX(row+r,col,n)];
    for (long j=0;j<cols;++j) {
      float sum = c[j];
      for (long k=0;k<n;++k)
        sum += (float)A[A_IDX(row+r,k,n)] *
               (float)B[B_IDX(k,col+j,n)];
      c[j] = sum;
    }
  }
}

#if defined(__riscv)
static inline void rvv_mr8_panel(const fp16_t * RESTRICT A,
                                 const fp16_t * RESTRICT B,
                                 float * RESTRICT C,
                                 long n,
                                 long row,
                                 long col,
                                 size_t vl)
{
  vfloat32m1_t c0 = __riscv_vle32_v_f32m1(&C[C_IDX(row+0,col,n)],vl);
  vfloat32m1_t c1 = __riscv_vle32_v_f32m1(&C[C_IDX(row+1,col,n)],vl);
  vfloat32m1_t c2 = __riscv_vle32_v_f32m1(&C[C_IDX(row+2,col,n)],vl);
  vfloat32m1_t c3 = __riscv_vle32_v_f32m1(&C[C_IDX(row+3,col,n)],vl);
  vfloat32m1_t c4 = __riscv_vle32_v_f32m1(&C[C_IDX(row+4,col,n)],vl);
  vfloat32m1_t c5 = __riscv_vle32_v_f32m1(&C[C_IDX(row+5,col,n)],vl);
  vfloat32m1_t c6 = __riscv_vle32_v_f32m1(&C[C_IDX(row+6,col,n)],vl);
  vfloat32m1_t c7 = __riscv_vle32_v_f32m1(&C[C_IDX(row+7,col,n)],vl);

  for (long k=0;k<n;++k) {
    /*
     * e16,mf2 has the same number of elements as e32,m1, so widening
     * gives one e32,m1 B vector for the current column panel.
     */
    vfloat16mf2_t b16 =
      __riscv_vle16_v_f16mf2(&B[B_IDX(k,col,n)],vl);
    vfloat32m1_t b =
      __riscv_vfwcvt_f_f_v_f32m1(b16,vl);

    const float a0=(float)A[A_IDX(row+0,k,n)];
    const float a1=(float)A[A_IDX(row+1,k,n)];
    const float a2=(float)A[A_IDX(row+2,k,n)];
    const float a3=(float)A[A_IDX(row+3,k,n)];
    const float a4=(float)A[A_IDX(row+4,k,n)];
    const float a5=(float)A[A_IDX(row+5,k,n)];
    const float a6=(float)A[A_IDX(row+6,k,n)];
    const float a7=(float)A[A_IDX(row+7,k,n)];

    c0=__riscv_vfmacc_vf_f32m1(c0,a0,b,vl);
    c1=__riscv_vfmacc_vf_f32m1(c1,a1,b,vl);
    c2=__riscv_vfmacc_vf_f32m1(c2,a2,b,vl);
    c3=__riscv_vfmacc_vf_f32m1(c3,a3,b,vl);
    c4=__riscv_vfmacc_vf_f32m1(c4,a4,b,vl);
    c5=__riscv_vfmacc_vf_f32m1(c5,a5,b,vl);
    c6=__riscv_vfmacc_vf_f32m1(c6,a6,b,vl);
    c7=__riscv_vfmacc_vf_f32m1(c7,a7,b,vl);
  }

  __riscv_vse32_v_f32m1(&C[C_IDX(row+0,col,n)],c0,vl);
  __riscv_vse32_v_f32m1(&C[C_IDX(row+1,col,n)],c1,vl);
  __riscv_vse32_v_f32m1(&C[C_IDX(row+2,col,n)],c2,vl);
  __riscv_vse32_v_f32m1(&C[C_IDX(row+3,col,n)],c3,vl);
  __riscv_vse32_v_f32m1(&C[C_IDX(row+4,col,n)],c4,vl);
  __riscv_vse32_v_f32m1(&C[C_IDX(row+5,col,n)],c5,vl);
  __riscv_vse32_v_f32m1(&C[C_IDX(row+6,col,n)],c6,vl);
  __riscv_vse32_v_f32m1(&C[C_IDX(row+7,col,n)],c7,vl);
}
#endif

void gemm_hetero_fp16_x100(const fp16_t * RESTRICT A,
                           const fp16_t * RESTRICT B,
                           float * RESTRICT C,
                           long order,
                           long col_begin,
                           long col_end,
                           int workers,
                           int worker_id)
{
  if (workers <= 0 || col_begin >= col_end) return;

#if defined(__riscv)
  /*
   * Determine the natural e32,m1 panel width (32 floats for VLEN=1024).
   * Divide complete vector panels contiguously among workers.
   */
  const size_t vlmax = __riscv_vsetvlmax_e32m1();
  const long np = (col_end-col_begin + (long)vlmax-1)/(long)vlmax;
  const long p0 = (np*worker_id)/workers;
  const long p1 = (np*(worker_id+1))/workers;

  for (long p=p0; p<p1; ++p) {
    const long col=col_begin+p*(long)vlmax;
    if (col>=col_end) break;
    const size_t vl=__riscv_vsetvl_e32m1((size_t)(col_end-col));

    long row=0;
    for (; row+8<=order; row+=8)
      rvv_mr8_panel(A,B,C,order,row,col,vl);

    if (row<order)
      scalar_rows_cols(A,B,C,order,row,(int)(order-row),
                       col,(long)vl);
  }
#else
  const long total=col_end-col_begin;
  const long begin=col_begin+(total*worker_id)/workers;
  const long end=col_begin+(total*(worker_id+1))/workers;
  scalar_rows_cols(A,B,C,order,0,(int)order,begin,end-begin);
#endif
}
