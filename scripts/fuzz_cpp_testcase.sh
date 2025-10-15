#!/bin/bash
# This script is used to execute llt testcase.
# Copyright Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
FUZZ_WORK_PATH=${TOP_DIR}/test/fuzz_test/build_fuzz
FUZZ_RESULT_PATH=${TOP_DIR}/test/fuzz_test/result
COLLECTOR_FUZZ_REPORT_PATH=${TOP_DIR}/test/fuzz_test/cpp_fuzz/report/collector
COLLECTOR_FUZZ_BINARY_PATH=${FUZZ_WORK_PATH}/collector_fuzz-prefix/src/collector_fuzz-build/prof_api/collector_fuzz
FUZZ_COVERAGE_TOOL_PATH=${TOP_DIR}/test/opensource/SecTracy

declare -g FUZZ_RUN_TIMES=1000000
declare -g FUZZ_RUN_MODULE="all"

function download_thirdparty() {
    echo "*************[INFO] download thirdparty start..."
    if [ ! -d ${TOP_DIR}/test/opensource ]; then
        bash ${TOP_DIR}/scripts/download_thirdparty.sh fuzz
    fi
    echo "*************[INFO] download thirdparty succ"
}

function check_gcc_version() {
    gcc_version=$(gcc -dumpversion)
    IFS=. read -r major minor patch <<< "$gcc_version"
    if [ $major -lt 8 ]; then
        echo "*************[INFO] gcc version is $gcc_version"
        echo "*************[INFO] please upgrade gcc version >= 8.0.0 before run fuzz testcase"
        exit 1
    else
        echo "*************[INFO] get gcc version: $gcc_version"
    fi
}

function build_fuzz_binary() {
    echo "*************[INFO] build fuzz binary start..."
    mkdir -p ${TOP_DIR}/test/fuzz_test/build_fuzz
    cd ${TOP_DIR}/test/fuzz_test/build_fuzz
    cmake ../ -DMODE=$FUZZ_RUN_MODULE
    make -j$(nproc)
    if [ $? -ne 0 ]; then
        echo "*************[ERROR] build fuzz binary fail!!!"
        exit 1
    fi
    echo "*************[INFO] build fuzz binary succ"
}

function run_collector_fuzz_binary() {
    if [ -d ${COLLECTOR_FUZZ_REPORT_PATH} ]; then
        rm -rf ${COLLECTOR_FUZZ_REPORT_PATH}
    fi
    mkdir -p ${COLLECTOR_FUZZ_REPORT_PATH} && cd ${COLLECTOR_FUZZ_REPORT_PATH}
    mkdir log
    echo "*************[INFO] run collector fuzz binary start..."
    ${COLLECTOR_FUZZ_BINARY_PATH} ${FUZZ_RUN_TIMES} > log/test_collector_${FUZZ_RUN_TIMES}.log
    if [ $? != 0 ]; then
        echo "*************[ERROR] run collector fuzz binary fail!!!"
        exit 1
    fi
    echo "*************[INFO] run collector fuzz binary succ"
}

function run_fuzz_binary() {
    echo "*************[INFO] run fuzz binary start..."
    asan_path=$(find /usr/lib -name "libasan.so" | sort -r | head -n 1)
    ubsan_path=$(find /usr/lib -name "libubsan.so" | sort -r | head -n 1)
    export LD_PRELOAD=${asan_path}:${ubsan_path}
    if [[ $FUZZ_RUN_MODULE == "all" || $FUZZ_RUN_MODULE == "collector" ]]; then
        if [ -e ${COLLECTOR_FUZZ_BINARY_PATH} ]; then
            run_collector_fuzz_binary
        else
            echo "*************[ERROR] cannot find ${COLLECTOR_FUZZ_BINARY_PATH}"
        fi
    else
        echo "*************[ERROR] invalid fuzz execution module"
        exit 1
    fi
    echo "*************[INFO] run fuzz binary succ"
}

function package_result_retult() {
    echo "*************[INFO] package fuzz result start..."
    if [ -d ${FUZZ_RESULT_PATH} ]; then
        rm -rf ${FUZZ_RESULT_PATH}
    fi
    mkdir -p ${FUZZ_RESULT_PATH}
    tar -zcvf ${FUZZ_RESULT_PATH}/fuzz.tar.gz ${TOP_DIR}/test/fuzz_test/cpp_fuzz/report
    echo "*************[INFO] package fuzz result succ"
}

while getopts "t:m:h" opt; do
    case ${opt} in
        t )
            if [[ $OPTARG =~ ^[0-9]+$ ]]; then
                FUZZ_RUN_TIMES=$OPTARG
            else
                echo "*************[WARNING] invalid fuzz execution time, please input valid number"
                exit 1
            fi
            ;;
        m )
            if [[ $OPTARG == "all" || $OPTARG == "mspti" || $OPTARG == "analysis" || $OPTARG == "collector" ]]; then
                FUZZ_RUN_MODULE=$OPTARG
            else
                echo "*************[WARNING] invalid fuzz execution module"
                echo "*************[WARNING] please input in [all|collector|mspti|analysis]"
                exit 1
            fi
            ;;
        h )
            echo "[INFO] Description for msprof fuzz testcase shell..."
            echo "[INFO] Options:"
            echo "[INFO] -t------------Fuzz execution time, default time: 1000000"
            echo "[INFO] -m------------Fuzz execution module, can be set in [all(default)|collector|mspti|analysis]"
            echo "[INFO] -h------------Fuzz execution shell help"
            exit 0
            ;;
        \? )
            echo "Invalid option: -$OPTARG" 1>&2
            exit 1
            ;;
        : )
            echo "Option -$OPTARG requires an argument." 1>&2
            exit 1
            ;;
    esac
done

echo "*************[INFO] run $FUZZ_RUN_MODULE fuzz testcase for $FUZZ_RUN_TIMES..."

check_gcc_version

download_thirdparty

build_fuzz_binary

run_fuzz_binary

package_result_retult

echo "*************[INFO] run $FUZZ_RUN_MODULE fuzz testcase succ"
echo "*************[INFO] you can see fuzz result in ${FUZZ_RESULT_PATH}"