# GEMM Performance Summary

Parsed 330 result files. Valid: 290. Non-valid: 40.

## Top 30 fp64 Runs By MFlops/s

| Rank | MFlops/s | Family | Mode | Compiler | Precision | Core | Variant | LMUL | Threads | A100 Share | File |
|---:|---:|---|---|---|---|---|---|---|---|---|---|
| 1 | 20751.196121 | base | autovec_lmul | clang | double |  | autovec_4 | 4 | 8 |  | `gemm_clang_double_autovec_4_t8_n2000.txt` |
| 2 | 18115.441722 | base | autovec_lmul | gcc | double |  | autovec_4 | 4 | 8 |  | `gemm_gcc_double_autovec_4_t8_n2000.txt` |
| 3 | 15694.939287 | base | autovec | clang | double |  | autovec |  | 8 |  | `gemm_clang_double_autovec_t8_n2000.txt` |
| 4 | 15655.087141 | base | autovec_lmul | clang | double |  | autovec_2 | 2 | 8 |  | `gemm_clang_double_autovec_2_t8_n2000.txt` |
| 5 | 14897.842713 | base | autovec_lmul | gcc | double |  | autovec_2 | 2 | 8 |  | `gemm_gcc_double_autovec_2_t8_n2000.txt` |
| 6 | 9923.268370 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share70_n2000.txt` |
| 7 | 9921.432779 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share70_n2000.txt` |
| 8 | 9742.327551 | base | autovec_lmul | clang | double |  | autovec_8 | 8 | 8 |  | `gemm_clang_double_autovec_8_t8_n2000.txt` |
| 9 | 9640.647854 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 60 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share60_n2000.txt` |
| 10 | 9497.333880 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 80 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share80_n2000.txt` |
| 11 | 9480.888749 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 60 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share60_n2000.txt` |
| 12 | 9379.096253 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 80 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share80_n2000.txt` |
| 13 | 9198.092576 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 90 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share90_n2000.txt` |
| 14 | 9081.410287 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 50 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share50_n2000.txt` |
| 15 | 8940.212162 | base | autovec_lmul | gcc | double |  | autovec_1 | 1 | 8 |  | `gemm_gcc_double_autovec_1_t8_n2000.txt` |
| 16 | 8927.330433 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 100 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share100_n2000.txt` |
| 17 | 8894.718364 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 90 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share90_n2000.txt` |
| 18 | 8882.366190 | base | autovec_lmul | clang | double |  | autovec_1 | 1 | 8 |  | `gemm_clang_double_autovec_1_t8_n2000.txt` |
| 19 | 8842.857871 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 100 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share100_n2000.txt` |
| 20 | 8830.515356 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 50 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share50_n2000.txt` |
| 21 | 8527.486864 | rvv | rvv | clang | double | a100 | rvv_m8 | 8 | 8 |  | `gemm_rvv_8_a100_clang_double_t8_n2000.txt` |
| 22 | 8474.675426 | rvv | rvv | gcc | double | a100 | rvv_m8 | 8 | 8 |  | `gemm_rvv_8_a100_gcc_double_t8_n2000.txt` |
| 23 | 8325.819664 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 40 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share40_n2000.txt` |
| 24 | 8216.763129 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 40 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share40_n2000.txt` |
| 25 | 8055.658569 | base | nonvec | clang | double |  | nonvec |  | 8 |  | `gemm_clang_double_nonvec_t8_n2000.txt` |
| 26 | 7862.121892 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m4 | 4 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_4_clang_double_x8_a8_share70_n2000.txt` |
| 27 | 7846.809905 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m8 | 8 | x8/a8 | 30 | `gemm_hetero_rvv_rvv_8_clang_double_x8_a8_share30_n2000.txt` |
| 28 | 7832.076778 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m4 | 4 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_4_gcc_double_x8_a8_share70_n2000.txt` |
| 29 | 7813.406864 | hetero | rvv_rvv | gcc | double |  | rvv_rvv_m8 | 8 | x8/a8 | 30 | `gemm_hetero_rvv_rvv_8_gcc_double_x8_a8_share30_n2000.txt` |
| 30 | 7793.544297 | hetero | rvv_rvv | clang | double |  | rvv_rvv_m4 | 4 | x8/a8 | 60 | `gemm_hetero_rvv_rvv_4_clang_double_x8_a8_share60_n2000.txt` |

## Top 30 fp32 Runs By MFlops/s

| Rank | MFlops/s | Family | Mode | Compiler | Precision | Core | Variant | LMUL | Threads | A100 Share | File |
|---:|---:|---|---|---|---|---|---|---|---|---|---|
| 1 | 28517.342501 | base | autovec | clang | float |  | autovec |  | 8 |  | `gemm_clang_float_autovec_t8_n2000.txt` |
| 2 | 28469.158462 | base | autovec_lmul | clang | float |  | autovec_2 | 2 | 8 |  | `gemm_clang_float_autovec_2_t8_n2000.txt` |
| 3 | 23075.124021 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share70_n2000.txt` |
| 4 | 22155.451233 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share70_n2000.txt` |
| 5 | 21424.504320 | base | autovec_lmul | gcc | float |  | autovec_2 | 2 | 8 |  | `gemm_gcc_float_autovec_2_t8_n2000.txt` |
| 6 | 20988.703845 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 60 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share60_n2000.txt` |
| 7 | 20689.449740 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 60 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share60_n2000.txt` |
| 8 | 20634.072316 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 80 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share80_n2000.txt` |
| 9 | 20135.600609 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 80 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share80_n2000.txt` |
| 10 | 19922.220979 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 50 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share50_n2000.txt` |
| 11 | 19846.049304 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m4 | 4 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_4_gcc_float_x8_a8_share70_n2000.txt` |
| 12 | 19333.234961 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 90 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share90_n2000.txt` |
| 13 | 19297.871119 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 50 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share50_n2000.txt` |
| 14 | 19262.227825 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m4 | 4 | x8/a8 | 60 | `gemm_hetero_rvv_rvv_4_clang_float_x8_a8_share60_n2000.txt` |
| 15 | 19184.456685 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 90 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share90_n2000.txt` |
| 16 | 19061.177788 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m4 | 4 | x8/a8 | 60 | `gemm_hetero_rvv_rvv_4_gcc_float_x8_a8_share60_n2000.txt` |
| 17 | 19039.452308 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m4 | 4 | x8/a8 | 70 | `gemm_hetero_rvv_rvv_4_clang_float_x8_a8_share70_n2000.txt` |
| 18 | 18561.368627 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m4 | 4 | x8/a8 | 80 | `gemm_hetero_rvv_rvv_4_gcc_float_x8_a8_share80_n2000.txt` |
| 19 | 18512.585281 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m4 | 4 | x8/a8 | 50 | `gemm_hetero_rvv_rvv_4_clang_float_x8_a8_share50_n2000.txt` |
| 20 | 18373.820627 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m4 | 4 | x8/a8 | 50 | `gemm_hetero_rvv_rvv_4_gcc_float_x8_a8_share50_n2000.txt` |
| 21 | 18373.820627 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 40 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share40_n2000.txt` |
| 22 | 18045.105730 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 40 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share40_n2000.txt` |
| 23 | 17783.409301 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m4 | 4 | x8/a8 | 80 | `gemm_hetero_rvv_rvv_4_clang_float_x8_a8_share80_n2000.txt` |
| 24 | 17398.768745 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m4 | 4 | x8/a8 | 40 | `gemm_hetero_rvv_rvv_4_clang_float_x8_a8_share40_n2000.txt` |
| 25 | 17153.167986 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m4 | 4 | x8/a8 | 40 | `gemm_hetero_rvv_rvv_4_gcc_float_x8_a8_share40_n2000.txt` |
| 26 | 17052.698941 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 100 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share100_n2000.txt` |
| 27 | 17044.146942 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 100 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share100_n2000.txt` |
| 28 | 16940.293001 | hetero | rvv_rvv | gcc | float |  | rvv_rvv_m8 | 8 | x8/a8 | 30 | `gemm_hetero_rvv_rvv_8_gcc_float_x8_a8_share30_n2000.txt` |
| 29 | 16860.996606 | hetero | rvv_rvv | clang | float |  | rvv_rvv_m8 | 8 | x8/a8 | 30 | `gemm_hetero_rvv_rvv_8_clang_float_x8_a8_share30_n2000.txt` |
| 30 | 16633.896270 | base | autovec_lmul | clang | float |  | autovec_1 | 1 | 8 |  | `gemm_clang_float_autovec_1_t8_n2000.txt` |

## Top 30 fp16 Runs By MFlops/s

| Rank | MFlops/s | Family | Mode | Compiler | Precision | Core | Variant | LMUL | Threads | A100 Share | File |
|---:|---:|---|---|---|---|---|---|---|---|---|---|
| 1 | 18449.546043 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 | x8/a8 | 50 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share50_n2000.txt` |
| 2 | 18429.907850 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 50 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share50_n2000.txt` |
| 3 | 18424.500371 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 50 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share50_n2000.txt` |
| 4 | 16247.219514 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 40 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share40_n2000.txt` |
| 5 | 16072.987688 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 | x8/a8 | 40 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share40_n2000.txt` |
| 6 | 15634.828343 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 | x8/a8 | 50 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share50_n2000.txt` |
| 7 | 15587.181457 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 | x8/a8 | 60 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share60_n2000.txt` |
| 8 | 15571.071462 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 60 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share60_n2000.txt` |
| 9 | 15467.866870 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 60 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share60_n2000.txt` |
| 10 | 14915.079841 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 30 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share30_n2000.txt` |
| 11 | 14710.728283 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 20 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share20_n2000.txt` |
| 12 | 14584.592754 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 | x8/a8 | 60 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share60_n2000.txt` |
| 13 | 14567.374433 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 | x8/a8 | 30 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share30_n2000.txt` |
| 14 | 14151.960764 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 30 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share30_n2000.txt` |
| 15 | 14085.307996 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 40 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share40_n2000.txt` |
| 16 | 14022.866917 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 | x8/a8 | 20 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share20_n2000.txt` |
| 17 | 13478.483249 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 70 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share70_n2000.txt` |
| 18 | 13461.123163 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 | x8/a8 | 70 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share70_n2000.txt` |
| 19 | 13447.240529 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 70 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share70_n2000.txt` |
| 20 | 13434.837911 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 | x8/a8 | 70 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share70_n2000.txt` |
| 21 | 13098.604041 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 20 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share20_n2000.txt` |
| 22 | 13036.992739 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 | x8/a8 | 20 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share20_n2000.txt` |
| 23 | 12783.763785 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 0 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share0_n2000.txt` |
| 24 | 12723.021652 | hetero | rvv_ime | gcc | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 0 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share0_n2000.txt` |
| 25 | 12601.283805 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 | x8/a8 | 0 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share0_n2000.txt` |
| 26 | 12486.381037 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 | x8/a8 | 40 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share40_n2000.txt` |
| 27 | 12448.702769 | ime | ime | gcc | fp16 | a100 | ime |  | 8 |  | `gemm_ime_gcc_fp16_t8_n2000.txt` |
| 28 | 12448.473004 | hetero | rvv_ime | gcc | fp16 |  | rvv_ime_m8 | 8 | x8/a8 | 0 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share0_n2000.txt` |
| 29 | 12371.719632 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 | x8/a8 | 30 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share30_n2000.txt` |
| 30 | 12139.254493 | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 | x8/a8 | 10 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share10_n2000.txt` |


## Non-Valid Runs

| Status | Family | Mode | Compiler | Precision | Variant | A100 Share | File |
|---|---|---|---|---|---|---|---|
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 100 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 10 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 20 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 30 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 40 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 50 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 60 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 70 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 80 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 90 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share90_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 100 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 10 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 20 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 30 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 40 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 50 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 60 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 70 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 80 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 90 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share90_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 100 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 10 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 20 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 30 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 40 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 50 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 60 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 70 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 80 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 90 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share90_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 100 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 10 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 20 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 30 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 40 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 50 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 60 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 70 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 80 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 90 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share90_n2000.txt` |
