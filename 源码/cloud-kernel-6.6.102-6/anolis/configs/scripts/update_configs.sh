#! /bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# To update kconfigs.
#
# Copyright (C) 2023 Qiao Ma <mqaio@linux.alibaba.com>

set -e

SCRIPT_DIR=$(realpath $(dirname $0))
BASE_CONFIG_DIR=$(realpath ${SCRIPT_DIR}/..)
TMP_DIR=${DIST_OUTPUT}/configs
OLD_CONFIG_DIR=${TMP_DIR}/old
NEW_CONFIG_DIR=${TMP_DIR}/new
BACKUP_CONFIG_DIR=${BASE_CONFIG_DIR}/configs.${DIST_CONFIG_KERNEL_NAME}.old

if [ "${DIST_CONFIG_KERNEL_NAME}" != "ANCK" ]; then
    OLD_DIST_CONFIG_DIR=${BASE_CONFIG_DIR}/OVERRIDE/${DIST_CONFIG_KERNEL_NAME}
    NEW_DIST_CONFIG_DIR=${NEW_CONFIG_DIR}/OVERRIDE/${DIST_CONFIG_KERNEL_NAME}
else
    OLD_DIST_CONFIG_DIR=${BASE_CONFIG_DIR}
    NEW_DIST_CONFIG_DIR=${NEW_CONFIG_DIR}
fi

if [ -n "$DO_IMPORT_CONFIGS" ]; then
    IMPORT_ACTION=${DIST_SRCROOT}/${DIST_CONFIG_ACTIONS_IMPORTS}
else
    IMPORT_ACTION=${DIST_SRCROOT}/${DIST_CONFIG_ACTIONS_REFRESH}
fi


function log() {
    echo $@
}

function prepare_env() {
    rm -rf ${TMP_DIR}
    mkdir -p ${OLD_CONFIG_DIR}
    mkdir -p ${NEW_CONFIG_DIR}
}

function generate_configs() {
    # log "collect all old configs..."
    # generate old config files
    bash ${SCRIPT_DIR}/generate_configs.sh
}

function split_new_configs() {
    # split new config files
    # echo "split new configs..."
    cp ${IMPORT_ACTION} ${DIST_OUTPUT}/kconfig_import
    sed -i "s#%%DIST_OUTPUT%%#\${DIST_OUTPUT}#" ${DIST_OUTPUT}/kconfig_import
    sed -i "s#%%DIST_SRCROOT%%#\${DIST_SRCROOT}#" ${DIST_OUTPUT}/kconfig_import
    python3 ${SCRIPT_DIR}/anolis_kconfig.py import_tanslate \
        --input_dir ${BASE_CONFIG_DIR} \
        --output_dir ${NEW_CONFIG_DIR} \
        --src_root ${DIST_SRCROOT} ${DIST_OUTPUT}/kconfig_import > ${DIST_OUTPUT}/import.sh
    bash -e ${DIST_OUTPUT}/import.sh
}

function replace_with_new_configs() {
    # log "replace old configs with new configs...."

    rm -rf ${BACKUP_CONFIG_DIR}
    mkdir -p ${BACKUP_CONFIG_DIR}
    mkdir -p ${OLD_DIST_CONFIG_DIR}
    for level in ${DIST_LEVELS};
    do
        if [ -d ${OLD_DIST_CONFIG_DIR}/${level} ]; then
            mv ${OLD_DIST_CONFIG_DIR}/${level} ${BACKUP_CONFIG_DIR}
        fi
    done

    for level in ${DIST_LEVELS}
    do
        if [ -d ${NEW_DIST_CONFIG_DIR}/${level} ]; then
            mv ${NEW_DIST_CONFIG_DIR}/${level} ${OLD_DIST_CONFIG_DIR}
        fi
    done
}

function check_configs() {
    # check unknown config files
    echo ""
    echo "******************************************************************************"
    local unknown_dir=${OLD_DIST_CONFIG_DIR}/UNKNOWN
    if [ -d ${unknown_dir} ] && [ -n "$(ls ${unknown_dir})" ]; then
        echo "There are some UNKNOWN level's new configs."
        echo ""
        ls ${unknown_dir}
        echo ""
        echo "Need to classify above configs manually !!!"
        echo "See: ${unknown_dir}"
        echo "HINT: \`make dist-configs-move\` can help you."
        echo "eg: make dist-configs-move C=CONFIG_CAN* L=L2"
    else
        echo ""
        echo "Congratulations, all configs has a determined level."
        echo "**DO NOT FORGET** to add changelogs if any config is changed"
        rm -rf ${BACKUP_CONFIG_DIR}
    fi
    echo ""
    echo "******************************************************************************"
    echo ""
}

prepare_env
if [ -z "$DO_IMPORT_CONFIGS" ]; then
    generate_configs
fi
split_new_configs
replace_with_new_configs
check_configs