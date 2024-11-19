/**
* @file channel_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/
#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "activity/ascend/parser/hccl_reporter.h"
#include "activity/ascend/entity/hccl_op_desc.h"
#include "common/inject/hccl_inject.h"
#include "common/utils.h"
#include "securec.h"

namespace {
class HcclReporterUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(HcclReporterUtest, ShouldRetSuccessWhenRecordUnknowMarker)
{
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    uint64_t markId = 1;
    auto instance = Mspti::Parser::HcclReporter::GetInstance();
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->RecordHcclMarker(nullptr));
    msptiActivityMarker* startMarkActivity = new msptiActivityMarker();
    startMarkActivity->timestamp = startTime;
    startMarkActivity->objectId.ds.deviceId = 1;
    startMarkActivity->objectId.ds.streamId = 1;
    startMarkActivity->flag = MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE;
    startMarkActivity->id = 1;

    msptiActivityMarker* endMarkActivity = new msptiActivityMarker();
    endMarkActivity->timestamp = endTime;
    endMarkActivity->objectId.ds.deviceId = 1;
    endMarkActivity->objectId.ds.streamId = 1;
    endMarkActivity->flag = MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE;
    endMarkActivity->id = 1;

    EXPECT_EQ(MSPTI_ERROR_INNER, instance->RecordHcclMarker(startMarkActivity));
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->RecordHcclMarker(endMarkActivity));
}

TEST_F(HcclReporterUtest, ShouldRetSuccessWhenRecordHcclMarker)
{
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    auto instance = Mspti::Parser::HcclReporter::GetInstance();
    EXPECT_EQ(MSPTI_ERROR_INNER, instance->RecordHcclMarker(nullptr));
    uint64_t markId = 1;
    std::shared_ptr<HcclOpDesc> hcclOpDesc = std::make_shared<HcclOpDesc>();
    hcclOpDesc->opName = "hcclAllReduce";
    hcclOpDesc->streamId = 1;
    hcclOpDesc->deviceId = 1;
    EXPECT_EQ(MSPTI_SUCCESS, instance->RecordHcclOp(markId, hcclOpDesc));

    msptiActivityMarker* startMarkActivity = new msptiActivityMarker();
    startMarkActivity->timestamp = startTime;
    startMarkActivity->objectId.ds.deviceId = 1;
    startMarkActivity->objectId.ds.streamId = 1;
    startMarkActivity->flag = MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE;
    startMarkActivity->id = 1;

    msptiActivityMarker* endMarkActivity = new msptiActivityMarker();
    endMarkActivity->timestamp = endTime;
    endMarkActivity->objectId.ds.deviceId = 1;
    endMarkActivity->objectId.ds.streamId = 1;
    endMarkActivity->flag = MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE;
    endMarkActivity->id = 1;

    EXPECT_EQ(MSPTI_SUCCESS, instance->RecordHcclMarker(startMarkActivity));
    EXPECT_EQ(MSPTI_SUCCESS, instance->RecordHcclMarker(endMarkActivity));
}

TEST_F(HcclReporterUtest, ShouldRetSuccessWhenRecordHcclOp)
{
    uint64_t markId = 1;
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    auto instance = Mspti::Parser::HcclReporter::GetInstance();
    std::shared_ptr<HcclOpDesc> hcclOpDesc = std::make_shared<HcclOpDesc>();
    hcclOpDesc->opName = "hcclAllReduce";
    hcclOpDesc->streamId = 1;
    hcclOpDesc->deviceId = 1;
    hcclOpDesc->bandWidth = 0.1f;
    hcclOpDesc->start = startTime;
    hcclOpDesc->end = endTime;
    EXPECT_EQ(MSPTI_SUCCESS, instance->RecordHcclOp(markId, hcclOpDesc));
}

TEST_F(HcclReporterUtest, ShouldRetSuccessWhenReportHcclActivity)
{
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    auto instance = Mspti::Parser::HcclReporter::GetInstance();
    std::shared_ptr<HcclOpDesc> hcclOpDesc = std::make_shared<HcclOpDesc>();
    hcclOpDesc->opName = "hcclAllReduce";
    hcclOpDesc->streamId = 1;
    hcclOpDesc->deviceId = 1;
    hcclOpDesc->bandWidth = 0.1f;
    hcclOpDesc->start = startTime;
    hcclOpDesc->end = endTime;
    EXPECT_EQ(MSPTI_SUCCESS, instance->ReportHcclActivity(hcclOpDesc));
}
}