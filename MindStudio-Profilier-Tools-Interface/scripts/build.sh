#!/bin/bash
# This script is used to build libmspti.so, mspti_C.so
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
VERSION=""
BUILD_TYPE="Release"

# input param check
if [[ $# -gt 2 ]]; then
    echo "[ERROR]Please input valid param, for example:"
    echo "       ./build.sh                 # Default"
    echo "       ./build.sh Debug           # Debug"
    echo "       ./build.sh [version]       # With Version"
    exit
fi

if [ $# -eq 1 ] && [ "$1" = "Debug" ]; then
    BUILD_TYPE="Debug"
elif [ $# -eq 1 ] && [ "$1" != "Debug" ]; then
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

bash ${CUR_DIR}/download_thirdparty.sh
echo "download thirdparty start."
cmake -S ${TOP_DIR} -B ${TOP_DIR}/build -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${TOP_DIR}/prefix -DSECUREC_LIB_DIR=${TOP_DIR}/prefix/securec_shared
cd ${TOP_DIR}/build; make -j$(nproc); make install

bash ${TOP_DIR}/scripts/make_run.sh ${VERSION}
