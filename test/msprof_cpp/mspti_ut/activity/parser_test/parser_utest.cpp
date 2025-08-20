/**
* @file channel_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "activity/ascend/parser/parser_manager.h"
#include "activity/ascend/parser/cann_hash_cache.h"
#include "activity/ascend/parser/mstx_parser.h"
#include "activity/ascend/parser/device_task_calculator.h"
#include "activity/ascend/parser/kernel_parser.h"
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
    auto hashId = Mspti::Parser::CannHashCache::GetInstance().GenHashId(hashInfo);
    EXPECT_STREQ(hashInfo.c_str(), Mspti::Parser::CannHashCache::GetInstance().GetHashInfo(hashId).c_str());
    constexpr uint16_t level = 20000;
    constexpr uint32_t typeId = 1;
    const std::string typeName = "acl_api";
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
    auto& instance = Mspti::Parser::KernelParser::GetInstance();
    constexpr uint16_t flipId = 0;
    constexpr uint16_t taskId = 1;
    constexpr uint32_t deviceId = 0;
    constexpr uint32_t streamId = 3;
    constexpr uint32_t BIT_NUM = 16;
    MsprofCompactInfo compactInfo;
    (void)memset_s(&compactInfo, sizeof(compactInfo), 0, sizeof(compactInfo));
    compactInfo.data.runtimeTrack.deviceId = deviceId;
    compactInfo.data.runtimeTrack.streamId = streamId;
    compactInfo.data.runtimeTrack.taskInfo = static_cast<uint32_t>(flipId) << BIT_NUM | static_cast<uint32_t>(taskId);
    compactInfo.data.runtimeTrack.taskType = TS_TASK_TYPE_KERNEL_AIVEC;
    EXPECT_EQ(MSPTI_SUCCESS, instance.ReportRtTaskTrack(1, &compactInfo));
    EXPECT_EQ(MSPTI_SUCCESS, instance.ReportRtTaskTrack(1, &compactInfo));

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

    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Parser::DeviceTaskCalculator::GetInstance().ReportStarsSocLog(deviceId,
        reinterpret_cast<StarsSocHeader*>(&socLogStart)));
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Parser::DeviceTaskCalculator::GetInstance().ReportStarsSocLog(deviceId,
        reinterpret_cast<StarsSocHeader*>(&socLogEnd)));
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Parser::DeviceTaskCalculator::GetInstance().ReportStarsSocLog(deviceId,
        reinterpret_cast<StarsSocHeader*>(&socLogStart)));
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Parser::DeviceTaskCalculator::GetInstance().ReportStarsSocLog(deviceId,
        reinterpret_cast<StarsSocHeader*>(&socLogEnd)));
}

TEST_F(ParserUtest, ShouldRetSccessWhenReportMstxData)
{
    GlobalMockObject::verify();
    MOCKER_CPP(rtProfilerTraceEx)
        .stubs()
        .will(returnValue(static_cast<RtErrorT>(MSPTI_SUCCESS)));
    auto instance = Mspti::Parser::MstxParser::GetInstance();
    const char* message = "UserMark";
    RtStreamT stream = (void*)0x1234567;
    const char* domain = "UserDomain";
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMark(message, stream, domain));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMark(message, nullptr, domain));
    uint64_t markId;
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportRangeStartA(message, stream, markId, domain));
    EXPECT_EQ(3UL, markId);
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportRangeEnd(markId));
}

TEST_F(ParserUtest, ShouldRetErrorWhenTryCacheMarkmsgFailed)
{
    GlobalMockObject::verify();
    const std::string* nullPtr = nullptr;
    MOCKER_CPP(&Mspti::Parser::MstxParser::TryCacheMarkMsg)
        .stubs()
        .will(returnValue(nullPtr));
    const char* message = "UserMark";
    const char* domain = "UserDomain";
    auto instance = Mspti::Parser::MstxParser::GetInstance();
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->ReportMark(message, nullptr, domain));
    uint64_t markId;
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->ReportRangeStartA(message, nullptr, markId, domain));
}

TEST_F(ParserUtest, ShouldRecordKernelNameWhenReportRtTaskTrack)
{
    auto& instance = Mspti::Parser::KernelParser::GetInstance();
    constexpr uint16_t flipId = 0;
    constexpr uint16_t taskId = 1;
    constexpr uint32_t deviceId = 0;
    constexpr uint32_t streamId = 3;
    constexpr uint32_t BIT_NUM = 16;
    const std::string kernelName = "test_kernalName";
    auto kernelNameHash = Mspti::Parser::CannHashCache::GetInstance().GenHashId(kernelName);
    MsprofCompactInfo compactInfo;
    (void)memset_s(&compactInfo, sizeof(compactInfo), 0, sizeof(compactInfo));
    compactInfo.data.runtimeTrack.deviceId = deviceId;
    compactInfo.data.runtimeTrack.streamId = streamId;
    compactInfo.data.runtimeTrack.taskInfo = static_cast<uint32_t>(flipId) << BIT_NUM | static_cast<uint32_t>(taskId);
    compactInfo.data.runtimeTrack.taskType = TS_TASK_TYPE_KERNEL_AIVEC;
    compactInfo.data.runtimeTrack.kernelName = kernelNameHash;
    EXPECT_EQ(MSPTI_SUCCESS, instance.ReportRtTaskTrack(1, &compactInfo));
}

TEST_F(ParserUtest, ShouldRetErrorWhenMarkFail)
{
    GlobalMockObject::verify();
    std::shared_ptr<std::string> nullPtr{nullptr};
    MOCKER_CPP(rtProfilerTraceEx)
        .stubs()
        .will(returnValue(static_cast<RtErrorT>(MSPTI_ERROR_INNER)));
    const char* message = "UserMark";
    uint64_t markId = 0;
    RtStreamT stream = (void*)0x1234567;
    auto instance = Mspti::Parser::MstxParser::GetInstance();
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->InnerDeviceStartA(message, stream, markId));
    EXPECT_EQ(MSPTI_SUCCESS, instance->InnerDeviceEndA(markId));
}

TEST_F(ParserUtest, ShouldRetErrorWhenStreamNull)
{
    GlobalMockObject::verify();
    std::shared_ptr<std::string> nullPtr{nullptr};
    const char* message = "UserMark";
    uint64_t markId = 0;
    RtStreamT stream = (void*)0x1234567;
    auto instance = Mspti::Parser::MstxParser::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->InnerDeviceStartA(message, stream, markId));
    MOCKER_CPP(rtProfilerTraceEx)
    .stubs()
    .will(returnValue(static_cast<RtErrorT>(MSPTI_ERROR_INNER)));
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->InnerDeviceEndA(markId));
}

TEST_F(ParserUtest, ShouldRetSuccessWhenInnerMark)
{
    GlobalMockObject::verify();
    std::shared_ptr<std::string> nullPtr{nullptr};
    MOCKER_CPP(rtProfilerTraceEx)
        .stubs()
        .will(returnValue(static_cast<RtErrorT>(MSPTI_SUCCESS)));
    const char* message = "InnerMark";
    RtStreamT stream = (void*)0x1234567;
    uint64_t markId = 0;
    auto instance = Mspti::Parser::MstxParser::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->InnerDeviceStartA(message, stream, markId));
    EXPECT_EQ(MSPTI_SUCCESS, instance->InnerDeviceEndA(markId));
    MOCKER_CPP(rtProfilerTraceEx)
    .stubs()
    .will(returnValue(static_cast<RtErrorT>(MSPTI_ERROR_INNER)));
    uint64_t modelId = 0;
    uint64_t timestamp = 100;
    uint16_t streamId = 1;
    StepTrace* stepTrace = new StepTrace();
    stepTrace->timestamp = timestamp;
    stepTrace->indexId = markId;
    stepTrace->modelId = modelId;
    stepTrace->streamId = streamId;
    stepTrace->tagId = STEP_TRACE_TAG_MARKEX;
    Mspti::Parser::ParserManager::GetInstance()->ReportStepTrace(0, stepTrace);
}
}
