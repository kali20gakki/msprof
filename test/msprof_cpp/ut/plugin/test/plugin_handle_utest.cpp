#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "mmpa/mmpa_api.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

class PLUGIN_HANDLE_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(PLUGIN_HANDLE_TEST, GetSoName) {
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    EXPECT_STREQ(soName.c_str(), pluginHandle.GetSoName().c_str());
}

TEST_F(PLUGIN_HANDLE_TEST, OpenPlugin) {
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

TEST_F(PLUGIN_HANDLE_TEST, CloseHandle) {
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    pluginHandle.handle_ = NULL;
    pluginHandle.CloseHandle();
    pluginHandle.load_ = true;
    EXPECT_EQ(true, pluginHandle.HasLoad());
}

TEST_F(PLUGIN_HANDLE_TEST, GetSoPath) {
    GlobalMockObject::verify();
    std::string soName = "libascend_hal.so";
    PluginHandle pluginHandle(soName);
    char *pathEnv = "/usr/local/Ascend/driver/lib64";
    std::string path = "PATH";
    MOCKER(std::getenv)
        .stubs()
        .will(returnValue((char *)nullptr))
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
