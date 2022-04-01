#/bin/bash

set -e # 不是true直接退出
CUR_DIR=$(dirname $(readlink -f $0)) # 获取脚本绝对路径
TOP_DIR=${CUR_DIR}/..

OPENSOURCE_DIR=${TOP_DIR}/opensource
HISI_DIR=${TOP_DIR}/hisi
GTEST_DIR=${TOP_DIR}/testcode/opensource

THIRDPARTY_LIST="${OPENSOURCE_DIR}/protobuf    \\
                 ${OPENSOURCE_DIR}/json        \\
                 ${OPENSOURCE_DIR}/securec       \\
                 ${HISI_DIR}/graphengine           \\
                 ${HISI_DIR}/driver        \\
                 ${HISI_DIR}/inc           \\
                 ${HISI_DIR}/runtime       \\
                 ${HISI_DIR}/acl           \\
                 ${HISI_DIR}/metadef       \\
                 ${HISI_DIR}/mmpa          \\
                 ${HISI_DIR}/adump         \\
                 ${HISI_DIR}/slog          \\
                 ${GTEST_DIR}/googletest"

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

###################################### Hisi组件下载（待加权限） #####################################
cd ${HISI_DIR}


###################################### gtest下载 #####################################
# 下载 gtest
cd ${GTEST_DIR}
git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/googletest.git googletest -b release-1.8.1
