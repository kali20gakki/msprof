/**
* @file callback_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <atomic>

#include "activity/activity_manager.h"
#include "activity/ascend/dev_task_manager.h"

#include "mspti.h"

namespace {
std::atomic<uint64_t> g_records{0};
constexpr uint64_t G_MARK_MAX_NUM = 10;

class ActivityUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

void UserBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    printf("========== UserBufferRequest ============\n");
    constexpr uint32_t bufSize = 2 * 1024 * 1024;
    *buffer = static_cast<uint8_t*>(malloc(bufSize));
    *size = bufSize;
    *maxNumRecords = 0;
}

static void ActivityParser(msptiActivity *pRecord)
{
    g_records++;
    if (pRecord->kind == MSPTI_ACTIVITY_KIND_MARKER) {
        msptiActivityMark* activity = reinterpret_cast<msptiActivityMark*>(pRecord);
        if (activity->id == G_MARK_MAX_NUM - 1 && activity->sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_HOST) {
            printf("kind: %d, mode: %d, timestamp: %lu, markId: %lu, processId: %d, threadId: %u, name: %s\n",
                activity->kind, activity->sourceKind, activity->timestamp, activity->id,
                activity->objectId.pt.processId,
                activity->objectId.pt.threadId, activity->name);
        }
    }
}

void UserBufferComplete(uint8_t *buffer, size_t size, size_t validSize)
{
    printf("========== UserBufferComplete ============\n");
    if (validSize > 0) {
        msptiActivity *pRecord = NULL;
        msptiResult status = MSPTI_SUCCESS;
        do {
            status = msptiActivityGetNextRecord(buffer, validSize, &pRecord);
            if (status == MSPTI_SUCCESS) {
                ActivityParser(pRecord);
            } else if (status == MSPTI_ERROR_MAX_LIMIT_REACHED) {
                break;
            }
        } while (1);
    }
    free(buffer);
}

TEST_F(ActivityUtest, ActivityApiUtest)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::GetInstance)
        .stubs()
        .will(returnValue(nullptr));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityRegisterCallbacks(UserBufferRequest, UserBufferComplete));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER));
    msptiActivityMark activity;
    constexpr uint32_t hostId = 64;
    constexpr uint64_t timeStamp = 1614659207688700;
    for (size_t i = 0; i < G_MARK_MAX_NUM ; ++i) {
        activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
        activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
        activity.timestamp = timeStamp;
        activity.id = i;
        activity.objectId.pt.processId = 0;
        activity.objectId.pt.threadId = 0;
        activity.name = "UserMark";
        Mspti::Activity::ActivityManager::GetInstance()->Record(
            reinterpret_cast<msptiActivity*>(&activity), sizeof(activity));
    }
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARKER));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityFlushAll(1));
    EXPECT_EQ(G_MARK_MAX_NUM, g_records.load());
}

TEST_F(ActivityUtest, ActivityManagerTest)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::GetInstance)
        .stubs()
        .will(returnValue(nullptr));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    instance ->SetDevice(0);
    instance ->DeviceReset(0);
    
    EXPECT_EQ(G_MARK_MAX_NUM, g_records.load());
}
}
