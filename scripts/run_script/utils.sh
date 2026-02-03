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

MSPROF_RUN_NAME="mindstudio-profiler"
# product constant
LIB_MS_PROFILER="libmsprofiler.so"
LIB_PROF_API="libprofapi.so"
LIB_PROF_IMPL="libprofimpl.so"
ACL_PROF_H="acl_prof.h"
GE_PROF_H="ge_prof.h"

# never use analysis/, or remove important file by softlink
ANALYSIS="analysis"
MSPROF="msprof"

ANALYSIS_PATH="tools/profiler/profiler_tool"
MSPROF_PATH="tools/profiler/bin"
SOFT_LINK_MSPROF_PATH="bin/msprof"

# msprof analysis whl
MSPROF_ANALYSIS_WHL="msprof-0.0.1-py3-none-any.whl"

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

function check_path() {
    local path_str=${1}
    # check the existence of the path
    if [ ! -e "${path_str}" ]; then
        print "ERROR" "The path ${path_str} does not exist, please check."
        exit 1
    fi
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

function check_cann_path() {
    local cann_path=${1}
    local current_user=$(whoami)
    local cann_path_owner=$(stat -c '%U' "$cann_path")

    if [ "$current_user" != "root" ]; then
        # 1. Check the current executing user and the installation user of cann_path.
        if [ "$current_user" != "$cann_path_owner" ]; then
            print "ERROR" "Current user ($current_user) is not the same as the owner of cann_path ($cann_path_owner)."
            exit 1
        fi

        # 2. Check whether the current cann_path is writable (except for the root user).
        if [ ! -w "$cann_path" ]; then
            print "ERROR" "cann_path ($cann_path) is not writable by the current user ($current_user)."
            exit 1
        fi
    fi
}

# init log file
log_file=$(get_log_file)
log_init