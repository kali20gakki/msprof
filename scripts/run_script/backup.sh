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
    cp -r -p -a ${ROLLBACK_PRECHECK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
    cp -r -p -a ${ROLLBACK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
    cp -r -p -a ${UNINSTALL} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
}

function place_common_script() {
    cp -r -p -a ${COMMON_DIR}/${COMMON_ROLLBACK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
    cp -r -p -a ${COMMON_DIR}/${COMMON_UNINSTALL} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
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
UNINSTALL=uninstall.sh
COMMON_ROLLBACK=rollback.sh
COMMON_UNINSTALL=uninstall.sh

# common dir
COMMON_DIR="common_script"

backup_product
backup_script
place_common_script

