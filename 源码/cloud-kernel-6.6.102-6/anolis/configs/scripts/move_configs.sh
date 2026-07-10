#! /bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# To adjust the level of kconfig.
#
# Copyright (C) 2023 Qiao Ma <mqaio@linux.alibaba.com>

set -e

SCRIPT_DIR=$(realpath $(dirname $0))
BASE_CONFIG_DIR=$(realpath ${SCRIPT_DIR}/..)

function die() {
    echo ""
    echo $@
    echo "usage:"
    echo "    make dist-config-move OLD=<old_level> C=<conf_name> L=<new_level>"
    echo "    OLD: the old level, default is UNKONWN"
    echo "    C: config name"
    echo "    L: the new level"
    echo "example:"
    echo "  - to move CONFIG_CAN to L1"
    echo "    make dist-config-move OLD=L2 C=CONFIG_CAN L=L1"
    echo ""
    exit 1
}

function check_args() {
    if [ -z "$OLD" ]; then
        OLD="UNKNOWN"
    fi
    if [ -z "$C" ]; then
        die "config name \$C is not specified"
    fi
    if [ -z "$L" ]; then
        die "config level \$L is not specified"
    fi
}

function do_move() {
    python3 ${SCRIPT_DIR}/anolis_kconfig.py move \
        --top_dir ${BASE_CONFIG_DIR} \
        --dist ${DIST_CONFIG_KERNEL_NAME} \
        --old "$OLD" "$C" "$L"
}

check_args
do_move
