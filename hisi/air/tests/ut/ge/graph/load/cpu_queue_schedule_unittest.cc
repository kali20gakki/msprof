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

#define private public
#define protected public
#include "graph/load/model_manager/cpu_queue_schedule.h"
#include "graph/def_types.h"
#include "runtime_stub.h"
#undef private
#undef protected

using namespace std;
extern std::string g_runtime_stub_mock;

namespace ge {
class UtestCpuQueueSchedule : public testing::Test {
 protected:
  void SetUp() {
    g_runtime_stub_mock = "";
  }

  void TearDown() {
    g_runtime_stub_mock = "";
  }
};

// test Init_CpuTaskZeroCopy_succ
TEST_F(UtestCpuQueueSchedule, CpuTaskZeroCopy_Init) {
  CpuTaskZeroCopy cpu_task_zero_copy(nullptr);
  std::vector<uintptr_t> mbuf_list;
  map<uint32_t, ZeroCopyOffset> outside_addrs;
  ZeroCopyOffset addr_mapping;
  addr_mapping.addr_count_ = 1;
  std::vector<uintptr_t> addr_offset{0x11110000U};
  uintptr_t addr = 0x12340000U;
  std::map<uintptr_t, std::vector<uintptr_t>> outside_addr{{addr, addr_offset}};
  addr_mapping.outside_addrs_.emplace_back(outside_addr);
  mbuf_list.emplace_back(addr);
  uint32_t index = 0;
  outside_addrs[index] = addr_mapping;
  std::vector<bool> no_tiling_list = {false};
  ZeroCpyArgs args = {
    .cpy_type = ZeroCpyType::kAllStatic,
    .has_no_tiling = false,
    .need_distribute = true
  };
  EXPECT_EQ(cpu_task_zero_copy.Init(mbuf_list, outside_addrs, no_tiling_list, args), SUCCESS);
}

TEST_F(UtestCpuQueueSchedule, CpuTaskZeroCopy_InitAddrs_fail) {
  std::vector<uintptr_t> mbuf_list;
  std::map<uint32_t, ZeroCopyOffset> outside_addrs;
  std::vector<bool> is_no_tiling_list;
  ZeroCpyArgs cpy_args;

  ZeroCopyOffset member;
  outside_addrs[0] = member;

  CpuTaskZeroCopy cpu_task_zero_copy(nullptr);
  EXPECT_EQ(cpu_task_zero_copy.InitAddrs(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args), PARAM_INVALID);

  std::map<uintptr_t, std::vector<uintptr_t>> m;
  m[0] = std::vector<uintptr_t>({0});
  outside_addrs[0].outside_addrs_.push_back(m);
  cpy_args.cpy_type = ZeroCpyType::kAllDynamic;
  is_no_tiling_list.push_back(true);

  //EXPECT_EQ(cpu_task_zero_copy.InitAddrs(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args), SUCCESS); //??
}

TEST_F(UtestCpuQueueSchedule, CpuTaskInfo_Init_args_valid) {
  CpuTaskZeroCopy cpu_task_zero_copy(nullptr);
  CpuTaskActiveEntry cpu_task_active_entry(nullptr);
  CpuTaskModelDequeue cpu_task_model_dequeue(nullptr);
  CpuTaskModelRepeat cpu_task_model_repeat(nullptr);
  CpuTaskWaitEndGraph cpu_task_wait_end_graph(nullptr);
  CpuTaskModelEnqueue cpu_task_model_enqueue(nullptr);
  CpuTaskProcessOutput cpu_task_post_dynamic_output(nullptr, ProcessStage::kPostDynamic);
  CpuTaskProcessOutput cpu_task_post_static_output(nullptr, ProcessStage::kPostStatic);
  CpuTaskProcessOutput cpu_task_prepare_output(nullptr, ProcessStage::kPrepare);
  EXPECT_EQ(cpu_task_zero_copy.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_active_entry.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_model_dequeue.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_model_repeat.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_wait_end_graph.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_model_enqueue.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_prepare_output.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_post_dynamic_output.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_post_static_output.Distribute(), FAILED);

  rtStream_t stream = (rtStream_t)99;
  cpu_task_zero_copy.stream_ = stream;
  cpu_task_active_entry.stream_ = stream;
  cpu_task_model_dequeue.stream_ = stream;
  cpu_task_model_repeat.stream_ = stream;
  cpu_task_wait_end_graph.stream_ = stream;
  cpu_task_model_enqueue.stream_ = stream;
  cpu_task_prepare_output.stream_ = stream;
  cpu_task_post_dynamic_output.stream_ = stream;
  cpu_task_post_static_output.stream_ = stream;

  cpu_task_zero_copy.args_ = ValueToPtr(99);
  cpu_task_zero_copy.args_size_ = 1;

  cpu_task_active_entry.args_ = ValueToPtr(99);
  cpu_task_active_entry.args_size_ = 1;

  cpu_task_model_dequeue.args_ = ValueToPtr(99);
  cpu_task_model_dequeue.args_size_ = 1;

  cpu_task_model_repeat.args_ = ValueToPtr(99);
  cpu_task_model_repeat.args_size_ = 1;

  cpu_task_wait_end_graph.args_ = ValueToPtr(99);
  cpu_task_wait_end_graph.args_size_ = 1;

  cpu_task_model_enqueue.args_ = ValueToPtr(99);
  cpu_task_model_enqueue.args_size_ = 1;

  cpu_task_prepare_output.args_ = ValueToPtr(99);
  cpu_task_prepare_output.args_size_ = 1;

  cpu_task_post_dynamic_output.args_ = ValueToPtr(99);
  cpu_task_post_dynamic_output.args_size_ = 1;

  cpu_task_post_static_output.args_ = ValueToPtr(99);
  cpu_task_post_static_output.args_size_ = 1;

  g_runtime_stub_mock = "rtCpuKernelLaunch";
  EXPECT_EQ(cpu_task_zero_copy.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_model_dequeue.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_model_repeat.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_wait_end_graph.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_model_enqueue.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_prepare_output.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_post_dynamic_output.Distribute(), FAILED);
  EXPECT_EQ(cpu_task_post_static_output.Distribute(), FAILED);

  g_runtime_stub_mock = "rtStreamActive";
  EXPECT_EQ(cpu_task_active_entry.Distribute(), FAILED);

  cpu_task_zero_copy.args_ = nullptr;
  cpu_task_active_entry.args_ = nullptr;
  cpu_task_model_dequeue.args_ = nullptr;
  cpu_task_model_repeat.args_ = nullptr;
  cpu_task_wait_end_graph.args_ = nullptr;
  cpu_task_model_enqueue.args_ = nullptr;
  cpu_task_prepare_output.args_ = nullptr;
  cpu_task_post_dynamic_output.args_ = nullptr;
  cpu_task_post_static_output.args_ = nullptr;
}

TEST_F(UtestCpuQueueSchedule, CpuTaskModelDequeue_Init_failed) {
  rtStream_t stream = nullptr;
  CpuTaskModelDequeue cpu_task_model_dequeue(stream);
  uint32_t queue_id = 0U;
  uintptr_t in_mbuf = 0U;

  domi::TaskDef task_def;
  DavinciModel *davinci_model = nullptr;
  EXPECT_EQ(cpu_task_model_dequeue.Init(task_def, davinci_model), SUCCESS);

  cpu_task_model_dequeue.args_size_ = 1;
  auto ret = cpu_task_model_dequeue.Init(queue_id, in_mbuf);
  EXPECT_EQ(ret, FAILED);

  cpu_task_model_dequeue.args_size_ = 0;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_NE(cpu_task_model_dequeue.Init(queue_id, in_mbuf), SUCCESS);

  g_runtime_stub_mock = "rtMemcpy";
  EXPECT_NE(cpu_task_model_dequeue.Init(queue_id, in_mbuf), SUCCESS);
}

TEST_F(UtestCpuQueueSchedule, CpuTaskZeroCopy_Init_failed) {
  rtStream_t stream = nullptr;
  CpuTaskZeroCopy cpu_task_zero_copy(stream);
  std::vector<uintptr_t> mbuf_list;
  std::map<uint32_t, ZeroCopyOffset> outside_addrs;
  std::vector<bool> is_no_tiling_list;
  ZeroCpyArgs cpy_args;

  EXPECT_EQ(cpu_task_zero_copy.Init(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args), SUCCESS);

  cpu_task_zero_copy.args_size_ = 1;
  auto ret = cpu_task_zero_copy.Init(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args);
  EXPECT_EQ(ret, FAILED);

  cpu_task_zero_copy.args_size_ = 0;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_EQ(cpu_task_zero_copy.Init(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args), SUCCESS);  //??

  g_runtime_stub_mock = "rtMemcpy";
  EXPECT_EQ(cpu_task_zero_copy.Init(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args), SUCCESS);  //??
}

TEST_F(UtestCpuQueueSchedule, CpuTaskProcessOutput_Init_failed) {
  rtStream_t stream = nullptr;
  ProcessStage stage;
  CpuTaskProcessOutput cpu_task_process_output(stream, stage);
  uintptr_t addr = 0U;
  uint32_t size = 0U;
  uintptr_t in_mbuf = 0U;
  uintptr_t out_mbuf = 0U;

  EXPECT_EQ(cpu_task_process_output.Init(addr, size, in_mbuf, out_mbuf), SUCCESS);

  cpu_task_process_output.args_size_ = 1;
  auto ret = cpu_task_process_output.Init(addr, size, in_mbuf, out_mbuf);
  EXPECT_EQ(ret, FAILED);

  cpu_task_process_output.args_size_ = 0;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_NE(cpu_task_process_output.Init(addr, size, in_mbuf, out_mbuf), SUCCESS);

  g_runtime_stub_mock = "rtMemcpy";
  EXPECT_NE(cpu_task_process_output.Init(addr, size, in_mbuf, out_mbuf), SUCCESS);
}

TEST_F(UtestCpuQueueSchedule, CpuTaskModelEnqueue_Init_failed) {
  rtStream_t stream = nullptr;
  CpuTaskModelEnqueue cpu_task_model_enqueue(stream);
  uint32_t queue_id = 0;
  uintptr_t out_mbuf = 0U;

  EXPECT_EQ(cpu_task_model_enqueue.Init(queue_id, out_mbuf), SUCCESS);

  cpu_task_model_enqueue.args_size_ = 1;
  auto ret = cpu_task_model_enqueue.Init(queue_id, out_mbuf);
  EXPECT_EQ(ret, FAILED);

  cpu_task_model_enqueue.args_size_ = 0;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_NE(cpu_task_model_enqueue.Init(queue_id, out_mbuf), SUCCESS);

  g_runtime_stub_mock = "rtMemcpy";
  EXPECT_NE(cpu_task_model_enqueue.Init(queue_id, out_mbuf), SUCCESS);
}

TEST_F(UtestCpuQueueSchedule, CpuTaskActiveEntry_Init_failed) {
  rtStream_t stream = nullptr;
  CpuTaskActiveEntry cpu_task_active_entry(stream);
  rtStream_t stream2 = nullptr;

  EXPECT_NE(cpu_task_active_entry.Init(stream2), SUCCESS);   //??

  cpu_task_active_entry.args_size_ = 1;
  auto ret = cpu_task_active_entry.Init(stream2);
  EXPECT_EQ(ret, FAILED);

  cpu_task_active_entry.args_size_ = 0;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_NE(cpu_task_active_entry.Init(stream2), SUCCESS);

  g_runtime_stub_mock = "rtMemcpy";
  EXPECT_NE(cpu_task_active_entry.Init(stream2), SUCCESS);
}

TEST_F(UtestCpuQueueSchedule, CpuTaskWaitEndGraph_Init_failed) {
  rtStream_t stream = nullptr;
  CpuTaskWaitEndGraph cpu_task_wait_end_graph(stream);
  uint32_t model_id = 0U;

  EXPECT_EQ(cpu_task_wait_end_graph.Init(model_id), SUCCESS);

  cpu_task_wait_end_graph.args_size_ = 1;
  auto ret = cpu_task_wait_end_graph.Init(model_id);
  EXPECT_EQ(ret, FAILED);

  cpu_task_wait_end_graph.args_size_ = 0;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_NE(cpu_task_wait_end_graph.Init(model_id), SUCCESS);

  g_runtime_stub_mock = "rtMemcpy";
  EXPECT_NE(cpu_task_wait_end_graph.Init(model_id), SUCCESS);
}

TEST_F(UtestCpuQueueSchedule, CpuTaskModelRepeat_Init_failed) {
  rtStream_t stream = nullptr;
  CpuTaskModelRepeat cpu_task_model_repeat(stream);
  uint32_t model_id = 0U;

  EXPECT_EQ(cpu_task_model_repeat.Init(model_id), SUCCESS);

  cpu_task_model_repeat.args_size_ = 1;
  auto ret = cpu_task_model_repeat.Init(model_id);
  EXPECT_EQ(ret, FAILED);

  cpu_task_model_repeat.args_size_ = 0;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_NE(cpu_task_model_repeat.Init(model_id), SUCCESS);

  g_runtime_stub_mock = "rtMemcpy";
  EXPECT_NE(cpu_task_model_repeat.Init(model_id), SUCCESS);
}

}  // namespace ge
