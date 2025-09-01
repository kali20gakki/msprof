/**
* @file driver_plugin_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
*
*/
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "runtime_plugin.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;
using namespace analysis::dvvp::common::error;

int32_t RtGetVisibleDeviceIdByLogicDeviceId(int32_t logicDeviceId, int32_t* visibleDeviceId)
{
    *visibleDeviceId = 1;
    return PROFILING_SUCCESS;
}

int32_t RtProfilerTraceEx(uint64_t deviceId, uint64_t modelId, uint16_t tagId, aclrtStream streamId)
{
    return PROFILING_SUCCESS;
}

class RuntimePluginUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(RuntimePluginUtest, LoadRuntimeSoWillCreatePluginHandleWhenFirstLoad)
{
    GlobalMockObject::verify();
    auto runtimePlugin = RuntimePlugin::instance();
    MOCKER_CPP(&RuntimePlugin::GetAllFunction)
        .stubs();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(nullptr, runtimePlugin->pluginHandle_);
    runtimePlugin->LoadRuntimeSo();
    EXPECT_NE(nullptr, runtimePlugin->pluginHandle_);
    runtimePlugin->pluginHandle_ = nullptr;
}

TEST_F(RuntimePluginUtest, LoadRuntimeSoWillReturnWhenOpenPluginFromEnvAndOpenPluginFromLdcfgFail)
{
    GlobalMockObject::verify();
    auto runtimePlugin = RuntimePlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    runtimePlugin->LoadRuntimeSo();
    EXPECT_EQ(nullptr, runtimePlugin->rtGetVisibleDeviceIdByLogicDeviceIdFunc_);
    runtimePlugin->pluginHandle_ = nullptr;
}

TEST_F(RuntimePluginUtest, LoadRuntimeSoWillGetFunctionWhenOpenPluginFromEnvSucc)
{
    GlobalMockObject::verify();
    auto runtimePlugin = RuntimePlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    runtimePlugin->LoadRuntimeSo();
    runtimePlugin->pluginHandle_ = nullptr;
}

TEST_F(RuntimePluginUtest, LoadRuntimeSoWillGetFunctionWhenOpenPluginFromLdcfgSucc)
{
    GlobalMockObject::verify();
    auto runtimePlugin = RuntimePlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    runtimePlugin->LoadRuntimeSo();
    runtimePlugin->pluginHandle_ = nullptr;
}

TEST_F(RuntimePluginUtest, LoadRuntimeSo)
{
    GlobalMockObject::verify();
    auto runtimePlugin = RuntimePlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    runtimePlugin->LoadRuntimeSo();
}

TEST_F(RuntimePluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto runtimePlugin = RuntimePlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    std::string funcName = "rtGetVisibleDeviceIdByLogicDeviceId";
    EXPECT_EQ(true, runtimePlugin->IsFuncExist(funcName));
    EXPECT_EQ(false, runtimePlugin->IsFuncExist(funcName));
}

TEST_F(RuntimePluginUtest, MsprofRtGetVisibleDeviceIdByLogicDeviceId)
{
    GlobalMockObject::verify();
    int32_t logicDeviceId = 0;
    int32_t visibleDeviceId = 0;
    auto runtimePlugin = RuntimePlugin::instance();
    runtimePlugin->rtGetVisibleDeviceIdByLogicDeviceIdFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, runtimePlugin->MsprofRtGetVisibleDeviceIdByLogicDeviceId(logicDeviceId,
        &visibleDeviceId));
    runtimePlugin->rtGetVisibleDeviceIdByLogicDeviceIdFunc_ = RtGetVisibleDeviceIdByLogicDeviceId;
    EXPECT_EQ(PROFILING_SUCCESS, runtimePlugin->MsprofRtGetVisibleDeviceIdByLogicDeviceId(logicDeviceId,
        &visibleDeviceId));
    EXPECT_EQ(1, visibleDeviceId);
}

TEST_F(RuntimePluginUtest, MsprofRtProfilerTraceEx)
{
    GlobalMockObject::verify();
    uint64_t deviceId = 0;
    uint64_t modelId = 0;
    uint16_t tagId = 0;
    aclrtStream stmId = nullptr;
    auto runtimePlugin = RuntimePlugin::instance();
    runtimePlugin->rtProfilerTraceExFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, runtimePlugin->MsprofRtProfilerTraceEx(deviceId, modelId, tagId, stmId));
    runtimePlugin->rtProfilerTraceExFunc_ = RtProfilerTraceEx;
    EXPECT_EQ(PROFILING_SUCCESS, runtimePlugin->MsprofRtProfilerTraceEx(deviceId, modelId, tagId, stmId));
}