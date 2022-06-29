#!/bin/bash
# right constant
root_right=555
user_right=550

root_ini_right=444
user_ini_right=400

script_right=500

mindstudio_msprof_spc_right=500

root_libmsprofiler_right=444
user_libmsprofiler_right=440

PATH_LENGTH=4096

MSPROF_RUN_NAME="mindstudio-msprof"
# product constant
LIBMSPROFILER="libmsprofiler.so"
LIBMSPROFILER_STUB="stub/libmsprofiler.so"
# never use analysis/, or remove important file by softlink
ANALYSIS="analysis"
MSPROF="msprof"

LIBMSPROFILER_PATH="/runtime/lib64/"
ANALYSIS_PATH="/tools/profiler/profiler_tool/"
MSPROF_PATH="/tools/profiler/bin/"

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"

# hete path
HETE_PATH="hetero-arch-scripts"

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

function rm_file_safe() {
    local file_path=$1
    if [ -n "${file_path}" ]; then
        if [ -f "${file_path}" ] || [ -h "${file_path}" ]; then
            rm -f "${file_path}"
            if [ $? -ne 0 ]; then
                print "ERROR" "delete file ${file_path} failed, please delete it by yourself."
            else
            print "INFO" "delete file ${file_path} successfully"
            fi
        else
            print "WARNING" "the file is not exist"
        fi
    else
        print "WARNING" "the file path is NULL"
    fi
}

function check_path() {
    local path_str=${1}
    # check the length of path
    if [ ${#path_str} -gt ${PATH_LENGTH} ]; then
        print "ERROR" "parameter error $path_str, the length exceeds ${PATH_LENGTH}."
        exit 1
    fi
    # check absolute path
    if [[ ! "${path_str}" =~ ^/.* ]]; then
        print "ERROR" "parameter error $path_str, must be an absolute path."
        exit 1
    fi
    # black list
    if echo "${path_str}" | grep -Eq '\/{2,}|\.{3,}'; then
        print "ERROR" "The path ${path_str} is invalid, cannot contain the following characters: // ...!"
        exit 1
    fi
    # white list
    if echo "${path_str}" | grep -Eq '^\~?[a-zA-Z0-9./_-]*$'; then
        return
    else
        print "ERROR" "The path ${path_str} is invalid, only [a-z,A-Z,0-9,-,_] is support!"
        exit 1
    fi
}

# init log file
log_file=$(get_log_file)
log_init