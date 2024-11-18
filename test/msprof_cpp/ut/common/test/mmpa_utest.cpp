/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: UT for analyzer module
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-07-01
 */
#include <string>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "mmpa_api.h"
#include "errno/error_code.h"
#include "utils/utils.h"

using namespace Collector::Dvvp::Mmpa;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
class MmpaUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(MmpaUtest, MmGetAndFreeCpuInfo)
{
    MmCpuDesc *cpuInfo = nullptr;
    int32_t count = 0;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmCpuInfoFree(cpuInfo, count));

    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetCpuInfo(nullptr, &count));
    
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetCpuInfo(&cpuInfo, nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, MmGetCpuInfo(&cpuInfo, &count));
    EXPECT_EQ(PROFILING_SUCCESS, MmCpuInfoFree(cpuInfo, count));
}

TEST_F(MmpaUtest, MmGetOsName)
{
    std::string name = "";
    int32_t nameSize = 0;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetOsName(nullptr, nameSize));

    EXPECT_EQ(PROFILING_SUCCESS, MmGetOsName(CHAR_PTR(name.c_str()), MMPA_MAX_PATH));
}

TEST_F(MmpaUtest, MmGetOsVersion)
{
    std::string versionInfo = "";
    int32_t len = 0;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetOsVersion(nullptr, len));

    EXPECT_EQ(PROFILING_SUCCESS, MmGetOsVersion(CHAR_PTR(versionInfo.c_str()), MMPA_MAX_PATH));
}

TEST_F(MmpaUtest, MmGetTimeOfDay)
{
    MmTimeval tv;
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetTimeOfDay(nullptr, nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, MmGetTimeOfDay(&tv, nullptr));
}

TEST_F(MmpaUtest, MmGetAndFreeMac)
{
    MmMacInfo *macInfo = nullptr;
    int32_t count = 0;
    
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetMacFree(macInfo, count));
    
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmGetMac(nullptr, nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, MmGetMac(&macInfo, &count));
    EXPECT_EQ(PROFILING_SUCCESS, MmGetMacFree(macInfo, count));
}


TEST_F(MmpaUtest, MmWaitPid)
{
    MmProcess pid1 = 2;
    int32_t status1 = -1;
    int32_t options = 3;
    // option is invalid
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmWaitPid(pid1, &status1, options));

    // waitpid return PROFILING_FAILED
    int32_t ret1 = 1;
    int32_t ret2 = 2;
    MOCKER_CPP(&waitpid).stubs().will(returnValue(PROFILING_FAILED)).then(returnValue(ret1)).then(returnValue(ret2));
    options = M_WAIT_NOHANG;
    EXPECT_EQ(PROFILING_FAILED, MmWaitPid(pid1, &status1, options));

    // waitpid return success, ret is 1, status1 < 0
    EXPECT_EQ(PROFILING_FAILED, MmWaitPid(pid1, &status1, options));

    // waitpid return success, ret is 2, ret == pid, status2 > 0
    int32_t status2 = 1;
    EXPECT_EQ(PROFILING_ERROR, MmWaitPid(pid1, &status2, options));

    // waitpid return success, ret is 2, ret != pid, status2 > 0
    MmProcess pid2 = 3;
    EXPECT_EQ(PROFILING_SUCCESS, MmWaitPid(pid2, &status2, options));
}

TEST_F(MmpaUtest, MmRmdir)
{
    // 超出递归深度限制
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("mmpaTest", 6)); // 6是递归深度

    uint32_t depth = 0;
    // 指定删除目录文件名为空
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("", depth));

    MOCKER_CPP(&MmIsSoftLink).stubs().will(returnValue(true)).then(returnValue(false));
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("testtesttest", depth));

    // 指定删除文件名不存在，opendir拿到空指针
    EXPECT_EQ(PROFILING_INVALID_PARAM, MmRmdir("mmpaTestjdflsaihhgepaihglehgllhgelahl", depth));

    // 递归删除真实目录
    std::string dirName = Utils::RelativePathToAbsolutePath("mmpaTestTest");
    EXPECT_EQ(Utils::CreateDir(dirName), PROFILING_SUCCESS);
    EXPECT_EQ(Utils::CreateDir(dirName + "/test"), PROFILING_SUCCESS);
    EXPECT_EQ(PROFILING_SUCCESS, MmRmdir(dirName, depth));

    MOCKER_CPP(&MmIsSoftLink).reset();
}

TEST_F(MmpaUtest, MmIsSoftLink)
{
    // 路径名空
    EXPECT_FALSE(MmIsSoftLink(""));
    // memset_s 失败
    MOCKER_CPP(&memset_s).stubs().will(returnValue(1)).then(returnValue(0));
    EXPECT_FALSE(MmIsSoftLink("mmpaTest"));
    // lstat 失败
    MOCKER_CPP(&lstat).stubs().will(returnValue(1)).then(returnValue(0));
    EXPECT_FALSE(MmIsSoftLink("mmpaTest"));

    MOCKER_CPP(&lstat).reset();
    MOCKER_CPP(&memset_s).reset();

    // 新建目录 校验结果预期非软链
    const std::string trueDirName = "./MmIsSoftLink";
    EXPECT_EQ(Utils::CreateDir(trueDirName), PROFILING_SUCCESS);
    EXPECT_FALSE(MmIsSoftLink(trueDirName));
    uint32_t depth = 0;
    EXPECT_EQ(PROFILING_SUCCESS, MmRmdir(trueDirName, depth));

    // 创建假的软链并校验
    const std::string softLinkDirName = "./MmIsSoftLink_soft_link";
    EXPECT_EQ(0, symlink(trueDirName.c_str(), softLinkDirName.c_str()));  // 创建软连接
    EXPECT_TRUE(MmIsSoftLink(softLinkDirName));
    // 利用unlink删除软链
    EXPECT_EQ(0, unlink(softLinkDirName.c_str()));
}
