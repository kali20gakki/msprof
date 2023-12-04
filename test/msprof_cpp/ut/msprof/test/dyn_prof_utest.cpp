/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: UT for dynamic profiling
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-11-30
 */

#include <chrono>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "dyn_prof_mgr.h"
#include "dyn_prof_server.h"
#include "dyn_prof_thread.h"
#include "dyn_prof_client.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "config/config.h"
#include "thread/thread.h"
#include "message/prof_params.h"
#include "mmpa_api.h"
#include "prof_acl_mgr.h"
#include "msprof_callback_impl.h"
#include "prof_callback.h"

using namespace Collector::Dvvp::DynProf;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::thread;
using namespace Collector::Dvvp::Mmpa;

class DynProfUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

class DynProfThreadUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(DynProfThreadUtest, GetDelayAndDurationTime)
{
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    std::string emptyParams;
    std::string invalidParams = "xxxxxx";
    std::string validParamsWithDelayOrDurationUnset = "{\"delayTime\":\"\", \"durationTime\":\"\"}";
    std::string validParamsWithDelaySetInvalidValue = "{\"delayTime\":\"xx\", \"durationTime\":\"\"}";
    std::string validParamsWithDurationSetInvalidValue = "{\"delayTime\":\"\", \"durationTime\":\"xx\"}";
    std::string validParamsWithDelaySetValidValue = "{\"delayTime\":\"10\", \"durationTime\":\"\"}";
    std::string validParamsWithDurationSetValidValue = "{\"delayTime\":\"\", \"durationTime\":\"10\"}";
    std::string validParamsWithDelayAndDurationSetValidValue = "{\"delayTime\":\"20\", \"durationTime\":\"20\"}";
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(emptyParams))
        .then(returnValue(invalidParams))
        .then(returnValue(validParamsWithDelayOrDurationUnset))
        .then(returnValue(validParamsWithDelaySetInvalidValue))
        .then(returnValue(validParamsWithDurationSetInvalidValue))
        .then(returnValue(validParamsWithDelaySetValidValue))
        .then(returnValue(validParamsWithDurationSetValidValue))
        .then(returnValue(validParamsWithDelayAndDurationSetValidValue));
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(10, dynProfThread_->delayTime_); // 10 delay time from env
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(10, dynProfThread_->durationTime_); // 10 duration time from env
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->GetDelayAndDurationTime());
    EXPECT_EQ(20, dynProfThread_->delayTime_); // 20 delay time from env
    EXPECT_EQ(20, dynProfThread_->durationTime_); // 20 duration time from env
}

TEST_F(DynProfThreadUtest, DynProfThread_StopAfterDelay)
{
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    std::string Params = "{\"delayTime\":\"1\"}";
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(Params));
    dynProfThread_->Start(); // thread sleep 1s, then return

    // sleep 2s
    std::chrono::seconds sleepTime(2); // 2 sleep 2s as real task time
    std::this_thread::sleep_for(sleepTime);
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->Stop());
}

TEST_F(DynProfThreadUtest, DynProfThread_StopBeforeDelay)
{
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    std::string Params = "{\"delayTime\":\"10\"}";
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(Params));
    dynProfThread_->Start(); // thread sleep 1s, then return

    // sleep 2s
    std::chrono::seconds sleepTime(2); // 2 sleep 2s as real task time
    std::this_thread::sleep_for(sleepTime);
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->Stop());
}

TEST_F(DynProfThreadUtest, DynProfThread_StopBeforeDuration)
{
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    std::string Params = "{\"durationTime\":\"10\"}";
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(Params));
    dynProfThread_->Start(); // thread sleep 1s, then return

    // sleep 2s
    std::chrono::seconds sleepTime(2); // 2 sleep 2s as real task time
    std::this_thread::sleep_for(sleepTime);
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->Stop());
}

TEST_F(DynProfThreadUtest, DynProfThread_StopAfterDuration)
{
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    std::string Params = "{\"durationTime\":\"1\"}";
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(Params));
    dynProfThread_->Start(); // thread sleep 1s, then return

    // sleep 2s
    std::chrono::seconds sleepTime(2); // 2 sleep 2s as real task time
    std::this_thread::sleep_for(sleepTime);
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->Stop());
}

TEST_F(DynProfThreadUtest, DynProfThread_StopBetweenDelayAndDuration)
{
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    std::string Params = "{\"delayTime\":\"1\", \"durationTime\":\"10\"}";
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(Params));
    dynProfThread_->Start(); // thread sleep 1s, then return

    // sleep 2s
    std::chrono::seconds sleepTime(2); // 2 sleep 2s as real task time
    std::this_thread::sleep_for(sleepTime);
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->Stop());
}

class DynProfSrvUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(DynProfSrvUtest, DynProfServerStart)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = true;
    int ret = dynProfSrv->Start();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
 
    GlobalMockObject::verify();
    MOCKER_CPP(&DynProfServer::DynProfServerCreateSock)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
 
    dynProfSrv->srvStarted_ = false;
    ret = dynProfSrv->Start();
    EXPECT_EQ(PROFILING_FAILED, ret);
 
    ret = dynProfSrv->Start();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    dynProfSrv->Stop();
    dynProfSrv->Join();
}
 
TEST_F(DynProfSrvUtest, DynProfServerStop)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = false;
    int ret = dynProfSrv->Stop();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    MOCKER(close)
        .stubs();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->srvSockFd_ = 0;
    dynProfSrv->cliSockFd_ = 0;
    ret = dynProfSrv->Stop();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(false, dynProfSrv->srvStarted_);
}
 
TEST_F(DynProfSrvUtest, DynProfServerCreateSock)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    GlobalMockObject::verify();
    MOCKER(socket)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(-1))
        .then(returnValue(0));
    MOCKER(bind)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(listen)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
 
    int ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
}

TEST_F(DynProfSrvUtest, IdleConnectOverTime)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->profHasStarted_ = true;
    uint32_t time = 0;
    EXPECT_EQ(false, dynProfSrv->IdleConnectOverTime(time));
    dynProfSrv->profHasStarted_ = false;
    EXPECT_EQ(false, dynProfSrv->IdleConnectOverTime(time));
    time = DYN_PROF_IDLE_LINK_HOLD_TIME;
    EXPECT_EQ(true, dynProfSrv->IdleConnectOverTime(time));
}

TEST_F(DynProfSrvUtest, DynProfServerProcMsg)
{
    MOCKER(close)
        .stubs();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->DynProfServerProcMsg();
}
 
TEST_F(DynProfSrvUtest, DynProfSrvProcStart)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    
    MOCKER_CPP(&DynProfServer::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));
    dynProfSrv->profHasStarted_ = true;
    dynProfSrv->DynProfSrvProcStart();

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitAclEnv)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofHostHandle)
        .stubs();
    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    
    dynProfSrv->profHasStarted_ = false;
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
    EXPECT_EQ(0, dynProfSrv->startTimes_);
}
 
TEST_F(DynProfSrvUtest, DynProfSrvProcStop)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->profHasStarted_ = false;
    dynProfSrv->cliSockFd_ = 1;
    dynProfSrv->DynProfSrvProcStop();
 
    GlobalMockObject::verify();
    MOCKER_CPP(&DynProfServer::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofFinalizeHandle)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    
    dynProfSrv->profHasStarted_ = true;
    dynProfSrv->DynProfSrvProcStop();
    EXPECT_EQ(true, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStop();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
}
 
TEST_F(DynProfSrvUtest, DynProfSrvProcQuit)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->profHasStarted_ = true;
    dynProfSrv->cliSockFd_ = 1;
    GlobalMockObject::verify();
    MOCKER_CPP(&DynProfServer::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofFinalizeHandle)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    dynProfSrv->DynProfSrvProcQuit();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
}
 
TEST_F(DynProfSrvUtest, DynProfServerRsqMsg)
{
    std::string tips;
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->cliSockFd_ = 0;
    GlobalMockObject::verify();
    MOCKER(write)
        .stubs()
        .will(returnValue(-1));
    dynProfSrv->DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_FAIL, tips);
}

class DynProfMgrUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(DynProfMgrUtest, StartDynProf_ReturnFailed_WithInvalidProfilingMode)
{
    GlobalMockObject::verify();
    auto dynProfMgr = DynProfMgr::instance();
    std::string invalidMode;
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(invalidMode));
    EXPECT_EQ(PROFILING_FAILED, dynProfMgr->StartDynProf());
}

TEST_F(DynProfMgrUtest, StartDynProfSrv_WithDynamicProfilingMode)
{
    GlobalMockObject::verify();
    auto dynProfMgr = DynProfMgr::instance();
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(DAYNAMIC_PROFILING_VALUE));
    MOCKER_CPP(&DynProfServer::DynProfServerCreateSock)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, dynProfMgr->StartDynProf());
    EXPECT_EQ(false, dynProfMgr->IsDynProfStarted());
    dynProfMgr->StopDynProf(); // start failed, just return when StopDynProf is called
    
    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    EXPECT_EQ(true, dynProfMgr->IsDynProfStarted());
    // repeat start
    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    dynProfMgr->StopDynProf();
    EXPECT_EQ(false, dynProfMgr->IsDynProfStarted()); // success to stop when StopDynProf is called after start succeed
}

TEST_F(DynProfMgrUtest, StartDynProfThread_WithDelayOrDurationProfilingMode)
{
    GlobalMockObject::verify();
    auto dynProfMgr = DynProfMgr::instance();
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(DELAY_DURARION_PROFILING_VALUE));
    MOCKER_CPP(&DynProfThread::GetDelayAndDurationTime)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, dynProfMgr->StartDynProf());
    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    // repeat start
    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    EXPECT_EQ(true, dynProfMgr->IsDynProfStarted());
    ProfSetDevPara data;
    dynProfMgr->SaveDevicesInfo(data);
    dynProfMgr->StopDynProf();
    EXPECT_EQ(false, dynProfMgr->IsDynProfStarted()); // success to stop when StopDynProf is called after start succeed
}
