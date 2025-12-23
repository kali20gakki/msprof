/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/
#include <fstream>
#include <sys/syscall.h>
#include <linux/limits.h>
#include "gtest/gtest.h"
#include "securec.h"
#include "mockcpp/mockcpp.hpp"
#include "csrc/common/utils.h"
#include "csrc/common/runtime_utils.h"

namespace {
class UtilsUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(UtilsUtest, ShouldGetRealPathWhenInputRelativePath)
{
    std::string path = "./test";
    auto realPath = Mspti::Common::Utils::RealPath(path);
    char buf[PATH_MAX];
    getcwd(buf, PATH_MAX);
    std::string targetPath = std::string(buf) + "/test";
    EXPECT_STREQ(targetPath.c_str(), realPath.c_str());
}

TEST_F(UtilsUtest, ShouldGetRealPathWhenInputAbsolutePath)
{
    char buf[PATH_MAX];
    getcwd(buf, PATH_MAX);
    std::string path = std::string(buf) + "/test";
    auto realPath = Mspti::Common::Utils::RealPath(path);
    EXPECT_STREQ(path.c_str(), realPath.c_str());
}

TEST_F(UtilsUtest, GetClockMonotonicRawNsTest)
{
    auto monoTime = Mspti::Common::Utils::GetClockMonotonicRawNs();
    EXPECT_GT(monoTime, 0UL);
}

TEST_F(UtilsUtest, GetClockRealTimeNsTest)
{
    auto realTime = Mspti::Common::Utils::GetClockRealTimeNs();
    EXPECT_GT(realTime, 0UL);
}

TEST_F(UtilsUtest, GetClockRealTimeNsMemsetFail)
{
    MOCKER_CPP(&memset_s).stubs().will(returnValue(1));
    uint64_t result = Mspti::Common::Utils::GetClockRealTimeNs();
    EXPECT_EQ(result, 0);
}

TEST_F(UtilsUtest, GetHostSysCntTest)
{
    auto sysCnt = Mspti::Common::Utils::GetHostSysCnt();
    EXPECT_GT(sysCnt, 0UL);
}

TEST_F(UtilsUtest, GetHostSysCntTestMemsetFail)
{
    MOCKER_CPP(&memset_s).stubs().will(returnValue(1));
    uint64_t result = Mspti::Common::Utils::GetClockRealTimeNs();
    EXPECT_EQ(result, 0);
}

TEST_F(UtilsUtest, GetPidTest)
{
    auto pid = Mspti::Common::Utils::GetPid();
    EXPECT_EQ(pid, static_cast<uint32_t>(getpid()));
}

TEST_F(UtilsUtest, GetTidTest)
{
    auto tid = Mspti::Common::Utils::GetTid();
    EXPECT_EQ(tid, static_cast<uint32_t>(syscall(SYS_gettid)));
}

TEST_F(UtilsUtest, GetDeviceIdTest)
{
    const uint32_t expectDeviceId = 0U;
    EXPECT_EQ(expectDeviceId, Mspti::Common::GetDeviceId());
}

TEST_F(UtilsUtest, GetStreamIdTest)
{
    const uint32_t expectStreamId = 0U;
    AclrtStream stm = nullptr;
    EXPECT_EQ(expectStreamId, Mspti::Common::GetStreamId(stm));
}

TEST_F(UtilsUtest, RelativeToAbsPathTest)
{
    std::string stubPath = "test.txt";
    char pwdPath[PATH_MAX] = {0};
    if (getcwd(pwdPath, PATH_MAX) != nullptr) {
        std::string targetPath = std::string(pwdPath) + "/" + stubPath;
        std::string path = Mspti::Common::Utils::RelativeToAbsPath(stubPath);
        EXPECT_STREQ(targetPath.c_str(), path.c_str());
    } else {
        std::string targetPath = "";
        std::string path = Mspti::Common::Utils::RelativeToAbsPath(stubPath);
        EXPECT_STREQ(targetPath.c_str(), path.c_str());
    }
}

TEST_F(UtilsUtest, EmptyPath)
{
    std::string result = Mspti::Common::Utils::RelativeToAbsPath("");
    EXPECT_EQ(result, "");
}

TEST_F(UtilsUtest, PathTooLong)
{
    std::string longPath(PATH_MAX + 1, 'a');
    std::string result = Mspti::Common::Utils::RelativeToAbsPath(longPath);
    EXPECT_EQ(result, "");
}

TEST_F(UtilsUtest, ShouldGetTrueWhenFileExist)
{
    std::string stubPath = "test.txt";
    std::ofstream f(stubPath);
    if (f.is_open()) {
        f.close();
    }
    EXPECT_EQ(true, Mspti::Common::Utils::FileExist(stubPath));
    std::remove(stubPath.c_str());
}

TEST_F(UtilsUtest, ShouldGetFalseWhenFileNotExist)
{
    std::string stubPath = "test.txt";
    EXPECT_EQ(false, Mspti::Common::Utils::FileExist(stubPath));
}

TEST_F(UtilsUtest, FileReadableShouldGetFalseWhenFileEmpty)
{
    std::string path = "";
    EXPECT_EQ(false, Mspti::Common::Utils::FileReadable(path));
}

TEST_F(UtilsUtest, CheckCharValidShouldReturnFalseWhileMsgContainsSpecialCharacter)
{
    const char *msg = "record&";
    EXPECT_FALSE(Mspti::Common::Utils::CheckCharValid(msg));
}
}
