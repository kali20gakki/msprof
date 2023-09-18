/**
* @file plugin_handle_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
*/

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
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    std::string envStrEmpty = "";
    std::string envStr = "LD_LIBRARY_PATH";
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStrEmpty));
    std::string retStrEmpty = "";
    std::string retStr = "/usr/local/Ascend/driver/lib64/libascend_hal.so";
    MOCKER_CPP(&PluginHandle::GetSoPath)
        .stubs()
        .will(returnValue(retStrEmpty))
        .then(returnValue(retStr));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr));

    MOCKER_CPP(&MmRealPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr));

    MOCKER_CPP(&PluginHandle::CheckSoValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr));

    int openfd = 0;
    MOCKER_CPP(&PluginHandle::DlopenSo)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPluginFromEnv(envStr));
    EXPECT_EQ(PROFILING_SUCCESS, pluginHandle.OpenPluginFromEnv(envStr));
}

TEST_F(PluginHandleUtest, CloseHandle)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    pluginHandle.handle_ = NULL;
    pluginHandle.CloseHandle();
    pluginHandle.load_ = true;
    EXPECT_EQ(true, pluginHandle.HasLoad());
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

TEST_F(PluginHandleUtest, DlopenSo)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.DlopenSo(nullptr));
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
    EXPECT_EQ(false, pluginHandle.CheckSoValid(nullptr));
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
        .then(returnValue(fp));
    MOCKER(pclose)
        .stubs()
        .then(returnValue(0));
    MOCKER(MmClose)
        .stubs()
        .then(returnValue(0));

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
    EXPECT_EQ(1, soPaths.size());
}