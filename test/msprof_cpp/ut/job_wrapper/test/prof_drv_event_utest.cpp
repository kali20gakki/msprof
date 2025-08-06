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
    TaskEventAttr eventAttr {0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false,
                             false, false, 0, false, false, ""};
    ProfDrvEvent profDrvEvent;
    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.SubscribeEventThreadInit(&eventAttr));
    EXPECT_EQ(PROFILING_SUCCESS, profDrvEvent.SubscribeEventThreadInit(&eventAttr));
}

TEST_F(ProfDrvEventUtest, TestEventThreadHandle)
{
    GlobalMockObject::verify();
    TaskEventAttr eventAttr {0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false,
                             false, false, 0, false, false, ""};
    ProfDrvEvent profDrvEvent;

    EXPECT_EQ(nullptr, profDrvEvent.EventThreadHandle(nullptr));

    MOCKER_CPP(&ProfDrvEvent::QueryDevPid)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(nullptr, profDrvEvent.EventThreadHandle((void*)&eventAttr));

    MOCKER_CPP(&DriverPlugin::MsprofHalEschedAttachDevice)
        .stubs()
        .will(returnValue(DRV_ERROR_INVALID_HANDLE))
        .then(returnValue(DRV_ERROR_NONE));
    EXPECT_EQ(nullptr, profDrvEvent.EventThreadHandle((void*)&eventAttr));

    MOCKER_CPP(&ProfDrvEvent::QueryGroupId)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    EXPECT_EQ(nullptr, profDrvEvent.EventThreadHandle((void*)&eventAttr));

    MOCKER_CPP(&DriverPlugin::MsprofHalEschedCreateGrpEx)
        .stubs()
        .will(returnValue(DRV_ERROR_INVALID_HANDLE))
        .then(returnValue(DRV_ERROR_NONE));
    EXPECT_EQ(nullptr, profDrvEvent.EventThreadHandle((void*)&eventAttr));

    MOCKER_CPP(&DriverPlugin::MsprofHalEschedSubscribeEvent)
        .stubs()
        .will(returnValue(DRV_ERROR_INVALID_HANDLE))
        .then(returnValue(DRV_ERROR_NONE));
    EXPECT_EQ(nullptr, profDrvEvent.EventThreadHandle((void*)&eventAttr));

    EXPECT_EQ(nullptr, profDrvEvent.EventThreadHandle((void*)&eventAttr));
}

TEST_F(ProfDrvEventUtest, TestQueryGroupId)
{
    GlobalMockObject::verify();
    uint32_t devId = 0;
    uint32_t grpId = 0;
    ProfDrvEvent profDrvEvent;

    MOCKER(strcpy_s)
        .stubs()
        .will(returnValue(EOK - 1))
        .then(returnValue(EOK));
    MOCKER_CPP(&DriverPlugin::MsprofHalEschedQueryInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_INVALID_HANDLE));
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.QueryGroupId(devId, grpId, "aicpu"));
    EXPECT_EQ(PROFILING_SUCCESS, profDrvEvent.QueryGroupId(devId, grpId, "aicpu"));
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.QueryGroupId(devId, grpId, "aicpu"));
}

TEST_F(ProfDrvEventUtest, TestQueryDevPid)
{
    GlobalMockObject::verify();
    const int32_t WAIT_COUNT = 80;
    TaskEventAttr eventAttr {0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false,
                             false, false, 0, false, false, ""};
    ProfDrvEvent profDrvEvent;

    eventAttr.channelId = PROF_CHANNEL_MAX; // not aicpu channel
    EXPECT_EQ(PROFILING_SUCCESS, profDrvEvent.QueryDevPid(&eventAttr));
 
    eventAttr.channelId = PROF_CHANNEL_AICPU;
    MOCKER_CPP(&DriverPlugin::MsprofHalQueryDevpid)
        .stubs()
        .will(repeat(DRV_ERROR_INVALID_HANDLE, WAIT_COUNT))
        .then(returnValue(DRV_ERROR_NONE));
    EXPECT_EQ(PROFILING_FAILED, profDrvEvent.QueryDevPid(&eventAttr));
    EXPECT_EQ(PROFILING_SUCCESS, profDrvEvent.QueryDevPid(&eventAttr));
}

TEST_F(ProfDrvEventUtest, TestWaitEvent)
{
    GlobalMockObject::verify();
    TaskEventAttr eventAttr {0, PROF_CHANNEL_AICPU, AICPU_COLLECTION_JOB, false, false,
                             false, false, 0, false, false, ""};
    ProfDrvEvent profDrvEvent;
    uint32_t grpId = 0;

    MOCKER_CPP(&DriverPlugin::MsprofHalEschedWaitEvent)
        .stubs()
        .will(returnValue(DRV_ERROR_NONE))
        .then(returnValue(DRV_ERROR_SCHED_WAIT_TIMEOUT))
        .then(returnValue(DRV_ERROR_INVALID_HANDLE));
    profDrvEvent.WaitEvent(&eventAttr, grpId);

    MOCKER_CPP(&DrvChannelsMgr::GetAllChannels)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    profDrvEvent.WaitEvent(&eventAttr, grpId);

    profDrvEvent.WaitEvent(&eventAttr, grpId);
}

TEST_F(ProfDrvEventUtest, TestSubscribeEventThreadUninit)
{
    GlobalMockObject::verify();
    uint32_t devId = 0;
    ProfDrvEvent profDrvEvent;
    profDrvEvent.SubscribeEventThreadUninit(devId);
}
