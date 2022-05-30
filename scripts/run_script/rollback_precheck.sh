#!/bin/bash
function file_check() {
    if [ ! -f "$1" ]; then
        print "ERROR" "the source file $2 does not exist, rollback failed"
        exit 1
    fi
}

function deal_precheck() {
    # check spc backup so
    file_check ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER} ${LIBMSPROFILER}

    exit 0
}

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"

# script for spc
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

# use utils function and constant
source $(dirname "$0")/utils.sh

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

deal_precheck