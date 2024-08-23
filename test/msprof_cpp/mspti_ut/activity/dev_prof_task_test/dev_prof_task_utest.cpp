/**
* @file callback_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "mspti.h"
#include "activity/ascend/dev_task_manager.h"
#include "common/context_manager.h"

namespace {
class DevProfTaskUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(DevProfTaskUtest, DevProfTaskFactoryTest)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Common::ContextManager::GetChipType)
        .stubs()
        .will(returnValue(Mspti::Common::PlatformType::CHIP_910B));
    constexpr uint32_t deviceId = 0;
    msptiActivityKind kind = MSPTI_ACTIVITY_KIND_MARKER;
    auto profTasks = Mspti::Ascend::DevProfTaskFactory::CreateTasks(deviceId, kind);
    constexpr size_t MARKER_PROF_TASK_NUM = 1;
    EXPECT_EQ(MARKER_PROF_TASK_NUM, profTasks.size());
    kind = MSPTI_ACTIVITY_KIND_KERNEL;
    constexpr size_t KERNEL_PROF_TASK_NUM = 2;
    profTasks = Mspti::Ascend::DevProfTaskFactory::CreateTasks(deviceId, kind);
    EXPECT_EQ(KERNEL_PROF_TASK_NUM, profTasks.size());
}
}
