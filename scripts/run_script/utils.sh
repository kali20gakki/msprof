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

# init log file
log_file=$(get_log_file)
log_init