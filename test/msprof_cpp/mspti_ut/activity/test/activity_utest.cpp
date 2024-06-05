/**
* @file callback_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/
#include "gtest/gtest.h"

#include <atomic>

#include "activity/activity_manager.h"
#include "mspti.h"

std::atomic<uint64_t> g_records{0};
constexpr uint64_t G_MARK_MAX_NUM = 10;

class ActivityUtest : public testing::Test {
protected:
    virtual void SetUp() {}
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
    if (pRecord->kind == MSPTI_ACTIVITY_KIND_MARK) {
        msptiActivityMark* activity = reinterpret_cast<msptiActivityMark*>(pRecord);
        if (activity->markId == G_MARK_MAX_NUM - 1) {
            printf("kind: %d, mode: %d, timestamp: %lu, markId: %lu, stream: %d, deviceId: %u, name: %s\n",
                activity->kind, activity->mode, activity->timestamp, activity->markId, activity->streamId,
                activity->deviceId, activity->name);
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
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityRegisterCallbacks(UserBufferRequest, UserBufferComplete));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARK));
    msptiActivityMark activity;
    constexpr uint32_t hostId = 64;
    constexpr uint64_t timeStamp = 1614659207688700;
    for (size_t i = 0; i < G_MARK_MAX_NUM ; ++i) {
        activity.kind = MSPTI_ACTIVITY_KIND_MARK;
        activity.mode = 0;
        activity.timestamp = timeStamp;
        activity.markId = i;
        activity.streamId = 0;
        activity.deviceId = hostId;
        activity.name = "UserMark";
        Mspti::Activity::ActivityManager::GetInstance()->Record(
            reinterpret_cast<msptiActivity*>(&activity), sizeof(activity));
    }
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARK));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityFlushAll(1));
    EXPECT_EQ(G_MARK_MAX_NUM, g_records.load());
}
