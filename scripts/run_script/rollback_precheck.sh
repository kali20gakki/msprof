#!/bin/bash
# script for spc
install_path="$(
    cd "$(dirname "$0")/../../../"
    pwd
)"

function file_check() {
    if [ ! -f "$1" ]; then
        print "ERROR" "the source file $2 does not exist, rollback failed"
        exit 1
    fi
}

function dir_check() {
    if [ ! -d "$1" ]; then
        print "ERROR" "dir $2 is not installed, rollback failed"
        exit 1
    fi
}

function get_cann_package_name() {
	local name_list=(${install_path//// })
	cann_package_name=${name_list[-2]}
	if [ "$cann_package_name" = "ascend-toolkit" ] || [ "$cann_package_name" = "nnrt" ] || [ "$cann_package_name" = "nnae" ]; then
		return
	fi
	print "ERROR" "There is no ascend-toolkit, nnrt or nnae."
	exit 1
}

function deal_precheck() {
    # check spc backup so
    local backup_dir=${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
	if [ "$cann_package_name" = "ascend-toolkit" ]; then
		dir_check ${backup_dir}/${ANALYSIS_PATH}/${ANALYSIS} ${ANALYSIS}
		file_check ${backup_dir}/${MSPROF_PATH}/${MSPROF} ${MSPROF}
	fi
	
	if [ "$cann_package_name" != "nnae" ]; then
		file_check ${backup_dir}${LIBMSPROFILER_PATH}${LIBMSPROFILER_STUB} ${LIBMSPROFILER_STUB}
	fi
    
	file_check ${backup_dir}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER} ${LIBMSPROFILER}
    exit 0
}

# use utils function and constant
source $(dirname "$0")/utils.sh

get_cann_package_name
deal_precheck