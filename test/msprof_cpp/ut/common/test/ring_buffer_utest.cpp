#include <string>
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "queue/ring_buffer.h"

using namespace analysis::dvvp::common::queue;

class COMMON_QUEUE_RING_BUFFER_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
private:
    std::string _log_file;
};

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer) {
    std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));
    EXPECT_NE(nullptr, bq);
    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_Init) {
    std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));
    EXPECT_NE(nullptr, bq);
    bq->Init(2);
    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_UnInit) {
    std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));
    EXPECT_NE(nullptr, bq);
    bq->Init(2);
    bq->UnInit();
    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_SetQuit) {
    std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));
    EXPECT_NE(nullptr, bq);
    bq->Init(2);
    bq->SetQuit();
    bq->UnInit();
    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_TryPush) {
    std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));

    //not inited
    EXPECT_EQ(false, bq->TryPush(1));

    bq->Init(2);

    //exceeded max cycles
    bq->maxCycles_ = 0;
    EXPECT_EQ(false, bq->TryPush(1));

    //not exceed max cycles
    bq->maxCycles_ = 1024;
    EXPECT_EQ(true, bq->TryPush(1));

    //queue is full
    EXPECT_EQ(false, bq->TryPush(1));

    //queue quits
    bq->SetQuit();
    EXPECT_EQ(false, bq->TryPush(1));

    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_TryPop) {
    std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));

    int data = -1;
    //not inited
    EXPECT_EQ(false, bq->TryPop(data));

    bq->Init(2);

    //queue is empty
    EXPECT_EQ(false, bq->TryPop(data));


    bq->TryPush(1);
    //not ready
    // bq->dataAvails_[0] = RingBuffer<int>::DATA_STATUS_NOT_READY;
    bq->dataAvails_[0] = static_cast<int>(DataStatus::DATA_STATUS_NOT_READY);
    EXPECT_EQ(false, bq->TryPop(data));

    bq->dataAvails_[0] = static_cast<int>(DataStatus::DATA_STATUS_READY);
    EXPECT_EQ(true, bq->TryPop(data));
    EXPECT_EQ(1, data);

    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_GetUsedSize) {
    std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));

    bq->Init(4);
    EXPECT_EQ(0, bq->GetUsedSize());

    bq->TryPush(1);
    EXPECT_EQ(1, bq->GetUsedSize());

    bq->TryPush(2);
    EXPECT_EQ(2, bq->GetUsedSize());

    bq->TryPush(3);
    EXPECT_EQ(3, bq->GetUsedSize());

    EXPECT_EQ(false, bq->TryPush(4));
    EXPECT_EQ(3, bq->GetUsedSize());

    EXPECT_EQ(0, bq->readIndex_.load());
    EXPECT_EQ(3, bq->writeIndex_.load());

    int data = -1;
    EXPECT_EQ(true, bq->TryPop(data));
    EXPECT_EQ(1, data);
    EXPECT_EQ(2, bq->GetUsedSize());

    EXPECT_EQ(1, bq->readIndex_.load());
    EXPECT_EQ(3, bq->writeIndex_.load());

    EXPECT_EQ(true, bq->TryPop(data));
    EXPECT_EQ(2, data);
    EXPECT_EQ(1, bq->GetUsedSize());

    EXPECT_EQ(2, bq->readIndex_.load());
    EXPECT_EQ(3, bq->writeIndex_.load());

    EXPECT_EQ(true, bq->TryPop(data));
    EXPECT_EQ(3, data);
    EXPECT_EQ(0, bq->GetUsedSize());

    EXPECT_EQ(3, bq->readIndex_.load());
    EXPECT_EQ(3, bq->writeIndex_.load());

    bq->TryPush(4);
    EXPECT_EQ(1, bq->GetUsedSize());

    EXPECT_EQ(3, bq->readIndex_.load());
    EXPECT_EQ(4, bq->writeIndex_.load());

    bq->TryPush(5);
    EXPECT_EQ(2, bq->GetUsedSize());

    EXPECT_EQ(3, bq->readIndex_.load());
    EXPECT_EQ(5, bq->writeIndex_.load());

    EXPECT_EQ(true, bq->TryPop(data));
    EXPECT_EQ(4, data);
    EXPECT_EQ(1, bq->GetUsedSize());

    EXPECT_EQ(4, bq->readIndex_.load());
    EXPECT_EQ(5, bq->writeIndex_.load());

    EXPECT_EQ(true, bq->TryPop(data));
    EXPECT_EQ(5, data);
    EXPECT_EQ(0, bq->GetUsedSize());

    EXPECT_EQ(5, bq->readIndex_.load());
    EXPECT_EQ(5, bq->writeIndex_.load());

    EXPECT_EQ(false, bq->TryPop(data));

    bq->readIndex_ = ((size_t)0xFFFFFFFFFFFFFFFF) - 2;
    bq->writeIndex_ = 2;
    EXPECT_EQ(5, bq->GetUsedSize());
}