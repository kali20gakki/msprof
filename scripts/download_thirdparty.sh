#/bin/bash

#set -e
CUR_DIR=$(dirname $(readlink -f $0))
TOP_DIR=${CUR_DIR}/..

OPENSOURCE_DIR=${TOP_DIR}/opensource
HISI_DIR=${TOP_DIR}/hisi
LLT_DIR=${TOP_DIR}/test/llt/opensource

THIRDPARTY_LIST="${OPENSOURCE_DIR}/protobuf    \\
                 ${OPENSOURCE_DIR}/json        \\
                 ${OPENSOURCE_DIR}/securec     \\
                 ${LLT_DIR}/googletest         \\
                 ${LLT_DIR}/mockcpp"

if [ -n "$1" ]; then
    if [ "$1" == "force" ]; then
        echo "force delete origin opensource files"
        echo ${THIRDPARTY_LIST}
        rm -rf ${THIRDPARTY_LIST}
    fi
fi

###################################### 三方库下载 #####################################
cd ${OPENSOURCE_DIR}
git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/protobuf.git protobuf -b v3.13.0
git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/nlohmann/json.git json -b v3.7.3
git clone ssh://git@codehub-dg-y.huawei.com:2222/hwsecurec_group/huawei_secure_c.git securec -b tag_Huawei_Secure_C_V100R001C01SPC011B003_00001
git clone  --branch 2.4.5-h0.computing.cann.r3 ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/makeself.git

###################################### gtest下载 #####################################
# 下载 gtest和mockcpp
cd ${LLT_DIR}
git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/googletest.git googletest -b release-1.8.1
git clone ssh://git@szv-y.codehub.huawei.com:2222/d00437232/mock_cpp.git mockcpp -b msprof
