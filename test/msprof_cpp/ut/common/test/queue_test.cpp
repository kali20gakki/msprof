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