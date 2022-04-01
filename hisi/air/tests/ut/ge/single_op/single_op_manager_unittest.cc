/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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
#include <vector>

#include "runtime/rt.h"

#define protected public
#define private public
#include "single_op/single_op_manager.h"
#include "hybrid/common/npu_memory_allocator.h"
#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace ge;

class UtestSingleOpManager : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

TEST_F(UtestSingleOpManager, test_get_resource) {
  rtStream_t stream = (rtStream_t)0x01;
  auto &instance = SingleOpManager::GetInstance();
  ASSERT_NE(instance.GetResource(0x01, stream), nullptr);
}
/*
TEST_F(UtestSingleOpManager, test_get_op_from_model) {
  auto stream = (rtStream_t)0x1;
  uintptr_t resource_id = 0x1;
  auto &instance = SingleOpManager::GetInstance();

  SingleOp *single_op = nullptr;
  ModelData model_data;
  string model_str = "123456789";
  model_data.model_data = (void *)model_str.c_str();
  model_data.model_len = model_str.size();

  ASSERT_EQ(instance.GetOpFromModel("model", model_data, stream, &single_op), FAILED);
  ASSERT_EQ(instance.GetResource(resource_id, stream)->GetOperator(model_data.model_data), nullptr);
}
*/
TEST_F(UtestSingleOpManager, test_relesase_resource) {
  auto stream = (rtStream_t)0x99;
  auto &instance = SingleOpManager::GetInstance();

  auto allocator = hybrid::NpuMemoryAllocator::GetAllocator(stream);
  ASSERT_NE(allocator, nullptr);
  ASSERT_EQ(instance.ReleaseResource(stream), SUCCESS);
  instance.GetResource(0x99, stream);
  ASSERT_EQ(instance.ReleaseResource(stream), SUCCESS);
}

TEST_F(UtestSingleOpManager, test_delete_resource) {
  int32_t fake_stream = 0;
  uint64_t op_id = 999;
  auto stream = (rtStream_t)fake_stream;
  auto &instance = SingleOpManager::GetInstance();
  uintptr_t resource_id = 0U;
  ASSERT_EQ(instance.GetResourceId(stream, resource_id), SUCCESS);
  auto res = instance.GetResource(resource_id, stream);
  ASSERT_NE(res, nullptr);
  auto new_op = MakeUnique<SingleOp>(res, &res->stream_mu_, res->stream_);
  res->op_map_[op_id] = std::move(new_op);
  auto new_dynamic_op = MakeUnique<DynamicSingleOp>(&res->tensor_pool_, res->resource_id_, &res->stream_mu_, res->stream_);
  res->dynamic_op_map_[op_id] = std::move(new_dynamic_op);
  ASSERT_EQ(ge::GeExecutor::UnloadSingleOp(op_id), SUCCESS);
  ASSERT_EQ(ge::GeExecutor::UnloadDynamicSingleOp(op_id), SUCCESS);
  ASSERT_EQ(instance.ReleaseResource(stream), SUCCESS);
}


/*
TEST_F(UtestSingleOpManager, test_get_op_from_model_with_null_stream) {
  void *stream = nullptr;

  SingleOp *single_op = nullptr;
  ModelData model_data;
  string model_str = "123456789";
  model_data.model_data = (void *)model_str.c_str();
  model_data.model_len = model_str.size();
  auto &instance = SingleOpManager::GetInstance();

  ASSERT_EQ(instance.GetOpFromModel("model", model_data, stream, &single_op), FAILED);
}

TEST_F(UtestSingleOpManager, get_resource_failed) {
  auto stream = (rtStream_t)0x1;

  SingleOp *single_op = nullptr;
  ModelData model_data;
  string model_str = "123456789";
  model_data.model_data = (void *)model_str.c_str();
  model_data.model_len = model_str.size();
  auto &instance = SingleOpManager::GetInstance();

  ASSERT_EQ(instance.GetOpFromModel("model", model_data, stream, &single_op), FAILED);
}
*/
