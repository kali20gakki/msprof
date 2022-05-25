#!/bin/bash
install_path=${1}
MSPROF_RUN_NAME=${2}
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"
MSPROF_RUN_NAME="mindstudio-msprof"

# script for spc
ROLLBACK_PRECHECK=rollback_precheck.sh
ROLLBACK=rollback.sh
UNINSTALL=uninstall.sh

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

function backup_product() {
    create_backup_dir
    copy_product
}

function create_backup_dir() {
    mkdir -p ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}
}

function copy_product() {
    cp -r -p ${LIBMSPROFILER} ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}/
}

function backup_script() {
    create_script_dir
    copy_script
}

function create_script_dir() {
    mkdir -p ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
}

function copy_script() {
    cp -r -p ${ROLLBACK_PRECHECK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
    cp -r -p ${ROLLBACK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
    cp -r -p ${UNINSTALL} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
}

backup_product
backup_script
