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

#define protected public
#define private public
#include "hybrid/node_executor/task_context.h"
#include "hybrid/model/graph_item.h"
#include "hybrid/executor/subgraph_context.h"
#include "hybrid/executor/node_state.h"
#include "graph/load/model_manager/model_manager.h"
#include "common/dump/exception_dumper.h"
#include "common/debug/log.h"
#include "common/ge_inner_error_codes.h"
#undef private
#undef protected

namespace ge {
class UTEST_dump_exception : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UTEST_dump_exception, SaveProfilingTaskDescInfo) {
  hybrid::GraphItem *graph_item = nullptr;
  hybrid::GraphExecutionContext *exec_ctx = nullptr;
  hybrid::SubgraphContext subgraph_context(graph_item, exec_ctx);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  OpDescPtr op_desc = std::make_shared<OpDesc>("name", "type");
  NodePtr node = std::make_shared<Node>(op_desc, graph);
  hybrid::NodeItem node_item = hybrid::NodeItem(node);
  node_item.input_start = 0;
  node_item.output_start = 0;
  hybrid::FrameState frame_state = hybrid::FrameState();
  hybrid::NodeState node_state(node_item, &subgraph_context, frame_state);
  auto task_context = hybrid::TaskContext::Create(&node_state, &subgraph_context);
  task_context->task_id_ = 1;
  EXPECT_EQ(task_context->SaveProfilingTaskDescInfo("test", 1, "test"), SUCCESS);
}

TEST_F(UTEST_dump_exception, SaveProfilingTaskDescInfo_fail) {
  hybrid::GraphItem *graph_item = nullptr;
  hybrid::GraphExecutionContext *exec_ctx = nullptr;
  hybrid::SubgraphContext subgraph_context(graph_item, exec_ctx);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  OpDescPtr op_desc = std::make_shared<OpDesc>("name", "type");
  NodePtr node = std::make_shared<Node>(op_desc, graph);
  hybrid::NodeItem node_item = hybrid::NodeItem(node);
  node_item.input_start = 0;
  node_item.output_start = 0;
  hybrid::FrameState frame_state = hybrid::FrameState();
  hybrid::NodeState node_state(node_item, &subgraph_context, frame_state);
  auto task_context = hybrid::TaskContext::Create(&node_state, &subgraph_context);
  ModelManager::GetInstance().dump_exception_flag_ = true;
  task_context->task_id_ = 999;
  EXPECT_EQ(task_context->SaveProfilingTaskDescInfo("test", 1, "test"), -1);
}

TEST_F(UTEST_dump_exception, save_dump_op_info_success) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("GatherV2", "GatherV2");
  uint32_t task_id = 1;
  uint32_t stream_id = 233;
  ExtraOpInfo extra_op_info;
  ExceptionDumper exception_dumper;
  exception_dumper.SaveDumpOpInfo(op_desc, task_id, stream_id, extra_op_info);
}

TEST_F(UTEST_dump_exception, dump_exception_info) {
  const char_t * const kEnvRecordPath = "NPU_COLLECT_PATH";
  char_t npu_collect_path[MMPA_MAX_PATH] = "valid_path";
  mmSetEnv(kEnvRecordPath, &npu_collect_path[0U], MMPA_MAX_PATH);
  rtExceptionInfo exception_info = {1, 2, 3, 4, 5};
  std::vector<rtExceptionInfo> exception_infos = { exception_info };

  std::vector<uint8_t> input_stub(8);
  std::vector<uint8_t> output_stub(8);
  std::vector<void *> mock_arg{input_stub.data(), output_stub.data()};

  OpDescInfo op_desc_info;
  op_desc_info.op_name = "Save";
  op_desc_info.op_type = "Save";
  op_desc_info.task_id = 1;
  op_desc_info.stream_id = 2;
  op_desc_info.args = reinterpret_cast<uintptr_t>(mock_arg.data());
  op_desc_info.imply_type = static_cast<uint32_t>(domi::ImplyType::TVM);
  op_desc_info.input_format = {FORMAT_NCHW};
  op_desc_info.input_shape = {{1}};
  op_desc_info.input_data_type = {DT_FLOAT};
  op_desc_info.input_addrs = {nullptr};
  op_desc_info.input_size = {2};
  op_desc_info.output_format = {FORMAT_NCHW};
  op_desc_info.output_shape = {{1}};
  op_desc_info.output_data_type = {DT_FLOAT};
  op_desc_info.output_addrs = {nullptr};
  op_desc_info.output_size = {2};
  ExceptionDumper exception_dumper;
  exception_dumper.op_desc_info_ = { op_desc_info };
  exception_dumper.DumpExceptionInfo(exception_infos);
  unsetenv(kEnvRecordPath);
}
}  // namespace ge