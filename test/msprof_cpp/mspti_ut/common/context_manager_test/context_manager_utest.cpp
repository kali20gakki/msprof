/**
* @file callback_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"

#include "mspti.h"
#include "common/context_manager.h"

namespace {
class ContextManagerUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(ContextManagerUtest, CorrelationIdTest)
{
    EXPECT_EQ(0, Mspti::Common::ContextManager::GetInstance()->CorrelationId());
    Mspti::Common::ContextManager::GetInstance()->UpdateCorrelationId();
    EXPECT_EQ(1, Mspti::Common::ContextManager::GetInstance()->CorrelationId());
}
}
