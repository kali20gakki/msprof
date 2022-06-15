#!/bin/bash

# use utils function and constant
source utils.sh

# spc dir
SPC_DIR="spc"
BACKUP_DIR="backup"
SCRIPT_DIR="script"

# product
LIBMSPROFILER_PATH="/runtime/lib64/"
LIBMSPROFILER="libmsprofiler.so"

# get install path
install_path=${1}
backup_path=${install_path}/${SPC_DIR}/${BACKUP_DIR}/${MSPROF_RUN_NAME}

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
	prepar_backup_path ${backup_path}

	if [ "$cann_package_name" = "ascend-toolkit" ]; then
		copy_product ${install_path}/${MSPROF_PATH}/${MSPROF} ${MSPROF}
		copy_product ${install_path}/${ANALYSIS_PATH}/${ANALYSIS} ${ANALYSIS}
	fi
	
	if [ "$cann_package_name" != "nnae" ]; then
		copy_product ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER_STUB} ${LIBMSPROFILER_STUB}
	fi

	copy_product ${install_path}/${LIBMSPROFILER_PATH}/${LIBMSPROFILER} ${LIBMSPROFILER}
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

function prepar_backup_path() {
	if [ -d ${backup_path} ]; then
		return
	fi

	mkdir -p ${backup_path}/stub
	chmod -R ${mindstudio_msprof_spc_right} ${backup_path}
}

function copy_product() {
	local installed_filename=$(readlink -f ${1})
    local backup_filename=${backup_path}/${2}

	if [ ! -f ${installed_filename} ] && [ ! -d ${installed_filename} ]; then
		return
	fi

	chmod -R u+w ${backup_path}
	cp -rp ${installed_filename} ${backup_filename}
	chmod -R u-w ${backup_path}
}

function backup_script() {
	if [ ! -d ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME} ]; then
		create_script_dir
		chmod -R u+w ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
		copy_script
		chmod -R u-w ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
	fi
}

function create_script_dir() {
    mkdir -p ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
	chmod -R ${mindstudio_msprof_spc_right} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}/${MSPROF_RUN_NAME}
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

function set_common_dir_right() {
	local common_spc_right
	if [ "$UID" = "0" ]; then
		common_spc_right=755
	else
		common_spc_right=750
	fi
	chmod ${common_spc_right} ${install_path}/${SPC_DIR}
	chmod ${common_spc_right} ${install_path}/${SPC_DIR}/${BACKUP_DIR}
	chmod ${common_spc_right} ${install_path}/${SPC_DIR}/${SCRIPT_DIR}
}

backup_product
backup_script
place_common_script
set_common_dir_right

