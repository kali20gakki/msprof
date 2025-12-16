/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
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
