/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>
#include <memory>

#define protected public
#define private public
#include "graph/manager/rdma_pool_allocator.h"
#include <framework/common/debug/log.h>
#include "framework/common/debug/ge_log.h"
#include "graph/ge_context.h"
#include "runtime/dev.h"
#include "graph/manager/graph_mem_manager.h"
#undef protected
#undef private

namespace ge {
class UtestRdmaPoolAllocatorTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestRdmaPoolAllocatorTest, RdmaPoolAllocator) {
  RdmaPoolAllocator allocator(RT_MEMORY_HBM);
  Status ret = allocator.Initialize();
  ret = allocator.InitMemory(1024*1024);
  uint32_t device_id = 0;
  uint8_t *addr = allocator.Malloc(1024, device_id);
  uint64_t base_addr; 
  uint64_t mem_size;
  ret = allocator.GetBaseAddr(base_addr, mem_size);
  ret = allocator.Free(addr, device_id);
}

TEST_F(UtestRdmaPoolAllocatorTest, malloc_with_reuse) {
  std::vector<rtMemType_t> mem_type;
  mem_type.push_back(RT_MEMORY_HBM);
  EXPECT_EQ(SUCCESS, MemManager::Instance().Initialize(mem_type));
  size_t size = 1024 * 10;
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).InitMemory(size));
  size_t node_mem = 512;
  auto node1 = MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Malloc(node_mem);
  EXPECT_NE(nullptr, node1);
  auto node2 = MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Malloc(node_mem);
  EXPECT_NE(nullptr, node2);
  auto node3 = MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Malloc(node_mem);
  EXPECT_NE(nullptr, node3);
  auto node4 = MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Malloc(node_mem);
  EXPECT_NE(nullptr, node4);
  auto node5 = MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Malloc(node_mem);
  EXPECT_NE(nullptr, node5);

  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Free(node1));
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Free(node3));
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Free(node2));
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Free(node4));
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Free(node5));
  MemManager::Instance().Finalize();
}

TEST_F(UtestRdmaPoolAllocatorTest, initrdma_success) {
  std::vector<rtMemType_t> mem_type;
  mem_type.push_back(RT_MEMORY_HBM);
  EXPECT_EQ(SUCCESS, MemManager::Instance().Initialize(mem_type));
  size_t size = 1024;
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).InitMemory(size));
  MemManager::Instance().Finalize();
}

TEST_F(UtestRdmaPoolAllocatorTest, repeat_initrdma) {
  std::vector<rtMemType_t> mem_type;
  mem_type.push_back(RT_MEMORY_HBM);
  EXPECT_EQ(SUCCESS, MemManager::Instance().Initialize(mem_type));
  size_t size = 1024;
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).InitMemory(size));
  EXPECT_NE(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).InitMemory(size));
  MemManager::Instance().Finalize();
}

TEST_F(UtestRdmaPoolAllocatorTest, malloc_falled) {
  std::vector<rtMemType_t> mem_type;
  mem_type.push_back(RT_MEMORY_HBM);
  EXPECT_EQ(SUCCESS, MemManager::Instance().Initialize(mem_type));
  size_t size = 1024 * 10;
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).InitMemory(size));
  EXPECT_EQ(nullptr, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Malloc(size * 2));
  MemManager::Instance().Finalize();
}

TEST_F(UtestRdmaPoolAllocatorTest, get_baseaddr) {
  std::vector<rtMemType_t> mem_type;
  mem_type.push_back(RT_MEMORY_HBM);
  EXPECT_EQ(SUCCESS, MemManager::Instance().Initialize(mem_type));
  uint64_t base_addr;
  uint64_t base_size;
  EXPECT_NE(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).GetBaseAddr(base_addr, base_size));
  size_t size = 1024;
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).InitMemory(size));
  EXPECT_EQ(SUCCESS, MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).GetBaseAddr(base_addr, base_size));
  MemManager::Instance().Finalize();
}

} // namespace ge
