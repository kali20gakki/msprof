#!/bin/bash
# This script is used to config the perf iotop ltrace permission used by profiling.
# Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

username=$1
script_dir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function judge_input(){
    if [ $# -ne 1 ]; then
        echo "The number of parameters is incorrect, please enter one parameter"
        exit 1
    fi
    if [ -z "${username}" ]; then
        echo "Please input a username"
        exit 1
    fi
    echo "${username}" | grep -q -E '^[ 0-9a-zA-Z./:]*$'
    result=$?
    if [ "$result" -ne 0 ]; then
        echo "Parameter:${username} error!"
        exit
    fi
    if ! id -u "${username}" >/dev/null 2>&1 ; then
        echo "User:${username} does not exist"
        exit
    fi
}

function copy_script_to_usr_bin(){
    if [ ! -f "${script_dir}/msprof_data_collection.sh" ]; then
        echo "The script file ${script_dir}/msprof_data_collection.sh dose not exist"
        exit 1
    fi
    if [ -f "/usr/bin/msprof_data_collection.sh" ]; then
        echo "The script file /usr/bin/msprof_data_collection.sh already exist"
    fi
    cp -rf "${script_dir}"/msprof_data_collection.sh /usr/bin/
    result=$?
    if [ "$result" -ne 0 ]; then
        echo "Permission configure failed for cp!"
        exit 1
    fi
    chmod u+x /usr/bin/msprof_data_collection.sh
    result=$?
    if [ "$result" -ne 0 ]; then
        echo "Permission configure failed for chmod!"
        exit 1
    fi
    echo "Copy msprof_data_collection.sh to /usr/bin/ success"
}

function main(){
    judge_input "$@"
    copy_script_to_usr_bin
    /usr/bin/msprof_data_collection.sh set-sudoers "${username}"
}

main "$@"
