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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "tsdclient_plugin.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::error;

uint32_t TsdCapabilityGet(uint32_t logicDeviceId, int32_t type, uint64_t ptr)
{
    return TSD_OK;
}

uint32_t TsdProcessOpen(uint32_t logicDeviceId, ProcOpenArgs *openArgs)
{
    return TSD_OK;
}

uint32_t MsprofTsdGetProcListStatus(uint32_t logicDeviceId, ProcStatusParam *pidInfo, uint32_t arrayLen)
{
    return TSD_OK;
}

uint32_t ProcessCloseSubProcList(uint32_t logicDeviceId, ProcStatusParam *closeList, uint32_t listSize)
{
    return TSD_OK;
}

class TsdClientPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(TsdClientPluginUtest, LoadTsdClientSoWillCreatePluginHandleWhenFirstLoad)
{
    GlobalMockObject::verify();
    auto tsdClientPlugin = TsdClientPlugin::instance();
    MOCKER_CPP(&TsdClientPlugin::GetAllFunction).stubs();
    MOCKER_CPP(&PluginHandle::HasLoad).stubs().will(returnValue(true));
    EXPECT_EQ(nullptr, tsdClientPlugin->pluginHandle_);
    tsdClientPlugin->LoadTsdClientSo();
    EXPECT_NE(nullptr, tsdClientPlugin->pluginHandle_);
    tsdClientPlugin->pluginHandle_ = nullptr;
}

TEST_F(TsdClientPluginUtest, LoadTsdClientSoWillReturnWhenOpenPluginFromEnvAndOpenPluginFromLdcfgFail)
{
    GlobalMockObject::verify();
    auto tsdClientPlugin = TsdClientPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad).stubs().will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv).stubs().will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg).stubs().will(returnValue(PROFILING_FAILED));
    tsdClientPlugin->LoadTsdClientSo();
    EXPECT_EQ(nullptr, tsdClientPlugin->tsdCapabilityGetFunc_);
    tsdClientPlugin->pluginHandle_ = nullptr;
}

TEST_F(TsdClientPluginUtest, LoadTsdClientSoWillGetFunctionWhenOpenPluginFromEnvSucc)
{
    GlobalMockObject::verify();
    auto tsdClientPlugin = TsdClientPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad).stubs().will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv).stubs().will(returnValue(PROFILING_SUCCESS));
    tsdClientPlugin->LoadTsdClientSo();
    tsdClientPlugin->pluginHandle_ = nullptr;
}

TEST_F(TsdClientPluginUtest, LoadTsdClientSoWillGetFunctionWhenOpenPluginFromLdcfgSucc)
{
    GlobalMockObject::verify();
    auto tsdClientPlugin = TsdClientPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad).stubs().will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv).stubs().will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg).stubs().will(returnValue(PROFILING_SUCCESS));
    tsdClientPlugin->LoadTsdClientSo();
    tsdClientPlugin->pluginHandle_ = nullptr;
}

TEST_F(TsdClientPluginUtest, LoadTsdClientSo)
{
    GlobalMockObject::verify();
    auto tsdClientPlugin = TsdClientPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad).stubs().will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv).stubs().will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg).stubs().will(returnValue(PROFILING_SUCCESS));
    tsdClientPlugin->LoadTsdClientSo();
}

TEST_F(TsdClientPluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto tsdClientPlugin = TsdClientPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist).stubs().will(returnValue(true)).then(returnValue(false));
    std::string funcName = "TsdCapabilityGet";
    EXPECT_EQ(true, tsdClientPlugin->IsFuncExist(funcName));
    EXPECT_EQ(false, tsdClientPlugin->IsFuncExist(funcName));
}

TEST_F(TsdClientPluginUtest, MsprofTsdCapabilityGet)
{
    GlobalMockObject::verify();
    uint32_t logicDeviceId = 0;
    int32_t type = 0;
    uint64_t ptr = 0;
    auto tsdClientPlugin = TsdClientPlugin::instance();
    tsdClientPlugin->tsdCapabilityGetFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, tsdClientPlugin->MsprofTsdCapabilityGet(logicDeviceId, type, ptr));
    tsdClientPlugin->tsdCapabilityGetFunc_ = TsdCapabilityGet;
    EXPECT_EQ(PROFILING_SUCCESS, tsdClientPlugin->MsprofTsdCapabilityGet(logicDeviceId, type, ptr));
}

TEST_F(TsdClientPluginUtest, MsprofTsdProcessOpen)
{
    GlobalMockObject::verify();
    uint32_t logicDeviceId = 0;
    ProcOpenArgs openArgs;
    auto tsdClientPlugin = TsdClientPlugin::instance();
    tsdClientPlugin->tsdProcessOpenFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, tsdClientPlugin->MsprofTsdProcessOpen(logicDeviceId, &openArgs));
    tsdClientPlugin->tsdProcessOpenFunc_ = TsdProcessOpen;
    EXPECT_EQ(PROFILING_SUCCESS, tsdClientPlugin->MsprofTsdProcessOpen(logicDeviceId, &openArgs));
}

TEST_F(TsdClientPluginUtest, MsprofTsdGetProcListStatus)
{
    GlobalMockObject::verify();
    uint32_t logicDeviceId = 0;
    ProcStatusParam pidInfo;
    uint32_t arrayLen = 0;
    auto tsdClientPlugin = TsdClientPlugin::instance();
    tsdClientPlugin->tsdGetProcListStatusFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, tsdClientPlugin->MsprofTsdGetProcListStatus(logicDeviceId, &pidInfo, arrayLen));
    tsdClientPlugin->tsdGetProcListStatusFunc_ = MsprofTsdGetProcListStatus;
    EXPECT_EQ(PROFILING_SUCCESS, tsdClientPlugin->MsprofTsdGetProcListStatus(logicDeviceId, &pidInfo, arrayLen));
}

TEST_F(TsdClientPluginUtest, MsprofProcessCloseSubProcList)
{
    GlobalMockObject::verify();
    uint32_t logicDeviceId = 0;
    ProcStatusParam closeList;
    uint32_t listSize = 0;
    auto tsdClientPlugin = TsdClientPlugin::instance();
    tsdClientPlugin->processCloseSubProcListFunc_ = nullptr;
    EXPECT_EQ(PROFILING_NOTSUPPORT, tsdClientPlugin->MsprofProcessCloseSubProcList(logicDeviceId, &closeList,
                                                                                   listSize));
    tsdClientPlugin->processCloseSubProcListFunc_ = ProcessCloseSubProcList;
    EXPECT_EQ(PROFILING_SUCCESS, tsdClientPlugin->MsprofProcessCloseSubProcList(logicDeviceId, &closeList,
                                                                                listSize));
}