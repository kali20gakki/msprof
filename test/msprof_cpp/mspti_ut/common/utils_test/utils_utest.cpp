/**
* @file utils_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/
#include <fstream>
#include <sys/syscall.h>
#include <linux/limits.h>

#include "gtest/gtest.h"
#include "common/utils.h"

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

TEST_F(UtilsUtest, ShouldGetRealPathWhenInputAbsoultePath)
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

TEST_F(UtilsUtest, GetHostSysCntTest)
{
    auto sysCnt = Mspti::Common::Utils::GetHostSysCnt();
    EXPECT_GT(sysCnt, 0UL);
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
    const char* msg = "record&";
    EXPECT_FALSE(Mspti::Common::Utils::CheckCharValid(msg));
}

}
