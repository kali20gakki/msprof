#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "errno/error_code.h"
#include "thread/thread.h"
#include "stub/common-utils-stub.h"
#include "utils/utils.h"

using namespace analysis::dvvp::common::thread;
using namespace analysis::dvvp::common::error;

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
    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(EN_ERROR))
        .then(returnValue(EN_OK));

    MOCKER(mmJoinTask)
        .stubs()
        .will(returnValue(EN_OK));

    EXPECT_EQ(PROFILING_FAILED, my_thread.Start());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Start());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Start());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Stop());
}

TEST_F(COMMON_THREAD_TEST, join) {
    GlobalMockObject::verify();

    ThreadClass my_thread;

    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(EN_OK));

    MOCKER(mmJoinTask)
        .stubs()
        .will(returnValue(EN_ERROR))
        .then(returnValue(EN_OK));

    my_thread.Start();
    my_thread.tid_ = 1;

    EXPECT_EQ(PROFILING_FAILED, my_thread.Join());
    EXPECT_EQ(PROFILING_SUCCESS, my_thread.Join());
}

TEST_F(COMMON_THREAD_TEST, IsQuit) {
    GlobalMockObject::verify();

    ThreadClass my_thread;

    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(EN_OK));

    MOCKER(mmJoinTask)
        .stubs()
        .will(returnValue(EN_OK));

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
