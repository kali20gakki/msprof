/**
* @file plugin_handle_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
*/

#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "mmpa/mmpa_api.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

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

TEST_F(PluginHandleUtest, OpenPlugin)
{
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    std::string envStrEmpty = "";
    std::string envStr = "LD_LIBRARY_PATH";
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPlugin(envStrEmpty));
    std::string retStrEmpty = "";
    std::string retStr = "/usr/local/Ascend/driver/lib64/libascend_hal.so";
    MOCKER_CPP(&PluginHandle::GetSoPath)
        .stubs()
        .will(returnValue(retStrEmpty))
        .then(returnValue(retStr));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPlugin(envStr));

    MOCKER_CPP(&MmRealPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPlugin(envStr));

    MOCKER(dlopen)
        .stubs()
        .will(returnValue(true));
    pluginHandle.handle_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, pluginHandle.OpenPlugin(envStr));
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
