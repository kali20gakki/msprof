/**
* @file activity_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <atomic>
#include "securec.h"
#include "csrc/activity/activity_manager.h"
#include "csrc/activity/ascend/dev_task_manager.h"
#include "csrc/activity/ascend/parser/parser_manager.h"
#include "csrc/activity/ascend/reporter/external_correlation_reporter.h"

#include "mspti.h"

namespace {
std::atomic<uint64_t> g_records{0};

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

void UserLittleBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    printf("========== UserBufferRequest ============\n");
    constexpr uint32_t bufSize = 1024;
    *buffer = static_cast<uint8_t*>(malloc(bufSize));
    *size = bufSize;
    *maxNumRecords = 0;
}

static void ActivityParser(msptiActivity *pRecord)
{
    g_records++;
    if (pRecord->kind == MSPTI_ACTIVITY_KIND_MARKER) {
        msptiActivityMarker* activity = reinterpret_cast<msptiActivityMarker*>(pRecord);
        if (activity->sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_HOST) {
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

void TestActivityApi()
{
    constexpr uint64_t timeStamp = 1614659207688700;
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_API));
    msptiActivityApi api;
    api.kind = MSPTI_ACTIVITY_KIND_API;
    api.start = timeStamp;
    api.end = timeStamp;
    api.pt.processId = 0;
    api.pt.threadId = 0;
    api.correlationId = 1;
    api.name = "Api";
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&api), sizeof(api));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_API));
}

void TestActivityKernel()
{
    constexpr uint64_t timeStamp = 1614659207688700;
    constexpr uint32_t streamId = 3;
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_KERNEL));
    msptiActivityKernel kernel;
    kernel.kind = MSPTI_ACTIVITY_KIND_KERNEL;
    kernel.start = timeStamp;
    kernel.end = timeStamp;
    kernel.ds.deviceId = 0;
    kernel.ds.streamId = streamId;
    kernel.correlationId = 1;
    kernel.type = "KERNEL_AIVEC";
    kernel.name = "Kernel";
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&kernel), sizeof(kernel));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_KERNEL));
}

TEST_F(ActivityUtest, ShouldRetSuccessWhenSetAllKindWithCorrectApiInvocationSequence)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityRegisterCallbacks(UserBufferRequest, UserBufferComplete));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance ->SetDevice(0));
    EXPECT_EQ(MSPTI_SUCCESS, instance ->ResetAllDevice());
    msptiActivityMarker activity;
    constexpr uint64_t timeStamp = 1614659207688700;
    constexpr uint32_t markNum = 10;
    uint64_t totalActivitys = 0;
    for (size_t i = 0; i < markNum ; ++i) {
        activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
        activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
        activity.timestamp = timeStamp;
        activity.id = i;
        activity.objectId.pt.processId = 0;
        activity.objectId.pt.threadId = 0;
        activity.name = "UserMark";
        instance->Record(reinterpret_cast<msptiActivity*>(&activity), sizeof(activity));
        totalActivitys += 1;
    }
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARKER));
    TestActivityApi();
    totalActivitys += 1;
    TestActivityKernel();
    totalActivitys += 1;
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityFlushAll(1));
    EXPECT_EQ(totalActivitys, g_records.load());
}

TEST_F(ActivityUtest, ShouldRetInvalidParameterErrorWhenSetWrongParam)
{
    EXPECT_EQ(MSPTI_ERROR_INVALID_PARAMETER, msptiActivityRegisterCallbacks(nullptr, nullptr));
    EXPECT_EQ(MSPTI_ERROR_INVALID_PARAMETER, msptiActivityEnable(MSPTI_ACTIVITY_KIND_FORCE_INT));
    EXPECT_EQ(MSPTI_ERROR_INVALID_PARAMETER, msptiActivityDisable(MSPTI_ACTIVITY_KIND_FORCE_INT));
    msptiActivity* activity;
    EXPECT_EQ(MSPTI_ERROR_INVALID_PARAMETER, msptiActivityGetNextRecord(nullptr, 0, &activity));
}

TEST_F(ActivityUtest, IsActivityKindEnableWillReturnTrueWhenEnableMarkerKind)
{
    msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER);
    EXPECT_EQ(true, Mspti::Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER));
    msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARKER);
}

TEST_F(ActivityUtest, IsActivityKindEnableWillReturnFalseWhenNotEnableMarkerKind)
{
    msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARKER);
    EXPECT_EQ(false, Mspti::Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER));
}

TEST_F(ActivityUtest, ShouldRetSuccessWhenSetPeriodFlushTime)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityRegisterCallbacks(UserLittleBufferRequest, UserBufferComplete));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance ->SetDevice(0));

    msptiActivityMarker activity;
    constexpr uint64_t timeStamp = 1614659207688700;
    constexpr uint32_t markNum = 10;
    constexpr uint32_t testPeriodFlushTime = 1;
    constexpr uint32_t sleepTime = 100000;
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityFlushPeriod(testPeriodFlushTime));
    for (size_t i = 0; i < markNum ; ++i) {
        usleep(sleepTime);
        activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
        activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
        activity.timestamp = timeStamp;
        activity.id = i;
        activity.objectId.pt.processId = 0;
        activity.objectId.pt.threadId = 0;
        activity.name = "UserMark";
        instance->Record(reinterpret_cast<msptiActivity*>(&activity), sizeof(activity));
    }
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityFlushPeriod(0));
        for (size_t i = 0; i < markNum ; ++i) {
        usleep(sleepTime);
        activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
        activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
        activity.timestamp = timeStamp;
        activity.id = i + markNum;
        activity.objectId.pt.processId = 0;
        activity.objectId.pt.threadId = 0;
        activity.name = "UserMark";
        instance->Record(reinterpret_cast<msptiActivity*>(&activity), sizeof(activity));
    }
    EXPECT_EQ(MSPTI_SUCCESS, instance ->ResetAllDevice());
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARKER));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityFlushAll(1));
}

TEST_F(ActivityUtest, ShouldRetSuccessWhenPushAndPopExternalCorrelationId)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask).stubs().will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask).stubs().will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityRegisterCallbacks(UserBufferRequest, UserBufferComplete));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->SetDevice(0));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityPushExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, 0));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityPushExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, 1));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityPushExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_UNKNOWN, 1));

    EXPECT_EQ(
        MSPTI_SUCCESS, Mspti::Reporter::ExternalCorrelationReporter::GetInstance()->ReportExternalCorrelationId(1));

    uint64_t value = 12345678901234567890; // 一个具体的uint64_t类型的变量
    uint64_t *test = &value;
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityPopExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, test));
    EXPECT_EQ(1, *test);

    EXPECT_EQ(
        MSPTI_SUCCESS, Mspti::Reporter::ExternalCorrelationReporter::GetInstance()->ReportExternalCorrelationId(1));

    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityPopExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_UNKNOWN, test));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityPopExternalCorrelationId(MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, test));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ResetAllDevice());
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARKER));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityFlushAll(1));
}

TEST_F(ActivityUtest, GetRecordSuccessWhenBufferNull)
{
    size_t validSize = sizeof(msptiActivityMarker);
    uint8_t* buffer = nullptr;
    msptiActivity *pRecord = NULL;
    EXPECT_EQ(MSPTI_ERROR_INVALID_PARAMETER, msptiActivityGetNextRecord(buffer, validSize, &pRecord));

    buffer = static_cast<uint8_t*>(malloc(validSize));
    msptiActivityMarker* activity = new msptiActivityMarker();
    activity->kind = MSPTI_ACTIVITY_KIND_MARKER;
    memcpy_s(buffer, validSize, activity, sizeof(msptiActivityMarker));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityGetNextRecord(buffer, validSize, &pRecord));

    validSize = 1;
    EXPECT_EQ(MSPTI_ERROR_MAX_LIMIT_REACHED, msptiActivityGetNextRecord(buffer, validSize, &pRecord));
    free(buffer);
}
}
