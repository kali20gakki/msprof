#!/bin/bash
# This script is used to execute llt testcase.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..
mkdir -p ${TOP_DIR}/test/build_llt
cd ${TOP_DIR}/test/build_llt
if [[ -n "$1" && "$1" == "analysis" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=analysis
elif [[ -n "$1" && "$1" == "collector" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=collector
elif [[ -n "$1" && "$1" == "mspti" ]]; then
    cmake ../ -DPACKAGE=ut -DMODE=mspti
else
    cmake ../ -DPACKAGE=ut -DMODE=all
fi
make -j$(nproc)
