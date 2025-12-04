#!/bin/bash
# right constant
root_right=555
user_right=550

script_right=500

root_libmspti_right=444
user_libmspti_right=440

PATH_LENGTH=4096

MSPTI_RUN_NAME="MindStudio-Profilier-Tools-Interface"
# product constant
LIBMSPTI="libmspti.so"
LIBMSPTI_PATH="tools/mspti"
MSPTI_WHL="mspti-0.0.1-py3-none-any.whl"

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"

# log level
LEVEL_ERROR="ERROR"
LEVEL_WARN="WARNING"
LEVEL_INFO="INFO"

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[${MSPTI_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[${MSPTI_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
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
            print $LEVEL_ERROR "touch $log_file permission denied"
            exit 1
        fi
    fi
    chmod 640 $log_file
}

function check_path() {
    local path_str=${1}
    # check the length of path
    if [ ${#path_str} -gt ${PATH_LENGTH} ]; then
        print $LEVEL_ERROR "parameter error $path_str, the length exceeds ${PATH_LENGTH}."
        exit 1
    fi
    # check absolute path
    if [[ ! "${path_str}" =~ ^/.* ]]; then
        print $LEVEL_ERROR "parameter error $path_str, must be an absolute path."
        exit 1
    fi
    # black list
    if echo "${path_str}" | grep -Eq '\/{2,}|\.{3,}'; then
        print $LEVEL_ERROR "The path ${path_str} is invalid, cannot contain the following characters: // ...!"
        exit 1
    fi
    # white list
    if echo "${path_str}" | grep -Eq '^\~?[a-zA-Z0-9./_-]*$'; then
        :
    else
        print $LEVEL_ERROR "The path ${path_str} is invalid, only [a-z,A-Z,0-9,-,_] is support!"
        exit 1
    fi
    # check the existence of the path
    if [ ! -e "$install_path/ascend-toolkit" ]; then
        print $LEVEL_ERROR "The path ${install_path} does not exist, please check."
        exit 1
    fi
}

# init log file
log_file=$(get_log_file)
log_init