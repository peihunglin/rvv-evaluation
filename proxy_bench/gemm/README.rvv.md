# GEMM RVV Build and Run

This directory builds the PRK OpenMP GEMM benchmark for RISC-V scalar and RVV auto-vectorized variants.

## Build

Run the build script from this directory. The current default is optimized/tuning builds (`BUILD_BASE=0`, `OPT=1`):

```bash
./compiler.sh
```

To build the original scalar/autovec baseline instead, enable `BUILD_BASE` and disable `OPT`:

```bash
BUILD_BASE=1 OPT=0 ./compiler.sh
```

The baseline path builds both `float` and `double` variants for both compiler families:

```text
build/clang/float/
build/clang/double/
build/gcc/float/
build/gcc/double/
```

Each directory contains the same six executables:

```text
gemm_nonvec
gemm_autovec
gemm_autovec_1
gemm_autovec_2
gemm_autovec_4
gemm_autovec_8
```

`gemm_nonvec` uses `-march=rv64gc` and disables auto-vectorization. `gemm_autovec` uses `-march=rv64gcv` without forcing LMUL. The numbered `gemm_autovec_<LMUL>` binaries force LMUL through `RVV_LMUL`.

## Compile Publication Suites

Run from this directory:

```bash
cd proxy_bench/gemm
BUILD_BASE=1 OPT=1 ./compiler.sh
```

This builds the original scalar/autovec baseline plus the optimized suite:

```text
build/{clang,gcc}/{float,double}/gemm_nonvec
build/{clang,gcc}/{float,double}/gemm_autovec
build/{clang,gcc}/{float,double}/gemm_autovec_{1,2,4,8}
build/opt/{clang,gcc}/{float,double}/gemm_opt_nr*
build/opt/x100/{clang,gcc}/{float,double}/gemm_rvv_x100_opt_mc*_nc*_nr*_ku*
build/opt/a100/{clang,gcc}/{float,double}/gemm_rvv_a100_opt_mc*_nc*_nr*_ku*
build/opt/{clang,gcc}/fp16/gemm_ime_opt
```

The fully explicit equivalent is:

```bash
PRECISIONS="float double" \
COMPILERS="clang gcc" \
LMULS="1 2 4 8" \
OPT=1 OPT_TARGETS="gemm rvv_x100 rvv_a100 ime" OPT_COMPILERS="clang gcc" OPT_PRECISIONS="float double" \
BUILD_BASE=1 \
./compiler.sh
```

To skip rebuilding the original scalar/autovec binaries and build only the optimized suite:

```bash
BUILD_BASE=0 OPT=1 ./compiler.sh
```

Focused build examples:

```bash
BUILD_BASE=0 OPT=1 OPT_TARGETS="gemm rvv_x100 rvv_a100 ime" OPT_COMPILERS="clang gcc" OPT_PRECISIONS="float double" ./compiler.sh
BUILD_BASE=0 OPT=1 OPT_TARGETS=gemm OPT_COPT_NRS="2 4 6 8" ./compiler.sh
BUILD_BASE=0 OPT=1 OPT_TARGETS="rvv_x100 rvv_a100" OPT_X100_MCS="24 32 40 48" OPT_X100_NCS="48 64 80 96" ./compiler.sh
BUILD_BASE=0 OPT=1 OPT_TARGETS=ime OPT_COMPILERS="clang gcc" ./compiler.sh
```

Small smoke-build examples for K3:

```bash
BUILD_BASE=0 OPT=1 OPT_TARGETS=rvv_x100 OPT_COMPILERS=clang OPT_PRECISIONS=float OPT_X100_MCS=32 OPT_X100_NCS=64 OPT_X100_NRS=8 ./compiler.sh
BUILD_BASE=0 OPT=1 OPT_TARGETS=ime OPT_COMPILERS=clang ./compiler.sh
```

## IME Build

The SpacemiT IME path is a separate FP16-input, FP32-accumulate benchmark because IME does not provide a direct FP32/FP64 replacement for the default GEMM loop. Build it with:

```bash
IME=1 ./compiler.sh
```

This keeps the normal float/double builds and adds:

```text
build/{clang,gcc}/fp16/gemm_ime
```

To build only the IME binary, disable the base float/double build loop:

```bash
BUILD_BASE=0 IME=1 ./compiler.sh
```

The IME target uses `smt.vfwmadot` through inline assembly and always passes `-march=rv64gcv_zfh_zvfh_zvl1024b_xsmtvdotii -mcpu=spacemit-a100` from the Makefile when `ime=1`. `compiler.sh` forwards this ISA string through `IME_MARCH`, whose default is `rv64gcv_zfh_zvfh_zvl1024b_xsmtvdotii`. Use `IME_COMPILERS="clang gcc"` to select compilers, and override `IME_CLANG_CC`, `IME_CLANG_CXX`, `IME_GCC_CC`, or `IME_GCC_CXX` only for compiler command prefixes, not for the target CPU.

Run it with the same CLI shape as the regular benchmark:

```bash
./build/clang/fp16/gemm_ime 8 10 2000 8
./build/gcc/fp16/gemm_ime 8 10 2000 8
```

`gemm_ime` has its own checksum validation and exits with failure on mismatch. The optional tile-size argument is accepted for script compatibility, but the IME microkernel uses fixed `8x8x8` tiles.

## RVV Intrinsic Build

The RVV intrinsic path is separate from `gemm_autovec`: it uses `<riscv_vector.h>` and explicit `vle`/`vfmacc`/`vse` intrinsics over the row dimension. Build it for both SpacemiT K3 core types with:

```bash
RVV=1 ./compiler.sh
```

This keeps the normal float/double builds and adds:

```text
build/{x100,a100}/{clang,gcc}/{float,double}/gemm_rvv
build/{x100,a100}/{clang,gcc}/{float,double}/gemm_rvv_{1,2,4,8}
```

To build only RVV intrinsic binaries, disable the base float/double build loop:

```bash
BUILD_BASE=0 RVV=1 ./compiler.sh
```

Narrow RVV builds with:

```bash
BUILD_BASE=0 RVV=1 RVV_CORES=x100 PRECISION=float RVV_COMPILERS=clang ./compiler.sh
BUILD_BASE=0 RVV=1 RVV_CORES=a100 PRECISION=double RVV_COMPILERS=gcc ./compiler.sh
```

Override the intrinsic LMUL list with:

```bash
RVV_LMULS="1 2 4 8" RVV=1 ./compiler.sh
```

The RVV target uses `-march=rv64gcv_zfh_zvfh` and selects `-mcpu=spacemit-x100` or `-mcpu=spacemit-a100` from `RVV_CORES`, depending on the experiment. `gemm_rvv` is the m1 baseline. The numbered `gemm_rvv_<LMUL>` binaries use matching RVV intrinsic types such as `vfloat32m4_t` or `vfloat64m4_t`; all variants adapt to implementation VLEN through `vsetvl`.

Run it with the regular GEMM CLI shape:

```bash
./build/x100/clang/float/gemm_rvv 8 10 2000 16
./build/x100/clang/float/gemm_rvv_4 8 10 2000 16
./build/a100/clang/float/gemm_rvv 8 10 2000 16
~/bin/ai ./build/a100/clang/float/gemm_rvv_4 8 10 2000 16
```

`gemm_rvv` has its own checksum validation and exits with failure on mismatch.

## Heterogeneous X100/A100 Build

The heterogeneous path is a separate OpenMP benchmark that binds X100 workers to CPUs `0-7` with `sched_setaffinity()` and moves A100 worker TIDs through `/proc/set_ai_thread` inside the process. Do not wrap these binaries with `~/bin/ai`; they must use both CPU groups.

Build both supported hetero scenarios with:

```bash
BUILD_BASE=0 HETERO=1 ./compiler.sh
```

Supported modes:

```text
rvv_rvv: X100 RVV kernel compiled with -mcpu=spacemit-x100 plus A100 RVV kernel compiled with -mcpu=spacemit-a100
rvv_ime: X100 RVV FP16 widening kernel plus A100 IME kernel compiled with -mcpu=spacemit-a100
```

Outputs:

```text
build/hetero/{clang,gcc}/{float,double}/gemm_hetero_rvv_rvv_{1,2,4,8}
build/hetero/{clang,gcc}/fp16/gemm_hetero_rvv_ime_{1,2,4,8}
```

Narrow builds:

```bash
BUILD_BASE=0 HETERO=1 HETERO_MODES=rvv_rvv PRECISION=float HETERO_COMPILERS=clang HETERO_LMULS="4" ./compiler.sh
BUILD_BASE=0 HETERO=1 HETERO_MODES=rvv_ime HETERO_COMPILERS="clang gcc" HETERO_LMULS="4" ./compiler.sh
```

Runtime controls:

```text
X100_THREADS=8
A100_THREADS=8
A100_SHARE=50
A100_BIND=thread
X100_CPUS=0-7
```

`A100_SHARE` is the percentage of output columns assigned to the A100 side. `A100_BIND=thread` writes each A100 OpenMP worker TID to `/proc/set_ai_thread`; `A100_BIND=process` is only valid for A100-only runs with `X100_THREADS=0` because it is equivalent to `echo $$ > /proc/set_ai_thread` and moves the whole process.

`run.sh` sweeps hetero sharing by default with:

```text
HETERO_A100_SHARES="0 10 20 30 40 50 60 70 80 90 100"
```

This produces one result file per share, making it easier to identify the best workload split between X100 and A100.

Example mixed runs:

```bash
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_BIND=thread A100_SHARE=50 ./build/hetero/clang/float/gemm_hetero_rvv_rvv_4 10 2000 16
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_BIND=thread A100_SHARE=70 ./build/hetero/clang/fp16/gemm_hetero_rvv_ime_4 10 2000 8
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_BIND=thread A100_SHARE=70 ./build/hetero/gcc/fp16/gemm_hetero_rvv_ime_4 10 2000 8
```

Focused share-sweep examples:

```bash
for share in 0 10 20 30 40 50 60 70 80 90 100; do
  OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_BIND=thread A100_SHARE=${share} ./build/hetero/clang/float/gemm_hetero_rvv_rvv_4 10 2000 16
done
```

```bash
for share in 0 10 20 30 40 50 60 70 80 90 100; do
  OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_BIND=thread A100_SHARE=${share} ./build/hetero/clang/fp16/gemm_hetero_rvv_ime_4 10 2000 8
done
```

Example A100-only affinity smoke runs using process-level binding:

```bash
OMP_DYNAMIC=false X100_THREADS=0 A100_THREADS=1 A100_BIND=process A100_SHARE=100 ./build/hetero/clang/fp16/gemm_hetero_rvv_ime_4 1 64 8
OMP_DYNAMIC=false X100_THREADS=0 A100_THREADS=8 A100_BIND=process A100_SHARE=100 ./build/hetero/clang/fp16/gemm_hetero_rvv_ime_4 1 64 8
```

The hetero CLI is `<iterations> <matrix order> [tile size]`; thread counts come from the environment so the binary can keep X100 and A100 teams separate.

## Optimized Build

The optimized series is separate from the baseline binaries and is intended for baseline-vs-optimized performance comparisons.

Build all optimized variants with:

```bash
BUILD_BASE=0 OPT=1 ./compiler.sh
```

Outputs:

```text
build/opt/{clang,gcc}/{float,double}/gemm_opt
build/opt/x100/{clang,gcc}/{float,double}/gemm_rvv_x100_opt
build/opt/a100/{clang,gcc}/{float,double}/gemm_rvv_a100_opt
build/opt/{clang,gcc}/fp16/gemm_ime_opt
```

Focused optimized builds:

```bash
BUILD_BASE=0 OPT=1 OPT_TARGETS=gemm OPT_PRECISIONS="float double" ./compiler.sh
BUILD_BASE=0 OPT=1 OPT_TARGETS="rvv_x100 rvv_a100" OPT_PRECISIONS="float double" ./compiler.sh
BUILD_BASE=0 OPT=1 OPT_TARGETS=ime OPT_COMPILERS="clang gcc" ./compiler.sh
```

The optimized RVV binaries follow the same precision model as `gemm_rvv.c`: `float=1` builds FP32 intrinsics and `float=0` builds FP64 intrinsics. The optimized IME binary is FP16-input/FP32-accumulate and uses `IME_MARCH=rv64gcv_zfh_zvfh_zvl1024b_xsmtvdotii`.

Run examples:

```bash
./build/opt/clang/float/gemm_opt 8 10 2000 256
./build/opt/x100/clang/float/gemm_rvv_x100_opt 8 10 2000 128
~/bin/ai ./build/opt/a100/clang/float/gemm_rvv_a100_opt 8 10 2000 256
~/bin/ai ./build/opt/clang/fp16/gemm_ime_opt 8 10 2000 8
```

## Build Options

Build only one precision:

```bash
PRECISION=float ./compiler.sh
PRECISION=double ./compiler.sh
```

Override the precision list explicitly:

```bash
PRECISIONS="float double" ./compiler.sh
```

Override the LMUL list:

```bash
LMULS="1 2 4 8" ./compiler.sh
```

Override output location:

```bash
BUILD_DIR=/tmp/gemm-build ./compiler.sh
```

The compiler commands can also be overridden:

```bash
CLANG_CC="clang -std=c11 -pthread -static" ./compiler.sh
GCC_CC="riscv64-unknown-linux-gnu-gcc -std=c11 -pthread -static" ./compiler.sh
```

## Manual Make Examples

Float precision uses `float=1`; double precision uses `float=0` or omits the flag because `gemm.c` defaults to `double`.

```bash
make gemm clang=1 nonvec=1 float=1
make gemm clang=1 nonvec=0 RVV_LMUL=4 float=1
make gemm clang=0 nonvec=0 RVV_LMUL=4 float=1
make gemm clang=1 nonvec=1 float=0
make gemm clang=0 nonvec=0 RVV_LMUL=4 float=0
make gemm_ime CC="clang -std=c11 -pthread" clang=1 nonvec=0 ime=1
make gemm_ime CC="gcc -std=c11 -pthread" clang=0 nonvec=0 ime=1
make gemm_rvv CC="clang -std=c11 -pthread" clang=1 nonvec=0 rvv=1 RVV_MCPU=spacemit-x100 float=1
make gemm_rvv CC="clang -std=c11 -pthread" clang=1 nonvec=0 rvv=1 RVV_MCPU=spacemit-a100 float=1
make gemm_rvv CC="clang -std=c11 -pthread" clang=1 nonvec=0 rvv=1 RVV_MCPU=spacemit-x100 RVV_INTRINSIC_LMUL=4 float=1
```

## Run

`run.sh` only executes existing binaries. It does not build, clean, or modify source files. It runs `build/clang/fp16/gemm_ime`, `build/{x100,a100}/{gcc,clang}/{float,double}/gemm_rvv{,_1,_2,_4,_8}`, `build/hetero/.../gemm_hetero_*`, and `build/opt/.../*_opt` only when those binaries exist. IME and a100-only RVV runs are wrapped with `$HOME/bin/ai`; hetero runs are not wrapped because they bind both CPU groups internally.

The current staged runner is controlled by `RUN_MODE`:

```text
copt             optimized plain-C NR/KC sweep
rvv_kc           optimized RVV KC sweep using current best MC/NC/NR/KU
rvv_nr           optimized RVV NR sweep using current best MC/NC/KU/KC
rvv_block        optimized RVV MC/NC sweep using current best NR/KU/KC
rvv_scaling      optimized RVV thread-scaling using current best MC/NC/NR/KU/KC
rvv_all          run rvv_kc, rvv_nr, rvv_block, and rvv_scaling
ime_opt          optimized IME run
hetero_rvv_rvv   heterogeneous RVV+RVV share sweep
hetero_rvv_ime   heterogeneous RVV+IME share sweep
hetero_all       run both hetero share sweeps
publication_all  run copt, rvv_all, ime_opt, and hetero_all
```

Run the full available set and write logs under `results/`:

```bash
RUN_MODE=publication_all ./run.sh
```

The default `RUN_MODE` is `rvv_block`, which is intended for staged optimized-RVV tuning rather than complete data collection.

Override result directory or perf events with:

```bash
RESULT_DIR=results_k3 PERF_EVENTS=instructions,cycles,L1-dcache-loads,L1-dcache-load-misses ./run.sh
```

Override the hetero share sweep with:

```bash
HETERO_A100_SHARES="0 25 50 75 100" ./run.sh
```

## Run Individual Tests

Run from this directory:

```bash
cd proxy_bench/gemm
```

Base scalar/autovec binaries use `<threads> <iterations> <matrix_order> <tile_size>`:

```bash
./build/clang/float/gemm_nonvec 8 10 2000 16
./build/clang/float/gemm_autovec 8 10 2000 16
./build/clang/float/gemm_autovec_4 8 10 2000 16
./build/gcc/double/gemm_autovec_8 8 10 2000 16
```

IME is A100-only, so run through `~/bin/ai`:

```bash
~/bin/ai ./build/clang/fp16/gemm_ime 8 10 2000 8
~/bin/ai ./build/gcc/fp16/gemm_ime 8 10 2000 8
```

RVV intrinsic x100 binaries run directly:

```bash
./build/x100/clang/float/gemm_rvv 8 10 2000 16
./build/x100/clang/float/gemm_rvv_4 8 10 2000 16
./build/x100/gcc/double/gemm_rvv_8 8 10 2000 16
```

RVV intrinsic a100 binaries run through `~/bin/ai`:

```bash
~/bin/ai ./build/a100/clang/float/gemm_rvv 8 10 2000 16
~/bin/ai ./build/a100/clang/float/gemm_rvv_4 8 10 2000 16
~/bin/ai ./build/a100/gcc/double/gemm_rvv_8 8 10 2000 16
```

Heterogeneous RVV+RVV binaries must not be wrapped with `~/bin/ai`; they bind x100 and a100 workers internally. Their CLI is `<iterations> <matrix_order> <tile_size>`:

```bash
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_SHARE=50 ./build/hetero/clang/float/gemm_hetero_rvv_rvv_4 10 2000 16
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_SHARE=70 ./build/hetero/gcc/double/gemm_hetero_rvv_rvv_8 10 2000 16
```

Heterogeneous RVV+IME binaries are also not wrapped with `~/bin/ai`:

```bash
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_SHARE=70 ./build/hetero/clang/fp16/gemm_hetero_rvv_ime_4 10 2000 8
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_SHARE=80 ./build/hetero/gcc/fp16/gemm_hetero_rvv_ime_8 10 2000 8
```

Override the X100 CPU list if needed. A100 workers are moved with `/proc/set_ai_thread`, not Linux CPU affinity:

```bash
OMP_DYNAMIC=false X100_CPUS=0-7 X100_THREADS=8 A100_THREADS=8 A100_BIND=thread A100_SHARE=60 ./build/hetero/clang/float/gemm_hetero_rvv_rvv_4 10 2000 16
```

Small smoke runs:

```bash
./build/x100/clang/float/gemm_rvv_4 1 1 64 16
~/bin/ai ./build/a100/clang/float/gemm_rvv_4 1 1 64 16
~/bin/ai ./build/clang/fp16/gemm_ime 1 1 64 8
OMP_DYNAMIC=false X100_THREADS=1 A100_THREADS=1 A100_SHARE=50 ./build/hetero/clang/float/gemm_hetero_rvv_rvv_4 1 64 16
OMP_DYNAMIC=false X100_THREADS=1 A100_THREADS=1 A100_SHARE=50 ./build/hetero/clang/fp16/gemm_hetero_rvv_ime_4 1 64 8
```

Manual one-off logging with `perf`:

```bash
mkdir -p results
perf stat -e instructions,L1-dcache-loads,L1-dcache-load-misses ./build/x100/clang/float/gemm_rvv_4 8 10 2000 16 2>&1 | tee results/gemm_rvv_x100_clang_float_m4.txt
perf stat -e instructions,L1-dcache-loads,L1-dcache-load-misses ~/bin/ai ./build/a100/clang/float/gemm_rvv_4 8 10 2000 16 2>&1 | tee results/gemm_rvv_a100_clang_float_m4.txt
OMP_DYNAMIC=false X100_THREADS=8 A100_THREADS=8 A100_SHARE=70 perf stat -e instructions,L1-dcache-loads,L1-dcache-load-misses ./build/hetero/clang/fp16/gemm_hetero_rvv_ime_4 10 2000 8 2>&1 | tee results/gemm_hetero_rvv_ime_clang_m4_share70.txt
```

## Summarize Results

Use `extract_results.py` to summarize all `run.sh` logs with MFlops/s as the primary metric:

```bash
python3 extract_results.py results --csv results_summary.csv --md results_summary.md --top 30
```

This also writes:

```text
results_best.csv
results_hetero_share.csv
```

The CSV includes one row per result file with fields such as:

```text
series, family, mode, compiler, precision, precision_group, core, variant, lmul, mc, nc, nr, ku, kc, threads, x100_threads, a100_threads, a100_share, status, mflops, avg_time_s, elapsed_s, instructions, l1_loads, l1_load_misses
```

The Markdown report ranks valid runs by `Rate (MFlops/s)` separately for `fp64`, `fp32`, and `fp16`, then lists non-valid runs separately. To summarize a different result directory:

```bash
python3 extract_results.py results_k3 --csv results_k3_summary.csv --best-csv results_k3_best.csv --hetero-csv results_k3_hetero_share.csv --md results_k3_summary.md --top 50
```
