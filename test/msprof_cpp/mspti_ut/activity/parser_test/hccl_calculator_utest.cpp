/**
* @file channel_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "activity/ascend/parser/hccl_calculator.h"
#include "activity/ascend/entity/hccl_op_desc.h"
#include "common/inject/hccl_inject.h"
#include "common/utils.h"
#include "securec.h"

namespace {
class HcclCalculatorUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(HcclCalculatorUtest, P2pOpDescShouldRetSuccessAndFailWhenBandWidth)
{
    HcclDataType dataType = HCCL_DATA_TYPE_INT8;
    uint64_t dataTypeSize = 1;
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    uint64_t count = 1;

    P2pOpDesc* hcclOpDesc = new P2pOpDesc();
    hcclOpDesc->opName = "HcclAllReduce";
    hcclOpDesc->streamId = 1;
    hcclOpDesc->deviceId = 1;
    hcclOpDesc->end = endTime;
    hcclOpDesc->start = startTime;
    hcclOpDesc->count = count;
    hcclOpDesc->dataType = dataType;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclOpDesc);
    hcclOpDesc->start = endTime;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclOpDesc);
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)nullptr);

    double expectBandWidth = (double)(dataTypeSize * count) / (endTime - startTime);
    EXPECT_EQ(expectBandWidth, hcclOpDesc->bandWidth);

    P2pOpDesc* unknowHcclOpDesc = new P2pOpDesc();
    unknowHcclOpDesc->opName = "HcclAllReduce";
    unknowHcclOpDesc->streamId = 1;
    unknowHcclOpDesc->deviceId = 1;
    unknowHcclOpDesc->end = endTime;
    unknowHcclOpDesc->start = startTime;
    unknowHcclOpDesc->count = count;
    unknowHcclOpDesc->dataType = HCCL_DATA_TYPE_RESERVED;
    
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclOpDesc);
    unknowHcclOpDesc->bandWidth = -1;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)unknowHcclOpDesc);
    EXPECT_EQ(-1, unknowHcclOpDesc->bandWidth);
}

TEST_F(HcclCalculatorUtest, CollectiveOpDescShouldRetSuccessAndFailWhenBandWidth)
{
    HcclDataType dataType = HCCL_DATA_TYPE_INT8;
    uint64_t dataTypeSize = 1;
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    uint64_t count = 1;
    uint64_t rankSize = 2;

    CollectiveOpDesc* hcclOpDesc = new CollectiveOpDesc();
    hcclOpDesc->opName = "HcclReduceScatter";
    hcclOpDesc->streamId = 1;
    hcclOpDesc->deviceId = 1;
    hcclOpDesc->end = endTime;
    hcclOpDesc->start = startTime;
    hcclOpDesc->count = count;
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->rankSize = rankSize;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclOpDesc);
    double expectBandWidth = (double)(dataTypeSize * count * rankSize) / (endTime - startTime);
    EXPECT_EQ(expectBandWidth, hcclOpDesc->bandWidth);

    CollectiveOpDesc* unknowHcclOpDesc = new CollectiveOpDesc();
    unknowHcclOpDesc->opName = "HcclReduceScatter";
    unknowHcclOpDesc->streamId = 1;
    unknowHcclOpDesc->deviceId = 1;
    unknowHcclOpDesc->end = endTime;
    unknowHcclOpDesc->start = startTime;
    unknowHcclOpDesc->count = count;
    unknowHcclOpDesc->dataType = HCCL_DATA_TYPE_RESERVED;
    unknowHcclOpDesc->rankSize = rankSize;
    unknowHcclOpDesc->bandWidth = -1;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)unknowHcclOpDesc);
    EXPECT_EQ(-1, unknowHcclOpDesc->bandWidth);
}

TEST_F(HcclCalculatorUtest, All2AllVOpDescShouldRetSuccessAndFailWhenBandWidth)
{
    HcclDataType dataType = HCCL_DATA_TYPE_INT8;
    uint64_t dataTypeSize = 1;
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    uint64_t sendCounts[2] = {1, 2};
    uint64_t recvCounts[2] = {1, 2};
    uint64_t rankSize = 2;

    All2AllVOpDesc* hcclOpDesc = new All2AllVOpDesc();
    hcclOpDesc->opName = "HcclAlltoAllV";
    hcclOpDesc->streamId = 1;
    hcclOpDesc->deviceId = 1;
    hcclOpDesc->end = endTime;
    hcclOpDesc->start = startTime;
    hcclOpDesc->rankSize = rankSize;
    hcclOpDesc->sendCounts = sendCounts;
    hcclOpDesc->sendType = dataType;
    hcclOpDesc->recvCounts = recvCounts;
    hcclOpDesc->recvType = dataType;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclOpDesc);
    
    double expectBandWidth = (double)(3 * dataTypeSize) / (endTime - startTime);
    EXPECT_EQ(expectBandWidth, hcclOpDesc->bandWidth);

    All2AllVOpDesc* hcclTypeNull = new All2AllVOpDesc();
    hcclTypeNull->opName = "HcclAlltoAllV";
    hcclTypeNull->streamId = 1;
    hcclTypeNull->deviceId = 1;
    hcclTypeNull->end = endTime;
    hcclTypeNull->start = startTime;
    hcclTypeNull->rankSize = rankSize;
    hcclTypeNull->sendCounts = sendCounts;
    hcclTypeNull->recvCounts = recvCounts;
    hcclTypeNull->sendType = HCCL_DATA_TYPE_RESERVED;
    hcclTypeNull->recvType = HCCL_DATA_TYPE_RESERVED;
    hcclTypeNull->bandWidth = -1;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclTypeNull);
    EXPECT_EQ(-1, hcclTypeNull->bandWidth);

    hcclTypeNull->sendType = dataType;
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclTypeNull);
    EXPECT_EQ(-1, hcclTypeNull->bandWidth);
}

TEST_F(HcclCalculatorUtest, BatchSendOpDescShouldRetSuccessAndFailWhenBandWidth)
{
    HcclDataType dataType = HCCL_DATA_TYPE_INT8;
    uint64_t dataTypeSize = 1;
    uint64_t startTime = 100;
    uint64_t endTime = 200;
    uint64_t rankSize = 2;
    uint64_t count = 1;

    BatchSendRecvOp* hcclOpDesc = new BatchSendRecvOp();
    hcclOpDesc->opName = "HcclBatchSendRecv";
    hcclOpDesc->streamId = 1;
    hcclOpDesc->deviceId = 1;
    hcclOpDesc->end = endTime;
    hcclOpDesc->start = startTime;
    hcclOpDesc->bandWidth = -1;

    P2pOpDesc item1;
    item1.opName = "HcclAllReduce";
    item1.streamId = 1;
    item1.deviceId = 1;
    item1.end = endTime;
    item1.start = startTime;
    item1.count = count;
    item1.dataType = HCCL_DATA_TYPE_INT8;

    P2pOpDesc item2;
    item2.opName = "HcclAllReduce";
    item2.streamId = 1;
    item2.deviceId = 1;
    item2.end = endTime;
    item2.start = startTime;
    item2.count = count;
    item2.dataType = HCCL_DATA_TYPE_RESERVED;

    hcclOpDesc->batchSendRecvItem.emplace_back(item1);
    hcclOpDesc->batchSendRecvItem.emplace_back(item2);
    Mspti::Parser::HcclCalculator::CalculateBandWidth((HcclOpDesc*)hcclOpDesc);
    EXPECT_EQ(-1, hcclOpDesc->bandWidth);
}
}
