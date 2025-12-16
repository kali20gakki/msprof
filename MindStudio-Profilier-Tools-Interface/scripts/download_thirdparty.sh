#!/bin/bash
# This script is used to download thirdpart needed by mspti.
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

OPENSOURCE_DIR=${TOP_DIR}/opensource
PLATFORM_DIR=${TOP_DIR}/platform
LLT_DIR=${TOP_DIR}/test/opensource

THIRDPARTY_LIST="${OPENSOURCE_DIR}/json        \\
                 ${OPENSOURCE_DIR}/makeself    \\
                 ${OPENSOURCE_DIR}/rapidjson    \\
                 ${PLATFORM_DIR}/securec       \\
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
[ ! -d "json" ] && git clone ssh://git@szv-open.codehub.huawei.com:2222/OpenSourceCenter/nlohmann/json.git json -b v3.11.3
[ ! -d "rapidjson" ] && git clone ssh://git@szv-open.codehub.huawei.com:2222/OpenSourceCenter/Tencent/rapidjson.git -b 6089180ecb704cb2b136777798fa1be303618975-htrunk1
[ ! -d "makeself" ] && git clone ssh://git@szv-open.codehub.huawei.com:2222/OpenSourceCenter/megastep/makeself.git -b 2.5.0-h0.computing.cann.r2

mkdir -p ${LLT_DIR} && cd ${LLT_DIR}
[ ! -d "googletest" ] && git clone ssh://git@szv-open.codehub.huawei.com:2222/OpenSourceCenter/google/googletest.git -b release-1.12.1
[ ! -d "mockcpp" ] && git clone ssh://git@szv-y.codehub.huawei.com:2222/mindstudio/MindStudio_Opensource/mock_cpp.git mockcpp -b msprof

mkdir -p ${PLATFORM_DIR} && cd ${PLATFORM_DIR}
[ ! -d "securec" ] && git clone ssh://git@codehub-dg-y.huawei.com:2222/hwsecurec_group/huawei_secure_c.git securec -b tag_Huawei_Secure_C_V100R001C01SPC012B002_00001
cd ${TOP_DIR}