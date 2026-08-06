# GEMM Performance Summary

Parsed 423 result files. Valid: 383. Non-valid: 40.

## Top 30 fp64 Runs By MFlops/s

| Rank | MFlops/s | Series | Family | Mode | Compiler | Precision | Core | Variant | LMUL | MC | NC | NR | KU | KC | Threads | A100 Share | File |
|---:|---:|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 51027.679582 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 24 | 64 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc24_nc64_nr8_ku2_kc64_t8_n2000.txt` |
| 2 | 50452.701690 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 80 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc80_t8_n2000.txt` |
| 3 | 50386.020815 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 72 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc72_t8_n2000.txt` |
| 4 | 49609.279835 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc64_t8_n2000.txt` |
| 5 | 49414.518247 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 48 | 64 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc48_nc64_nr8_ku2_kc64_t8_n2000.txt` |
| 6 | 48633.417688 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 48 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc48_nr8_ku2_kc64_t8_n2000.txt` |
| 7 | 48322.121826 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 40 | 64 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc40_nc64_nr8_ku2_kc64_t8_n2000.txt` |
| 8 | 48224.421617 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 40 | 48 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc40_nc48_nr8_ku2_kc64_t8_n2000.txt` |
| 9 | 48141.761127 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 56 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc56_t8_n2000.txt` |
| 10 | 47884.808507 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 24 | 48 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc24_nc48_nr8_ku2_kc64_t8_n2000.txt` |
| 11 | 47597.430277 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 48 | 48 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc48_nc48_nr8_ku2_kc64_t8_n2000.txt` |
| 12 | 47385.953509 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 96 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc96_t8_n2000.txt` |
| 13 | 47333.036254 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 48 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc48_t8_n2000.txt` |
| 14 | 46435.751165 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 48 | 96 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc48_nc96_nr8_ku2_kc64_t8_n2000.txt` |
| 15 | 45898.246836 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 24 | 96 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc24_nc96_nr8_ku2_kc64_t8_n2000.txt` |
| 16 | 45514.475557 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 40 | 80 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc40_nc80_nr8_ku2_kc64_t8_n2000.txt` |
| 17 | 45490.719018 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 48 | 80 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc48_nc80_nr8_ku2_kc64_t8_n2000.txt` |
| 18 | 45272.965659 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 80 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc80_nr8_ku2_kc64_t8_n2000.txt` |
| 19 | 45017.972947 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 40 | 96 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc40_nc96_nr8_ku2_kc64_t8_n2000.txt` |
| 20 | 44712.069652 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 96 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc32_nc96_nr8_ku2_kc64_t8_n2000.txt` |
| 21 | 43871.614226 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 24 | 80 | 8 | 2 | 64 | 8 |  | `opt_rvv_x100_clang_double_mc24_nc80_nr8_ku2_kc64_t8_n2000.txt` |
| 22 | 40635.218355 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 64 | 6 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc64_t6_n2000.txt` |
| 23 | 34272.756415 | optimized | opt_rvv | opt_rvv | clang | double | a100 | rvv_a100_opt |  | 64 | 64 | 8 | 2 | 64 | 8 |  | `opt_rvv_a100_clang_double_mc64_nc64_nr8_ku2_kc64_t8_n2000.txt` |
| 24 | 32501.755396 | optimized | opt_rvv | opt_rvv | clang | double | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 64 | 4 |  | `opt_rvv_x100_clang_double_mc32_nc64_nr8_ku2_kc64_t4_n2000.txt` |
| 25 | 30982.666218 | optimized | opt_base | opt_gemm | clang | double |  | gemm_opt |  |  |  | 8 |  | 64 | 8 |  | `opt_gemm_clang_double_nr8_kc64_t8_n2000.txt` |
| 26 | 30733.609151 | optimized | opt_base | opt_gemm | clang | double |  | gemm_opt |  |  |  | 4 |  | 64 | 8 |  | `opt_gemm_clang_double_nr4_kc64_t8_n2000.txt` |
| 27 | 28721.425956 | optimized | opt_rvv | opt_rvv | clang | double | a100 | rvv_a100_opt |  | 64 | 64 | 6 | 2 | 64 | 8 |  | `opt_rvv_a100_clang_double_mc64_nc64_nr6_ku2_kc64_t8_n2000.txt` |
| 28 | 27719.105664 | optimized | opt_rvv | opt_rvv | clang | double | a100 | rvv_a100_opt |  | 64 | 64 | 4 | 2 | 64 | 8 |  | `opt_rvv_a100_clang_double_mc64_nc64_nr4_ku2_kc64_t8_n2000.txt` |
| 29 | 26972.140133 | optimized | opt_base | opt_gemm | clang | double |  | gemm_opt |  |  |  | 6 |  | 64 | 8 |  | `opt_gemm_clang_double_nr6_kc64_t8_n2000.txt` |
| 30 | 23872.886661 | optimized | opt_rvv | opt_rvv | clang | double | a100 | rvv_a100_opt |  | 64 | 64 | 4 | 2 | 80 | 8 |  | `opt_rvv_a100_clang_double_mc64_nc64_nr4_ku2_kc80_t8_n2000.txt` |

## Top 30 fp32 Runs By MFlops/s

| Rank | MFlops/s | Series | Family | Mode | Compiler | Precision | Core | Variant | LMUL | MC | NC | NR | KU | KC | Threads | A100 Share | File |
|---:|---:|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 125007.127818 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 96 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc64_nr8_ku2_kc96_t8_n2000.txt` |
| 2 | 122730.452225 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 112 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc64_nr8_ku2_kc112_t8_n2000.txt` |
| 3 | 121186.408112 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc64_nr8_ku2_kc128_t8_n2000.txt` |
| 4 | 118030.297594 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 96 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc96_nr8_ku2_kc128_t8_n2000.txt` |
| 5 | 115249.365139 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 48 | 64 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc48_nc64_nr8_ku2_kc128_t8_n2000.txt` |
| 6 | 114138.815627 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 48 | 96 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc48_nc96_nr8_ku2_kc128_t8_n2000.txt` |
| 7 | 112797.201116 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 144 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc64_nr8_ku2_kc144_t8_n2000.txt` |
| 8 | 108967.407948 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 48 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc48_nr8_ku2_kc128_t8_n2000.txt` |
| 9 | 108705.467827 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 80 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc80_nr8_ku2_kc128_t8_n2000.txt` |
| 10 | 107809.664446 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 160 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc64_nr8_ku2_kc160_t8_n2000.txt` |
| 11 | 107326.941373 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 48 | 80 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc48_nc80_nr8_ku2_kc128_t8_n2000.txt` |
| 12 | 106721.728441 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 40 | 96 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc40_nc96_nr8_ku2_kc128_t8_n2000.txt` |
| 13 | 104663.890035 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 48 | 48 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc48_nc48_nr8_ku2_kc128_t8_n2000.txt` |
| 14 | 102605.411224 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 40 | 64 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc40_nc64_nr8_ku2_kc128_t8_n2000.txt` |
| 15 | 102153.127863 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 24 | 64 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc24_nc64_nr8_ku2_kc128_t8_n2000.txt` |
| 16 | 100174.369944 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 24 | 96 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc24_nc96_nr8_ku2_kc128_t8_n2000.txt` |
| 17 | 97875.669689 | optimized | opt_rvv | opt_rvv | clang | float | a100 | rvv_a100_opt |  | 64 | 64 | 8 | 2 | 128 | 8 |  | `opt_rvv_a100_clang_float_mc64_nc64_nr8_ku2_kc128_t8_n2000.txt` |
| 18 | 97828.214770 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 24 | 48 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc24_nc48_nr8_ku2_kc128_t8_n2000.txt` |
| 19 | 97439.528681 | optimized | opt_rvv | opt_rvv | clang | float | a100 | rvv_a100_opt |  | 64 | 64 | 8 | 2 | 96 | 8 |  | `opt_rvv_a100_clang_float_mc64_nc64_nr8_ku2_kc96_t8_n2000.txt` |
| 20 | 95362.517988 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 40 | 80 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc40_nc80_nr8_ku2_kc128_t8_n2000.txt` |
| 21 | 94964.670309 | optimized | opt_rvv | opt_rvv | clang | float | a100 | rvv_a100_opt |  | 64 | 64 | 8 | 2 | 192 | 8 |  | `opt_rvv_a100_clang_float_mc64_nc64_nr8_ku2_kc192_t8_n2000.txt` |
| 22 | 94960.437434 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 24 | 80 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc24_nc80_nr8_ku2_kc128_t8_n2000.txt` |
| 23 | 94401.232709 | optimized | opt_rvv | opt_rvv | clang | float | a100 | rvv_a100_opt |  | 64 | 64 | 8 | 2 | 160 | 8 |  | `opt_rvv_a100_clang_float_mc64_nc64_nr8_ku2_kc160_t8_n2000.txt` |
| 24 | 92213.488213 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 192 | 8 |  | `opt_rvv_x100_clang_float_mc32_nc64_nr8_ku2_kc192_t8_n2000.txt` |
| 25 | 91802.810081 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 40 | 48 | 8 | 2 | 128 | 8 |  | `opt_rvv_x100_clang_float_mc40_nc48_nr8_ku2_kc128_t8_n2000.txt` |
| 26 | 89093.695602 | optimized | opt_rvv | opt_rvv | clang | float | a100 | rvv_a100_opt |  | 64 | 64 | 8 | 2 | 64 | 8 |  | `opt_rvv_a100_clang_float_mc64_nc64_nr8_ku2_kc64_t8_n2000.txt` |
| 27 | 88008.658593 | optimized | opt_rvv | opt_rvv | clang | float | x100 | rvv_x100_opt |  | 32 | 64 | 8 | 2 | 128 | 6 |  | `opt_rvv_x100_clang_float_mc32_nc64_nr8_ku2_kc128_t6_n2000.txt` |
| 28 | 80554.787125 | optimized | opt_rvv | opt_rvv | clang | float | a100 | rvv_a100_opt |  | 64 | 64 | 6 | 2 | 64 | 8 |  | `opt_rvv_a100_clang_float_mc64_nc64_nr6_ku2_kc64_t8_n2000.txt` |
| 29 | 75679.567445 | optimized | opt_rvv | opt_rvv | clang | float | a100 | rvv_a100_opt |  | 64 | 64 | 4 | 2 | 64 | 8 |  | `opt_rvv_a100_clang_float_mc64_nc64_nr4_ku2_kc64_t8_n2000.txt` |
| 30 | 68400.741974 | optimized | opt_base | opt_gemm | clang | float |  | gemm_opt |  |  |  | 8 |  | 128 | 8 |  | `opt_gemm_clang_float_nr8_kc128_t8_n2000.txt` |

## Top 30 fp16 Runs By MFlops/s

| Rank | MFlops/s | Series | Family | Mode | Compiler | Precision | Core | Variant | LMUL | MC | NC | NR | KU | KC | Threads | A100 Share | File |
|---:|---:|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 34124.117072 | optimized | opt_ime | opt_ime | clang | fp16 | a100 | ime_opt |  |  |  |  |  |  | 8 |  | `opt_ime_clang_fp16_t8_n2000.txt` |
| 2 | 18443.155329 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 50 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share50_n2000.txt` |
| 3 | 18408.934226 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 50 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share50_n2000.txt` |
| 4 | 16718.705546 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 40 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share40_n2000.txt` |
| 5 | 16336.269059 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 |  |  |  |  |  | x8/a8 | 50 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share50_n2000.txt` |
| 6 | 16129.664799 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 |  |  |  |  |  | x8/a8 | 40 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share40_n2000.txt` |
| 7 | 15619.146125 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 |  |  |  |  |  | x8/a8 | 60 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share60_n2000.txt` |
| 8 | 15572.209612 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 |  |  |  |  |  | x8/a8 | 50 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share50_n2000.txt` |
| 9 | 15558.339149 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 60 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share60_n2000.txt` |
| 10 | 15551.074791 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 60 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share60_n2000.txt` |
| 11 | 15383.449691 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 40 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share40_n2000.txt` |
| 12 | 14772.123319 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 |  |  |  |  |  | x8/a8 | 30 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share30_n2000.txt` |
| 13 | 14334.619733 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 30 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share30_n2000.txt` |
| 14 | 14121.972686 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 30 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share30_n2000.txt` |
| 15 | 14065.169933 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 20 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share20_n2000.txt` |
| 16 | 13936.130284 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 |  |  |  |  |  | x8/a8 | 60 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share60_n2000.txt` |
| 17 | 13660.087228 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 |  |  |  |  |  | x8/a8 | 20 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share20_n2000.txt` |
| 18 | 13477.279785 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 70 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share70_n2000.txt` |
| 19 | 13462.624866 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 70 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share70_n2000.txt` |
| 20 | 13458.059227 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m2 | 2 |  |  |  |  |  | x8/a8 | 70 | `gemm_hetero_rvv_ime_2_clang_fp16_x8_a8_share70_n2000.txt` |
| 21 | 13452.101909 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 |  |  |  |  |  | x8/a8 | 70 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share70_n2000.txt` |
| 22 | 13199.899906 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 |  |  |  |  |  | x8/a8 | 20 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share20_n2000.txt` |
| 23 | 12966.798695 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 20 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share20_n2000.txt` |
| 24 | 12723.021652 | baseline | hetero | rvv_ime | gcc | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 0 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share0_n2000.txt` |
| 25 | 12581.011009 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 |  |  |  |  |  | x8/a8 | 30 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share30_n2000.txt` |
| 26 | 12545.063309 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 0 | `gemm_hetero_rvv_ime_8_clang_fp16_x8_a8_share0_n2000.txt` |
| 27 | 12448.702769 | baseline | ime | ime | gcc | fp16 | a100 | ime |  |  |  |  |  |  | 8 |  | `gemm_ime_gcc_fp16_t8_n2000.txt` |
| 28 | 12448.473004 | baseline | hetero | rvv_ime | gcc | fp16 |  | rvv_ime_m8 | 8 |  |  |  |  |  | x8/a8 | 0 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share0_n2000.txt` |
| 29 | 12416.403681 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m1 | 1 |  |  |  |  |  | x8/a8 | 40 | `gemm_hetero_rvv_ime_1_clang_fp16_x8_a8_share40_n2000.txt` |
| 30 | 12408.804147 | baseline | hetero | rvv_ime | clang | fp16 |  | rvv_ime_m4 | 4 |  |  |  |  |  | x8/a8 | 0 | `gemm_hetero_rvv_ime_4_clang_fp16_x8_a8_share0_n2000.txt` |


## Non-Valid Runs

| Status | Series | Family | Mode | Compiler | Precision | Variant | A100 Share | File |
|---|---|---|---|---|---|---|---|---|
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 100 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 10 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 20 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 30 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 40 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 50 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 60 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 70 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 80 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m1 | 90 | `gemm_hetero_rvv_ime_1_gcc_fp16_x8_a8_share90_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 100 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 10 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 20 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 30 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 40 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 50 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 60 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 70 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 80 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m2 | 90 | `gemm_hetero_rvv_ime_2_gcc_fp16_x8_a8_share90_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 100 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 10 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 20 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 30 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 40 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 50 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 60 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 70 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 80 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m4 | 90 | `gemm_hetero_rvv_ime_4_gcc_fp16_x8_a8_share90_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 100 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share100_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 10 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share10_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 20 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share20_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 30 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share30_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 40 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share40_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 50 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share50_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 60 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share60_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 70 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share70_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 80 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share80_n2000.txt` |
| unknown | baseline | hetero | rvv_ime | gcc | fp16 | rvv_ime_m8 | 90 | `gemm_hetero_rvv_ime_8_gcc_fp16_x8_a8_share90_n2000.txt` |
