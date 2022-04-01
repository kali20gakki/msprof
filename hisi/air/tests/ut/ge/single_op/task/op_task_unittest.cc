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
#include <vector>

#define protected public
#define private public
#include "single_op/task/op_task.h"
#include "graph/utils/graph_utils.h"
#include "aicpu/common/aicpu_task_struct.h"
using namespace std;
using namespace testing;
using namespace ge;
using namespace optiling;

class UtestOpTask : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestOpTask, test_tbe_launch_kernel) {
  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("Add", "Add");
  GeTensorDesc desc;
  op_desc->AddInputDesc(desc);
  op_desc->AddOutputDesc(desc);
  auto node = graph->AddNode(op_desc);

  TbeOpTask task;
  ge::DataBuffer data_buffer;
  vector<GeTensorDesc> input_desc;
  vector<DataBuffer> input_buffers = { data_buffer };
  vector<GeTensorDesc> output_desc;
  vector<DataBuffer> output_buffers = { data_buffer };
  task.op_desc_ = op_desc;
  auto op = OpDescUtils::CreateOperatorFromNode(node);
  task.op_ = std::move(std::unique_ptr<Operator>(new(std::nothrow) Operator(op)));
  task.node_ = node;
  OpTilingFuncV2 op_tiling_func = [](const ge::Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &) -> bool {return true;};
  REGISTER_OP_TILING_UNIQ_V2(Add, op_tiling_func, 1);
  OpTilingRegistryInterf_V2("Add", op_tiling_func);
  ge::AttrUtils::SetStr(op_desc, "compile_info_key", "op_compile_info_key");
  ge::AttrUtils::SetStr(op_desc, "compile_info_json", "op_compile_info_json");
  char c = '0';
  char* buffer = &c;
  task.max_tiling_size_ = 64;
  task.need_tiling_ = true;
  task.arg_index_ = {0};
  task.input_num_ = 1;
  task.output_num_ = 1;
  task.arg_size_ = sizeof(void *) * 3 + 64;
  task.args_.reset(new (std::nothrow) uint8_t[sizeof(void *) * 3 + 64]);
  task.args_ex_.args = task.args_.get();
  task.args_ex_.argsSize = task.arg_size_;
  task.run_info_= MakeUnique<optiling::utils::OpRunInfo>();
  ASSERT_NE(task.run_info_, nullptr);

  rtStream_t stream_ = nullptr;
  ASSERT_EQ(task.LaunchKernel(input_desc, input_buffers, output_desc, output_buffers, stream_), SUCCESS);
  char *handle = "00";
  task.SetHandle(handle);
  ASSERT_EQ(task.LaunchKernel(input_desc, input_buffers, output_desc, output_buffers, stream_), SUCCESS);
}

TEST_F(UtestOpTask, test_update_args) {
  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("Add", "Add");
  auto node = graph->AddNode(op_desc);

  TbeOpTask task;
  task.EnableDynamicSupport(node, 64);
  task.max_tiling_size_ = 64;
  task.input_num_ = 1;
  task.output_num_ = 1;
  task.arg_size_ = sizeof(void *) * 3 + 64 + kMaxHostMemInputLen;
  task.args_.reset(new (std::nothrow) uint8_t[task.arg_size_]);
  task.args_ex_.args = task.args_.get();
  task.args_ex_.argsSize = task.arg_size_;
  task.args_item_offsets_.tiling_addr_offset = sizeof(void *) * 2;
  task.args_item_offsets_.tiling_data_offset = sizeof(void *) * 3;
  task.arg_num = 2;
  task.run_info_ = MakeUnique<optiling::utils::OpRunInfo>();
  ASSERT_NE(task.run_info_, nullptr);

  EXPECT_EQ(task.UpdateTilingArgs(), SUCCESS);
  uint64_t *new_tiling_addr =
      PtrToPtr<void, uint64_t>(ValueToPtr(PtrToValue(task.args_.get()) + task.args_item_offsets_.tiling_addr_offset));
  char_t *new_tiling_data =
      PtrToPtr<void, char_t>(ValueToPtr(PtrToValue(task.args_.get()) + task.args_item_offsets_.tiling_data_offset));
  EXPECT_EQ(*new_tiling_addr, PtrToValue(new_tiling_data));
  size_t arg_count = 0;
  uintptr_t *arg_base = nullptr;
  task.GetIoAddr(arg_base, arg_count);
  EXPECT_EQ(arg_base, reinterpret_cast<uintptr_t *>(task.args_.get()));
  ASSERT_NE(arg_count, 2);

  AtomicAddrCleanOpTask atomic_task;
  atomic_task.node_ = node;
  atomic_task.need_tiling_ = true;
  atomic_task.arg_size_ = sizeof(void *) * 3 + 64;
  atomic_task.args_.reset(new (std::nothrow) uint8_t[sizeof(void *) * 3 + 64]);
  atomic_task.args_ex_.args = atomic_task.args_.get();
  atomic_task.args_ex_.argsSize = atomic_task.arg_size_;
  atomic_task.run_info_ = MakeUnique<optiling::utils::OpRunInfo>();
  ASSERT_NE(atomic_task.run_info_, nullptr);

  atomic_task.max_tiling_size_ = 64;
  EXPECT_EQ(atomic_task.UpdateTilingArgs(), SUCCESS);
  EXPECT_EQ(atomic_task.args_ex_.tilingAddrOffset, 0);
}

// for dynamic single op
TEST_F(UtestOpTask, test_tbe_update_args) {
  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("Add", "Add");
  GeTensorDesc tensor_desc;
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);
  size_t total_size = 0U;

  TbeOpTask task;
  task.node_ = node;
  task.input_num_ = 2;
  task.output_num_ = 1;
  task.arg_size_ = 64;
  task.args_ = MakeUnique<uint8_t []>(task.arg_size_);
  task.args_ex_.args = task.args_.get();
  task.args_ex_.argsSize = task.arg_size_;
  task.arg_num = 3;

  // input + output
  total_size += (task.arg_num * sizeof(void *));
  task.op_desc_ = op_desc;

  op_desc->SetIsInputConst({false, false});

  // workspace
  task.workspaces_.emplace_back((void *)222);
  task.workspaces_.emplace_back((void *)333);
  total_size += (task.workspaces_.size() * sizeof(void *));

  // tiling
  task.max_tiling_size_ = 64;
  task.need_tiling_ = true;
  total_size += task.max_tiling_size_; // tiling data
  total_size += sizeof(void *); // tiling addr
  
  // overflow
  uint64_t overflow_value = 111;
  task.overflow_addr_ = &overflow_value;
  total_size += sizeof(void *);

  // host mem input
  task.need_host_mem_opt_ = true;
  task.extend_args_for_host_input_ = true;
  task.args_item_offsets_.host_input_data_offset =
      (task.arg_num + 1 + task.workspaces_.size() + 1) * sizeof(uintptr_t) + task.max_tiling_size_;
  task.arg_index_.emplace_back(0);
  task.arg_index_.emplace_back(1);
  total_size += kMaxHostMemInputLen;

  // input
  uint64_t host_value0 = 333;
  DataBuffer buffer0;
  buffer0.placement = kHostMemType;
  buffer0.length = 8;
  buffer0.data = &host_value0;

  uint64_t host_value1[4] = {444, 555, 666, 777};
  DataBuffer buffer1;
  buffer1.placement = kHostMemType;
  buffer1.length = 32;
  buffer1.data = &host_value1[0];

  vector<DataBuffer> inputs;
  inputs.emplace_back(buffer1);
  inputs.emplace_back(buffer0);

  EXPECT_EQ(task.UpdateArgsItem(inputs, {}), SUCCESS);
  // chck args size
  EXPECT_EQ(task.arg_size_, total_size);

  // check overflow offset
  size_t overflow_addr_offset = (task.arg_num + 1 + task.workspaces_.size()) * sizeof(uintptr_t);
  EXPECT_EQ(task.args_item_offsets_.overflow_addr_offset, overflow_addr_offset);
  // check overflow addr
  uint64_t *args_base = reinterpret_cast<uint64_t *>(task.args_.get());
  EXPECT_EQ(args_base[overflow_addr_offset / sizeof(uintptr_t)], PtrToValue(&overflow_value));

  // check workspace offset
  size_t ws_offset = task.arg_num * sizeof(uintptr_t);
  EXPECT_EQ(task.args_item_offsets_.workspace_addr_offset, ws_offset);
  // check workspace addr
  EXPECT_EQ(args_base[ws_offset / sizeof(uintptr_t)], PtrToValue(task.workspaces_.at(0U)));

  // check tiling addr offset、tiling data offset
  size_t tiling_addr_offset = (task.arg_num + task.workspaces_.size()) * sizeof(uintptr_t);
  EXPECT_EQ(task.args_item_offsets_.tiling_addr_offset, tiling_addr_offset);
  size_t tiling_data_offset = (task.arg_num + 1 + task.workspaces_.size() + 1) * sizeof(uintptr_t);
  EXPECT_EQ(task.args_item_offsets_.tiling_data_offset, tiling_data_offset);
  uint64_t tiling_data_addr_in_args = PtrToValue(args_base + tiling_data_offset / sizeof(uintptr_t));
  EXPECT_EQ(args_base[tiling_addr_offset / sizeof(uintptr_t)], tiling_data_addr_in_args);

  EXPECT_EQ(task.args_ex_.tilingAddrOffset, tiling_addr_offset);
  EXPECT_EQ(task.args_ex_.tilingDataOffset, tiling_data_offset);
  EXPECT_TRUE(task.args_ex_.hasTiling);

  // check host mem offset
  size_t host_mem0_offset = tiling_data_offset + task.max_tiling_size_;
  EXPECT_EQ(task.args_item_offsets_.host_input_data_offset, host_mem0_offset);
  uint64_t host_mem0_data_in_args = PtrToValue(args_base + host_mem0_offset / sizeof(uintptr_t));
  EXPECT_EQ(args_base[0U], host_mem0_data_in_args);
  uint64_t host_mem1_data_in_args = host_mem0_data_in_args + inputs.at(0).length;
  EXPECT_EQ(args_base[1U], host_mem1_data_in_args);

  // check host mem data
  EXPECT_EQ(args_base[host_mem0_offset / sizeof(uintptr_t)], host_value1[0U]);
  auto host_mem1_offset = host_mem0_offset + inputs.at(0).length;
  EXPECT_EQ(args_base[host_mem1_offset / sizeof(uintptr_t)], host_value0);

  // check host mem args
  ASSERT_NE(task.args_ex_.hostInputInfoPtr, nullptr);
  EXPECT_EQ(task.args_ex_.hostInputInfoNum, inputs.size());
  EXPECT_EQ(task.args_ex_.hostInputInfoPtr[0].dataOffset, host_mem0_offset);
  EXPECT_EQ(task.args_ex_.hostInputInfoPtr[1].dataOffset, host_mem1_offset);
  EXPECT_EQ(task.args_ex_.hostInputInfoPtr[0].addrOffset, 0);
  EXPECT_EQ(task.args_ex_.hostInputInfoPtr[1].addrOffset, sizeof(uintptr_t));

}

TEST_F(UtestOpTask, test_tbe_update_args_notiling) {
  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("Add", "Add");
  GeTensorDesc tensor_desc;
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);
  size_t total_size = 0U;

  TbeOpTask task;
  task.node_ = node;
  task.input_num_ = 2;
  task.output_num_ = 1;
  task.arg_size_ = 64;
  task.args_ = MakeUnique<uint8_t []>(task.arg_size_);
  task.args_ex_.args = task.args_.get();
  task.args_ex_.argsSize = task.arg_size_;
  task.arg_num = 3;

  // input + output
  total_size += (task.arg_num * sizeof(void *));
  task.op_desc_ = op_desc;

  op_desc->SetIsInputConst({false, false});

  // workspace
  task.workspaces_.emplace_back((void *)222);
  task.workspaces_.emplace_back((void *)333);
  total_size += (task.workspaces_.size() * sizeof(void *));

  // tiling
  task.max_tiling_size_ = 64;
  task.need_tiling_ = false;
  total_size += task.max_tiling_size_; // tiling data
  total_size += sizeof(void *); // tiling addr
  
  // overflow
  uint64_t overflow_value = 111;
  task.overflow_addr_ = &overflow_value;
  total_size += sizeof(void *);

  // host mem input
  task.need_host_mem_opt_ = true;
  task.extend_args_for_host_input_ = true;
  task.args_item_offsets_.host_input_data_offset =
      (task.arg_num + 1 + task.workspaces_.size() + 1) * sizeof(uintptr_t) + task.max_tiling_size_;
  task.arg_index_.emplace_back(0);
  task.arg_index_.emplace_back(1);
  total_size += kMaxHostMemInputLen;

  // input
  uint64_t host_value0 = 333;
  DataBuffer buffer0;
  buffer0.placement = kHostMemType;
  buffer0.length = 8;
  buffer0.data = &host_value0;

  uint64_t host_value1[4] = {444, 555, 666, 777};
  DataBuffer buffer1;
  buffer1.placement = kHostMemType;
  buffer1.length = 32;
  buffer1.data = &host_value1[0];

  vector<DataBuffer> inputs;
  inputs.emplace_back(buffer1);
  inputs.emplace_back(buffer0);

  EXPECT_EQ(task.UpdateArgsItem(inputs, {}), SUCCESS);
}

TEST_F(UtestOpTask, test_tf_aicpu_update_host_mem_input_args) {
  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("Add", "Add");
  GeTensorDesc tensor_desc;
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);

  AiCpuTask task;
  size_t io_num = 3U;
  // host args
  task.arg_size_ = io_num * sizeof(void *) + kMaxHostMemInputLen;
  // tf cpu input address must be 64 bytes aligned
  task.arg_size_ = (task.arg_size_ + 64 - 1) / 64 * 64;
  uint8_t args[task.arg_size_];
  task.args_ = args;
  for (size_t i = 0; i < task.arg_size_ / sizeof(void *); i++) {
    task.io_addr_host_.emplace_back(nullptr);
  }

  // device args
  task.io_addr_size_ = task.arg_size_;
  uint8_t device_args[task.io_addr_size_];
  task.io_addr_ = device_args;

  task.op_desc_ = op_desc;
  op_desc->SetIsInputConst({false, false});

  // host mem input
  task.need_host_mem_opt_ = true;
  task.host_mem_input_data_offset_ = task.arg_size_ - kMaxHostMemInputLen;

  // input
  uint64_t host_value0 = 333;
  DataBuffer buffer0;
  buffer0.placement = kHostMemType;
  buffer0.length = 8;
  buffer0.data = &host_value0;

  uint64_t host_value1[4] = {444, 555, 666, 777};
  DataBuffer buffer1;
  buffer1.placement = kHostMemType;
  buffer1.length = 32;
  buffer1.data = &host_value1[0];

  vector<DataBuffer> inputs;
  inputs.emplace_back(buffer1);
  inputs.emplace_back(buffer0);

  EXPECT_EQ(task.UpdateHostMemInputArgs(inputs, {}), SUCCESS);

  // check host mem data
  uint64_t *args_base = reinterpret_cast<uint64_t *>(task.io_addr_host_.data());
  size_t host_mem0_data_offset = task.host_mem_input_data_offset_;
  EXPECT_EQ(args_base[host_mem0_data_offset / sizeof(void *)], host_value1[0]);
  size_t host_mem1_data_offset = host_mem0_data_offset + buffer1.length;
  // tf cpu input address must be 64 bytes aligned
  host_mem1_data_offset = (host_mem1_data_offset + 64 - 1) / 64 * 64;
  EXPECT_EQ(args_base[host_mem1_data_offset / sizeof(void *)], host_value0);

  // check host mem addr
  size_t addr0_offset = 0U;
  size_t addr1_offset = sizeof(void *);
  uint64_t device_io_addr = PtrToValue(task.io_addr_);
  EXPECT_EQ(args_base[addr0_offset / sizeof(void *)], device_io_addr + host_mem0_data_offset);
  EXPECT_EQ(args_base[addr1_offset / sizeof(void *)], device_io_addr + host_mem1_data_offset);
  task.args_ = nullptr;
  task.io_addr_ = nullptr;
}

TEST_F(UtestOpTask, test_aicpu_update_host_mem_input_args) {
  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("Add", "Add");
  GeTensorDesc tensor_desc;
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);

  AiCpuCCTask task;
  size_t io_num = 3U;
  // host args
  task.arg_size_ = sizeof(aicpu::AicpuParamHead) + io_num * sizeof(void *) + kMaxHostMemInputLen;
  task.args_ = MakeUnique<uint8_t[]>(task.arg_size_);
  task.io_addr_ = PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(task.args_.get()) +
                                                       sizeof(aicpu::AicpuParamHead)));
  task.io_addr_num_ = io_num;

  task.op_desc_ = op_desc;
  op_desc->SetIsInputConst({false, false});

  // host mem input
  task.need_host_mem_opt_ = true;
  task.host_mem_input_data_offset_ = io_num * sizeof(void *);

  // input
  uint64_t host_value0 = 333;
  DataBuffer buffer0;
  buffer0.placement = kHostMemType;
  buffer0.length = 8;
  buffer0.data = &host_value0;

  uint64_t host_value1[4] = {444, 555, 666, 777};
  DataBuffer buffer1;
  buffer1.placement = kHostMemType;
  buffer1.length = 32;
  buffer1.data = &host_value1[0];

  vector<DataBuffer> inputs;
  inputs.emplace_back(buffer1);
  inputs.emplace_back(buffer0);

  EXPECT_EQ(task.UpdateHostMemInputArgs(inputs, {}), SUCCESS);

  // check host mem data
  uint64_t *args_base = reinterpret_cast<uint64_t *>(task.io_addr_);
  size_t host_mem0_data_offset = task.host_mem_input_data_offset_;
  EXPECT_EQ(args_base[host_mem0_data_offset / sizeof(void *)], host_value1[0]);
  size_t host_mem1_data_offset = host_mem0_data_offset + buffer1.length;
  EXPECT_EQ(args_base[host_mem1_data_offset / sizeof(void *)], host_value0);

  // check host mem addr
  size_t addr0_offset = 0U;
  size_t addr1_offset = sizeof(void *);
  uint64_t io_addr = PtrToValue(task.io_addr_);
  EXPECT_EQ(args_base[addr0_offset / sizeof(void *)], io_addr + host_mem0_data_offset);
  EXPECT_EQ(args_base[addr1_offset / sizeof(void *)], io_addr + host_mem1_data_offset);

  // check task.args_ex_
  ASSERT_FALSE(task.args_ex_.hasTiling);
  ASSERT_NE(task.args_ex_.hostInputInfoPtr, nullptr);
  EXPECT_EQ(task.args_ex_.hostInputInfoNum, 2);
  EXPECT_EQ(task.args_ex_.hostInputInfoPtr[0].dataOffset, host_mem0_data_offset + sizeof(aicpu::AicpuParamHead));
  EXPECT_EQ(task.args_ex_.hostInputInfoPtr[1].dataOffset, host_mem1_data_offset + sizeof(aicpu::AicpuParamHead));
  task.args_ = nullptr;
  task.io_addr_ = nullptr;
}