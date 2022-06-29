#!/bin/bash
# get install path
# use utils function and constant
source $(dirname "$0")/utils.sh

install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

install_path=$(readlink -f ${install_path})
check_path ${install_path}

# script for spc
UNINSTALL_SCRIPT="uninstall.sh"

function deal_rollback() {
	local backup_dir=${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
    copy_file ${backup_dir}/${LIBMSPROFILER} ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER}
    copy_file ${backup_dir}/${ANALYSIS} ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
	copy_file ${backup_dir}/${MSPROF} ${install_path}/${MSPROF_PATH}/${MSPROF}
	copy_file ${backup_dir}/${LIBMSPROFILER_STUB} ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER_STUB}
	
	# uninstall backup
	uninstall_backup
	print "INFO" "${MSPROF_RUN_NAME} rollback successfully !"
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
		
		chmod u+w ${target_parent_dir}
		chmod -R u+w ${target_file}
		rm -rf ${target_file}
		
		cp -rp ${filename} ${target_file}

		chmod ${parent_right} ${target_parent_dir}
		
		print "INFO" "$filename is replaced."
		return
	fi
}

deal_rollback