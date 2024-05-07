/**
* @file prof_drv_event_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "prof_drv_event.h"
#include "errno/error_code.h"
#include "driver_plugin.h"
#include "collection_register.h"
#include "ai_drv_prof_api.h"
#include "mmpa_api.h"

using namespace Analysis::Dvvp::JobWrapper;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
class ProfDrvEventUtest : public testing::Test {
protected:
    virtual void SetUp()
    {}
    virtual void TearDown()
    {}
};

TEST_F(ProfDrvEventUtest, TestSubscribeEventThreadInit)
{
    GlobalMockObject::verify();
    TaskEventAttr eventAttr {0, 0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false, false, false, 0};
    uint32_t deviceId = 0;
    ProfDrvEvent profDrvEvent;
    MOCKER(&ProfDrvEvent::CreateWaitEventThread)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.SubscribeEventThreadInit(deviceId, &eventAttr, "aicpu"));
    EXPECT_EQ(PROFILING_SUCCESS, profDrvEvent.SubscribeEventThreadInit(deviceId, &eventAttr, "aicpu"));
}

TEST_F(ProfDrvEventUtest, TestCreateWaitEventThread)
{
    GlobalMockObject::verify();
    TaskEventAttr eventAttr {0, 0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false, false, false, 0};
    ProfDrvEvent profDrvEvent;
    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.CreateWaitEventThread(&eventAttr));
    EXPECT_EQ(PROFILING_SUCCESS, profDrvEvent.CreateWaitEventThread(&eventAttr));
}

TEST_F(ProfDrvEventUtest, TestHandleEvent)
{
    GlobalMockObject::verify();
    TaskEventAttr eventAttr {0, 0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false, false, false, 0};
    drvError_t err = DRV_ERROR_NONE;
    ProfDrvEvent profDrvEvent;
    struct event_info event;
    event.comm.event_id = EVENT_USR_START;
    bool onceFlag = true;
    MOCKER(&CollectionRegisterMgr::CollectionJobRun)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.HandleEvent(err, event, &eventAttr, onceFlag));
    err = DRV_ERROR_WAIT_TIMEOUT;
    MOCKER_CPP(&DrvChannelsMgr::GetAllChannels)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, profDrvEvent.HandleEvent(err, event, &eventAttr, onceFlag));
    err = DRV_ERROR_NO_DEVICE;
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.HandleEvent(err, event, &eventAttr, onceFlag));
}

TEST_F(ProfDrvEventUtest, TestWaitEventThread)
{
    GlobalMockObject::verify();
    ProfDrvEvent profDrvEvent;
    EXPECT_EQ(nullptr, profDrvEvent.WaitEventThread(nullptr));
    TaskEventAttr eventAttr {0, 0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false, false, false, 0};
    MOCKER_CPP(&ProfDrvEvent::HandleEvent)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(nullptr, profDrvEvent.WaitEventThread((void*)(&eventAttr)));
}

TEST_F(ProfDrvEventUtest, TestSubscribeEventThreadUninit)
{
    GlobalMockObject::verify();
    uint32_t devId = 0;
    ProfDrvEvent profDrvEvent;
    profDrvEvent.SubscribeEventThreadUninit(devId);
}
