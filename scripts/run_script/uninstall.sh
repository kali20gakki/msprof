#!/bin/bash

function print() {
    # 将关键信息打印到屏幕上
    echo "[Mindstudio-msprof] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
}

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

function del_file() {
    file_path=$1
    # 判断是否是文件
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

#安全删除文件夹
function del_dir() {
    local dir_path=$1

    # 判断变量不为空且不是系统根盘
    if [ -n "${dir_path}" ] && [[ ! "${dir_path}" =~ ^/+$ ]]; then
        # 判断是否是目录
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

function deal_uninstall() {
    #if [ -d "$spc_path/backup/pyACL" ]; then
    #    chmod 750 -R $spc_path/backup/pyACL
    #fi

    # delete product
    del_file ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER}

    # delete script
    del_dir ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
    remove_empty_dir ${install_path}/${SPC_DIR}/${BACKUP_DIR}

    # delete backup
    del_dir ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
    remove_empty_dir ${install_path}/${SPC_DIR}/${SCRIPT_DIR}

    print "INFO" "${PACKAGE_NAME}-${PACKAGE_VERSION} uninstalled successfully, the directory spc/backup/pyACL has been deleted"
}

# location of log
if [ $(id -u) -ne 0 ]; then
    log_dir="${HOME}/var/log/ascend_seclog"
else
    log_dir="/var/log/ascend_seclog"
fi
log_file="${log_dir}/ascend_install.log"

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"
MSPROF_RUN_NAME="mindstudio-msprof"

# uninstall script
UNINSTALL_SCRIPT="uninstall.sh"

# get install path
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"


log_init
deal_uninstall