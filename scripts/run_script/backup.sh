#!/bin/bash
function backup_product() {
    create_backup_dir
    copy_product
}

function create_backup_dir() {
    mkdir -p ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}
}

function copy_product() {
    cp -r -p -a ${LIBMSPROFILER} ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}/
}

function backup_script() {
    create_script_dir
    copy_script
}

function create_script_dir() {
    mkdir -p ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
}

function copy_script() {
    cp -p ${ROLLBACK_PRECHECK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
    cp -p ${ROLLBACK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/rollback.sh
    cp -p ${UNINSTALL_SPC} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/uninstall.sh
}

function place_common_script() {
    cp -np ${COMMON_DIR}/${COMMON_ROLLBACK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
    cp -np ${COMMON_DIR}/${COMMON_UNINSTALL} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
}

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"
MSPROF_RUN_NAME="mindstudio-msprof"

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

# get install path
install_path=${1}

# script for spc
ROLLBACK_PRECHECK=rollback_precheck.sh
ROLLBACK=rollback.sh
UNINSTALL_SPC=uninstall_spc.sh
COMMON_ROLLBACK=rollback.sh
COMMON_UNINSTALL=uninstall.sh

# common dir
COMMON_DIR="common_script"

backup_product
backup_script
place_common_script

