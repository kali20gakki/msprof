#!/bin/bash
# This script is used to build msprofbin&&libmsprofiler.so&&stub/libmsprofiler.so
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
VERSION=""
BUILD_TYPE=""

# input param check
if [ $# != 0 ] && [ $# != 1 ];
    then
        echo "[ERROR]Please check input params. For example:"
        echo "./build.sh           # offline (Release)"
        echo "./build.sh Debug     # offline (Debug)"
        echo "./build.sh [version] # online (Release)"    
        exit
fi

if [ $# = 1 ] && [ "$1" = "Debug" ];
    then
        BUILD_TYPE="Debug"
elif [ $# = 1 ] && [ "$1" != "Debug" ];
    then
        VERSION=$1
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

cd ${TOP_DIR}/build
cmake ../cmake/superbuild/ -DMSPROF_BUILD_TYPE=${BUILD_TYPE}
make -j64

bash ${TOP_DIR}/scripts/create_run_package.sh ${VERSION}
