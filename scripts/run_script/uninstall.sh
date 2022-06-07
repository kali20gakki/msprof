#!/bin/bash

function unregist_uninstall() {
    if [ -f "${install_path}/cann_uninstall.sh" ]; then
        chmod u+w ${install_path}/cann_uninstall.sh
        remove_uninstall_package "${install_path}/cann_uninstall.sh"
        if [ -f "${install_path}/cann_uninstall.sh" ]; then
            chmod u-w ${install_path}/cann_uninstall.sh
        fi
    fi
}

function remove_uninstall_package() {
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

function uninstall_product() {
	delete_product ${install_path}/${ANALYSIS_PATH}/${ANALYSIS}
    delete_mindstudio_msprof
    delete_parent_dir
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

function delete_mindstudio_msprof() {
    chmod -R u+w ${install_path}/${MSPROF_RUN_NAME}
    rm -rf ${install_path}/${MSPROF_RUN_NAME}
}

function delete_parent_dir(){
    # must from in to out
    remove_dir_by_order ${install_path}/tools/profiler/profiler_tool
    remove_dir_by_order ${install_path}/tools/profiler
    remove_dir_by_order ${install_path}/tools/
    remove_dir_by_order ${install_path}
}

function remove_dir_by_order() {
    local dir_name=${1}

	if [ ! -d "${dir_name}" ]; then
        return
	fi

    local parent_dir=$(dirname ${dir_name})
    local dir_right=$(stat -c '%a' ${dir_name})
    local parent_dir_right=$(stat -c '%a' ${parent_dir})

    chmod u+w ${dir_name}
    chmod u+w ${parent_dir}

    remove_empty_dir ${dir_name}
	if [ -d "${dir_name}" ]; then
        chmod ${dir_right} ${dir_name}
	fi
    chmod ${parent_dir_right} ${parent_dir}
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
    local mindstudio_msprof_abs_path=$(dirname $(readlink -f ${latest_path}/${leader_package}/../${MSPROF_RUN_NAME}))
    local version_name=$(basename ${mindstudio_msprof_abs_path})
    ln -sf ../${version_name}/${MSPROF_RUN_NAME} ${latest_path}/${MSPROF_RUN_NAME}
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

# product
ANALYSIS="analysis"
ANALYSIS_PATH="/tools/profiler/profiler_tool/"
MSPROF_RUN_NAME="mindstudio-msprof"

unregist_uninstall
uninstall_product
uninstall_latest