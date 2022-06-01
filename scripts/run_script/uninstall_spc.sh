#!/bin/bash
# get install path
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

function deal_uninstall() {
    if [ -d ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME} ]; then
        uninstall_product
        uninstall_backup
    fi

    uninstall_script
}

# use utils function and constant
source $(dirname "$0")/utils.sh

deal_uninstall
