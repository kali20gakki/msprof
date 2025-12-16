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
 * -------------------------------------------------------------------------
*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "mspti/csrc/activity_buffer_pool.h"

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
