#!/bin/bash

# location of log
if [ $(id -u) -ne 0 ]; then
    log_dir="${HOME}/var/log/ascend_seclog"
else
    log_dir="/var/log/ascend_seclog"
fi
log_file="${log_dir}/ascend_install.log"

function log_init() {
    if [ ! -f "$log_file" ]; then
        touch $log_file
        if [ $? -ne 0 ]; then
            print "ERROR" "touch $log_file permission denied"
            exit 1
        fi
    fi
    chmod 640 $log_file
}

function print() {
    echo "[Mindstudio-msprof] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
}

function dir_check() {
    if [ ! -d "$1" ]; then
        print "ERROR" "dir $2 is not installed, rollback failed"
        exit 1
    fi
}

function file_check() {
    if [ ! -f "$1" ]; then
        print "ERROR" "the source file $2 does not exist, rollback failed"
        exit 1
    fi
}

function deal_precheck() {
    # check spc path
    dir_check ${install_path}/${SPC_DIR} ${SPC_DIR}

    # check real install path
    dir_check $install_path/${LIBMSPROFILER_PATH} ${MSPROF_RUN_NAME}

    # check spc backup so
    file_check ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER} ${LIBMSPROFILER}

    exit 0
}

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
MSPROF_RUN_NAME="mindstudio-msprof"

# get install path
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

log_init
deal_precheck