#!/bin/bash
set -euo pipefail

make clean
make gemm_hetero \
  CC="${CC:-clang -std=c11 -pthread}" \
  CXX="${CXX:-clang++ -std=gnu++20 -pthread}" \
  clang=1 hetero=1 HETERO_MODE=rvv_rvv \
  float="${FLOAT:-0}"

echo
echo "Suggested FP64 split sweep:"
echo 'for s in 30 35 40 45 50; do'
echo '  A100_SHARE=$s X100_THREADS=8 A100_THREADS=8 A100_BIND=thread X100_CPUS=0-7 \'
echo '    perf stat -e instructions,L1-dcache-loads,L1-dcache-load-misses \'
echo '    ./gemm_hetero 10 2000 64'
echo 'done'
