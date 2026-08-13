#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

RUN_MODE=${RUN_MODE:-publication_all}
COMPILERS=${COMPILERS:-"clang gcc"}
PRECISIONS=${PRECISIONS:-"float double"}
SIZES=${SIZES:-"2000"}
DRY_RUN=${DRY_RUN:-0}

OPT_COPT_NRS=${OPT_COPT_NRS:-"2 4 6 8"}
OPT_C_KCS_FLOAT=${OPT_C_KCS_FLOAT:-"128"}
OPT_C_KCS_DOUBLE=${OPT_C_KCS_DOUBLE:-"64"}
COPT_THREADS=${COPT_THREADS:-"8"}

X100_MCS=${X100_MCS:-"24 32 40 48"}
X100_NCS=${X100_NCS:-"48 64 80 96"}
X100_NRS=${X100_NRS:-"8"}
X100_KCS_DOUBLE=${X100_KCS_DOUBLE:-"48 56 64 72 80 96"}
X100_KCS_FLOAT=${X100_KCS_FLOAT:-"96 112 128 144 160 192"}
X100_THREADS_KC=${X100_THREADS_KC:-"8"}
X100_THREADS_SCALING=${X100_THREADS_SCALING:-"1 2 4 6 8"}

A100_MCS=${A100_MCS:-"64"}
A100_NCS=${A100_NCS:-"64"}
A100_NRS=${A100_NRS:-"4 6 8"}
A100_KCS_DOUBLE=${A100_KCS_DOUBLE:-"32 48 64 80 96 128"}
A100_KCS_FLOAT=${A100_KCS_FLOAT:-"64 96 128 160 192"}
A100_THREADS_KC=${A100_THREADS_KC:-"8"}
A100_THREADS_SCALING=${A100_THREADS_SCALING:-"1 2 4 8"}

HETERO_LMULS=${HETERO_LMULS:-"1 2 4 8"}
HETERO_A100_SHARES=${HETERO_A100_SHARES:-"0 10 20 30 40 50 60 70 80 90 100"}

INCLUDE_BASELINE=${INCLUDE_BASELINE:-0}
BASELINE_RUNNER=${BASELINE_RUNNER:-}

word_count() {
    local list=$1
    if [ -z "${list}" ]; then
        echo 0
        return
    fi

    set -- ${list}
    echo $#
}

per_precision_copt_kcs() {
    case "$1" in
        float) echo "${OPT_C_KCS_FLOAT}" ;;
        double) echo "${OPT_C_KCS_DOUBLE}" ;;
        *)
            echo "Unsupported precision: $1" >&2
            exit 1
            ;;
    esac
}

per_precision_x100_kcs() {
    case "$1" in
        float) echo "${X100_KCS_FLOAT}" ;;
        double) echo "${X100_KCS_DOUBLE}" ;;
        *)
            echo "Unsupported precision: $1" >&2
            exit 1
            ;;
    esac
}

per_precision_a100_kcs() {
    case "$1" in
        float) echo "${A100_KCS_FLOAT}" ;;
        double) echo "${A100_KCS_DOUBLE}" ;;
        *)
            echo "Unsupported precision: $1" >&2
            exit 1
            ;;
    esac
}

coverage_copt() {
    local total=0 precision

    for precision in ${PRECISIONS}; do
        total=$((total + $(word_count "${OPT_COPT_NRS}") * $(word_count "$(per_precision_copt_kcs "${precision}")") ))
    done

    echo $(( $(word_count "${COMPILERS}") * $(word_count "${SIZES}") * $(word_count "${COPT_THREADS}") * total ))
}

coverage_rvv_kc() {
    local total=0 precision

    for precision in ${PRECISIONS}; do
        total=$((total + $(word_count "$(per_precision_x100_kcs "${precision}")") * $(word_count "${X100_THREADS_KC}") ))
        total=$((total + $(word_count "$(per_precision_a100_kcs "${precision}")") * $(word_count "${A100_THREADS_KC}") ))
    done

    echo $(( $(word_count "${COMPILERS}") * $(word_count "${SIZES}") * total ))
}

coverage_rvv_nr() {
    local per_precision

    per_precision=$(( $(word_count "${X100_NRS}") * $(word_count "${X100_THREADS_KC}") + $(word_count "${A100_NRS}") * $(word_count "${A100_THREADS_KC}") ))
    echo $(( $(word_count "${COMPILERS}") * $(word_count "${PRECISIONS}") * $(word_count "${SIZES}") * per_precision ))
}

coverage_rvv_block() {
    local per_precision

    per_precision=$(( $(word_count "${X100_MCS}") * $(word_count "${X100_NCS}") * $(word_count "${X100_THREADS_KC}") + $(word_count "${A100_MCS}") * $(word_count "${A100_NCS}") * $(word_count "${A100_THREADS_KC}") ))
    echo $(( $(word_count "${COMPILERS}") * $(word_count "${PRECISIONS}") * $(word_count "${SIZES}") * per_precision ))
}

coverage_rvv_scaling() {
    local per_precision

    per_precision=$(( $(word_count "${X100_THREADS_SCALING}") + $(word_count "${A100_THREADS_SCALING}") ))
    echo $(( $(word_count "${COMPILERS}") * $(word_count "${PRECISIONS}") * $(word_count "${SIZES}") * per_precision ))
}

coverage_ime_opt() {
    echo $(( $(word_count "${COMPILERS}") * $(word_count "${SIZES}") * $(word_count "${A100_THREADS_KC}") ))
}

coverage_hetero_rvv_rvv() {
    echo $(( $(word_count "${COMPILERS}") * $(word_count "${PRECISIONS}") * $(word_count "${HETERO_LMULS}") * $(word_count "${HETERO_A100_SHARES}") * $(word_count "${SIZES}") ))
}

coverage_hetero_rvv_ime() {
    echo $(( $(word_count "${COMPILERS}") * $(word_count "${HETERO_LMULS}") * $(word_count "${HETERO_A100_SHARES}") * $(word_count "${SIZES}") ))
}

print_run_coverage() {
    local copt rvv_kc rvv_nr rvv_block rvv_scaling rvv_all ime_opt hetero_rvv_rvv hetero_rvv_ime hetero_all publication_all

    copt=$(coverage_copt)
    rvv_kc=$(coverage_rvv_kc)
    rvv_nr=$(coverage_rvv_nr)
    rvv_block=$(coverage_rvv_block)
    rvv_scaling=$(coverage_rvv_scaling)
    rvv_all=$((rvv_kc + rvv_nr + rvv_block + rvv_scaling))
    ime_opt=$(coverage_ime_opt)
    hetero_rvv_rvv=$(coverage_hetero_rvv_rvv)
    hetero_rvv_ime=$(coverage_hetero_rvv_ime)
    hetero_all=$((hetero_rvv_rvv + hetero_rvv_ime))
    publication_all=$((copt + rvv_all + ime_opt + hetero_all))

    echo "Run coverage (scheduled runs if binaries exist)"
    echo "  copt            : ${copt}"
    echo "  rvv_kc          : ${rvv_kc}"
    echo "  rvv_nr          : ${rvv_nr}"
    echo "  rvv_block       : ${rvv_block}"
    echo "  rvv_scaling     : ${rvv_scaling}"
    echo "  rvv_all         : ${rvv_all}"
    echo "  ime_opt         : ${ime_opt}"
    echo "  hetero_rvv_rvv  : ${hetero_rvv_rvv}"
    echo "  hetero_rvv_ime  : ${hetero_rvv_ime}"
    echo "  hetero_all      : ${hetero_all}"
    echo "  publication_all : ${publication_all}"
    if [ "${INCLUDE_BASELINE}" = "1" ]; then
        echo "  baseline        : external via BASELINE_RUNNER"
    fi
    echo
}

run_baseline_if_requested() {
    if [ "${INCLUDE_BASELINE}" != "1" ]; then
        return
    fi

    if [ -z "${BASELINE_RUNNER}" ]; then
        echo "INCLUDE_BASELINE=1 requires BASELINE_RUNNER=/path/to/script" >&2
        exit 1
    fi

    echo "Running baseline via ${BASELINE_RUNNER}"
    "${BASELINE_RUNNER}"
}

main() {
    print_run_coverage

    if [ "${DRY_RUN}" = "1" ]; then
        return
    fi

    run_baseline_if_requested

    env \
        RUN_MODE="${RUN_MODE}" \
        COMPILERS="${COMPILERS}" \
        PRECISIONS="${PRECISIONS}" \
        SIZES="${SIZES}" \
        OPT_COPT_NRS="${OPT_COPT_NRS}" \
        OPT_C_KCS_FLOAT="${OPT_C_KCS_FLOAT}" \
        OPT_C_KCS_DOUBLE="${OPT_C_KCS_DOUBLE}" \
        COPT_THREADS="${COPT_THREADS}" \
        X100_MCS="${X100_MCS}" \
        X100_NCS="${X100_NCS}" \
        X100_NRS="${X100_NRS}" \
        X100_KCS_DOUBLE="${X100_KCS_DOUBLE}" \
        X100_KCS_FLOAT="${X100_KCS_FLOAT}" \
        X100_THREADS_KC="${X100_THREADS_KC}" \
        X100_THREADS_SCALING="${X100_THREADS_SCALING}" \
        A100_MCS="${A100_MCS}" \
        A100_NCS="${A100_NCS}" \
        A100_NRS="${A100_NRS}" \
        A100_KCS_DOUBLE="${A100_KCS_DOUBLE}" \
        A100_KCS_FLOAT="${A100_KCS_FLOAT}" \
        A100_THREADS_KC="${A100_THREADS_KC}" \
        A100_THREADS_SCALING="${A100_THREADS_SCALING}" \
        HETERO_LMULS="${HETERO_LMULS}" \
        HETERO_A100_SHARES="${HETERO_A100_SHARES}" \
        "${SCRIPT_DIR}/run.sh"
}

main "$@"
