/**
* @file dev_prof_task_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"

#include "csrc/activity/ascend/channel/channel_pool_manager.h"
#include "csrc/activity/ascend/dev_task_manager.h"
#include "csrc/common/context_manager.h"
#include "csrc/common/inject/profapi_inject.h"
#include "mspti.h"

namespace {
class DevProfTaskUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(DevProfTaskUtest, ShouldGetRightProfTaskNumsWhenSetDifferentKind)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Common::ContextManager::GetChipType)
        .stubs()
        .will(returnValue(Mspti::Common::PlatformType::CHIP_910B));
    MOCKER_CPP(&Mspti::Ascend::Channel::ChannelPoolManager::CheckChannelValid)
        .stubs()
        .will(returnValue(true));
    constexpr uint32_t deviceId = 0;
    msptiActivityKind kind = MSPTI_ACTIVITY_KIND_MARKER;
    auto profTasks = Mspti::Ascend::DevProfTaskFactory::CreateTasks(deviceId, kind);
    constexpr size_t MARKER_PROF_TASK_NUM = 1;
    EXPECT_EQ(MARKER_PROF_TASK_NUM, profTasks.size());
    kind = MSPTI_ACTIVITY_KIND_KERNEL;
    constexpr size_t KERNEL_PROF_TASK_NUM = 2;
    profTasks = Mspti::Ascend::DevProfTaskFactory::CreateTasks(deviceId, kind);
    EXPECT_EQ(KERNEL_PROF_TASK_NUM, profTasks.size());
    msptiResult ret = MSPTI_SUCCESS;
    for (auto& profTask : profTasks) {
        if (profTask->Start() != MSPTI_SUCCESS) {
            ret = MSPTI_ERROR_INNER;
        }
    }
    for (auto& profTask : profTasks) {
        if (profTask->Stop() != MSPTI_SUCCESS) {
            ret = MSPTI_ERROR_INNER;
        }
    }
    EXPECT_EQ(MSPTI_SUCCESS, ret);
}

TEST_F(DevProfTaskUtest, ShouldGetZeroProfTaskNumsWhenPlatformInvalid)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Common::ContextManager::GetChipType)
        .stubs()
        .will(returnValue(Mspti::Common::PlatformType::END_TYPE));
    constexpr uint32_t deviceId = 0;
    msptiActivityKind kind = MSPTI_ACTIVITY_KIND_MARKER;
    auto profTasks = Mspti::Ascend::DevProfTaskFactory::CreateTasks(deviceId, kind);
    constexpr size_t ZERO_PROF_TASK_NUM = 0;
    EXPECT_EQ(ZERO_PROF_TASK_NUM, profTasks.size());
}

TEST_F(DevProfTaskUtest, ShouldGetZeroProfTaskNumsWhenKindInvalid)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Common::ContextManager::GetChipType)
        .stubs()
        .will(returnValue(Mspti::Common::PlatformType::CHIP_910B));
    constexpr uint32_t deviceId = 0;
    msptiActivityKind kind = MSPTI_ACTIVITY_KIND_COUNT;
    auto profTasks = Mspti::Ascend::DevProfTaskFactory::CreateTasks(deviceId, kind);
    constexpr size_t ZERO_PROF_TASK_NUM = 0;
    EXPECT_EQ(ZERO_PROF_TASK_NUM, profTasks.size());
}

TEST_F(DevProfTaskUtest, ShouldRetSuccessWhenUseDevTaskManagerNormal)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Ascend::Channel::ChannelPoolManager::GetAllChannels)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    auto instance = Mspti::Ascend::DevTaskManager::GetInstance();
    uint32_t deviceId = 0;
    Mspti::Ascend::DevTaskManager::ActivitySwitchType kinds;
    kinds[MSPTI_ACTIVITY_KIND_KERNEL] = true;
    auto ret = instance -> StartDevProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_SUCCESS, ret);

    ret = instance -> StopDevProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_SUCCESS, ret);
}

TEST_F(DevProfTaskUtest, ShouldRetErrorWhenDeviceOffline)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::CheckDeviceOnline)
        .stubs()
        .will(returnValue(false));
    auto instance = Mspti::Ascend::DevTaskManager::GetInstance();
    uint32_t deviceId = 0;
    Mspti::Ascend::DevTaskManager::ActivitySwitchType kinds;
    kinds[MSPTI_ACTIVITY_KIND_KERNEL] = true;
    auto ret = instance -> StartDevProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_ERROR_INNER, ret);

    ret = instance -> StopDevProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_ERROR_INNER, ret);
}

TEST_F(DevProfTaskUtest, ShouldRetErrorWhenGetChannelsError)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Ascend::Channel::ChannelPoolManager::GetAllChannels)
        .stubs()
        .will(returnValue(MSPTI_ERROR_INNER));
    uint32_t deviceId = 0;
    Mspti::Ascend::DevTaskManager::ActivitySwitchType kinds;
    kinds[MSPTI_ACTIVITY_KIND_KERNEL] = true;
    auto instance = Mspti::Ascend::DevTaskManager::GetInstance();
    auto ret = instance -> StartDevProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_ERROR_INNER, ret);
}

TEST_F(DevProfTaskUtest, ShouldRetErrorWhenStartOrStopCannProfTaskFailed)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::CheckDeviceOnline)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Mspti::Ascend::Channel::ChannelPoolManager::GetAllChannels)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Inject::profSetProfCommand)
        .stubs()
        .will(returnValue(static_cast<int32_t>(MSPTI_ERROR_INNER)));
    auto instance = Mspti::Ascend::DevTaskManager::GetInstance();
    uint32_t deviceId = 0;
    Mspti::Ascend::DevTaskManager::ActivitySwitchType kinds;
    kinds[MSPTI_ACTIVITY_KIND_KERNEL] = true;
    auto ret = instance -> StartDevProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_ERROR_INNER, ret);

    ret = instance -> StopDevProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_ERROR_INNER, ret);
}

TEST_F(DevProfTaskUtest, ShouldRetSuccessWhenCannProfNotSupport)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Inject::profSetProfCommand)
        .stubs()
        .will(returnValue(static_cast<int32_t>(MSPTI_SUCCESS)));
    auto instance = Mspti::Ascend::DevTaskManager::GetInstance();
    uint32_t deviceId = 0;
    Mspti::Ascend::DevTaskManager::ActivitySwitchType kinds;
    kinds[MSPTI_ACTIVITY_KIND_MARKER] = true;
    auto ret = instance -> StartCannProfTask(deviceId, kinds);
    EXPECT_EQ(MSPTI_SUCCESS, ret);

    ret = instance -> StopCannProfTask(deviceId);
    EXPECT_EQ(MSPTI_SUCCESS, ret);
}

}
