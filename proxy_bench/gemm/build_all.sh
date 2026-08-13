#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

BUILD_DIR=${BUILD_DIR:-build}
BUILD_BASE_ALL=${BUILD_BASE_ALL:-1}
BUILD_OPT_ALL=${BUILD_OPT_ALL:-1}
BUILD_HETERO_ALL=${BUILD_HETERO_ALL:-1}
DRY_RUN=${DRY_RUN:-0}

COMPILERS=${COMPILERS:-"clang gcc"}
PRECISIONS=${PRECISIONS:-"float double"}
LMULS=${LMULS:-"1 2 4 8"}

OPT_TARGETS=${OPT_TARGETS:-"gemm rvv_x100 rvv_a100 ime"}
OPT_COPT_NRS=${OPT_COPT_NRS:-"2 4 6 8"}
OPT_X100_MCS=${OPT_X100_MCS:-"24 32 40 48"}
OPT_X100_NCS=${OPT_X100_NCS:-"48 64 80 96"}
OPT_X100_NRS=${OPT_X100_NRS:-"8"}
OPT_X100_KUS=${OPT_X100_KUS:-"2"}
OPT_A100_MCS=${OPT_A100_MCS:-"64"}
OPT_A100_NCS=${OPT_A100_NCS:-"64"}
OPT_A100_NRS=${OPT_A100_NRS:-"4 6 8"}
OPT_A100_KUS=${OPT_A100_KUS:-"2"}

HETERO_COMPILERS=${HETERO_COMPILERS:-${COMPILERS}}
HETERO_PRECISIONS=${HETERO_PRECISIONS:-${PRECISIONS}}
HETERO_LMULS=${HETERO_LMULS:-${LMULS}}
HETERO_X100_MCPU=${HETERO_X100_MCPU:-spacemit-x100}
HETERO_A100_MCPU=${HETERO_A100_MCPU:-spacemit-a100}
IME_MARCH=${IME_MARCH:-rv64gcv_zfh_zvfh_zvl1024b_xsmtvdotii}

HETERO_CLANG_CC=${HETERO_CLANG_CC:-"clang -std=c11 -pthread"}
HETERO_CLANG_CXX=${HETERO_CLANG_CXX:-"clang++ -std=gnu++20 -pthread -fmax-errors=1"}
HETERO_GCC_CC=${HETERO_GCC_CC:-"gcc -std=c11 -pthread"}
HETERO_GCC_CXX=${HETERO_GCC_CXX:-"g++ -std=gnu++20 -pthread"}

word_count() {
    local list=$1
    if [ -z "${list}" ]; then
        echo 0
        return
    fi

    set -- ${list}
    echo $#
}

contains_word() {
    local needle=$1 item
    shift

    for item in "$@"; do
        if [ "${item}" = "${needle}" ]; then
            return 0
        fi
    done

    return 1
}

precision_flag() {
    case "$1" in
        float) echo 1 ;;
        double) echo 0 ;;
        *)
            echo "Unsupported precision: $1" >&2
            exit 1
            ;;
    esac
}

toolchain_for_hetero() {
    case "$1" in
        clang) echo "${HETERO_CLANG_CC}|${HETERO_CLANG_CXX}|1" ;;
        gcc) echo "${HETERO_GCC_CC}|${HETERO_GCC_CXX}|0" ;;
        *)
            echo "Unsupported compiler: $1" >&2
            exit 1
            ;;
    esac
}

print_build_coverage() {
    local compilers precisions lmuls
    local copt_nrs x100_mcs x100_ncs x100_nrs x100_kus a100_mcs a100_ncs a100_nrs a100_kus
    local base_count=0 opt_gemm_count=0 opt_x100_count=0 opt_a100_count=0 opt_ime_count=0
    local hetero_rvv_rvv_count=0 hetero_rvv_ime_count=0 total=0

    compilers=$(word_count "${COMPILERS}")
    precisions=$(word_count "${PRECISIONS}")
    lmuls=$(word_count "${LMULS}")

    copt_nrs=$(word_count "${OPT_COPT_NRS}")
    x100_mcs=$(word_count "${OPT_X100_MCS}")
    x100_ncs=$(word_count "${OPT_X100_NCS}")
    x100_nrs=$(word_count "${OPT_X100_NRS}")
    x100_kus=$(word_count "${OPT_X100_KUS}")
    a100_mcs=$(word_count "${OPT_A100_MCS}")
    a100_ncs=$(word_count "${OPT_A100_NCS}")
    a100_nrs=$(word_count "${OPT_A100_NRS}")
    a100_kus=$(word_count "${OPT_A100_KUS}")

    if [ "${BUILD_BASE_ALL}" = "1" ]; then
        base_count=$((compilers * precisions * (2 + lmuls)))
    fi

    if [ "${BUILD_OPT_ALL}" = "1" ]; then
        if contains_word gemm ${OPT_TARGETS}; then
            opt_gemm_count=$((compilers * precisions * copt_nrs))
        fi

        if contains_word rvv_x100 ${OPT_TARGETS}; then
            opt_x100_count=$((compilers * precisions * x100_mcs * x100_ncs * x100_nrs * x100_kus))
        fi

        if contains_word rvv_a100 ${OPT_TARGETS}; then
            opt_a100_count=$((compilers * precisions * a100_mcs * a100_ncs * a100_nrs * a100_kus))
        fi

        if contains_word ime ${OPT_TARGETS}; then
            opt_ime_count=${compilers}
        fi
    fi

    if [ "${BUILD_HETERO_ALL}" = "1" ]; then
        hetero_rvv_rvv_count=$(( $(word_count "${HETERO_COMPILERS}") * $(word_count "${HETERO_PRECISIONS}") * $(word_count "${HETERO_LMULS}") ))
        hetero_rvv_ime_count=$(( $(word_count "${HETERO_COMPILERS}") * $(word_count "${HETERO_LMULS}") ))
    fi

    total=$((base_count + opt_gemm_count + opt_x100_count + opt_a100_count + opt_ime_count + hetero_rvv_rvv_count + hetero_rvv_ime_count))

    echo "Build coverage"
    echo "  baseline/autovec : ${base_count}"
    echo "  opt plain-C      : ${opt_gemm_count}"
    echo "  opt RVV x100     : ${opt_x100_count}"
    echo "  opt RVV a100     : ${opt_a100_count}"
    echo "  opt IME          : ${opt_ime_count}"
    echo "  hetero RVV/RVV   : ${hetero_rvv_rvv_count}"
    echo "  hetero RVV/IME   : ${hetero_rvv_ime_count}"
    echo "  total            : ${total}"
    echo
}

build_hetero_one() {
    local mode=$1 compiler=$2 precision=$3 lmul=$4 outdir=$5 output=$6
    local cc cxx clang_flag float_flag

    IFS='|' read -r cc cxx clang_flag <<< "$(toolchain_for_hetero "${compiler}")"
    float_flag=$(precision_flag "${precision}")

    echo "Building ${outdir}/${output}"
    make clean
    make gemm_hetero \
        CC="${cc}" \
        CXX="${cxx}" \
        clang=${clang_flag} \
        hetero=1 \
        HETERO_MODE="${mode}" \
        HETERO_RVV_LMUL=${lmul} \
        HETERO_X100_MCPU="${HETERO_X100_MCPU}" \
        HETERO_A100_MCPU="${HETERO_A100_MCPU}" \
        IME_MARCH="${IME_MARCH}" \
        float=${float_flag}

    mkdir -p "${outdir}"
    mv gemm_hetero "${outdir}/${output}"
    make clean
}

build_hetero_rvv_rvv() {
    local compiler precision lmul outdir output

    for precision in ${HETERO_PRECISIONS}; do
        for compiler in ${HETERO_COMPILERS}; do
            for lmul in ${HETERO_LMULS}; do
                outdir="${BUILD_DIR}/hetero/${compiler}/${precision}"
                output="gemm_hetero_rvv_rvv_${lmul}"
                build_hetero_one rvv_rvv "${compiler}" "${precision}" "${lmul}" "${outdir}" "${output}"
            done
        done
    done
}

build_hetero_rvv_ime() {
    local compiler lmul outdir output

    for compiler in ${HETERO_COMPILERS}; do
        for lmul in ${HETERO_LMULS}; do
            outdir="${BUILD_DIR}/hetero/${compiler}/fp16"
            output="gemm_hetero_rvv_ime_${lmul}"
            build_hetero_one rvv_ime "${compiler}" float "${lmul}" "${outdir}" "${output}"
        done
    done
}

main() {
    print_build_coverage

    if [ "${DRY_RUN}" = "1" ]; then
        return
    fi

    if [ "${BUILD_BASE_ALL}" = "1" ] || [ "${BUILD_OPT_ALL}" = "1" ]; then
        env \
            BUILD_DIR="${BUILD_DIR}" \
            BUILD_BASE="${BUILD_BASE_ALL}" \
            OPT="${BUILD_OPT_ALL}" \
            COMPILERS="${COMPILERS}" \
            PRECISIONS="${PRECISIONS}" \
            LMULS="${LMULS}" \
            OPT_TARGETS="${OPT_TARGETS}" \
            OPT_COPT_NRS="${OPT_COPT_NRS}" \
            OPT_X100_MCS="${OPT_X100_MCS}" \
            OPT_X100_NCS="${OPT_X100_NCS}" \
            OPT_X100_NRS="${OPT_X100_NRS}" \
            OPT_X100_KUS="${OPT_X100_KUS}" \
            OPT_A100_MCS="${OPT_A100_MCS}" \
            OPT_A100_NCS="${OPT_A100_NCS}" \
            OPT_A100_NRS="${OPT_A100_NRS}" \
            OPT_A100_KUS="${OPT_A100_KUS}" \
            IME_MARCH="${IME_MARCH}" \
            "${SCRIPT_DIR}/compiler.sh"
    fi

    if [ "${BUILD_HETERO_ALL}" = "1" ]; then
        build_hetero_rvv_rvv
        build_hetero_rvv_ime
    fi
}

main "$@"
