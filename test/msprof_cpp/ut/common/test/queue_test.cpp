#include <string>
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
// #include "logger/logger.h"
#include "queue/bound_queue.h"

using namespace analysis::dvvp::common::queue;

class COMMON_QUEUE_BOUND_QUEUE_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }

};

TEST_F(COMMON_QUEUE_BOUND_QUEUE_TEST, BoundQueue) {
    std::shared_ptr<BoundQueue<int> > bq(new BoundQueue<int>(2));
    EXPECT_NE(nullptr, bq);
    bq.reset();
}

TEST_F(COMMON_QUEUE_BOUND_QUEUE_TEST, Push) {
    std::shared_ptr<BoundQueue<int> > bq(new BoundQueue<int>(2));

    EXPECT_TRUE(bq->Push(1));

    bq->Quit();
    EXPECT_FALSE(bq->Push(2));
}


TEST_F(COMMON_QUEUE_BOUND_QUEUE_TEST, TryPush) {
    std::shared_ptr<BoundQueue<int> > bq(new BoundQueue<int>(1));
    EXPECT_EQ(true, bq->TryPush(1));
    EXPECT_EQ(false, bq->TryPush(1));
}

TEST_F(COMMON_QUEUE_BOUND_QUEUE_TEST, TryPop) {
    std::shared_ptr<BoundQueue<int> > bq(new BoundQueue<int>(2));

    int data;
    EXPECT_EQ((size_t)0, bq->size());
    EXPECT_FALSE(bq->TryPop(data));

    bq->Push(1);
    EXPECT_EQ((size_t)1, bq->size());

    bq->Push(2);
    EXPECT_EQ((size_t)2, bq->size());

    EXPECT_TRUE(bq->TryPop(data));
    EXPECT_EQ(1, data);
    EXPECT_EQ((size_t)1, bq->size());
}

TEST_F(COMMON_QUEUE_BOUND_QUEUE_TEST, Pop) {
    std::shared_ptr<BoundQueue<int> > bq(new BoundQueue<int>(2));

    int data;
    bq->Push(1);
    bq->Push(2);
    EXPECT_EQ((size_t)2, bq->size());

    EXPECT_TRUE(bq->Pop(data));
    EXPECT_EQ(1, data);
    EXPECT_EQ((size_t)1, bq->size());

    bq->Quit();
    EXPECT_TRUE(bq->Pop(data));
    EXPECT_FALSE(bq->Pop(data));
}

TEST_F(COMMON_QUEUE_BOUND_QUEUE_TEST, Quit) {
    std::shared_ptr<BoundQueue<int> > bq(new BoundQueue<int>(2));
    EXPECT_NE(nullptr, bq);
    bq->Quit();
}

TEST_F(COMMON_QUEUE_BOUND_QUEUE_TEST, size) {
    std::shared_ptr<BoundQueue<int> > bq(new BoundQueue<int>(2));

    EXPECT_EQ((size_t)0, bq->size());

    bq->Push(1);
    EXPECT_EQ((size_t)1, bq->size());
}