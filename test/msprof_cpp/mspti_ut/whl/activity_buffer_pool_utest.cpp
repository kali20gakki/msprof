/**
* @file activity_buffer_pool_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "whl/csrc/activity_buffer_pool.h"

namespace {
using namespace Mspti::Adapter;
constexpr size_t MB = 1024 * 1024;

class ActivityBufferPoolUtest : public testing::Test {
protected:
    static void SetUpTestCase() {}

    static void TearDownTestCase()
    {
        GlobalMockObject::verify();
    }

    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }

    virtual void TearDown() {}
};

TEST_F(ActivityBufferPoolUtest, CheckCanAllocBufferWillSuccessWhenPoolNotFull)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 2;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    EXPECT_TRUE(pool.CheckCanAllocBuffer());
    EXPECT_NE(nullptr, pool.GetBuffer());
    EXPECT_TRUE(pool.CheckCanAllocBuffer());
    EXPECT_NE(nullptr, pool.GetBuffer());
}

TEST_F(ActivityBufferPoolUtest, CheckCanAllocBufferWillSuccessWhenPoolHasFreeBuffer)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 1;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    EXPECT_TRUE(pool.CheckCanAllocBuffer());
    auto buffer = pool.GetBuffer();
    EXPECT_NE(nullptr, buffer);
    EXPECT_TRUE(pool.RecycleBuffer(buffer));
    EXPECT_TRUE(pool.CheckCanAllocBuffer());
    EXPECT_NE(nullptr, pool.GetBuffer());
}

TEST_F(ActivityBufferPoolUtest, CheckCanAllocBufferWillFailWhenPoolIsFull)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 1;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    EXPECT_TRUE(pool.CheckCanAllocBuffer());
    EXPECT_NE(nullptr, pool.GetBuffer());
    EXPECT_FALSE(pool.CheckCanAllocBuffer());
}

TEST_F(ActivityBufferPoolUtest, GetBufferWillFailWhenBufferSizeIsZero)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 0;
    const size_t poolSize = 1;
    EXPECT_FALSE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    EXPECT_EQ(nullptr, pool.GetBuffer());
}

TEST_F(ActivityBufferPoolUtest, GetBufferWillFailWhenPoolSizeIsZero)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 0;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_FALSE(pool.SetPoolSize(poolSize));
    EXPECT_EQ(nullptr, pool.GetBuffer());
}

TEST_F(ActivityBufferPoolUtest, GetBufferWillSuccessWhenPoolNotFull)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 2;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    EXPECT_NE(nullptr, pool.GetBuffer());
    EXPECT_NE(nullptr, pool.GetBuffer());
}

TEST_F(ActivityBufferPoolUtest, GetBufferWillSuccessWhenPoolHasFreeBuffer)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 1;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    auto buffer = pool.GetBuffer();
    EXPECT_NE(nullptr, buffer);
    EXPECT_TRUE(pool.RecycleBuffer(buffer));
    EXPECT_NE(nullptr, pool.GetBuffer());
}

TEST_F(ActivityBufferPoolUtest, GetBufferWillFailWhenPoolIsFull)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 1;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    EXPECT_NE(nullptr, pool.GetBuffer());
    EXPECT_EQ(nullptr, pool.GetBuffer());
}

TEST_F(ActivityBufferPoolUtest, RecycleBufferWillFailWhenBufferIsNullOrNotFromPool)
{
    ActivityBufferPool pool;
    const size_t bufferSize = 1 * MB;
    const size_t poolSize = 1;
    EXPECT_TRUE(pool.SetBufferSize(bufferSize));
    EXPECT_TRUE(pool.SetPoolSize(poolSize));
    EXPECT_FALSE(pool.RecycleBuffer(nullptr));
    uint8_t *buffer = new uint8_t[bufferSize];
    EXPECT_FALSE(pool.RecycleBuffer(buffer));
}
}
