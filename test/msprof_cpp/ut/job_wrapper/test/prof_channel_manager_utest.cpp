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
 * -------------------------------------------------------------------------*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prof_channel_manager.h"
#include "mmpa_api.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace Analysis::Dvvp::JobWrapper;
using namespace Collector::Dvvp::Mmpa;

class PROF_CHANNEL_MANAGER_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
};

TEST_F(PROF_CHANNEL_MANAGER_UTEST, ProfChannelManager_Init) {
    auto entry = ProfChannelManager::instance();
    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(entry->Init(), PROFILING_SUCCESS);
    EXPECT_NE(nullptr, entry->GetChannelPoller());
}

TEST_F(PROF_CHANNEL_MANAGER_UTEST, ProfChannelManager_UnInit) {
    auto entry = ProfChannelManager::instance();
    // MOCKER_CPP(&analysis::dvvp::transport::ChannelPoll::Stop)
    //     .stubs()
    //     .will(returnValue(PROFILING_SUCCESS));
    MOCKER(&MmJoinTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_NE(nullptr, entry);
    entry->FlushChannel();
    entry->UnInit();
}
