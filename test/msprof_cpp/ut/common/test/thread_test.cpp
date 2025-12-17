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
#include "errno/error_code.h"
#include "thread/thread.h"
#include "stub/common-utils-stub.h"
#include "utils/utils.h"
#include "mmpa_api.h"

using namespace analysis::dvvp::common::thread;
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Mmpa;

class COMMON_THREAD_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

class ThreadClass : public Thread {
public:
    virtual void Run(const struct error_message::Context &errorContext) {
    }
};

TEST_F(COMMON_THREAD_TEST, start_stop) {
    GlobalMockObject::verify();

    ThreadClass my_thread;
    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER(&MmJoinTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, my_thread.Start());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Start());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Start());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Stop());
}

TEST_F(COMMON_THREAD_TEST, join) {
    GlobalMockObject::verify();

    ThreadClass my_thread;

    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(&MmJoinTask)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    my_thread.Start();
    my_thread.tid_ = 1;

    EXPECT_EQ(PROFILING_FAILED, my_thread.Join());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Join());
}

TEST_F(COMMON_THREAD_TEST, IsQuit) {
    GlobalMockObject::verify();

    ThreadClass my_thread;

    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(&MmJoinTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    my_thread.Start();
    EXPECT_FALSE(my_thread.IsQuit());
    my_thread.Stop();
    EXPECT_TRUE(my_thread.IsQuit());
}

TEST_F(COMMON_THREAD_TEST, ThrProcess) {
    GlobalMockObject::verify();

    ThreadClass *my_thread = new ThreadClass;

    EXPECT_EQ(NULL, ThreadClass::ThrProcess((void*)my_thread));
    EXPECT_EQ(NULL, ThreadClass::ThrProcess((void*)my_thread));

    delete my_thread;
}
