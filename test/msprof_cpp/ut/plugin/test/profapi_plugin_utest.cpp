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
#include "profapi_plugin.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

int32_t ReporterCallback(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len)
{
    return 0;
}

int32_t RegReporterCallback(ProfReportHandle handle)
{
    return 0;
}

int32_t RegProfilerCallback(int32_t callbackType, VOID_PTR callback, uint32_t len)
{
    return 0;
}

int32_t CtrlCallback(uint32_t type, VOID_PTR data, uint32_t len)
{
    return 0;
}

int32_t RegCtrlCallback(ProfCtrlHandle handle)
{
    return 0;
}

int32_t SetDeviceCallback(VOID_PTR data, uint32_t len)
{
    return 0;
}

int32_t RegSetDeviceCallback(ProfSetDeviceHandle handle)
{
    return 0;
}

int32_t GetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId)
{
    return 0;
}

int32_t ProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len)
{
    return 0;
}

int32_t ProfSetStepInfo(const uint64_t indexId, const uint16_t tagId, VOID_PTR const stream)
{
    return 0;
}

class ProfapiPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(ProfapiPluginUtest, LoadProfApiSo)
{
    GlobalMockObject::verify();
    auto profapiPlugin = ProfApiPlugin::instance();
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    profapiPlugin->LoadProfApiSo();
    EXPECT_EQ(nullptr, profapiPlugin->profRegReporterCallback_);
    profapiPlugin->LoadProfApiSo();
    EXPECT_EQ(nullptr, profapiPlugin->profRegReporterCallback_);
    profapiPlugin->LoadProfApiSo();
}

TEST_F(ProfapiPluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto profapiPlugin = ProfApiPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true));
    std::string funcName = "profRegReporterCallback";
    EXPECT_EQ(true, profapiPlugin->IsFuncExist(funcName));
}

TEST_F(ProfapiPluginUtest, MsprofProfRegReporterCallback)
{
    GlobalMockObject::verify();
    auto profapiPlugin = ProfApiPlugin::instance();
    profapiPlugin->profRegReporterCallback_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profapiPlugin->MsprofProfRegReporterCallback(ReporterCallback));
    profapiPlugin->profRegReporterCallback_ = RegReporterCallback;
    EXPECT_EQ(PROFILING_SUCCESS, profapiPlugin->MsprofProfRegReporterCallback(ReporterCallback));
}

TEST_F(ProfapiPluginUtest, MsprofProfRegProfilerCallback)
{
    GlobalMockObject::verify();
    int32_t callbackType = PROFILE_CTRL_CALLBACK;
    int callback = 0;
    uint32_t len = 0;
    auto profapiPlugin = ProfApiPlugin::instance();
    profapiPlugin->profRegProfilerCallback_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profapiPlugin->MsprofProfRegProfilerCallback(callbackType, (VOID_PTR)&callback, len));
    profapiPlugin->profRegProfilerCallback_ = RegProfilerCallback;
    EXPECT_EQ(PROFILING_SUCCESS, profapiPlugin->MsprofProfRegProfilerCallback(callbackType, (VOID_PTR)&callback, len));
}

TEST_F(ProfapiPluginUtest, MsprofProfRegCtrlCallback)
{
    GlobalMockObject::verify();
    auto profapiPlugin = ProfApiPlugin::instance();
    profapiPlugin->profRegCtrlCallback_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profapiPlugin->MsprofProfRegCtrlCallback(CtrlCallback));
    profapiPlugin->profRegCtrlCallback_ = RegCtrlCallback;
    EXPECT_EQ(PROFILING_SUCCESS, profapiPlugin->MsprofProfRegCtrlCallback(CtrlCallback));
}

TEST_F(ProfapiPluginUtest, MsprofProfRegDeviceStateCallback)
{
    GlobalMockObject::verify();
    auto profapiPlugin = ProfApiPlugin::instance();
    profapiPlugin->profRegDeviceStateCallback_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profapiPlugin->MsprofProfRegDeviceStateCallback(SetDeviceCallback));
    profapiPlugin->profRegDeviceStateCallback_ = RegSetDeviceCallback;
    EXPECT_EQ(PROFILING_SUCCESS, profapiPlugin->MsprofProfRegDeviceStateCallback(SetDeviceCallback));
}

TEST_F(ProfapiPluginUtest, MsprofProfGetDeviceIdByGeModelIdx)
{
    GlobalMockObject::verify();
    uint32_t modelIdx = 0;
    uint32_t deviceId = 0;
    auto profapiPlugin = ProfApiPlugin::instance();
    profapiPlugin->profGetDeviceIdByGeModelIdx_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profapiPlugin->MsprofProfGetDeviceIdByGeModelIdx(modelIdx, &deviceId));
    profapiPlugin->profGetDeviceIdByGeModelIdx_ = GetDeviceIdByGeModelIdx;
    EXPECT_EQ(PROFILING_SUCCESS, profapiPlugin->MsprofProfGetDeviceIdByGeModelIdx(modelIdx, &deviceId));
}

TEST_F(ProfapiPluginUtest, MsprofProfSetProfCommand)
{
    GlobalMockObject::verify();
    int command = 0;
    uint32_t len = 0;
    auto profapiPlugin = ProfApiPlugin::instance();
    profapiPlugin->profSetProfCommand_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profapiPlugin->MsprofProfSetProfCommand((PROFAPI_PROF_COMMAND_PTR)&command, len));
    profapiPlugin->profSetProfCommand_ = ProfSetProfCommand;
    EXPECT_EQ(PROFILING_SUCCESS, profapiPlugin->MsprofProfSetProfCommand((PROFAPI_PROF_COMMAND_PTR)&command, len));
}

TEST_F(ProfapiPluginUtest, MsprofProfSetStepInfo)
{
    GlobalMockObject::verify();
    uint64_t indexId = 0;
    int16_t tagId = 0;
    int stream = 0;
    auto profapiPlugin = ProfApiPlugin::instance();
    profapiPlugin->profSetStepInfo_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profapiPlugin->MsprofProfSetStepInfo(indexId, tagId, (VOID_PTR)&stream));
    profapiPlugin->profSetStepInfo_ = ProfSetStepInfo;
    EXPECT_EQ(PROFILING_SUCCESS, profapiPlugin->MsprofProfSetStepInfo(indexId, tagId, (VOID_PTR)&stream));
}
