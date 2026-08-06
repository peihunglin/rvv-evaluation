#!/bin/bash
set -euo pipefail

make clean
make gemm_hetero \
  CC="${CC:-clang -std=c11 -pthread}" \
  CXX="${CXX:-clang++ -std=gnu++20 -pthread}" \
  clang=1 hetero=1 HETERO_MODE=rvv_ime

echo
echo "A100-only IME sanity check:"
echo 'A100_SHARE=100 X100_THREADS=0 A100_THREADS=8 A100_BIND=process \'
echo '  /home/lin32/bin/ai ./gemm_hetero 10 2000'
echo
echo "X100-only FP16 RVV sanity check:"
echo 'A100_SHARE=0 X100_THREADS=8 A100_THREADS=0 X100_CPUS=0-7 \'
echo '  ./gemm_hetero 10 2000'
echo
echo "Mixed sweep:"
echo 'for s in 35 40 45 50 55; do'
echo ' A100_SHARE=$s X100_THREADS=8 A100_THREADS=8 A100_BIND=thread X100_CPUS=0-7 \'
echo '   ./gemm_hetero 10 2000'
echo 'done'
