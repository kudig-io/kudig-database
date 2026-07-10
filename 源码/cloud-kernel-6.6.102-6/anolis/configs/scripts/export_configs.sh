#! /bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# To export kconfigs as xlsx format.
#
# Copyright (C) 2023 Qiao Ma <mqaio@linux.alibaba.com>

set -e

SCRIPT_DIR=$(realpath $(dirname $0))
BASE_CONFIG_DIR=$(realpath ${SCRIPT_DIR}/..)
FILE_LIST=${DIST_OUTPUT}/file_list
LEVEL_INFO=${DIST_OUTPUT}/level_info

mkdir -p ${DIST_OUTPUT}

bash ${SCRIPT_DIR}/generate_configs.sh | tee ${FILE_LIST}

python3 ${SCRIPT_DIR}/anolis_kconfig.py collect_level --top_dir ${BASE_CONFIG_DIR} \
    --dist ${DIST_CONFIG_KERNEL_NAME}  > ${LEVEL_INFO}

files=$(cat ${FILE_LIST} | grep "generated" | awk '{print $4}' | xargs)

python3 ${SCRIPT_DIR}/anolis_kconfig.py export \
        --level_info ${LEVEL_INFO} \
        --output ${DIST_OUTPUT}/configs.xlsx\
        ${files}

echo "* file generated: ${DIST_OUTPUT}/configs.xlsx"