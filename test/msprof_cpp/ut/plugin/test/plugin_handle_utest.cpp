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
 * -------------------------------------------------------------------------*/

#include <memory>
#include <cstdio>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "mmpa/mmpa_api.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

const int MAX_BUF_SIZE = 1024;

class PluginHandleUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(PluginHandleUtest, GetSoName)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_STREQ(soName.c_str(), pluginHandle.GetSoName().c_str());
}

TEST_F(PluginHandleUtest, OpenPluginFromEnv)
{
    bool isSoValid = true;
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    std::string envStrEmpty = "";
    int maxPathLength = 1024;
    std::string invalidLenEnvStr(maxPathLength, 'x');
    std::string envStr = "LD_LIBRARY_PATH";
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStrEmpty, isSoValid));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(invalidLenEnvStr, isSoValid));
    std::string retStrEmpty = "";
    std::string retStr = "/usr/local/Ascend/driver/lib64/libascend_hal.so";
    MOCKER_CPP(&PluginHandle::GetSoPath)
        .stubs()
        .will(returnValue(retStrEmpty))
        .then(returnValue(retStr));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr, isSoValid));

    MOCKER_CPP(&MmRealPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr, isSoValid));

    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr, isSoValid));

    MOCKER_CPP(&PluginHandle::CheckSoValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr, isSoValid));
    EXPECT_EQ(false, isSoValid);

    int openfd = 0;
    MOCKER_CPP(&PluginHandle::DlopenSo)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr, isSoValid));
    EXPECT_EQ(PROFILING_SUCCESS, pluginHandle.OpenPluginFromEnv(envStr, isSoValid));
}

TEST_F(PluginHandleUtest, CloseHandleWillReturnWhenHandleIsNotValid)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    pluginHandle.CloseHandle();
    EXPECT_EQ(nullptr, pluginHandle.handle_);
}

TEST_F(PluginHandleUtest, CloseHandleWillReturnWhenLoadIsNotValid)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    pluginHandle.CloseHandle();
    EXPECT_EQ(false, pluginHandle.load_);
}

TEST_F(PluginHandleUtest, CloseHandleWillDlcloseWhenHandleAndLoadIsValid)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    pluginHandle.load_ = true;
    int handle = 0;
    pluginHandle.handle_ = static_cast<VOID_PTR>(&handle);
    MOCKER(dlclose)
        .stubs()
        .will(returnValue(0));
    pluginHandle.CloseHandle();
    EXPECT_EQ(nullptr, pluginHandle.handle_);
}

TEST_F(PluginHandleUtest, GetSoPath)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    char* pathEnv = "/usr/local/Ascend/driver/lib64";
    std::string path = "PATH";
    MOCKER(std::getenv)
        .stubs()
        .will(returnValue((char *)NULL))
        .then(returnValue(pathEnv));
    std::string strEmpty = "";
    EXPECT_EQ(strEmpty.c_str(), pluginHandle.GetSoPath(path));

    std::string retStr = "/usr/local/Ascend/driver/lib64/libascend_hal.so";
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    EXPECT_STREQ(retStr.c_str(), pluginHandle.GetSoPath(path).c_str());
    EXPECT_STREQ(strEmpty.c_str(), pluginHandle.GetSoPath(path).c_str());
}

TEST_F(PluginHandleUtest, GetSoPathWillReturnEmptyStrWhenMmAccess2Fail)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    CHAR_PTR pathEnv = "/usr/local/Ascend/driver/lib64";
    std::string path = "PATH";
    std::string retStr = "/usr/local/Ascend/driver/lib64/libascend_hal.so";
    std::string strEmpty = "";
    MOCKER(std::getenv)
        .stubs()
        .will(returnValue(pathEnv));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::GetAscendHalPath)
        .stubs()
        .will(returnValue(retStr));
    EXPECT_STREQ(strEmpty.c_str(), pluginHandle.GetSoPath(path).c_str());
}

TEST_F(PluginHandleUtest, GetSoPathWillReturnEmptyStrWhenGetAscendHalPathReturnEmptyStr)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    CHAR_PTR pathEnv = "/usr/local/Ascend/driver/lib64";
    std::string path = "PATH";
    std::string retStr = "/usr/local/Ascend/driver/lib64/libascend_hal.so";
    std::string strEmpty = "";
    MOCKER(std::getenv)
        .stubs()
        .will(returnValue(pathEnv));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::GetAscendHalPath)
        .stubs()
        .will(returnValue(strEmpty));
    EXPECT_STREQ(strEmpty.c_str(), pluginHandle.GetSoPath(path).c_str());
}

TEST_F(PluginHandleUtest, DlopenSo)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.DlopenSo(""));
    std::string path = "/usr/local/Ascend/driver/lib64";
    int openfd = 0;
    MOCKER(dlopen)
        .stubs()
        .will(returnValue((HandleType)nullptr))
        .then(returnValue((HandleType)&openfd));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.DlopenSo(path.c_str()));
    EXPECT_EQ(PROFILING_SUCCESS, pluginHandle.DlopenSo(path.c_str()));
    EXPECT_EQ(true, pluginHandle.load_);
}

TEST_F(PluginHandleUtest, CheckSoValid_if_null_sopath_return_failed)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(false, pluginHandle.CheckSoValid(""));
}

TEST_F(PluginHandleUtest, CheckSoValid_if_get_sostat_fail_return_failed)
{
    GlobalMockObject::verify();
    MOCKER(stat)
        .stubs()
        .will(returnValue(-1));
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(false, pluginHandle.CheckSoValid("/path/to/so"));
}

int StatWriteableByOthersStub(const char *fileName, struct stat *buf)
{
    buf->st_mode = S_IWOTH;
    return 0;
}

int StatBelongToRootStub(const char *fileName, struct stat *buf)
{
    buf->st_uid = 0;
    return 0;
}

int StatBelongToCurUserStub(const char *fileName, struct stat *buf)
{
    buf->st_uid = static_cast<uint32_t>(MmGetUid());
    return 0;
}

int StatBelongToOthersStub(const char *fileName, struct stat *buf)
{
    buf->st_uid = static_cast<uint32_t>(MmGetUid()) + 1;
    return 0;
}

TEST_F(PluginHandleUtest, CheckSoValid_if_so_writeable_by_others_return_failed)
{
    GlobalMockObject::verify();
    MOCKER(stat)
        .stubs()
        .will(invoke(StatWriteableByOthersStub));
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(false, pluginHandle.CheckSoValid("/path/to/so"));
}

TEST_F(PluginHandleUtest, CheckSoValid_if_so_belong_to_root_return_failed)
{
    GlobalMockObject::verify();
    MOCKER(stat)
        .stubs()
        .will(invoke(StatBelongToRootStub));
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(true, pluginHandle.CheckSoValid("/path/to/so"));
}

TEST_F(PluginHandleUtest, CheckSoValid_if_so_belong_to_current_user_return_failed)
{
    GlobalMockObject::verify();
    MOCKER(stat)
        .stubs()
        .will(invoke(StatBelongToCurUserStub));
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(true, pluginHandle.CheckSoValid("/path/to/so"));
}

TEST_F(PluginHandleUtest, CheckSoValid_if_so_belong_to_others_return_failed)
{
    GlobalMockObject::verify();
    MOCKER(stat)
        .stubs()
        .will(invoke(StatBelongToOthersStub));
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(false, pluginHandle.CheckSoValid("/path/to/so"));
}

TEST_F(PluginHandleUtest, GetSoPaths_select_fail)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    FILE *fp = popen("ls", "r");
    std::vector<std::string> soPaths;

    MOCKER(popen)
        .stubs()
        .will(returnValue(static_cast<FILE *>(nullptr)))
        .then(returnValue(fp));
    pluginHandle.GetSoPaths(soPaths);
    EXPECT_EQ(true, soPaths.empty());
    MOCKER(pclose)
        .stubs()
        .will(returnValue(0));
    MOCKER(MmClose)
        .stubs()
        .will(returnValue(0));

    MOCKER(fileno)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    pluginHandle.GetSoPaths(soPaths);
    EXPECT_EQ(true, soPaths.empty());

    MOCKER(select)
        .stubs()
        .will(returnValue(-1));
    pluginHandle.GetSoPaths(soPaths);
    EXPECT_EQ(true, soPaths.empty());
}

TEST_F(PluginHandleUtest, GetSoPaths_fread_fail)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    FILE *fp = popen("ls", "r");
    std::vector<std::string> soPaths;

    MOCKER(popen)
        .stubs()
        .then(returnValue(fp));
    MOCKER(pclose)
        .stubs()
        .then(returnValue(0));
    MOCKER(MmClose)
        .stubs()
        .then(returnValue(0));
    MOCKER(fileno)
        .stubs()
        .will(returnValue(0));
    MOCKER(select)
        .stubs()
        .will(returnValue(1));
    
    MOCKER(ferror)
        .stubs()
        .will(returnValue(1));
    MOCKER(fread)
        .stubs()
        .will(returnValue(MAX_BUF_SIZE));
    pluginHandle.GetSoPaths(soPaths);
    EXPECT_EQ(true, soPaths.empty());

    MOCKER(ferror)
        .stubs()
        .will(returnValue(0));
    MOCKER(fread)
        .stubs()
        .will(returnValue(MAX_BUF_SIZE - 1));
    MOCKER(feof)
        .stubs()
        .will(returnValue(0));
    pluginHandle.GetSoPaths(soPaths);
    EXPECT_EQ(true, soPaths.empty());
}

TEST_F(PluginHandleUtest, GetSoPaths_success)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    FILE *fp = popen("ls", "r");
    std::vector<std::string> soPaths;

    MOCKER(popen)
        .stubs()
        .then(returnValue(fp));
    MOCKER(pclose)
        .stubs()
        .then(returnValue(0));
    MOCKER(MmClose)
        .stubs()
        .then(returnValue(0));
    MOCKER(fileno)
        .stubs()
        .will(returnValue(0));
    MOCKER(select)
        .stubs()
        .will(returnValue(1));
    MOCKER(ferror)
        .stubs()
        .will(returnValue(0));
    MOCKER(fread)
        .stubs()
        .will(returnValue(MAX_BUF_SIZE));
    std::vector<std::string> invalidPaths = {"xxxxxxx"};
    std::vector<std::string> validPaths = {"libascend_hal.so => /usr/local/Ascend/driver/lib64/libascend_hal.so"};
    MOCKER(Utils::Split)
        .stubs()
        .will(returnValue(invalidPaths))
        .then(returnValue(validPaths));
    MOCKER(MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    pluginHandle.GetSoPaths(soPaths);
    EXPECT_EQ(true, soPaths.empty());
    pluginHandle.GetSoPaths(soPaths);
    size_t expectSoPathSize = 1;
    EXPECT_EQ(expectSoPathSize, soPaths.size());
}

TEST_F(PluginHandleUtest, OpenPluginFromLdcfgWillReturnFailWhenGetEmptySoPaths)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    bool isSoValid = true;
    std::vector<std::string> emptySoPaths;
    MOCKER_CPP(&PluginHandle::GetSoPaths)
        .stubs();
    pluginHandle.soPaths_ = emptySoPaths;
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromLdcfg(isSoValid));
}

TEST_F(PluginHandleUtest, OpenPluginFromLdcfgWillReturnFailWhenMmRealPathFail)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    bool isSoValid = true;
    std::vector<std::string> fakeSoPaths {"/usr/local/Ascend/driver/lib64/libascend_hal.so"};
    MOCKER_CPP(&PluginHandle::GetSoPaths)
        .stubs();
    MOCKER_CPP(&MmRealPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    pluginHandle.soPaths_ = fakeSoPaths;
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromLdcfg(isSoValid));
}

TEST_F(PluginHandleUtest, OpenPluginFromLdcfgWillReturnFailWhenRealPathReturnEmptyPath)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    bool isSoValid = true;
    std::vector<std::string> fakeSoPaths {"/path/to/libascend_hal.so"}; // so MmRealPath will return empty path
    MOCKER_CPP(&PluginHandle::GetSoPaths)
        .stubs();
    pluginHandle.soPaths_ = fakeSoPaths;
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromLdcfg(isSoValid));
}

TEST_F(PluginHandleUtest, OpenPluginFromLdcfgWillReturnFailWhenRealPathIsSoftLink)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    bool isSoValid = true;
    std::vector<std::string> fakeSoPaths {"/tmp"}; // so MmRealPath will return non empty path
    MOCKER_CPP(&PluginHandle::GetSoPaths)
        .stubs();
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true));
    pluginHandle.soPaths_ = fakeSoPaths;
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromLdcfg(isSoValid));
}

TEST_F(PluginHandleUtest, OpenPluginFromLdcfgWillReturnFailWhenDlopenReturnNull)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    bool isSoValid = true;
    std::vector<std::string> fakeSoPaths {"/tmp"}; // so MmRealPath will return non empty path
    MOCKER_CPP(&PluginHandle::GetSoPaths)
        .stubs();
    MOCKER_CPP(&PluginHandle::CheckSoValid)
        .stubs()
        .will(returnValue(true));
    MOCKER(dlopen)
        .stubs()
        .will(returnValue((VOID_PTR)nullptr));
    pluginHandle.soPaths_ = fakeSoPaths;
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromLdcfg(isSoValid));
    EXPECT_EQ(true, isSoValid);
}

TEST_F(PluginHandleUtest, OpenPluginFromLdcfgWillReturnSuccWhenDlopenReturnValidHandle)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    bool isSoValid = true;
    std::vector<std::string> fakeSoPaths {"/tmp"}; // so MmRealPath will return non empty path
    MOCKER_CPP(&PluginHandle::GetSoPaths)
        .stubs();
    int handle = 0;
    MOCKER_CPP(&PluginHandle::CheckSoValid)
        .stubs()
        .will(returnValue(false));
    MOCKER(dlopen)
        .stubs()
        .will(returnValue((VOID_PTR)&handle));
    pluginHandle.soPaths_ = fakeSoPaths;
    EXPECT_EQ(PROFILING_SUCCESS, pluginHandle.OpenPluginFromLdcfg(isSoValid));
    EXPECT_EQ(false, isSoValid);
}

TEST_F(PluginHandleUtest, GetAscendHalPathWillReturnEmptyStrWhenGetFileSizeReturnInvalidSize)
{
    GlobalMockObject::verify();
    static long long invalidAscendInstallInfoFileSize = 1025;
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    MOCKER_CPP(&Utils::GetFileSize)
        .stubs()
        .will(returnValue(invalidAscendInstallInfoFileSize));
    size_t expectPathSize = 0;
    EXPECT_EQ(expectPathSize, pluginHandle.GetAscendHalPath().size());
}

TEST_F(PluginHandleUtest, GetAscendHalPathWillReturnEmptyStrWhenAscendInstallInfoIsSoftLink)
{
    GlobalMockObject::verify();
    static long long validAscendInstallInfoFileSize = 1;
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    MOCKER_CPP(&Utils::GetFileSize)
        .stubs()
        .will(returnValue(validAscendInstallInfoFileSize));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(0, pluginHandle.GetAscendHalPath().size());
}

TEST_F(PluginHandleUtest, GetAscendHalPathWillReturnEmptyStrWhenFileOpenFail)
{
    GlobalMockObject::verify();
    static long long validAscendInstallInfoFileSize = 1;
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    MOCKER_CPP(&Utils::GetFileSize)
        .stubs()
        .will(returnValue(validAscendInstallInfoFileSize));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    // because ci env has no /etc/ascend_install.info, so open file will fail
    EXPECT_EQ(0, pluginHandle.GetAscendHalPath().size());
}

TEST_F(PluginHandleUtest, IsFuncExistWillReturnFalseWhenInputEmptyFuncName)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(false, pluginHandle.IsFuncExist(""));
}

TEST_F(PluginHandleUtest, IsFuncExistWillReturnFalseWhendlsymReturnNull)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    MOCKER(dlsym)
        .stubs()
        .will(returnValue((VOID_PTR)nullptr));
    EXPECT_EQ(false, pluginHandle.IsFuncExist("xx"));
}

TEST_F(PluginHandleUtest, IsFuncExistWillReturnTrueWhendlsymReturnNotNull)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    int func = 1;
    MOCKER(dlsym)
        .stubs()
        .will(returnValue((VOID_PTR)&func));
    EXPECT_EQ(true, pluginHandle.IsFuncExist("xx"));
}
