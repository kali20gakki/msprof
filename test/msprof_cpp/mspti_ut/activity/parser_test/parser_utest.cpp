/**
* @file channel_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "activity/ascend/parser/parser_manager.h"
#include "common/inject/runtime_inject.h"
#include "common/utils.h"
#include "securec.h"

namespace {
class ParserUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(ParserUtest, ShouldRetSuccessWhenReportApiSuccess)
{
    auto instance = Mspti::Parser::ParserManager::GetInstance();
    const std::string hashInfo = "aclnnAdd_AxpyAiCore_Axpy";
    auto hashId = instance->GenHashId(hashInfo);
    EXPECT_STREQ(hashInfo.c_str(), instance->GetHashInfo(hashId).c_str());
    constexpr uint16_t level = 20000;
    constexpr uint32_t typeId = 1;
    const std::string typeName = "acl_api";
    instance->RegReportTypeInfo(level, typeId, typeName);
    EXPECT_STREQ(typeName.c_str(), instance->GetTypeName(level, typeId).c_str());
    MsprofApi data;
    (void)memset_s(&data, sizeof(data), 0, sizeof(data));
    data.level = level;
    data.type = 1;
    data.threadId = Mspti::Common::Utils::GetPid();
    data.beginTime = Mspti::Common::Utils::GetClockRealTimeNs();
    data.endTime = data.beginTime;
    data.itemId = hashId;
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportApi(&data));
}

TEST_F(ParserUtest, ShouldRetSccessWhenReportKernelInfo)
{
    auto instance = Mspti::Parser::ParserManager::GetInstance();
    constexpr uint16_t flipId = 0;
    constexpr uint16_t taskId = 1;
    constexpr uint32_t deviceId = 0;
    constexpr uint32_t streamId = 3;
    constexpr uint32_t BIT_NUM = 16;
    MsprofRuntimeTrack data;
    (void)memset_s(&data, sizeof(data), 0, sizeof(data));
    data.deviceId = deviceId;
    data.streamId = streamId;
    data.taskInfo = static_cast<uint32_t>(flipId) << BIT_NUM | static_cast<uint32_t>(taskId);
    data.taskType = TS_TASK_TYPE_KERNEL_AIVEC;
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportRtTaskTrack(data));

    StarsSocLog socLogStart;
    (void)memset_s(&socLogStart, sizeof(socLogStart), 0, sizeof(socLogStart));
    socLogStart.funcType = STARS_FUNC_TYPE_BEGIN;
    socLogStart.streamId = streamId;
    socLogStart.taskId = taskId;

    StarsSocLog socLogEnd;
    (void)memset_s(&socLogEnd, sizeof(socLogEnd), 0, sizeof(socLogEnd));
    socLogEnd.funcType = STARS_FUNC_TYPE_END;
    socLogEnd.streamId = streamId;
    socLogEnd.taskId = taskId;

    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportStarsSocLog(deviceId, &socLogStart));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportStarsSocLog(deviceId, &socLogEnd));

    TaskFlipInfo flipData;
    (void)memset_s(&flipData, sizeof(flipData), 0, sizeof(flipData));
    flipData.streamId = streamId;
    flipData.flipId = flipId;
    instance->ReportFlipInfo(deviceId, &flipData);
}

TEST_F(ParserUtest, ShouldRetSccessWhenReportMstxData)
{
    GlobalMockObject::verify();
    MOCKER_CPP(rtProfilerTraceEx)
        .stubs()
        .will(returnValue(static_cast<RtErrorT>(MSPTI_SUCCESS)));
    auto instance = Mspti::Parser::ParserManager::GetInstance();
    const char* message = "UserMark";
    RtStreamT stream = (void*)0x1234567;
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMark(message, stream));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMark(message, nullptr));
    uint64_t markId;
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportRangeStartA(message, stream, markId));
    EXPECT_EQ(2UL, markId);
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportRangeEnd(markId));
}

TEST_F(ParserUtest, ShouldRetErrorWhenTryCacheMarkmsgFailed)
{
    GlobalMockObject::verify();
    std::shared_ptr<std::string> nullPtr{nullptr};
    MOCKER_CPP(&Mspti::Parser::ParserManager::TryCacheMarkMsg)
        .stubs()
        .will(returnValue(nullPtr));
    const char* message = "UserMark";
    auto instance = Mspti::Parser::ParserManager::GetInstance();
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->ReportMark(message, nullptr));
    uint64_t markId;
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->ReportRangeStartA(message, nullptr, markId));
}
}
