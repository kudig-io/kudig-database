#! /bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# To modify kconfigs.
#
# Copyright (C) 2024 Qiao Ma <mqaio@linux.alibaba.com>

set -e

SCRIPT_DIR=$(realpath $(dirname $0))
BASE_CONFIG_DIR=$(realpath ${SCRIPT_DIR}/..)
DIST_CONFIG_DIR=${BASE_CONFIG_DIR}

if [ "$DIST_CONFIG_KERNEL_NAME" != "ANCK" ]; then
    DIST_CONFIG_DIR=$(realpath ${BASE_CONFIG_DIR}/OVERRIDE/${DIST_CONFIG_KERNEL_NAME}/);
fi

function die() {
    echo ""
    echo $@
    echo "usage:"
    echo "    make dist-configs-modify" \
         "C=<conf_name> L=<level> [x86=<value>] [arm64=<value>] [loongarch=<value>] [riscv=<value>] [others=<value>] [all=<value>]"
    echo "    C: the config name, must be specified"
    echo "    L: the level of config, must be specified"
    echo "    x86: the value of x86 architecture"
    echo "    arm64: the value of arm64 architecture"
    echo "    loongarch: the value of loongarch architecture"
    echo "    riscv: the value of riscv architecture"
    echo "    others: the default value for the architectures that not be specified"
    echo "    all: the value for all architectures"
    echo ""
    echo "example:"
    echo "    - only set x86 to y"
    echo "      make dist-configs-modify C=CONFIG_CRYPTO_ECDSA x86=y arm=n others=n L=L1"
    echo "    - set all archs to y"
    echo "      make dist-configs-modify C=CONFIG_CRYPTO_ECDSA all=y L=L1"
    echo ""
    exit 1
}

declare -A ARCH_VALUES

function collect_ARCH_VALUES() {
    if [ -n "${x86}" ]; then ARCH_VALUES["x86"]=${x86}; fi
    if [ -n "${arm64}" ]; then ARCH_VALUES["arm64"]=${arm64}; fi
    if [ -n "${loongarch}" ]; then ARCH_VALUES["loongarch"]=${loongarch}; fi
    if [ -n "${riscv}" ]; then ARCH_VALUES["riscv"]=${riscv}; fi
    if [ -n "${others}" ]; then ARCH_VALUES["default"]=${others}; fi
    if [ -n "${all}" ]; then ARCH_VALUES["default"]=${all}; fi

    if [ ${#ARCH_VALUES[@]} -eq 0 ]; then
        die "need to specify at least one architecture's value";
    fi
}

function set_correct_level() {
    case $L in
        "L0"|"L0-MANDATORY")
        L="L0-MANDATORY"
        ;;
        "L1"|"L1-RECOMMEND")
        L="L1-RECOMMEND"
        ;;
        "L2"|"L2-OPTIONAL")
        L="L2-OPTIONAL"
        ;;
        *)
        die "unsupported level: $L"
        ;;
    esac
}

function check_args() {
    if [ -z "$C" ]; then die "the config name must be specified"; fi
    if [ -z "$L" ]; then die "the level must be specified"; fi
    collect_ARCH_VALUES
    set_correct_level
}

function remove_old_configs() {
    for f in $(find ${DIST_CONFIG_DIR}/L* -type f -name "$C")
    do
     echo "remove old file: $f"
     rm -f $f
    done
}

function add_new_configs() {
    for arch in ${!ARCH_VALUES[@]}; do
        local value=${ARCH_VALUES[${arch}]}
        local text="$C=$value"
        if [ "$value" = "n" ]; then text="# $C is not set"; fi

        mkdir -p ${DIST_CONFIG_DIR}/${L}/${arch}
        echo "$text" > ${DIST_CONFIG_DIR}/${L}/${arch}/$C;
        echo "created new file: ${DIST_CONFIG_DIR}/${L}/${arch}/$C"
    done
}

function refresh_configs() {
    echo "refresh configs"
    bash ${SCRIPT_DIR}/update_configs.sh
}

CHECK_FOUND_FILE=0

function check_config_for_one_arch() {
    local arch=$1
    if [ -f ${DIST_CONFIG_DIR}/${L}/${arch}/$C ]; then
        echo "$arch: $(cat ${DIST_CONFIG_DIR}/${L}/${arch}/$C)"
        CHECK_FOUND_FILE=1
    fi
}

function check_config() {
    local appears=0
    echo "The Final Configs After Refresh"
    check_config_for_one_arch "x86"
    check_config_for_one_arch "arm64"
    check_config_for_one_arch "loongarch"
    check_config_for_one_arch "riscv"
    check_config_for_one_arch "default"
    if [ "$CHECK_FOUND_FILE" == "0" ]; then
        echo "Not Found Any Valid config files, maybe some dependency not satisfied"
    fi
    echo ""
    echo "******************************************************************************"
}

function main() {
    check_args
    remove_old_configs
    add_new_configs
    refresh_configs
    check_config
}

main