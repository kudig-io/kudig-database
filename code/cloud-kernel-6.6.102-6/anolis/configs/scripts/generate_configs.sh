#! /bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Generate the whole kconfig files.
#
# Copyright (C) 2023 Qiao Ma <mqaio@linux.alibaba.com>

set -e

SCRIPT_DIR=$(realpath $(dirname $0))
FILE_LIST=${DIST_OUTPUT}/file_list

mkdir -p ${DIST_OUTPUT}

if [ -z "$@" ]; then
    python3 ${SCRIPT_DIR}/anolis_kconfig.py generate_translate \
        --input_dir ${SCRIPT_DIR}/../ \
        --output_dir ${DIST_OUTPUT} \
        --src_root ${DIST_SRCROOT} \
        ${DIST_SRCROOT}/${DIST_CONFIG_LAYOUTS} > ${DIST_OUTPUT}/generate.sh
else
    for target in $@
    do
    python3 ${SCRIPT_DIR}/anolis_kconfig.py generate_translate \
        --input_dir ${SCRIPT_DIR}/../ \
        --output_dir ${DIST_OUTPUT} \
        --src_root ${DIST_SRCROOT} \
        --target ${DIST_CONFIG_KERNEL_NAME}/${target} \
        ${DIST_SRCROOT}/${DIST_CONFIG_LAYOUTS} > ${DIST_OUTPUT}/generate.sh
    done
fi

bash ${DIST_OUTPUT}/generate.sh | tee ${FILE_LIST}

if [ "x${DIST_DO_GENERATE_DOT_CONFIG}" = "xY" ]; then
    file=$(cat ${FILE_LIST} | grep "processed" | awk '{print $4}' | head -1)
    cp -f ${file} ${DIST_SRCROOT}.config
fi