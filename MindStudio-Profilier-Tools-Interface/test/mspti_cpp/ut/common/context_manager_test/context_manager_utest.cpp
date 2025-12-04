/**
* @file context_manager_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "mspti.h"
#include "csrc/common/context_manager.h"
#include "csrc/common/inject/driver_inject.h"

namespace {
class ContextManagerUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(ContextManagerUtest, ShouldGetOneAfterUpdateAndReportCorrelationId)
{
    EXPECT_EQ(0UL, Mspti::Common::ContextManager::GetInstance()->GetCorrelationId());
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId();
    EXPECT_EQ(1UL, Mspti::Common::ContextManager::GetInstance()->GetCorrelationId());
}

TEST_F(ContextManagerUtest, ShouldInitDeviceFreqWithDefaultValueWhenDrvFailed)
{
    GlobalMockObject::verify();
    MOCKER_CPP(HalGetDeviceInfo)
        .stubs()
        .will(returnValue(DRV_ERROR_NOT_SUPPORT));
    constexpr uint32_t deviceId = 0;
    Mspti::Common::ContextManager::GetInstance()->InitDevTimeInfo(deviceId);
}

}
