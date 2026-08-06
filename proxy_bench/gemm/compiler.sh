#!/bin/bash
set -euo pipefail

BUILD_DIR=${BUILD_DIR:-build}
BUILD_BASE=${BUILD_BASE:-0}

PRECISIONS=${PRECISIONS:-"float double"}
COMPILERS=${COMPILERS:-"clang gcc"}

# ----------------------------------------------------------------------
# Baseline C / autovec controls
# ----------------------------------------------------------------------
LMULS=${LMULS:-"1 2 4 8"}

# ----------------------------------------------------------------------
# Optimized plain-C collection
# ----------------------------------------------------------------------
OPT=${OPT:-1}
OPT_TARGETS=${OPT_TARGETS:-"gemm rvv_x100 rvv_a100 ime"}
OPT_COMPILERS=${OPT_COMPILERS:-"${COMPILERS}"}
OPT_PRECISIONS=${OPT_PRECISIONS:-"${PRECISIONS}"}
OPT_COPT_NRS=${OPT_COPT_NRS:-"2 4 6 8"}

# ----------------------------------------------------------------------
# Optimized RVV compile-time tuning matrix
#
# KC is intentionally NOT here because it is a runtime argument.
#
# Defaults are small enough for staged collection. Override from shell
# for broader sweeps.
# ----------------------------------------------------------------------
OPT_X100_MCS=${OPT_X100_MCS:-"24 32 40 48"}
OPT_X100_NCS=${OPT_X100_NCS:-"48 64 80 96"}
OPT_X100_NRS=${OPT_X100_NRS:-"8"}

OPT_A100_MCS=${OPT_A100_MCS:-"64"}
OPT_A100_NCS=${OPT_A100_NCS:-"64"}
OPT_A100_NRS=${OPT_A100_NRS:-"4 6 8"}

# If/when source files support RVV_OPT_KU, set these to "1 2 4".
# Current sources may still hard-code KU=2; leave at "2" unless updated.
OPT_X100_KUS=${OPT_X100_KUS:-"2"}
OPT_A100_KUS=${OPT_A100_KUS:-"2"}

# ----------------------------------------------------------------------
# Toolchains
# ----------------------------------------------------------------------
CLANG_CC=${CLANG_CC:-"clang -std=c11 -pthread -march=rv64gcv_zfh_zvfh -mcpu=spacemit-a100"}
CLANG_CXX=${CLANG_CXX:-"clang++ -std=gnu++20 -pthread -fmax-errors=1 -march=rv64gcv_zfh_zvfh -mcpu=spacemit-a100"}
GCC_CC=${GCC_CC:-"gcc -std=c11 -pthread -march=rv64gcv_zfh_zvfh -mcpu=spacemit-a100"}
GCC_CXX=${GCC_CXX:-"g++ -std=gnu++20 -pthread -march=rv64gcv_zfh_zvfh -mcpu=spacemit-a100"}

RVV_CLANG_CC=${RVV_CLANG_CC:-"clang -std=c11 -pthread"}
RVV_CLANG_CXX=${RVV_CLANG_CXX:-"clang++ -std=gnu++20 -pthread -fmax-errors=1"}
RVV_GCC_CC=${RVV_GCC_CC:-"gcc -std=c11 -pthread"}
RVV_GCC_CXX=${RVV_GCC_CXX:-"g++ -std=gnu++20 -pthread"}

IME_CLANG_CC=${IME_CLANG_CC:-"clang -std=c11 -pthread"}
IME_CLANG_CXX=${IME_CLANG_CXX:-"clang++ -std=gnu++20 -pthread -fmax-errors=1"}
IME_GCC_CC=${IME_GCC_CC:-"gcc -std=c11 -pthread"}
IME_GCC_CXX=${IME_GCC_CXX:-"g++ -std=gnu++20 -pthread"}

IME_MARCH=${IME_MARCH:-"rv64gcv_zfh_zvfh_zvl1024b_xsmtvdotii"}

PROGRAM=gemm

precision_flag() {
    case "$1" in
        float)  echo "float=1" ;;
        double) echo "float=0" ;;
        *)
            echo "Precision must be float or double" >&2
            exit 1
            ;;
    esac
}

validate_nr() {
    case "$1" in
        2|4|6|8) ;;
        *)
            echo "NR must be one of 2, 4, 6, 8" >&2
            exit 1
            ;;
    esac
}

validate_posint() {
    local name=$1
    local value=$2
    if ! [[ "${value}" =~ ^[1-9][0-9]*$ ]]; then
        echo "${name} must be a positive integer: ${value}" >&2
        exit 1
    fi
}

toolchain_for() {
    local compiler=$1
    local family=$2

    case "${family}:${compiler}" in
        base:clang) echo "${CLANG_CC}|${CLANG_CXX}|1" ;;
        base:gcc)   echo "${GCC_CC}|${GCC_CXX}|0" ;;
        rvv:clang)  echo "${RVV_CLANG_CC}|${RVV_CLANG_CXX}|1" ;;
        rvv:gcc)    echo "${RVV_GCC_CC}|${RVV_GCC_CXX}|0" ;;
        ime:clang)  echo "${IME_CLANG_CC}|${IME_CLANG_CXX}|1" ;;
        ime:gcc)    echo "${IME_GCC_CC}|${IME_GCC_CXX}|0" ;;
        *)
            echo "Unsupported compiler/family combination: ${family}/${compiler}" >&2
            exit 1
            ;;
    esac
}

build_base_one() {
    local compiler=$1 precision=$2 output=$3 nonvec=$4 lmul=${5:-}
    local tc cc cxx clang_flag pflag

    IFS='|' read -r cc cxx clang_flag <<< "$(toolchain_for "${compiler}" base)"
    pflag=$(precision_flag "${precision}")

    echo "Building ${BUILD_DIR}/${compiler}/${precision}/${output}"
    make clean
    if [ -n "${lmul}" ]; then
        make ${PROGRAM} CC="${cc}" CXX="${cxx}" clang=${clang_flag} \
            nonvec=${nonvec} RVV_LMUL=${lmul} ${pflag}
    else
        make ${PROGRAM} CC="${cc}" CXX="${cxx}" clang=${clang_flag} \
            nonvec=${nonvec} ${pflag}
    fi

    mkdir -p "${BUILD_DIR}/${compiler}/${precision}"
    mv "${PROGRAM}" "${BUILD_DIR}/${compiler}/${precision}/${output}"
    make clean
}

build_copt_one() {
    local compiler=$1 precision=$2 nr=$3
    local cc cxx clang_flag pflag outdir output

    validate_nr "${nr}"
    IFS='|' read -r cc cxx clang_flag <<< "$(toolchain_for "${compiler}" base)"
    pflag=$(precision_flag "${precision}")

    outdir="${BUILD_DIR}/opt/${compiler}/${precision}"
    output="gemm_opt_nr${nr}"

    echo "Building ${outdir}/${output}"
    make clean
    make gemm_opt \
        CC="${cc} -DCOPT_NR=${nr}" \
        CXX="${cxx}" \
        clang=${clang_flag} \
        nonvec=0 \
        ${pflag}

    mkdir -p "${outdir}"
    mv gemm_opt "${outdir}/${output}"
    make clean
}

build_rvv_opt_one() {
    local core=$1 compiler=$2 precision=$3 mc=$4 nc=$5 nr=$6 ku=$7
    local cc cxx clang_flag pflag outdir output target mcpu defs

    validate_posint MC "${mc}"
    validate_posint NC "${nc}"
    validate_nr "${nr}"
    validate_posint KU "${ku}"

    IFS='|' read -r cc cxx clang_flag <<< "$(toolchain_for "${compiler}" rvv)"
    pflag=$(precision_flag "${precision}")

    case "${core}" in
        x100)
            target="gemm_rvv_x100_opt"
            mcpu="spacemit-x100"
            ;;
        a100)
            target="gemm_rvv_a100_opt"
            mcpu="spacemit-a100"
            ;;
        *)
            echo "core must be x100 or a100" >&2
            exit 1
            ;;
    esac

    outdir="${BUILD_DIR}/opt/${core}/${compiler}/${precision}"
    output="${target}_mc${mc}_nc${nc}_nr${nr}_ku${ku}"

    # RVV_OPT_KU is passed harmlessly if the source ignores it today.
    defs="-DRVV_OPT_MC=${mc} -DRVV_OPT_NC=${nc} -DRVV_OPT_NR=${nr} -DRVV_OPT_KU=${ku}"

    echo "Building ${outdir}/${output}"
    make clean
    make "${target}" \
        CC="${cc} ${defs}" \
        CXX="${cxx}" \
        clang=${clang_flag} \
        nonvec=0 \
        rvv=1 \
        RVV_MCPU="${mcpu}" \
        ${pflag}

    mkdir -p "${outdir}"
    mv "${target}" "${outdir}/${output}"
    make clean
}

build_ime_opt_one() {
    local compiler=$1
    local cc cxx clang_flag outdir

    IFS='|' read -r cc cxx clang_flag <<< "$(toolchain_for "${compiler}" ime)"
    outdir="${BUILD_DIR}/opt/${compiler}/fp16"

    echo "Building ${outdir}/gemm_ime_opt"
    make clean
    make gemm_ime_opt \
        CC="${cc}" CXX="${cxx}" clang=${clang_flag} \
        nonvec=0 ime=1 IME_MARCH="${IME_MARCH}" float=1

    mkdir -p "${outdir}"
    mv gemm_ime_opt "${outdir}/gemm_ime_opt"
    make clean
}

if [ "${BUILD_BASE}" = "1" ]; then
    for precision in ${PRECISIONS}; do
        precision_flag "${precision}" >/dev/null
        for compiler in ${COMPILERS}; do
            build_base_one "${compiler}" "${precision}" gemm_nonvec 1
            build_base_one "${compiler}" "${precision}" gemm_autovec 0
            for lmul in ${LMULS}; do
                build_base_one "${compiler}" "${precision}" "gemm_autovec_${lmul}" 0 "${lmul}"
            done
        done
    done
fi

if [ "${OPT}" = "1" ]; then
    for target in ${OPT_TARGETS}; do
        case "${target}" in
            gemm)
                for precision in ${OPT_PRECISIONS}; do
                    for compiler in ${OPT_COMPILERS}; do
                        for nr in ${OPT_COPT_NRS}; do
                            build_copt_one "${compiler}" "${precision}" "${nr}"
                        done
                    done
                done
                ;;

            rvv_x100)
                for precision in ${OPT_PRECISIONS}; do
                    for compiler in ${OPT_COMPILERS}; do
                        for mc in ${OPT_X100_MCS}; do
                            for nc in ${OPT_X100_NCS}; do
                                for nr in ${OPT_X100_NRS}; do
                                    for ku in ${OPT_X100_KUS}; do
                                        build_rvv_opt_one x100 "${compiler}" "${precision}" "${mc}" "${nc}" "${nr}" "${ku}"
                                    done
                                done
                            done
                        done
                    done
                done
                ;;

            rvv_a100)
                for precision in ${OPT_PRECISIONS}; do
                    for compiler in ${OPT_COMPILERS}; do
                        for mc in ${OPT_A100_MCS}; do
                            for nc in ${OPT_A100_NCS}; do
                                for nr in ${OPT_A100_NRS}; do
                                    for ku in ${OPT_A100_KUS}; do
                                        build_rvv_opt_one a100 "${compiler}" "${precision}" "${mc}" "${nc}" "${nr}" "${ku}"
                                    done
                                done
                            done
                        done
                    done
                done
                ;;

            ime)
                for compiler in ${OPT_COMPILERS}; do
                    build_ime_opt_one "${compiler}"
                done
                ;;

            *)
                echo "Unknown OPT_TARGET: ${target}" >&2
                exit 1
                ;;
        esac
    done
fi
