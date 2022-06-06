#!/bin/bash

function uninstall_product() {
	delete_product ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
    delete_parent_dir ${install_path}/tools
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

function delete_parent_dir(){
	local dir_name=${1}
    local right=400
 
	if [ -f "${dir_name}" ]; then
        return
	fi
	
	if [ -d "${dir_name}" ]; then
        right=$(stat -c '%a' ${dir_name})
        chmod u+w ${dir_name}

        file_list=`ls $dir_name`
        for f in $file_list
        do
            delete_parent_dir ${dir_name}/${f}
        done

        remove_empty_dir ${dir_name}
	fi

	if [ -f "${dir_name}" ] && [ -d "${dir_name}" ]; then
		chmod ${right} ${dir_name}
	fi
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
        if [ -f "${install_path}/cann_uninstall.sh" ]; then
            chmod u-w ${install_path}/cann_uninstall.sh
        fi
    fi
}

function __remove_uninstall_package() {
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

function delete_run_dir() {
    chmod -R u+w ${install_path}/${MSPROF_RUN_NAME}
    rm -rf ${install_path}/${MSPROF_RUN_NAME}
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

function print() {
    if [ ! -f "$log_file" ]; then
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2"
    else
        echo "[${MSPROF_RUN_NAME}] [$(date +"%Y-%m-%d %H:%M:%S")] [$1]: $2" | tee -a $log_file
    fi
}

source $(dirname "$0")/utils.sh

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