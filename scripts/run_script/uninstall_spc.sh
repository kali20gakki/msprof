#!/bin/bash
function del_file() {
    file_path=$1

    if [ -f "${file_path}" ]; then
        rm -f "${file_path}"
        if [ $? = 0 ]; then
            print "INFO" "delete file ${file_path} successfully"
        else
            print "ERROR" "delete file ${file_path} fail"
            exit 1
        fi
    else
        print "WARNING" "the file ${file_path} is not exist"
    fi
}

function del_dir() {
    local dir_path=$1

    if [ -n "${dir_path}" ] && [[ ! "${dir_path}" =~ ^/+$ ]]; then
        if [ -d "${dir_path}" ]; then
            chmod 750 -R ${dir_path}
            rm -rf "${dir_path}"
            if [ $? = 0 ]; then
                print "INFO" "delete directory ${dir_path} successfully"
            else
                print "ERROR" "delete directory ${dir_path} fail"
                exit 1
            fi
        else
            print "WARNING" "the directory ${dir_path} is not exist"
        fi
    else
        print "WARNING" "the directory ${dir_path} path is NULL"
    fi
}

function remove_empty_dir() {
    [ ! -d "$1" ] && return 1
    if [ -z "$(ls -A $1 2>&1)" ]; then
        rm -rf "$1"
        if [ $? = 0 ]; then
            print "INFO" "delete directory $1 successfully"
        else
            print "ERROR" "delete directory $1 fail"
            exit 1
        fi
    fi
}

is_dir_empty() {
    [ ! -d "$1" ] && return 1
    [ "$(ls -A "$1" 2>&1)" != "" ] && return 2
    return 0
}

remove_dir_if_empty() {
    local dirpath="$1"
    is_dir_empty "${dirpath}"
    if [ $? -eq 0 ]; then
        rm -rf "${dirpath}"
    fi
    return 0
}

function deal_uninstall() {
    # delete product
    del_file ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER}

    # delete backup
    del_dir ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
    remove_empty_dir ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
    remove_dir_if_empty ${install_path}/${SPC_DIR}/${BACKUP_DIR}
    print "INFO" "${MSPROF_RUN_NAME} uninstalled successfully, the directory ${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME} has been deleted"

    # delete script
    del_dir ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
    tree ${install_path}/${SPC_DIR}
    remove_empty_dir ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
    print "INFO" "${MSPROF_RUN_NAME} uninstalled successfully, the directory ${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME} has been deleted"
}

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

# get install path
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

# use utils function and constant
source $(dirname "$0")/utils.sh

# script for spc
UNINSTALL_SCRIPT="uninstall.sh"

deal_uninstall
