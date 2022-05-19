#/bin/bash
# This script is used to build msprofbin&&libmsprofiler.so&&stub/libmsprofiler.so
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
BUILD_TYPE="Release"
if [ $# = 1 ] && [ "$1" = "Debug" ];
then
    BUILD_TYPE="Debug"
fi
cd ${TOP_DIR}/build
cmake ../cmake/superbuild/ -DMSPROF_BUILD_TYPE=${BUILD_TYPE}
make -j64

bash ${TOP_DIR}/scripts/create_run_package.sh ${1}
