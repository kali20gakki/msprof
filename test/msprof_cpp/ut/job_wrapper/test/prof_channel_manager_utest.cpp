#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prof_channel_manager.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace Analysis::Dvvp::JobWrapper;

class PROF_CHANNEL_MANAGER_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
};

TEST_F(PROF_CHANNEL_MANAGER_UTEST, ProfChannelManager_Init) {
    auto entry = ProfChannelManager::instance();
    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(EN_OK));
    EXPECT_EQ(entry->Init(), PROFILING_SUCCESS);
    EXPECT_NE(nullptr, entry->GetChannelPoller());
}

TEST_F(PROF_CHANNEL_MANAGER_UTEST, ProfChannelManager_UnInit) {
    auto entry = ProfChannelManager::instance();
    // MOCKER_CPP(&analysis::dvvp::transport::ChannelPoll::Stop)
    //     .stubs()
    //     .will(returnValue(PROFILING_SUCCESS));
    MOCKER(mmJoinTask)
        .stubs()
        .will(returnValue(EN_OK));
    EXPECT_NE(nullptr, entry);
    entry->FlushChannel();
    entry->UnInit();
}
