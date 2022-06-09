#!/bin/bash

<<<<<<< HEAD
function unregist_uninstall() {
    if [ -f "${install_path}/cann_uninstall.sh" ]; then
        chmod u+w ${install_path}/cann_uninstall.sh
        remove_uninstall_package "${install_path}/cann_uninstall.sh"
=======
function uninstall_product() {
	delete_product ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
    delete_run_dir
}

function delete_product() {
	local target_path=$(readlink -f ${1})
    local parent_dir=$(dirname ${target_path})
    local right=$(stat -c '%a' ${parent_dir})
	if [ ! -f "$target_path" ] && [ ! -d "$target_path" ]; then
		return
	fi
	chmod u+w ${parent_dir}
	
    chmod -R u+w ${target_path}
    rm -rf ${target_path}

	chmod ${right} ${parent_dir}
}
 
function rm_file_safe() {
    local file_path=$1
    if [ -n "${file_path}" ]; then
        if [ -f "${file_path}" ] || [ -h "${file_path}" ]; then
            rm -f "${file_path}"
        else
            print "WARNING" "the file ${file_path} is not exist"
        fi
    else
        print "WARNING" "the file ${file_path} path is NULL"
    fi
}

function uninstall_latest() {
    # runtime has been uninstall
    local leader_package="runtime"

    rm_file_safe ${latest_path}/${MSPROF_RUN_NAME}

    # single version or multi version old
    if [ ! -L "${latest_path}/${leader_package}" ] || [ ! -d "${latest_path}/${leader_package}/../${MSPROF_RUN_NAME}" ]; then
        return
    fi

    # multi version new
    ln -sf ${latest_path}/${leader_package}/../${MSPROF_RUN_NAME} ${latest_path}/${MSPROF_RUN_NAME}
}

function unregist_uninstall() {
    if [ -f "${install_path}/cann_uninstall.sh" ]; then
        chmod u+w ${install_path}/cann_uninstall.sh
        __remove_uninstall_package "${install_path}/cann_uninstall.sh"
>>>>>>> 4faa55eee7f0cbe4d5bcfa7e9c7eb5e5f2e161e0
        if [ -f "${install_path}/cann_uninstall.sh" ]; then
            chmod u-w ${install_path}/cann_uninstall.sh
        fi
    fi
}

<<<<<<< HEAD
function remove_uninstall_package() {
=======
function __remove_uninstall_package() {
>>>>>>> 4faa55eee7f0cbe4d5bcfa7e9c7eb5e5f2e161e0
    local uninstall_file=$1
    if [ -f "${uninstall_file}" ]; then
        sed -i "/uninstall_package \"${MSPROF_RUN_NAME}\/script\"/d" "${uninstall_file}"
        if [ $? -ne 0 ]; then
            print "ERROR" "remove ${uninstall_file} uninstall_package command failed!"
            exit 1
        fi
    fi
    num=$(grep "^uninstall_package " ${uninstall_file} | wc -l)
    if [ ${num} -eq 0 ]; then
        rm -f "${uninstall_file}" > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            print "ERROR" "delete file: ${uninstall_file}failed, please delete it by yourself."
        fi
    fi
}

<<<<<<< HEAD
function uninstall_product() {
	delete_product ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
    delete_mindstudio_msprof
    delete_parent_dir
}

function delete_product() {
	local target_path=${1}

	if [ ! -f "$target_path" ] && [ ! -d "$target_path" ]; then
		return
	fi

    local parent_dir=$(dirname ${target_path})
    local right=$(stat -c '%a' ${parent_dir})

	chmod u+w ${parent_dir}
	
    chmod -R u+w ${target_path}
    rm -rf ${target_path}

	chmod ${right} ${parent_dir}
}

function delete_mindstudio_msprof() {
	if [ ! -d "${install_path}/${MSPROF_RUN_NAME}" ]; then
		return
	fi

=======
function delete_run_dir() {
>>>>>>> 4faa55eee7f0cbe4d5bcfa7e9c7eb5e5f2e161e0
    chmod -R u+w ${install_path}/${MSPROF_RUN_NAME}
    rm -rf ${install_path}/${MSPROF_RUN_NAME}
}

<<<<<<< HEAD
function delete_parent_dir(){
    # must from in to out
    remove_dir_by_order ${install_path}/tools/profiler/profiler_tool
    remove_dir_by_order ${install_path}/tools/profiler
    remove_dir_by_order ${install_path}/tools
    remove_dir_by_order ${install_path}
}

function remove_dir_by_order() {
    local dir_name=${1}

	if [ ! -d "${dir_name}" ]; then
        return
	fi

    local parent_dir=$(dirname ${dir_name})
    local parent_dir_right=$(stat -c '%a' ${parent_dir})

    chmod u+w ${parent_dir}
    remove_empty_dir ${dir_name}
    chmod ${parent_dir_right} ${parent_dir}
}

function uninstall_latest() {
    # runtime has been uninstall
    local leader_package="runtime"

    if [ -L "${latest_path}/${MSPROF_RUN_NAME}" ]; then
        rm_file_safe ${latest_path}/${MSPROF_RUN_NAME}
    fi

    # single version or multi version old
    if [ ! -L "${latest_path}/${leader_package}" ] || [ ! -d "${latest_path}/${leader_package}/../${MSPROF_RUN_NAME}" ]; then
        return
    fi

    # multi version new
    local mindstudio_msprof_abs_path=$(dirname $(readlink -f ${latest_path}/${leader_package}/../${MSPROF_RUN_NAME}))
    local version_name=$(basename ${mindstudio_msprof_abs_path})
    ln -sf ../${version_name}/${MSPROF_RUN_NAME} ${latest_path}/${MSPROF_RUN_NAME}
}

# must use readlink, or can not bash by latest
uninstall_location=$(readlink -f ${0})

source $(dirname ${uninstall_location})/utils.sh

# get path
install_path=$(dirname ${uninstall_location})/../../
if [ ! -d "${install_path}" ]; then
    print "ERROR" "Can not find install path."
    exit 1
fi
install_path="$(
    cd ${install_path}
    pwd
)"

latest_path=$(dirname ${uninstall_location})/../../../latest
if [ ! -d "${latest_path}" ]; then
    print "ERROR" "Can not find latest path."
    exit 1
fi
latest_path="$(
    cd ${latest_path}
    pwd
)"

unregist_uninstall
uninstall_product
uninstall_latest
=======
function get_log_file() {
	local log_dir
	if [ "$UID" = "0" ]; then
		log_dir="/var/log/ascend_seclog"
	else
		log_dir="${HOME}/var/log/ascend_seclog"
	fi
	echo "${log_dir}/ascend_install.log"
}

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
    fi
}

# get path
install_path="$(
    cd "$(dirname "$0")/../../"
    pwd
)"

latest_path="$(
    cd "$(dirname "$0")/../../../latest"
    pwd
)"

# log
log_file=$(get_log_file)
# product
ANALYSIS="analysis"
ANALYSIS_PATH="/tools/profiler/profiler_tool/"
MSPROF_RUN_NAME="mindstudio-msprof"

uninstall_product
uninstall_latest
unregist_uninstall
>>>>>>> 4faa55eee7f0cbe4d5bcfa7e9c7eb5e5f2e161e0
