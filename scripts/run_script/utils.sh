#!/bin/bash
# right constant
MSPROF_RUN_NAME="mindstudio-msprof"
root_right=555
user_right=550

root_ini_right=444
user_ini_right=400

# product constant
LIBMSPROFILER="libmsprofiler.so"
LIBMSPROFILER_STUB="stub/libmsprofiler.so"
ANALYSIS="analysis"
MSPROF="msprof"

LIBMSPROFILER_PATH="/runtime/lib64/"
ANALYSIS_PATH="/tools/profiler/profiler_tool/"
MSPROF_PATH="/tools/profiler/bin/"

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
    fi
}

function get_log_file() {
	local log_dir
	if [ "$UID" = "0" ]; then
		log_dir="/var/log/ascend_seclog"
	else
		log_dir="${HOME}/var/log/ascend_seclog"
	fi
	echo "${log_dir}/ascend_install.log"
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

function uninstall_backup() {
    if [ -d ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME} ]; then
        chmod -R u+w ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
        del_dir ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
        remove_empty_dir ${install_path}/${SPC_DIR}/${BACKUP_DIR}
    fi
}

function uninstall_script() {
    if [ -d ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME} ]; then
        chmod -R u+w ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
        del_dir ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
        remove_empty_dir ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
    fi
}

# init log file
log_file=$(get_log_file)
log_init