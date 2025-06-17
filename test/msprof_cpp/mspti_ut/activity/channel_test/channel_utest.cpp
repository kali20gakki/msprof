/**
* @file channel_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include <memory>
#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"
#include "common/inject/inject_base.h"
#include "activity/ascend/channel/channel_data.h"
#include "common/inject/driver_inject.h"
#include "activity/ascend/channel/channel_reader.h"
#include "activity/ascend/channel/channel_pool.h"

namespace {
class ChannelUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(ChannelUtest, ShouldRetSuccessWhenExecChannelReaderCorrectly)
{
    uint32_t deviceId = 0;
    AI_DRV_CHANNEL channelId = PROF_CHANNEL_TS_FW;
    auto reader = Mspti::Ascend::Channel::ChannelReader(deviceId, channelId);
    EXPECT_EQ(MSPTI_SUCCESS, reader.Init());
    reader.SetSchedulingStatus(true);
    EXPECT_EQ(true, reader.GetSchedulingStatus());
    EXPECT_EQ(MSPTI_SUCCESS, reader.Execute());
    reader.SetChannelStopped();
    EXPECT_EQ(MSPTI_SUCCESS, reader.Uinit());

    channelId = PROF_CHANNEL_STARS_SOC_LOG;
    reader = Mspti::Ascend::Channel::ChannelReader(deviceId, channelId);
    EXPECT_EQ(MSPTI_SUCCESS, reader.Init());
    reader.SetSchedulingStatus(true);
    EXPECT_EQ(true, reader.GetSchedulingStatus());
    EXPECT_EQ(MSPTI_SUCCESS, reader.Execute());
    reader.SetChannelStopped();
    EXPECT_EQ(MSPTI_SUCCESS, reader.Uinit());
}

TEST_F(ChannelUtest, ExecuteShouleBreakWhenGetInvalidDataLenFromDriver)
{
    constexpr unsigned int maxChannelId = 160;
    uint32_t deviceId = 0;
    AI_DRV_CHANNEL channelId = static_cast<AI_DRV_CHANNEL>(maxChannelId);
    auto reader = Mspti::Ascend::Channel::ChannelReader(deviceId, channelId);
    EXPECT_EQ(MSPTI_SUCCESS, reader.Init());
    EXPECT_EQ(MSPTI_SUCCESS, reader.Execute());
    reader.SetChannelStopped();
    EXPECT_EQ(MSPTI_SUCCESS, reader.Uinit());
}

TEST_F(ChannelUtest, ChannelPoolAddReader)
{
    std::unique_ptr<Mspti::Ascend::Channel::ChannelPool> drvChannelPoll_ =
        std::make_unique<Mspti::Ascend::Channel::ChannelPool>(1);
    EXPECT_EQ(MSPTI_SUCCESS, drvChannelPoll_->AddReader(1, PROF_CHANNEL_UNKNOWN));
    EXPECT_EQ(MSPTI_SUCCESS, drvChannelPoll_->RemoveReader(1, PROF_CHANNEL_UNKNOWN));
}
}
