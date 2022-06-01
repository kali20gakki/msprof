#!/bin/bash
# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

# get install path
install_path=${1}

# script for spc
ROLLBACK_PRECHECK=rollback_precheck.sh
ROLLBACK=rollback_spc.sh
UNINSTALL_SPC=uninstall_spc.sh
COMMON_ROLLBACK=rollback.sh
COMMON_UNINSTALL=uninstall.sh
UTILS_SCRIPT=utils.sh

# common dir
COMMON_DIR="common_script"

function backup_product() {
    get_cann_package_name
    
	if [ "$cann_package_name" = "ascend-toolkit" ]; then
		copy_product ${ANALYSIS} ${ANALYSIS_PATH}
		copy_product ${MSPROF} ${MSPROF_PATH}
	fi
	
	if [ "$cann_package_name" != "nnae" ]; then
		copy_product ${LIBMSPROFILER} ${LIBMSPROFILER_PATH}/stub
	fi

	copy_product ${LIBMSPROFILER} ${LIBMSPROFILER_PATH}
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

function copy_product() {
	local filename=${1}
    local relative_location=${2}
    local backup_path=${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}
    local target_file=$(readlink -f ${install_path}/${relative_location}/${filename})

	if [ ! -f ${filename} ] && [ ! -d ${filename} ]; then
		return
	fi

	if [ -f ${target_file} ] || [ -d ${target_file} ]; then
		if [ -d ${backup_path} ]; 
		then
			chmod -R a+w ${backup_path}
		else
			mkdir -p ${backup_path}
			chmod -R a+w ${backup_path}
		fi

		if [ ! -f ${backup_path}/${relative_location}/${filename} ] && [ ! -d ${backup_path}/${relative_location}/${filename} ]; 
		then
			mkdir -p ${backup_path}/${relative_location}
		fi
        cp -r ${target_file} ${backup_path}/${relative_location}
		chmod -R a-w ${backup_path}
	fi
}

function backup_script() {
	if [ ! -d ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME} ]; then
		create_script_dir
		copy_script
		chmod -R a-w ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
	fi
}

function create_script_dir() {
    mkdir -p ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
}

function copy_script() {
    cp -p ${ROLLBACK_PRECHECK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
    cp -p ${UTILS_SCRIPT} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/
    cp -p ${ROLLBACK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/rollback.sh
    cp -p ${UNINSTALL_SPC} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}/uninstall.sh
}

function place_common_script() {
    cp -np ${COMMON_DIR}/${COMMON_ROLLBACK} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
    cp -np ${COMMON_DIR}/${COMMON_UNINSTALL} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
}

# use utils function and constant
source utils.sh

backup_product
backup_script
place_common_script

