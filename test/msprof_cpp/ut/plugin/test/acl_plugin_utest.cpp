/**
* @file driver_plugin_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
*/
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "acl_plugin.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;
using namespace analysis::dvvp::common::error;

int32_t AclrtProfTrace(void *userData, int32_t length, aclrtStream streamId)
{
    return PROFILING_SUCCESS;
}

int32_t AclrtGetLogicDevIdByUserDevId(int32_t userDevid, int32_t *logicDevId)
{
    return PROFILING_SUCCESS;
}

class AclPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(AclPluginUtest, LoadAclSoWillCreatePluginHandleWhenFirstLoad)
{
    GlobalMockObject::verify();
    auto AclPlugin = AclPlugin::instance();
    MOCKER_CPP(&AclPlugin::GetAllFunction)
        .stubs();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(nullptr, AclPlugin->pluginHandle_);
    AclPlugin->LoadAclSo();
    EXPECT_NE(nullptr, AclPlugin->pluginHandle_);
    AclPlugin->pluginHandle_ = nullptr;
}

TEST_F(AclPluginUtest, LoadAclSoWillReturnWhenOpenPluginFromEnvAndOpenPluginFromLdcfgFail)
{
    GlobalMockObject::verify();
    auto AclPlugin = AclPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    AclPlugin->LoadAclSo();
    EXPECT_EQ(nullptr, AclPlugin->aclrtProfTraceFunc_);
    AclPlugin->pluginHandle_ = nullptr;
}

TEST_F(AclPluginUtest, LoadAclSoWillGetFunctionWhenOpenPluginFromEnvSucc)
{
    GlobalMockObject::verify();
    auto AclPlugin = AclPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    AclPlugin->LoadAclSo();
    AclPlugin->pluginHandle_ = nullptr;
}

TEST_F(AclPluginUtest, LoadAclSoWillGetFunctionWhenOpenPluginFromLdcfgSucc)
{
    GlobalMockObject::verify();
    auto AclPlugin = AclPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    AclPlugin->LoadAclSo();
    AclPlugin->pluginHandle_ = nullptr;
}

TEST_F(AclPluginUtest, LoadAclSo)
{
    GlobalMockObject::verify();
    auto AclPlugin = AclPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    AclPlugin->LoadAclSo();
}

TEST_F(AclPluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto AclPlugin = AclPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    std::string funcName = "aclrtProfTrace";
    EXPECT_EQ(true, AclPlugin->IsFuncExist(funcName));
    EXPECT_EQ(false, AclPlugin->IsFuncExist(funcName));
}

TEST_F(AclPluginUtest, MsprofAclrtProfTrace)
{
    GlobalMockObject::verify();
    uint64_t deviceId = 0;
    uint64_t modelId = 0;
    uint16_t tagId = 0;
    aclrtStream stmId = nullptr;
    auto AclPlugin = AclPlugin::instance();
    AclPlugin->aclrtProfTraceFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, AclPlugin->MsprofAclrtProfTrace(deviceId, modelId, tagId, stmId));
    AclPlugin->aclrtProfTraceFunc_ = AclrtProfTrace;
    EXPECT_EQ(PROFILING_SUCCESS, AclPlugin->MsprofAclrtProfTrace(deviceId, modelId, tagId, stmId));
}

TEST_F(AclPluginUtest, MsprofAclrtGetLogicDevIdByUserDevId)
{
    GlobalMockObject::verify();
    int32_t userDevId = 0;
    int32_t logicDevId = 0;
    auto AclPlugin = AclPlugin::instance();
    AclPlugin->aclrtGetLogicDevIdByUserDevIdFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, AclPlugin->MsprofAclrtGetLogicDevIdByUserDevId(userDevId, &logicDevId));
    AclPlugin->aclrtGetLogicDevIdByUserDevIdFunc_ = AclrtGetLogicDevIdByUserDevId;
    EXPECT_EQ(PROFILING_SUCCESS, AclPlugin->MsprofAclrtGetLogicDevIdByUserDevId(userDevId, &logicDevId));
}