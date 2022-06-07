#!/bin/bash
# get install path
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

function deal_uninstall() {
    if [ -d ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME} ]; then
        uninstall_backup
    fi

    uninstall_script
    print "INFO" "${MSPROF_RUN_NAME} uninstalled successfully !"
}

# use utils function and constant
source $(dirname "$0")/utils.sh

deal_uninstall
