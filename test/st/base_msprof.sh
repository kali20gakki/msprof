#!/bin/bash

cur_dir=$(dirname $(readlink -f $0))
export ASCEND_SLOG_PRINT_TO_STDOUT=1
export ASCEND_GLOBAL_LOG_LEVEL=1

file_name="$1"
MSPROF_OPTS="$2"
MODEL_NAME="$3"
output_path="$(readlink -f "$4")"

if [[ -z "${4:-}" ]]; then
    echo "Error: output_path is empty" >&2
    exit 1
fi

main(){
    testcase_result_dir="${output_path}/${file_name}"
    if [ ! -d ${testcase_result_dir} ]; then
        mkdir -p ${testcase_result_dir}
    else
        rm -rf ${testcase_result_dir}/*
    fi

    start_time=$(date "+%s")
    cd ${cur_dir}/src/

    # 1. The msprof approach is used to lift the model for training
    msprof --output=${testcase_result_dir}/result_dir ${MSPROF_OPTS} \
           python3 ${cur_dir}/src/train.py --model ${MODEL_NAME} \
           > ${testcase_result_dir}/plog.txt 2>&1
    python_exit_code=$?
    if [ 0 -ne $python_exit_code ]; then
        echo "${file_name} fail"
        exit 1
    fi

    # 2. Check if plog reports any errors
    if grep "ERROR" ${testcase_result_dir}/plog.txt | grep -q "PROFILING"; then
        echo "${file_name} fail"
        exit 1
    fi

    # 3. Verify the profiling data items
    python3 ${cur_dir}/src/${file_name}.py ${testcase_result_dir}/result_dir
    check_exit_code=$?
    end_time=$(date "+%s")
    if [ 0 -ne ${check_exit_code} ]; then
        echo "${file_name} fail"
        exit 1
    fi

    duration_time=$(( ${end_time} - ${start_time} ))
    echo "${file_name} pass ${duration_time}"
}

main
