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
#include "graph/manager/host_mem_manager.h"
#undef protected
#undef private

namespace ge {
class UtestSharedMemAllocatorTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestSharedMemAllocatorTest, malloc_zero_size) {
  SharedMemAllocator allocator;
  string var_name = "host_params";
  SharedMemInfo info;
  uint8_t tmp(0);
  info.device_address = &tmp;

  std::shared_ptr<AlignedPtr> aligned_ptr = std::make_shared<AlignedPtr>(100, 16);

  info.host_aligned_ptr = aligned_ptr;
  info.fd=0;
  info.mem_size = 100;
  info.op_name = var_name;
  Status ret = allocator.Allocate(info);
  ret = allocator.DeAllocate(info);
}
} // namespace ge
