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

#include <memory>
#include "gtest/gtest.h"

#include "mockcpp/mockcpp.hpp"
#include "csrc/common/inject/inject_base.h"
#include "csrc/activity/ascend/channel/channel_data.h"
#include "csrc/common/inject/driver_inject.h"
#include "csrc/activity/ascend/channel/channel_reader.h"
#include "csrc/activity/ascend/channel/channel_pool.h"

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

TEST_F(ChannelUtest, ExecuteShouldBreakWhenGetInvalidDataLenFromDriver)
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
