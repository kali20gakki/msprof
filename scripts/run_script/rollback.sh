#!/bin/bash
function deal_rollback() {
    copy_file ${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER} ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER}
    bash ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/${UNINSTALL_SCRIPT}
}

function copy_file() {
	local filename=${1}
	local target_file=$(readlink -f ${2})
	if [ ! -f "$filename" ] && [ ! -d "$filename" ]; then
		return
	fi

	if [ -f "$target_file" ] || [ -d "$target_file" ]; then
		chmod u+w $(dirname ${target_file})
		rm -r -f ${target_file}
		
		cp -r -p ${filename} ${target_file}
		chmod u-w $(dirname ${target_file})
		
		print "INFO" "$filename is replaced."
		return
	fi
}

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

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"
MSPROF_RUN_NAME="mindstudio-msprof"

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

# get install path
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

# script for spc
UNINSTALL_SCRIPT="uninstall.sh"

deal_rollback