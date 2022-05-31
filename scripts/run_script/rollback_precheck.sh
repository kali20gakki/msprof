#!/bin/bash
# script for spc
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

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

# use utils function and constant
source $(dirname "$0")/utils.sh

deal_precheck