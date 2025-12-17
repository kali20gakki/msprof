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
#include <iostream>
#include "errno/error_code.h"
#include "memory/chunk_pool.h"

using namespace analysis::dvvp::common::memory;

class COMMON_CHUNK_POOL_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(COMMON_CHUNK_POOL_TEST, chunk) {
    GlobalMockObject::verify();

    auto chunk = std::make_shared<analysis::dvvp::common::memory::Chunk>(0);
    EXPECT_EQ(true, chunk->Init());
}

TEST_F(COMMON_CHUNK_POOL_TEST, chunk_pool) {
    GlobalMockObject::verify();

    auto chunk_pool = std::make_shared<analysis::dvvp::common::memory::ChunkPool>(0, 16);
    EXPECT_EQ(false, chunk_pool->Init());

    chunk_pool->poolSize_  = 1;
    chunk_pool->Init();

    auto chunk1 = chunk_pool->TryAlloc();
    auto chunk2 = chunk_pool->TryAlloc();
    chunk_pool->Uninit();
}
