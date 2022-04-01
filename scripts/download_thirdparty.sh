#/bin/bash

set -e # 不是true直接退出
CUR_DIR=$(dirname $(readlink -f $0)) # 获取脚本绝对路径
TOP_DIR=${CUR_DIR}/..

OPENSOURCE_DIR=${TOP_DIR}/opensource
HISI_DIR=${TOP_DIR}/hisi
GTEST_DIR=${TOP_DIR}/testcode/opensource

THIRDPARTY_LIST="${OPENSOURCE_DIR}/protobuf    \\
                 ${OPENSOURCE_DIR}/json        \\
                 ${HISI_DIR}/securec       \\
                 ${HISI_DIR}/air           \\
                 ${HISI_DIR}/driver        \\
                 ${HISI_DIR}/inc           \\
                 ${HISI_DIR}/runtime       \\
                 ${HISI_DIR}/user_space    \\
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
# 下载 protobuf
# git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/protobuf.git protobuf -b v3.13.0
# # 下载 nlohmann/json
# git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/nlohmann/json.git json -b v3.7.3

###################################### 平台组件下载 #####################################
cd ${HISI_DIR}
# 下载 secure_c
git clone ssh://git@codehub-dg-y.huawei.com:2222/hwsecurec_group/huawei_secure_c.git securec -b tag_Huawei_Secure_C_V100R001C01SPC011B003_00001

# 配置权限(主线)
repo init -u ssh://gerrit.turing-ci.hisilicon.com/platform/manifest.git -b br_hisi_trunk_ai -m default.xml -g drv,adk_hcc_libs,llt_toolchain,open,mmpa,cmscbb --repo-branch=stable --no-repo-verify
# 下载 ge
# [Todo] 权限
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/air
# 下载 driver
repo sync air drv
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/inc/driver inc/driver
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/abl/ascend_hal/user_space user_space
# # 下载 acl
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/inc/external/acl inc/external/acl
# # 下载 runtime
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/inc/runtime inc/runtime
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/acl acl
# # 下载 metadef
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/metadef metadef
# # 下载 mmpa
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/mmpa mmpa
# # 下载 adump
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/toolchain/ide/ide-daemon adump
# # 下载 slog
# git clone http://q30022884@gerrit.turing-ci.hisilicon.com:8080/hisi/toolchain/log slog
###################################### gtest下载 #####################################
# 下载 gtest
cd ${GTEST_DIR}
# git clone ssh://git@codehub-dg-y.huawei.com:2222/OpenSourceCenter/googletest.git googletest -b release-1.8.1
