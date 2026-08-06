#!/bin/bash
set -euo pipefail

RESULT_DIR=${RESULT_DIR:-results}
ITERATIONS=${ITERATIONS:-10}
SIZES=${SIZES:-"2000"}
PERF_EVENTS=${PERF_EVENTS:-"instructions,L1-dcache-loads,L1-dcache-load-misses"}

# Use RUN_MODE to keep data collection staged instead of generating
# every possible combination by default.
#
# Supported:
#   baseline
#   copt
#   rvv_kc
#   rvv_nr
#   rvv_block
#   rvv_scaling
#   rvv_all
#   ime_opt
#   hetero_rvv_rvv
#   hetero_rvv_ime
#   hetero_all
#   publication_all
#
RUN_MODE=${RUN_MODE:-rvv_block}

COMPILERS=${COMPILERS:-"clang"}
PRECISIONS=${PRECISIONS:-"float double"}

# ----------------------------------------------------------------------
# Optimized plain-C collection
# ----------------------------------------------------------------------
OPT_COPT_NRS=${OPT_COPT_NRS:-"2 4 6 8"}
OPT_C_KCS_FLOAT=${OPT_C_KCS_FLOAT:-"128"}
OPT_C_KCS_DOUBLE=${OPT_C_KCS_DOUBLE:-"64"}
COPT_THREADS=${COPT_THREADS:-"8"}

# ----------------------------------------------------------------------
# X100 RVV collection defaults
# ----------------------------------------------------------------------
X100_MCS=${X100_MCS:-"24 32 40 48"}
X100_NCS=${X100_NCS:-"48 64 80 96"}
X100_NRS=${X100_NRS:-"8"}
X100_KUS=${X100_KUS:-"2"}

X100_KCS_DOUBLE=${X100_KCS_DOUBLE:-"48 56 64 72 80 96"}
X100_KCS_FLOAT=${X100_KCS_FLOAT:-"96 112 128 144 160 192"}

X100_THREADS_KC=${X100_THREADS_KC:-"8"}
X100_THREADS_SCALING=${X100_THREADS_SCALING:-"1 2 4 6 8"}

# Winner values used by scaling mode. Override after each tuning stage.
X100_BEST_MC_DOUBLE=${X100_BEST_MC_DOUBLE:-32}
X100_BEST_NC_DOUBLE=${X100_BEST_NC_DOUBLE:-64}
X100_BEST_NR_DOUBLE=${X100_BEST_NR_DOUBLE:-8}
X100_BEST_KU_DOUBLE=${X100_BEST_KU_DOUBLE:-2}
X100_BEST_KC_DOUBLE=${X100_BEST_KC_DOUBLE:-64}

X100_BEST_MC_FLOAT=${X100_BEST_MC_FLOAT:-32}
X100_BEST_NC_FLOAT=${X100_BEST_NC_FLOAT:-64}
X100_BEST_NR_FLOAT=${X100_BEST_NR_FLOAT:-8}
X100_BEST_KU_FLOAT=${X100_BEST_KU_FLOAT:-2}
X100_BEST_KC_FLOAT=${X100_BEST_KC_FLOAT:-128}

# ----------------------------------------------------------------------
# A100 RVV collection defaults
# ----------------------------------------------------------------------
A100_MCS=${A100_MCS:-"64"}
A100_NCS=${A100_NCS:-"64"}
A100_NRS=${A100_NRS:-"4 6 8"}
A100_KUS=${A100_KUS:-"2"}

A100_KCS_DOUBLE=${A100_KCS_DOUBLE:-"32 48 64 80 96 128"}
A100_KCS_FLOAT=${A100_KCS_FLOAT:-"64 96 128 160 192"}

A100_THREADS_KC=${A100_THREADS_KC:-"8"}
A100_THREADS_SCALING=${A100_THREADS_SCALING:-"1 2 4 8"}

A100_BEST_MC_DOUBLE=${A100_BEST_MC_DOUBLE:-64}
A100_BEST_NC_DOUBLE=${A100_BEST_NC_DOUBLE:-64}
A100_BEST_NR_DOUBLE=${A100_BEST_NR_DOUBLE:-4}
A100_BEST_KU_DOUBLE=${A100_BEST_KU_DOUBLE:-2}
A100_BEST_KC_DOUBLE=${A100_BEST_KC_DOUBLE:-64}

A100_BEST_MC_FLOAT=${A100_BEST_MC_FLOAT:-64}
A100_BEST_NC_FLOAT=${A100_BEST_NC_FLOAT:-64}
A100_BEST_NR_FLOAT=${A100_BEST_NR_FLOAT:-8}
A100_BEST_KU_FLOAT=${A100_BEST_KU_FLOAT:-2}
A100_BEST_KC_FLOAT=${A100_BEST_KC_FLOAT:-64}

# ----------------------------------------------------------------------
# Heterogeneous collection defaults
# ----------------------------------------------------------------------
HETERO_X100_THREADS=${HETERO_X100_THREADS:-8}
HETERO_A100_THREADS=${HETERO_A100_THREADS:-8}
HETERO_A100_SHARES=${HETERO_A100_SHARES:-"0 10 20 30 40 50 60 70 80 90 100"}
HETERO_LMULS=${HETERO_LMULS:-"1 2 4 8"}

mkdir -p "${RESULT_DIR}"

run_and_log() {
    local label=$1
    shift

    local logfile="${RESULT_DIR}/${label}.txt"
    echo "$*" | tee "${logfile}"
    perf stat -e "${PERF_EVENTS}" "$@" 2>&1 | tee -a "${logfile}"
}

needs_a100_wrapper() {
    [ "$1" = "a100" ]
}

run_binary() {
    local core=$1 label=$2 binary=$3
    shift 3

    if needs_a100_wrapper "${core}"; then
        run_and_log "${label}" "$HOME/bin/ai" "${binary}" "$@"
    else
        run_and_log "${label}" "${binary}" "$@"
    fi
}

kcs_for() {
    local core=$1 precision=$2
    case "${core}:${precision}" in
        x100:double) echo "${X100_KCS_DOUBLE}" ;;
        x100:float)  echo "${X100_KCS_FLOAT}" ;;
        a100:double) echo "${A100_KCS_DOUBLE}" ;;
        a100:float)  echo "${A100_KCS_FLOAT}" ;;
    esac
}

threads_kc_for() {
    case "$1" in
        x100) echo "${X100_THREADS_KC}" ;;
        a100) echo "${A100_THREADS_KC}" ;;
    esac
}

threads_scaling_for() {
    case "$1" in
        x100) echo "${X100_THREADS_SCALING}" ;;
        a100) echo "${A100_THREADS_SCALING}" ;;
    esac
}

mcs_for() {
    case "$1" in
        x100) echo "${X100_MCS}" ;;
        a100) echo "${A100_MCS}" ;;
    esac
}

ncs_for() {
    case "$1" in
        x100) echo "${X100_NCS}" ;;
        a100) echo "${A100_NCS}" ;;
    esac
}

nrs_for() {
    case "$1" in
        x100) echo "${X100_NRS}" ;;
        a100) echo "${A100_NRS}" ;;
    esac
}

kus_for() {
    case "$1" in
        x100) echo "${X100_KUS}" ;;
        a100) echo "${A100_KUS}" ;;
    esac
}

best_value() {
    local core=$1 precision=$2 field=$3
    local var

    var=$(echo "${core}_${field}_${precision}" | tr '[:lower:]' '[:upper:]')
    eval "echo \${${var}}"
}

rvv_binary() {
    local core=$1 compiler=$2 precision=$3 mc=$4 nc=$5 nr=$6 ku=$7
    echo "./build/opt/${core}/${compiler}/${precision}/gemm_rvv_${core}_opt_mc${mc}_nc${nc}_nr${nr}_ku${ku}"
}

run_rvv_kc_sweep() {
    local core=$1 compiler=$2 precision=$3
    local mc nc nr ku binary kcs threads

    mc=$(best_value "${core}" "${precision}" BEST_MC)
    nc=$(best_value "${core}" "${precision}" BEST_NC)
    nr=$(best_value "${core}" "${precision}" BEST_NR)
    ku=$(best_value "${core}" "${precision}" BEST_KU)

    binary=$(rvv_binary "${core}" "${compiler}" "${precision}" "${mc}" "${nc}" "${nr}" "${ku}")
    [ -x "${binary}" ] || return 0

    kcs=$(kcs_for "${core}" "${precision}")
    threads=$(threads_kc_for "${core}")

    for size in ${SIZES}; do
        for t in ${threads}; do
            for kc in ${kcs}; do
                run_binary "${core}" \
                    "opt_rvv_${core}_${compiler}_${precision}_mc${mc}_nc${nc}_nr${nr}_ku${ku}_kc${kc}_t${t}_n${size}" \
                    "${binary}" ${t} ${ITERATIONS} ${size} ${kc}
            done
        done
    done
}

run_rvv_nr_sweep() {
    local core=$1 compiler=$2 precision=$3
    local mc nc ku kc threads nr binary

    mc=$(best_value "${core}" "${precision}" BEST_MC)
    nc=$(best_value "${core}" "${precision}" BEST_NC)
    ku=$(best_value "${core}" "${precision}" BEST_KU)
    kc=$(best_value "${core}" "${precision}" BEST_KC)
    threads=$(threads_kc_for "${core}")

    for nr in $(nrs_for "${core}"); do
        binary=$(rvv_binary "${core}" "${compiler}" "${precision}" "${mc}" "${nc}" "${nr}" "${ku}")
        [ -x "${binary}" ] || continue

        for size in ${SIZES}; do
            for t in ${threads}; do
                run_binary "${core}" \
                    "opt_rvv_${core}_${compiler}_${precision}_mc${mc}_nc${nc}_nr${nr}_ku${ku}_kc${kc}_t${t}_n${size}" \
                    "${binary}" ${t} ${ITERATIONS} ${size} ${kc}
            done
        done
    done
}

run_rvv_block_sweep() {
    local core=$1 compiler=$2 precision=$3
    local nr ku kc threads mc nc binary

    nr=$(best_value "${core}" "${precision}" BEST_NR)
    ku=$(best_value "${core}" "${precision}" BEST_KU)
    kc=$(best_value "${core}" "${precision}" BEST_KC)
    threads=$(threads_kc_for "${core}")

    for mc in $(mcs_for "${core}"); do
        for nc in $(ncs_for "${core}"); do
            binary=$(rvv_binary "${core}" "${compiler}" "${precision}" "${mc}" "${nc}" "${nr}" "${ku}")
            [ -x "${binary}" ] || continue

            for size in ${SIZES}; do
                for t in ${threads}; do
                    run_binary "${core}" \
                        "opt_rvv_${core}_${compiler}_${precision}_mc${mc}_nc${nc}_nr${nr}_ku${ku}_kc${kc}_t${t}_n${size}" \
                        "${binary}" ${t} ${ITERATIONS} ${size} ${kc}
                done
            done
        done
    done
}

run_rvv_scaling() {
    local core=$1 compiler=$2 precision=$3
    local mc nc nr ku kc threads binary

    mc=$(best_value "${core}" "${precision}" BEST_MC)
    nc=$(best_value "${core}" "${precision}" BEST_NC)
    nr=$(best_value "${core}" "${precision}" BEST_NR)
    ku=$(best_value "${core}" "${precision}" BEST_KU)
    kc=$(best_value "${core}" "${precision}" BEST_KC)
    threads=$(threads_scaling_for "${core}")

    binary=$(rvv_binary "${core}" "${compiler}" "${precision}" "${mc}" "${nc}" "${nr}" "${ku}")
    [ -x "${binary}" ] || return 0

    for size in ${SIZES}; do
        for t in ${threads}; do
            run_binary "${core}" \
                "opt_rvv_${core}_${compiler}_${precision}_mc${mc}_nc${nc}_nr${nr}_ku${ku}_kc${kc}_t${t}_n${size}" \
                "${binary}" ${t} ${ITERATIONS} ${size} ${kc}
        done
    done
}

run_copt() {
    local compiler precision nr binary kcs kc
    for precision in ${PRECISIONS}; do
        case "${precision}" in
            float)  kcs="${OPT_C_KCS_FLOAT}" ;;
            double) kcs="${OPT_C_KCS_DOUBLE}" ;;
        esac

        for compiler in ${COMPILERS}; do
            for nr in ${OPT_COPT_NRS}; do
                binary="./build/opt/${compiler}/${precision}/gemm_opt_nr${nr}"
                [ -x "${binary}" ] || continue

                for size in ${SIZES}; do
                    for t in ${COPT_THREADS}; do
                        for kc in ${kcs}; do
                            run_and_log \
                                "opt_gemm_${compiler}_${precision}_nr${nr}_kc${kc}_t${t}_n${size}" \
                                "${binary}" ${t} ${ITERATIONS} ${size} ${kc}
                        done
                    done
                done
            done
        done
    done
}

run_ime_opt() {
    local compiler binary

    for compiler in ${COMPILERS}; do
        binary="./build/opt/${compiler}/fp16/gemm_ime_opt"
        [ -x "${binary}" ] || continue

        for size in ${SIZES}; do
            for t in ${A100_THREADS_KC}; do
                run_binary a100 \
                    "opt_ime_${compiler}_fp16_t${t}_n${size}" \
                    "${binary}" ${t} ${ITERATIONS} ${size} 8
            done
        done
    done
}

run_hetero_rvv_rvv() {
    local compiler precision lmul binary share

    for precision in ${PRECISIONS}; do
        for compiler in ${COMPILERS}; do
            for lmul in ${HETERO_LMULS}; do
                binary="./build/hetero/${compiler}/${precision}/gemm_hetero_rvv_rvv_${lmul}"
                [ -x "${binary}" ] || continue

                for share in ${HETERO_A100_SHARES}; do
                    for size in ${SIZES}; do
                        run_and_log \
                            "gemm_hetero_rvv_rvv_${lmul}_${compiler}_${precision}_x${HETERO_X100_THREADS}_a${HETERO_A100_THREADS}_share${share}_n${size}" \
                            env OMP_DYNAMIC=false X100_THREADS=${HETERO_X100_THREADS} A100_THREADS=${HETERO_A100_THREADS} A100_SHARE=${share} A100_BIND=thread X100_CPUS=0-7 \
                            "${binary}" ${ITERATIONS} ${size} 16
                    done
                done
            done
        done
    done
}

run_hetero_rvv_ime() {
    local compiler lmul binary share

    for compiler in ${COMPILERS}; do
        for lmul in ${HETERO_LMULS}; do
            binary="./build/hetero/${compiler}/fp16/gemm_hetero_rvv_ime_${lmul}"
            [ -x "${binary}" ] || continue

            for share in ${HETERO_A100_SHARES}; do
                for size in ${SIZES}; do
                    run_and_log \
                        "gemm_hetero_rvv_ime_${lmul}_${compiler}_fp16_x${HETERO_X100_THREADS}_a${HETERO_A100_THREADS}_share${share}_n${size}" \
                        env OMP_DYNAMIC=false X100_THREADS=${HETERO_X100_THREADS} A100_THREADS=${HETERO_A100_THREADS} A100_SHARE=${share} A100_BIND=thread X100_CPUS=0-7 \
                        "${binary}" ${ITERATIONS} ${size} 8
                done
            done
        done
    done
}

case "${RUN_MODE}" in
    copt)
        run_copt
        ;;

    rvv_kc)
        for core in x100 a100; do
            for precision in ${PRECISIONS}; do
                for compiler in ${COMPILERS}; do
                    run_rvv_kc_sweep "${core}" "${compiler}" "${precision}"
                done
            done
        done
        ;;

    rvv_nr)
        for core in x100 a100; do
            for precision in ${PRECISIONS}; do
                for compiler in ${COMPILERS}; do
                    run_rvv_nr_sweep "${core}" "${compiler}" "${precision}"
                done
            done
        done
        ;;

    rvv_block)
        for core in x100 a100; do
            for precision in ${PRECISIONS}; do
                for compiler in ${COMPILERS}; do
                    run_rvv_block_sweep "${core}" "${compiler}" "${precision}"
                done
            done
        done
        ;;

    rvv_scaling)
        for core in x100 a100; do
            for precision in ${PRECISIONS}; do
                for compiler in ${COMPILERS}; do
                    run_rvv_scaling "${core}" "${compiler}" "${precision}"
                done
            done
        done
        ;;

    rvv_all)
        for mode in rvv_kc rvv_nr rvv_block rvv_scaling; do
            RUN_MODE="${mode}" "$0"
        done
        ;;

    ime_opt)
        run_ime_opt
        ;;

    hetero_rvv_rvv)
        run_hetero_rvv_rvv
        ;;

    hetero_rvv_ime)
        run_hetero_rvv_ime
        ;;

    hetero_all)
        for mode in hetero_rvv_rvv hetero_rvv_ime; do
            RUN_MODE="${mode}" "$0"
        done
        ;;

    publication_all)
        for mode in copt rvv_all ime_opt hetero_all; do
            RUN_MODE="${mode}" "$0"
        done
        ;;

    baseline)
        echo "Baseline collection is intentionally unchanged; use your existing baseline runner."
        ;;

    *)
        echo "Unknown RUN_MODE=${RUN_MODE}" >&2
        exit 1
        ;;
esac
