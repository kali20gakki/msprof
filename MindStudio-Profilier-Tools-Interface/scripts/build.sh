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

bash ${CUR_DIR}/download_thirdparty.sh

rm -rf  ${TOP_DIR}/build
cmake -S ${TOP_DIR} -B ${TOP_DIR}/build -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${TOP_DIR}/prefix -DSECUREC_LIB_DIR=${TOP_DIR}/prefix/securec_shared
cd ${TOP_DIR}/build; make -j$(nproc); make install

bash ${TOP_DIR}/scripts/make_run.sh ${VERSION}
