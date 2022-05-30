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
		local target_parent_dir=$(dirname ${target_file})
		local parent_right=$(stat -c '%a' ${target_parent_dir})
		local filename_right=$(stat -c '%a' ${filename})
		
		chmod u+w ${target_parent_dir}
		rm -rf ${target_file}
		
		cp -r ${filename} ${target_file}
		chmod ${filename_right} ${target_file}

		chmod ${parent_right} ${target_parent_dir}
		
		print "INFO" "$filename is replaced."
		return
	fi
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

deal_rollback