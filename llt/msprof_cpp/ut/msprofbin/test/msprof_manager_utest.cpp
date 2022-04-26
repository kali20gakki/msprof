#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include "errno/error_code.h"
#include "msprof_manager.h"
#include "message/codec.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "param_validation.h"
#include "running_mode.h"
#include "input_parser.h"
#include "platform/platform.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Msprof;
using namespace Analysis::Dvvp::Common::Platform;
class MSPROF_MANAGER_UTEST : public testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};


TEST_F(MSPROF_MANAGER_UTEST, Init) { 
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);

    auto msprofManager = MsprofManager::instance();
    EXPECT_EQ(PROFILING_FAILED, MsprofManager::instance()->Init(nullptr));

    EXPECT_EQ(PROFILING_FAILED, MsprofManager::instance()->Init(params));
    MOCKER_CPP(&Analysis::Dvvp::Msprof::MsprofManager::GenerateRunningMode)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, MsprofManager::instance()->Init(params));

    MOCKER_CPP(&Analysis::Dvvp::Msprof::MsprofManager::ParamsCheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, MsprofManager::instance()->Init(params));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofManager::instance()->Init(params));
}

TEST_F(MSPROF_MANAGER_UTEST, MsProcessCmd) { 
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<Collector::Dvvp::Msprofbin::AppMode> rMode(
    new Collector::Dvvp::Msprofbin::AppMode("app", params));
    auto msprofManager = MsprofManager::instance();
    msprofManager->UnInit();
    EXPECT_EQ(PROFILING_FAILED, msprofManager->MsProcessCmd());
    msprofManager->params_ = params;
    msprofManager->rMode_ = rMode;
    MOCKER_CPP_VIRTUAL(rMode.get(), &Collector::Dvvp::Msprofbin::AppMode::RunModeTasks)
        .stubs()
        .then(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->MsProcessCmd());
}

TEST_F(MSPROF_MANAGER_UTEST, StopNoWait) { 
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<Collector::Dvvp::Msprofbin::AppMode> rMode(
    new Collector::Dvvp::Msprofbin::AppMode("app", params));
    auto msprofManager = MsprofManager::instance();
    msprofManager->UnInit();   
    msprofManager->StopNoWait();
    msprofManager->rMode_ = rMode;
    MOCKER_CPP_VIRTUAL((Collector::Dvvp::Msprofbin::RunningMode*)rMode.get(),
        &Collector::Dvvp::Msprofbin::AppMode::UpdateOutputDirInfo).stubs();;
    msprofManager->StopNoWait();
}

TEST_F(MSPROF_MANAGER_UTEST, GetTask) { 
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<Collector::Dvvp::Msprofbin::AppMode> rMode(
    new Collector::Dvvp::Msprofbin::AppMode("app", params));
    auto msprofManager = MsprofManager::instance();   
    msprofManager->UnInit();   
    EXPECT_EQ(nullptr, msprofManager->GetTask("1"));
    msprofManager->rMode_ = rMode;
    std::shared_ptr<Analysis::Dvvp::Msprof::ProfTask> info(new Analysis::Dvvp::Msprof::ProfSocTask(1, params));
    MOCKER_CPP(&Collector::Dvvp::Msprofbin::RunningMode::GetRunningTask)
        .stubs()
        .will(returnValue(info));
    EXPECT_EQ(info, msprofManager->GetTask("1"));
}

TEST_F(MSPROF_MANAGER_UTEST, GenerateRunningMode) { 
    auto msprofManager = MsprofManager::instance();
    EXPECT_EQ(PROFILING_FAILED, msprofManager->GenerateRunningMode());
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);
    params->app = "main";
    msprofManager->params_ = params;
    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->GenerateRunningMode());
    params->app = "";
    params->devices = "0";
    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->GenerateRunningMode());
    params->devices = "";
    params->host_sys = "on";
    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->GenerateRunningMode());
    params->host_sys = "";
    params->parseSwitch = "on";
    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->GenerateRunningMode());
    params->parseSwitch = "";
    params->querySwitch = "on";
    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->GenerateRunningMode());
    params->querySwitch = "";
    params->exportSwitch = "on";
    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->GenerateRunningMode());
    params->exportSwitch = "";
    EXPECT_EQ(PROFILING_FAILED, msprofManager->GenerateRunningMode());

    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
    params->devices = "0";
    EXPECT_EQ(PROFILING_FAILED, msprofManager->GenerateRunningMode());
    params->devices = "";
    params->host_sys = "on";
    EXPECT_EQ(PROFILING_FAILED, msprofManager->GenerateRunningMode());
}

TEST_F(MSPROF_MANAGER_UTEST, GenerateRunningMod_helper) { 
    auto msprofManager = MsprofManager::instance();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);
    
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(true));
    params->devices = "0";
    EXPECT_EQ(PROFILING_FAILED, msprofManager->GenerateRunningMode());
    params->host_sys = "on";
    EXPECT_EQ(PROFILING_FAILED, msprofManager->GenerateRunningMode());
}

TEST_F(MSPROF_MANAGER_UTEST, ParamsCheck) { 
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<Collector::Dvvp::Msprofbin::AppMode> rMode(
    new Collector::Dvvp::Msprofbin::AppMode("app", params));
    auto msprofManager = MsprofManager::instance();
    msprofManager->UnInit();   
    EXPECT_EQ(PROFILING_FAILED, msprofManager->ParamsCheck());
    msprofManager->params_ = params;
    EXPECT_EQ(PROFILING_FAILED, msprofManager->ParamsCheck());
    msprofManager->rMode_ = rMode;
    MOCKER_CPP_VIRTUAL(rMode.get(), &Collector::Dvvp::Msprofbin::AppMode::ModeParamsCheck)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, msprofManager->ParamsCheck());
    EXPECT_EQ(PROFILING_SUCCESS, msprofManager->ParamsCheck());
}