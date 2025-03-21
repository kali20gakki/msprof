#!/bin/bash
# This script is used to build msprofbin&&libmsprofiler.so&&stub/libmsprofiler.so
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
OPENSOURCE_DIR=${TOP_DIR}/opensource
VERSION=""
PACKAGE_TYPE=""
BUILD_TYPE=""

# input param check
if [[ $# > 2 ]];
    then
        echo "[ERROR]Please input valid param, for example:"
        echo "       ./build.sh                 # Default"
        echo "       ./build.sh Debug           # Debug"
        echo "       ./build.sh [version]       # With Version"    
        echo "       ./build.sh [version] [Patch] # With Version and Patch" 
        exit
fi

if [ $# = 1 ] && [ "$1" = "Debug" ];
    then
        BUILD_TYPE="Debug"
elif [ $# = 1 ] && [ "$1" != "Debug" ];
    then
        VERSION=$1
elif [ $# = 2 ];
    then
        VERSION=$1
        PACKAGE_TYPE=$2
fi

# binary check
function bep_env_init() {
    source /etc/profile
    local bep_env_config=${CUR_DIR}/bep/bep_env.conf
    local bep_sh=$(which bep_env.sh)
    echo "has bep sh :${bep_sh}"
    if [ ! -d "${SECBEPKIT_HOME}" ] && [ ! -f "$bep_sh" ]; then
        echo "BepKit is not installed, Please install the tool and configure the env var \$SECBEPKIT_HOME"
    else
        source ${SECBEPKIT_HOME}/bep_env.sh -s $bep_env_config
        if [ $? -ne 0 ]; then
            echo "build bep failed!"
            exit 1
        else
            echo "build bep success."
        fi
    fi
}

bep_env_init

function compile_protobuf() {
    # protobuf_3.14_change_version.patch来源：https://szv-cr-y.codehub.huawei.com/Turing_Solution/hisi/cmake.git
    cp ${TOP_DIR}/build/opensource_patch/protobuf_3.14_change_version.patch ${OPENSOURCE_DIR}/protobuf
    cd ${OPENSOURCE_DIR}/protobuf
    tar -zxvf protobuf-all-3.14.0.tar.gz
    cd protobuf-3.14.0
    ulimit -n 8192
    patch -p1 < ../0001-add-secure-compile-option-in-Makefile.patch
    patch -p1 < ../0002-add-secure-compile-fs-check-in-Makefile.patch
    patch -p1 < ../0003-fix-CVE-2021-22570.patch
    patch -p1 < ../0004-Improve-performance-of-parsing-unknown-fields-in-Jav.patch
    patch -p1 < ../0005-fix-CVE-2022-1941.patch
    patch -p1 < ../0006-fix-CVE-2022-3171.patch
    patch -p1 < ../0007-add-coverage-compile-option.patch
    patch -p1 < ../protobuf_3.14_change_version.patch
    cd ..
    cp -r protobuf-3.14.0 ${OPENSOURCE_DIR}
    cd ${OPENSOURCE_DIR}
    rm -rf protobuf
    mv protobuf-3.14.0 protobuf
}

compile_protobuf

# Hi Test
HI_TEST="off"
if [ ! -z "${TOOLKIT_HITEST}" ] && [ "${TOOLKIT_HITEST}" == "on" ]; then
    HI_TEST=${TOOLKIT_HITEST}
fi

mkdir -p ${TOP_DIR}/build/prefix
cmake -S ${TOP_DIR}/cmake/superbuild/ -B ${TOP_DIR}/build/build -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DHITEST=${HI_TEST} -DCMAKE_INSTALL_PREFIX=${TOP_DIR}/build/prefix -DOBJECT=all
cd ${TOP_DIR}/build/build; make -j$(nproc); make clean

bash ${TOP_DIR}/scripts/create_run_package_pack.sh ${VERSION} ${PACKAGE_TYPE}
