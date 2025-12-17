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

constexpr size_t BUFFER_SIZE = 2;

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_UnInit) {
    RingBuffer<int> bq;
    bq.Init(BUFFER_SIZE);
    EXPECT_EQ(true, bq.isInited_);
    EXPECT_EQ(BUFFER_SIZE, bq.dataQueue_.size());
    bq.UnInit();
    EXPECT_EQ(false, bq.isInited_);
    EXPECT_EQ(0, bq.dataQueue_.size());
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_SetQuit) {
    RingBuffer<int> bq;
    bq.Init(BUFFER_SIZE);
    EXPECT_EQ(true, bq.isInited_);
    EXPECT_EQ(BUFFER_SIZE, bq.dataQueue_.size());
    bq.SetQuit();
    bq.UnInit();
    EXPECT_EQ(false, bq.isInited_);
    EXPECT_EQ(0, bq.dataQueue_.size());
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_TryPush) {
    RingBuffer<ReporterDataChunk> bq;
    ReporterDataChunk data;
    //not inited
    EXPECT_EQ(false, bq.TryPush(data));

    bq.Init(BUFFER_SIZE);

    //exceeded max cycles
    bq.maxCycles_ = 0;
    EXPECT_EQ(false, bq.TryPush(data));

    //not exceed max cycles
    bq.maxCycles_ = 1024;
    EXPECT_EQ(true, bq.TryPush(data));

    //queue is full, will use Queue Level2
    EXPECT_EQ(true, bq.TryPush(data));
    EXPECT_EQ(true, bq.TryPush(data));

    //queue quits
    bq.SetQuit();
    EXPECT_EQ(false, bq.TryPush(data));
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_TryPop) {
    RingBuffer<ReporterDataChunk> bq;
    ReporterDataChunk data;

    // std::shared_ptr<RingBuffer<int> > bq(new RingBuffer<int>(-1));
    ReporterDataChunk data2;
    //not inited
    EXPECT_EQ(false, bq.TryPop(data2));

    bq.Init(BUFFER_SIZE);

    //queue is empty
    EXPECT_EQ(false, bq.TryPop(data2));

    bq.TryPush(data);
    //not ready
    // bq.dataAvails_[0] = RingBuffer<int>::DATA_STATUS_NOT_READY;
    bq.dataAvails_[0] = static_cast<int>(DataStatus::DATA_STATUS_NOT_READY);
    EXPECT_EQ(false, bq.TryPop(data2));

    bq.dataAvails_[0] = static_cast<int>(DataStatus::DATA_STATUS_READY);
    EXPECT_EQ(true, bq.TryPop(data2));
}

TEST_F(COMMON_QUEUE_RING_BUFFER_TEST, RingBuffer_GetUsedSize) {
    RingBuffer<ReporterDataChunk> bq;
    ReporterDataChunk data;

    bq.Init(BUFFER_SIZE);
    int value = 0;
    EXPECT_EQ(0, bq.GetUsedSize());
    for (int i = 0; i < BUFFER_SIZE; i++) {
        EXPECT_EQ(true, bq.TryPush(data));
        EXPECT_EQ(i + 1, bq.GetUsedSize());
    }
    EXPECT_EQ(0, bq.readIndex_.load());
    EXPECT_EQ(BUFFER_SIZE - 1, bq.writeIndex_.load());
    EXPECT_EQ(1, bq.extdDataQueue_.size()); // enable external buffer
 
    ReporterDataChunk data2;
    for (int i = 0; i < 1; i++) { // pop all buffer data
        EXPECT_EQ(true, bq.TryPop(data2));
        EXPECT_EQ(i + 1, bq.readIndex_.load());
    }
    EXPECT_EQ(true, bq.TryPop(data2));
    EXPECT_EQ(0, bq.extdDataQueue_.size()); // pop external buffer data
}