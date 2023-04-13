#!/bin/bash
# This script is used to download thirdpart needed by msprof.
# Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

OPENSOURCE_DIR=${TOP_DIR}/opensource
PLATFORM_DIR=${TOP_DIR}/platform
LLT_DIR=${TOP_DIR}/test/opensource

THIRDPARTY_LIST="${OPENSOURCE_DIR}/protobuf    \\
                 ${OPENSOURCE_DIR}/json        \\
                 ${PLATFORM_DIR}/securec     \\
                 ${LLT_DIR}/googletest         \\
                 ${LLT_DIR}/mockcpp"

if [ -n "$1" ]; then
    if [ "$1" == "force" ]; then
        echo "force delete origin opensource files"
        echo ${THIRDPARTY_LIST}
        rm -rf ${THIRDPARTY_LIST}
    fi
fi

mkdir -p ${OPENSOURCE_DIR} && cd ${OPENSOURCE_DIR}
git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/protobuf.git protobuf -b v3.13.0
git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/nlohmann/json.git json -b v3.7.3
git clone  --branch 2.4.5-h0.computing.cann.r3 ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/makeself.git

mkdir -p ${LLT_DIR} && cd ${LLT_DIR}
git clone ssh://git@szv-open.codehub.huawei.com:2222/OpenSourceCenter/google/googletest.git -b release-1.12.1
git clone ssh://git@szv-y.codehub.huawei.com:2222/d00437232/mock_cpp.git mockcpp -b msprof

mkdir -p ${PLATFORM_DIR} && cd ${PLATFORM_DIR}
git clone ssh://git@codehub-dg-y.huawei.com:2222/hwsecurec_group/huawei_secure_c.git securec -b tag_Huawei_Secure_C_V100R001C01SPC012B002_00001
