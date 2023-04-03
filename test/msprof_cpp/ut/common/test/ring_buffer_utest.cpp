#include <string>
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "queue/ring_buffer.h"
#include "utils/utils.h"

using namespace analysis::dvvp::common::queue;
using namespace analysis::dvvp::common::utils;

struct ReporterDataChunk {
    int32_t deviceId;
    uint32_t dataLen;
    uint64_t reportTime;
    ReporterDataChunkTag tag;
    ReporterDataChunkPayload data;
};

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
    ReporterDataChunk defaultData;
    std::shared_ptr<RingBuffer<ReporterDataChunk> > bq(new RingBuffer<ReporterDataChunk>(defaultData));
    ReporterData data;
    data.deviceId = 0;
    data.dataLen = sizeof(ReporterData);
    data.data = reinterpret_cast<UNSIGNED_CHAR_PTR>(&data);

    //not inited
    EXPECT_EQ(false, bq->TryPush(&data));

    bq->Init(2);

    //exceeded max cycles
    bq->maxCycles_ = 0;
    EXPECT_EQ(false, bq->TryPush(&data));

    //not exceed max cycles
    bq->maxCycles_ = 1024;
    EXPECT_EQ(true, bq->TryPush(&data));

    //queue is full, will use Queue Level2
    EXPECT_EQ(true, bq->TryPush(&data));

    //queue quits
    bq->SetQuit();
    EXPECT_EQ(false, bq->TryPush(&data));

    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_TryPop) {
    ReporterDataChunk defaultData;
    std::shared_ptr<RingBuffer<ReporterDataChunk> > bq(new RingBuffer<ReporterDataChunk>(defaultData));
    ReporterData data;
    data.deviceId = 0;
    data.dataLen = sizeof(ReporterData);
    data.data = reinterpret_cast<UNSIGNED_CHAR_PTR>(&data);
    
    // std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));
    ReporterDataChunk data2;
    //not inited
    EXPECT_EQ(false, bq->TryPop(data2));

    bq->Init(2);

    //queue is empty
    EXPECT_EQ(false, bq->TryPop(data2));


    bq->TryPush(&data);
    //not ready
    // bq->dataAvails_[0] = RingBuffer<int>::DATA_STATUS_NOT_READY;
    bq->dataAvails_[0] = static_cast<int>(DataStatus::DATA_STATUS_NOT_READY);
    EXPECT_EQ(false, bq->TryPop(data2));

    bq->dataAvails_[0] = static_cast<int>(DataStatus::DATA_STATUS_READY);
    EXPECT_EQ(true, bq->TryPop(data2));
    EXPECT_EQ(sizeof(ReporterData), data2.dataLen);

    bq.reset();
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_GetUsedSize) {
    ReporterDataChunk defaultData;
    std::shared_ptr<RingBuffer<ReporterDataChunk> > bq(new RingBuffer<ReporterDataChunk>(defaultData));
    ReporterData data;
    data.deviceId = 0;
    data.dataLen = sizeof(ReporterData);
    data.data = reinterpret_cast<UNSIGNED_CHAR_PTR>(&data);
 
    bq->Init(4);
    int value = 0;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(true, bq->TryPush(&data));
    value = 1;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(true, bq->TryPush(&data));
    value = 2;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(true, bq->TryPush(&data));
    value = 3;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(true, bq->TryPush(&data));
    value = 4;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    int readValue = 0;
    int writeValue = 3;
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    ReporterDataChunk data2;
    EXPECT_EQ(true, bq->TryPop(data2));
    EXPECT_EQ(sizeof(ReporterData), data2.dataLen);
    value = 3;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    readValue = 1;
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    EXPECT_EQ(true, bq->TryPop(data2));
    EXPECT_EQ(sizeof(ReporterData), data2.dataLen);
    value = 2;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    readValue = 2;
    writeValue = 3;
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    EXPECT_EQ(true, bq->TryPop(data2));
    EXPECT_EQ(sizeof(ReporterData), data2.dataLen);
    value = 1;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    readValue = 3;
    writeValue = 3;
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    bq->TryPush(&data);
    value = 2;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    bq->TryPush(&data);
    value = 3;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    EXPECT_EQ(true, bq->TryPop(data2));
    EXPECT_EQ(sizeof(ReporterData), data2.dataLen);
    value = 2;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    EXPECT_EQ(true, bq->TryPop(data2));
    EXPECT_EQ(sizeof(ReporterData), data2.dataLen);
    value = 1;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(readValue, bq->readIndex_.load());
    EXPECT_EQ(writeValue, bq->writeIndex_.load());
 
    EXPECT_EQ(true, bq->TryPop(data2));
    value = 0;
    EXPECT_EQ(value, bq->GetUsedSize());
 
    EXPECT_EQ(false, bq->TryPop(data2));

    bq->readIndex_ = ((size_t)0xFFFFFFFFFFFFFFFF) - 2;
    bq->writeIndex_ = 2;
    EXPECT_EQ(5, bq->GetUsedSize());
}