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
#include "prof_manager.h"
#include "prof_ge_core.h"

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
    MOCKER_CPP(&DynProfThread::StartProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DynProfThread::StopProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    dynProfThread_->Start(); // thread sleep 1s, then return
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->Start()); // repeat start

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
    MOCKER_CPP(&DynProfThread::StartProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DynProfThread::StopProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
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
    MOCKER_CPP(&DynProfThread::StartProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DynProfThread::StopProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
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
    MOCKER_CPP(&DynProfThread::StartProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DynProfThread::StopProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
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
    MOCKER_CPP(&DynProfThread::StartProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DynProfThread::StopProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    dynProfThread_->Start(); // thread sleep 1s, then return

    // sleep 2s
    std::chrono::seconds sleepTime(2); // 2 sleep 2s as real task time
    std::this_thread::sleep_for(sleepTime);
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->Stop());
}

TEST_F(DynProfThreadUtest, StartProfTaskWillReturnFailWhileHandleDevProfTaskFail)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    dynProfThread_->SaveDevicesInfo(data);
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitAclEnv)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofHostHandle)
        .stubs();
    MOCKER_CPP(&Collector::Dvvp::DynProf::DynProfThread::HandleDevProfTask)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->StartProfTask());
}

TEST_F(DynProfThreadUtest, StartProfTaskWillReturnFailWhileProfAclMgrInitFail)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    dynProfThread_->SaveDevicesInfo(data);
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitAclEnv)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofHostHandle)
        .stubs();
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->StartProfTask());
}

TEST_F(DynProfThreadUtest, StartProfTaskWillReturnSuccWhileHandleDevProfTaskSucc)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    dynProfThread_->SaveDevicesInfo(data);
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitAclEnv)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofHostHandle)
        .stubs();
    MOCKER_CPP(&Collector::Dvvp::DynProf::DynProfThread::HandleDevProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->StartProfTask());
}

TEST_F(DynProfThreadUtest, StopProfTaskWillReturnFailWhenMsprofFinalizeHandleReturnFail)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofFinalizeHandle)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->StopProfTask());
}

TEST_F(DynProfThreadUtest, StopProfTaskWillReturnSuccWhenMsprofFinalizeHandleReturnSucc)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofFinalizeHandle)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->StopProfTask());
}

TEST_F(DynProfThreadUtest, DynProfThreadWillStartDeviceCollectWhileProfHasStart)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Collector::Dvvp::DynProf::DynProfThread::HandleDevProfTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfThread_->profHasStarted_ = true;
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfThread_->deviceInfos_.size());
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfThread_->deviceInfos_.size());
}

TEST_F(DynProfThreadUtest, DynProfThreadWillInsertDeviceWhileSetNewDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfThread_->deviceInfos_.size());
    data.deviceId = 1;
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(2, dynProfThread_->deviceInfos_.size()); // 2 : device num
}

TEST_F(DynProfThreadUtest, DynProfThreadWillJustReturnWhileSetRepeatDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfThread_->deviceInfos_.size());
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfThread_->deviceInfos_.size());
}

TEST_F(DynProfThreadUtest, DynProfThreadWillEraseDeviceWhileResetExistDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfThread_->deviceInfos_.size());
    data.isOpen = false;
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(0, dynProfThread_->deviceInfos_.size());
}

TEST_F(DynProfThreadUtest, DynProfThreadWillJustReturnWhileResetNoneExistDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = false;
    dynProfThread_->SaveDevicesInfo(data);
    EXPECT_EQ(0, dynProfThread_->deviceInfos_.size());
}

TEST_F(DynProfThreadUtest, HandleDevProfTaskWillReturnSuccWhileStartDeviceSucc)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.isOpen = true;
    MOCKER_CPP(&analysis::dvvp::host::ProfManager::CheckIfDevicesOnline)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&ge::GeOpenDeviceHandle)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->HandleDevProfTask(data));
}

TEST_F(DynProfThreadUtest, HandleDevProfTaskWillReturnFailWhileStartDeviceFail)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.isOpen = true;
    MOCKER_CPP(&analysis::dvvp::host::ProfManager::CheckIfDevicesOnline)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->HandleDevProfTask(data));
}

TEST_F(DynProfThreadUtest, HandleDevProfTaskWillReturnSuccWhileStopDeviceSucc)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.isOpen = false;
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofResetDeviceHandle)
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(PROFILING_SUCCESS, dynProfThread_->HandleDevProfTask(data));
}

TEST_F(DynProfThreadUtest, HandleDevProfTaskWillReturnFailWhileStopDeviceFail)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
    MSVP_MAKE_SHARED0_VOID(dynProfThread_, DynProfThread);
    ProfSetDevPara data;
    data.isOpen = false;
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofResetDeviceHandle)
        .stubs()
        .will(returnValue(1));
    EXPECT_EQ(PROFILING_FAILED, dynProfThread_->HandleDevProfTask(data));
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

size_t RecvReturnInvalidReqStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    return 0;
}

size_t RecvReturnTestReqStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto reqMsg = reinterpret_cast<DynProfReqMsg *>(msg);
    reqMsg->msgType = DynProfMsgType::TEST_REQ;
    return sizeof(DynProfReqMsg);
}

size_t RecvReturnStartReqStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto reqMsg = reinterpret_cast<DynProfReqMsg *>(msg);
    reqMsg->msgType = DynProfMsgType::START_REQ;
    return sizeof(DynProfReqMsg);
}

size_t RecvReturnStopReqStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto reqMsg = reinterpret_cast<DynProfReqMsg *>(msg);
    reqMsg->msgType = DynProfMsgType::STOP_REQ;
    return sizeof(DynProfReqMsg);
}

size_t RecvReturnQuitReqStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto reqMsg = reinterpret_cast<DynProfReqMsg *>(msg);
    reqMsg->msgType = DynProfMsgType::QUIT_REQ;
    return sizeof(DynProfReqMsg);
}

TEST_F(DynProfSrvUtest, DynProfServerStart)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    MOCKER_CPP(&DynProfServer::DynProfServerCreateSock)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
 
    dynProfSrv->srvStarted_ = false;
    EXPECT_EQ(PROFILING_FAILED, dynProfSrv->Start());

    EXPECT_EQ(PROFILING_SUCCESS, dynProfSrv->Start());
    // repeat start server
    EXPECT_EQ(PROFILING_SUCCESS, dynProfSrv->Start());
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
    MOCKER_CPP(&MmUnlink)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&close)
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
    MOCKER(strncpy_s)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(EOK));
    MOCKER(bind)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(listen)
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
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(DynProfSrvUtest, IdleConnectOverTime)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->profHasStarted_.store(true);
    uint32_t time = 0;
    EXPECT_EQ(false, dynProfSrv->IdleConnectOverTime(time));
    dynProfSrv->profHasStarted_.store(false);
    EXPECT_EQ(false, dynProfSrv->IdleConnectOverTime(time));
    time = DYN_PROF_IDLE_LINK_HOLD_TIME;
    EXPECT_EQ(true, dynProfSrv->IdleConnectOverTime(time));
}

TEST_F(DynProfSrvUtest, DynProfServerProcMsgWillReturnWhenRecvReturnTestReqAndDynProfServerRsqMsgReturnFail)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnTestReqStub));
    MOCKER_CPP(&DynProfServer::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&DynProfServer::DynProfSrvProcQuit)
        .stubs();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->DynProfServerProcMsg();
}

TEST_F(DynProfSrvUtest, DynProfServerProcMsgWillReturnWhenRecvReturnTestReqAndDynProfServerRsqMsgReturnSucc)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnTestReqStub));
    MOCKER_CPP(&DynProfServer::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DynProfServer::DynProfSrvProcQuit)
        .stubs();
    dynProfSrv->srvStarted_ = true;
    for (uint32_t i = 0; i <= DYN_PROF_SERVER_PROC_MSG_MAX_NUM; i++) {
        dynProfSrv->DynProfServerProcMsg();
    }
}

TEST_F(DynProfSrvUtest, DynProfServerProcMsgWillReturnWhenRecvReturnStartReq)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnStartReqStub));
    MOCKER_CPP(&DynProfServer::DynProfSrvProcStart)
        .stubs();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->DynProfServerProcMsg();
}

TEST_F(DynProfSrvUtest, DynProfServerProcMsgWillReturnWhenRecvReturnStopReq)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnStopReqStub));
    MOCKER_CPP(&DynProfServer::DynProfSrvProcStop)
        .stubs();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->DynProfServerProcMsg();
}

TEST_F(DynProfSrvUtest, DynProfServerProcMsgWillReturnWhenRecvReturnQuitReq)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnQuitReqStub));
    MOCKER_CPP(&DynProfServer::DynProfSrvProcStop)
        .stubs();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->DynProfServerProcMsg();
}

TEST_F(DynProfSrvUtest, DynProfServerProcMsgWillReturnWhenRecvReturnInvalidReq)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnInvalidReqStub));
    MOCKER_CPP(&DynProfServer::DynProfSrvProcStop)
        .stubs();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->DynProfServerProcMsg();
}
 
TEST_F(DynProfSrvUtest, DynProfSrvProcStartWillSuccWhileStartDeviceSucc)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    
    MOCKER_CPP(&DynProfServer::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitAclEnv)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::SetModeToOff)
        .stubs();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofHostHandle)
        .stubs();
    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl)
        .stubs()
        .will(returnValue(0));

    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfSrv->SaveDevicesInfo(data);

    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_.load());
 
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_.load());

    // start for max times
    for (uint32_t i = 0; i <= DYN_PROF_MAX_STARTABLE_TIMES; i++) {
        dynProfSrv->DynProfSrvProcStart();
        EXPECT_EQ(true, dynProfSrv->profHasStarted_.load());
        dynProfSrv->profHasStarted_.store(false); // set false for next start
        EXPECT_EQ(i + 1, dynProfSrv->startTimes_);
    }
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(DYN_PROF_MAX_STARTABLE_TIMES + 1, dynProfSrv->startTimes_);
    EXPECT_EQ(false, dynProfSrv->profHasStarted_.load());
}

TEST_F(DynProfSrvUtest, DynProfSrvProcStartWillReturnWhenProfNotStart)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_.load());
}

TEST_F(DynProfSrvUtest, DynProfSrvProcStartWillFailWhileStartDeviceFail)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    
    MOCKER_CPP(&DynProfServer::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitAclEnv)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::SetModeToOff)
        .stubs();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofHostHandle)
        .stubs();
    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl)
        .stubs()
        .will(returnValue(1));

    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfSrv->SaveDevicesInfo(data);

    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_.load());
    EXPECT_EQ(0, dynProfSrv->startTimes_);
}

TEST_F(DynProfSrvUtest, DynProfSrvProcStop)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->profHasStarted_.store(false);
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
    
    dynProfSrv->profHasStarted_.store(true);
    dynProfSrv->DynProfSrvProcStop();
    EXPECT_EQ(true, dynProfSrv->profHasStarted_.load());
 
    dynProfSrv->DynProfSrvProcStop();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_.load());
}
 
TEST_F(DynProfSrvUtest, DynProfSrvProcQuit)
{
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    dynProfSrv->profHasStarted_.store(true);
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
    EXPECT_EQ(false, dynProfSrv->profHasStarted_.load());
}

TEST_F(DynProfSrvUtest, DynProfSrvWillInsertDeviceIdWhileSetNewDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfSrv->deviceInfos_.size());
    data.deviceId = 1;
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(2, dynProfSrv->deviceInfos_.size()); // 2 : device num
}

TEST_F(DynProfSrvUtest, DynProfSrvWillJustReturnWhileSetRepeatDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfSrv->deviceInfos_.size());
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfSrv->deviceInfos_.size());
}

TEST_F(DynProfSrvUtest, DynProfSrvWillEraseDeviceIdWhileResetExistDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfSrv->deviceInfos_.size());
    data.isOpen = false;
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(0, dynProfSrv->deviceInfos_.size());
}

TEST_F(DynProfSrvUtest, DynProfSrvWillJustReturnWhileResetNoneExistDevice)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = false;
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(0, dynProfSrv->deviceInfos_.size());
}

TEST_F(DynProfSrvUtest, DynProfSrvWillStartDeviceIdWhileSetDeviceAfterProfStart)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    ProfSetDevPara data;
    data.deviceId = 0;
    data.isOpen = true;
    dynProfSrv->profHasStarted_.store(true);
    MOCKER_CPP(&Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(1, dynProfSrv->deviceInfos_.size());
    data.deviceId = 1;
    dynProfSrv->SaveDevicesInfo(data);
    EXPECT_EQ(2, dynProfSrv->deviceInfos_.size()); // 2 : device num
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

TEST_F(DynProfSrvUtest, DynProfServerRsqMsgWillReturnFailWhenInputMsgLongerThanMaxLen)
{
    GlobalMockObject::verify();
    std::string tips(DYN_PROF_RSP_MSG_MAX_LEN, 'x');
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    EXPECT_EQ(PROFILING_FAILED, dynProfSrv->DynProfServerRsqMsg(DynProfMsgType::START_RSP,
        DynProfMsgProcRes::EXE_FAIL, tips));
}

TEST_F(DynProfSrvUtest, DynProfServerRsqMsgWillReturnFailWhenMemCpyFail)
{
    GlobalMockObject::verify();
    std::string tips;
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv;
    dynProfSrv = std::make_shared<DynProfServer>();
    MOCKER(memcpy_s)
        .stubs()
        .will(returnValue(EOK - 1));
    EXPECT_EQ(PROFILING_FAILED, dynProfSrv->DynProfServerRsqMsg(DynProfMsgType::START_RSP,
        DynProfMsgProcRes::EXE_FAIL, tips));
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
    ProfSetDevPara data;
    data.deviceId = 0;
    auto dynProfMgr = DynProfMgr::instance();
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(DAYNAMIC_PROFILING_VALUE));
    MOCKER_CPP(&DynProfServer::DynProfServerCreateSock)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DynProfServer::NotifyClientDisconnet)
        .stubs();
    MOCKER_CPP(&MmUnlink)
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(PROFILING_FAILED, dynProfMgr->StartDynProf());
    EXPECT_EQ(false, dynProfMgr->IsDynProfStarted());
    dynProfMgr->StopDynProf(); // start failed, just return when StopDynProf is called
    dynProfMgr->SaveDevicesInfo(data);

    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    EXPECT_EQ(true, dynProfMgr->IsDynProfStarted());
    // repeat start
    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    dynProfMgr->SaveDevicesInfo(data);
    dynProfMgr->StopDynProf();
    EXPECT_EQ(false, dynProfMgr->IsDynProfStarted()); // success to stop when StopDynProf is called after start succeed
    dynProfMgr->dynProfSrv_ = nullptr;
}

TEST_F(DynProfMgrUtest, StartDynProfThread_WithDelayOrDurationProfilingMode)
{
    GlobalMockObject::verify();
    ProfSetDevPara data;
    data.deviceId = 0;
    auto dynProfMgr = DynProfMgr::instance();
    MOCKER_CPP(&Utils::GetEnvString)
        .stubs()
        .will(returnValue(DELAY_DURARION_PROFILING_VALUE));
    MOCKER_CPP(&DynProfThread::GetDelayAndDurationTime)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, dynProfMgr->StartDynProf());
    dynProfMgr->SaveDevicesInfo(data);
    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    // repeat start
    EXPECT_EQ(PROFILING_SUCCESS, dynProfMgr->StartDynProf());
    EXPECT_EQ(true, dynProfMgr->IsDynProfStarted());
    dynProfMgr->SaveDevicesInfo(data);
    dynProfMgr->StopDynProf();
    EXPECT_EQ(false, dynProfMgr->IsDynProfStarted()); // success to stop when StopDynProf is called after start succeed
    dynProfMgr->dynProfThread_ = nullptr;
}
