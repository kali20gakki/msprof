#!/bin/bash
# This script is used to build msprofbin&&libmsprofiler.so&&stub/libmsprofiler.so
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
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

function collector_compile() {
    cd ${TOP_DIR}/build
    cmake ../cmake/superbuild/ -DMSPROF_BUILD_TYPE=${BUILD_TYPE}
    make -j64
}

function analysis_compile() {
    mkdir -p ${TOP_DIR}/analysis/csrc/build
    cd ${TOP_DIR}/analysis/csrc/build
    cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${TOP_DIR}/analysis
    make -j64 && make clean && make install
    if [ $? -ne 0 ]; then
        echo "analysis_compile failed"
        exit 1
    fi
    cd ${TOP_DIR}/build
}

collector_compile
analysis_compile
bash ${TOP_DIR}/scripts/create_run_package.sh ${VERSION} ${PACKAGE_TYPE}
