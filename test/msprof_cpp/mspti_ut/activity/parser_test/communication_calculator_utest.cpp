/**
 * @file channel_utest.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 *
 */

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include <memory>

#include "activity/ascend/parser/parser_manager.h"
#include "activity/ascend/parser/cann_hash_cache.h"
#include "activity/ascend/parser/mstx_parser.h"
#include "activity/ascend/parser/communication_calculator.h"
#include "activity/ascend/parser/cann_hash_cache.h"
#include "activity/activity_manager.h"
#include "common/inject/runtime_inject.h"
#include "common/inject/profapi_inject.h"
#include "common/utils.h"
#include "securec.h"

namespace {
class CommunicationCalculatorUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(CommunicationCalculatorUtest, ShouldReturnSuccessWhenChildEmpty)
{
    std::shared_ptr<Mspti::Parser::ApiEvent> api2TaskInfo = std::make_shared<Mspti::Parser::ApiEvent>();
    auto &instance = Mspti::Parser::CommunicationCalculator::GetInstance();
    EXPECT_EQ(instance.AppendApi2TaskInfo(api2TaskInfo), MSPTI_SUCCESS);
}

TEST_F(CommunicationCalculatorUtest, ShouldReturnSuccess)
{
    uint32_t threadId = 2;
    uint64_t beginTime = 100;
    uint64_t endTime = 100;
    uint64_t subBeginTime = 110;
    uint64_t subEndTime = 180;
    uint16_t level = MSPROF_REPORT_HCCL_NODE_LEVEL;
    std::shared_ptr<Mspti::Parser::ApiEvent> api2TaskInfo = std::make_shared<Mspti::Parser::ApiEvent>();
    api2TaskInfo->api.beginTime = beginTime;
    api2TaskInfo->api.endTime = endTime;
    api2TaskInfo->api.threadId = threadId;
    std::shared_ptr<Mspti::Parser::ApiEvent> subApi2TaskInfo = std::make_shared<Mspti::Parser::ApiEvent>();
    subApi2TaskInfo->api.beginTime = subBeginTime;
    subApi2TaskInfo->api.endTime = subEndTime;
    api2TaskInfo->childs.push_back(subApi2TaskInfo);
    auto &instance = Mspti::Parser::CommunicationCalculator::GetInstance();
    EXPECT_EQ(instance.AppendApi2TaskInfo(api2TaskInfo), MSPTI_SUCCESS);
}

TEST_F(CommunicationCalculatorUtest, ShouldReturnSuccessAppendCompactInfo)
{
    bool agingFlag = 1;
    uint8_t dataType = 1;
    uint64_t dataCount = 1;
    MsprofCompactInfo data;
    (void)memset_s(&data, sizeof(data), 0, sizeof(data));
    data.data.hcclopInfo.dataType = dataType;
    data.data.hcclopInfo.groupName = Mspti::Parser::CannHashCache::GetInstance().GenHashId("hcom_1");
    data.data.hcclopInfo.algType = Mspti::Parser::CannHashCache::GetInstance().GenHashId("mesh");
    data.data.hcclopInfo.algType = dataCount;
    auto &instance = Mspti::Parser::CommunicationCalculator::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance.AppendCompactInfo(agingFlag, &data));
    EXPECT_EQ(MSPTI_SUCCESS,
        Mspti::Activity::ActivityManager::GetInstance()->Register(MSPTI_ACTIVITY_KIND_COMMUNICATION));
    EXPECT_EQ(MSPTI_SUCCESS, instance.AppendCompactInfo(agingFlag, &data));
}
}