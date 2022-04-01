/**
 * Copyright 2021-2021 Huawei Technologies Co., Ltd
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#define private public
#define protected public
#include "framework/common/taskdown_common.h"
#include "hybrid/executor/subgraph_context.h"
#include "hybrid/node_executor/aicore/aicore_op_task.h"
#include "init/gelib.h"
#include "depends/runtime/src/runtime_stub.h"
#undef private
#undef protected
#include "common/tbe_handle_store/bin_register_utils.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;
using namespace optiling;

class UtestAiCoreOpTask : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

static ge::OpDescPtr CreateOpDesc(string name = "", string type = "", int in_num = 0, int out_num = 0) {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  static int32_t index = 0;
  op_desc->SetId(index++);

  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
  AttrUtils::SetInt(tensor, ATTR_NAME_PLACEMENT, 2);
  TensorUtils::SetSize(tensor, 64);
  vector<int64_t> input_offset;
  for (int i = 0; i < in_num; ++i) {
    op_desc->AddInputDesc(tensor);
    input_offset.emplace_back(index * 64 + i * 64);
  }
  op_desc->SetInputOffset(input_offset);

  vector<int64_t> output_offset;
  for (int i = 0; i < out_num; ++i) {
    op_desc->AddOutputDesc(tensor);
    output_offset.emplace_back(index * 64 + in_num * 64 + i * 64);
  }
  op_desc->SetOutputOffset(output_offset);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});

  ge::AttrUtils::SetStr(op_desc, ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  bool support_dynamic = true;
  ge::AttrUtils::GetBool(op_desc, "support_dynamicshape", support_dynamic);
  return op_desc;
}

static domi::TaskDef CreateTaskDef() {
  domi::TaskDef task_def;
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  std::vector<uint8_t> args(100, 0);
  task_def.mutable_kernel()->set_args(args.data(), args.size());
  task_def.mutable_kernel()->set_args_size(100);
  task_def.mutable_kernel()->mutable_context()->set_kernel_type(static_cast<uint32_t>(ccKernelType::TE));
  uint16_t args_offset = 0;
  task_def.mutable_kernel()->mutable_context()->set_args_offset(&args_offset, sizeof(args_offset));
  return task_def;
}

TEST_F(UtestAiCoreOpTask, Init_failed) {
  dlog_setlevel(0, 0, 0);
  std::unique_ptr<AiCoreOpTask> task1(new AiCoreOpTask());
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = CreateOpDesc("Add", "Add", 2, 1);
  ge::AttrUtils::SetInt(*op_desc, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_SHAPE_RANGE);
  auto node = graph->AddNode(op_desc);
  domi::TaskDef task_def = CreateTaskDef();
  EXPECT_EQ(task1->Init(node, task_def), SUCCESS);
  RTS_STUB_RETURN_VALUE(rtQueryFunctionRegistered, rtError_t, 0x78000001);

  const char tbe_bin[] = "tbe_bin";
  vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
  node = graph->AddNode(op_desc);
  OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
  op_desc->SetExtAttr(ge::OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
  void *stub_func = nullptr;
  AttrNameOfBinOnOp attr_names = {OP_EXTATTR_NAME_TBE_KERNEL, TVM_ATTR_NAME_METADATA, "_kernelname"};
  EXPECT_EQ(BinRegisterUtils::RegisterBin(*op_desc, task1->stub_name_, attr_names, stub_func), SUCCESS);
  dlog_setlevel(0, 3, 0);
}

TEST_F(UtestAiCoreOpTask, Init_success) {
  dlog_setlevel(0, 0, 0);
  std::unique_ptr<AiCoreOpTask> task1(new AiCoreOpTask());
  OpDescPtr op_desc = CreateOpDesc("Add", "Add", 2, 1);
  ge::AttrUtils::SetInt(*op_desc, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_SHAPE_RANGE);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  auto node = graph->AddNode(op_desc);
  domi::TaskDef task_def = CreateTaskDef();
  EXPECT_EQ(task1->Init(node, task_def), ge::SUCCESS);

  task1->need_tiling_ = true;
  EXPECT_EQ(task1->Init(node, task_def), ge::SUCCESS);
  dlog_setlevel(0, 3, 0);
}

TEST_F(UtestAiCoreOpTask, UpdateOutputsShape_success) {
  dlog_setlevel(0, 0, 0);
  std::unique_ptr<AiCoreOpTask> task1(new AiCoreOpTask());
  OpDescPtr op_desc = CreateOpDesc("Add", "Add", 2, 1);
  ge::AttrUtils::SetInt(*op_desc, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_SHAPE_RANGE);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  auto node = graph->AddNode(op_desc);
  domi::TaskDef task_def = CreateTaskDef();

  EXPECT_EQ(task1->Init(node, task_def), ge::SUCCESS);

  
  node = graph->AddNode(op_desc);
  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  node_item->input_start = 0;
  node_item->output_start = 0;
  node_item->is_dynamic = true;
  node_item->shape_inference_type = DEPEND_SHAPE_RANGE;

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(node_item);
  ASSERT_NE(node_state, nullptr);
  auto outputs_shape = reinterpret_cast<uint32_t(*)[9]>(task1->shape_buffer_->GetData());
  outputs_shape[0][0] = 2;
  outputs_shape[0][1] = 1;
  outputs_shape[0][2] = 2;
  ASSERT_EQ(task1->UpdateOutputsShape(*node_state->GetTaskContext()), SUCCESS);
  dlog_setlevel(0, 3, 0);
}

TEST_F(UtestAiCoreOpTask, RegisterKernelHandleTest) {
  auto op_desc = std::make_shared<ge::OpDesc>("data", DATA);
  std::vector<char> kernelBin;
  TBEKernelPtr tbe_kernel = std::make_shared<ge::OpKernelBin>("name/data", std::move(kernelBin));
  op_desc->SetExtAttr(ge::OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel);
  ge::AttrUtils::SetStr(op_desc, ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AICUBE");
  std::unique_ptr<AiCoreOpTask> task(new AiCoreOpTask());
  void *handle = nullptr;
  AttrNameOfBinOnOp attr_names = {OP_EXTATTR_NAME_TBE_KERNEL, TVM_ATTR_NAME_METADATA, "_kernelname"};
  Status ret = BinRegisterUtils::RegisterBinWithHandle(*op_desc, attr_names, handle);
  ASSERT_EQ(ret, SUCCESS);
}

TEST_F(UtestAiCoreOpTask, TestKernelLaunchTiling) {
  std::unique_ptr<AiCoreOpTask> task(new AiCoreOpTask());
  task->need_tiling_ = true;
  ASSERT_EQ(task->LaunchKernel(nullptr), SUCCESS);
  char *handle = "a";
  task->handle_ = handle;
  ASSERT_EQ(task->LaunchKernel(nullptr), SUCCESS);
}

TEST_F(UtestAiCoreOpTask, TestInitAtomicAddrCleanIndices) {
  std::unique_ptr<AtomicAddrCleanOpTask> task(new AtomicAddrCleanOpTask());
  OpDescPtr op_desc = CreateOpDesc("Atomic", "AtomicAddrClean", 0, 0);
  domi::TaskDef task_def = CreateTaskDef();

  std::vector<int64_t> atomic_output_indices = {};
  ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_indices);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  auto node = graph->AddNode(op_desc);
  ASSERT_EQ(task->Init(node, task_def), INTERNAL_ERROR);

  std::map<std::string, std::map<int64_t, int64_t>> workspace_info;
  workspace_info["test1"] = {{2, 3}};
  workspace_info["test2"] = {{3, 4}};
  atomic_output_indices = {1, 1};
  ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_indices);
  op_desc->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspace_info);
  auto node2 = graph->AddNode(op_desc);
  task->need_tiling_ = true;
  ASSERT_EQ(task->Init(node2, task_def), SUCCESS);
  task->max_arg_count_ = 0U;
  ASSERT_EQ(task->InitAtomicAddrCleanIndices(*op_desc), INTERNAL_ERROR);
}

TEST_F(UtestAiCoreOpTask, TestAtomicCalcTilingInfo) {
  std::unique_ptr<AtomicAddrCleanOpTask> task(new AtomicAddrCleanOpTask());
  OpDescPtr op_desc = CreateOpDesc("Atomic", "AtomicAddrClean", 0, 0);
  OpDescPtr add_op_desc = CreateOpDesc("Add", "Add", 2, 1);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = graph->AddNode(add_op_desc);
  task->tiling_info_ = MakeUnique<optiling::utils::OpRunInfo>();
  ASSERT_NE(task->tiling_info_, nullptr);
  Operator op("add");
  ASSERT_NE(task->CalcTilingInfo(add_node, op), SUCCESS);
}

TEST_F(UtestAiCoreOpTask, TestAtomicUpdateArgs) {
  std::unique_ptr<AtomicAddrCleanOpTask> task(new AtomicAddrCleanOpTask());

  task->atomic_output_indices_ = {0};
  task->atomic_workspace_indices_ = {0};

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr add_op_desc = CreateOpDesc("Add", "Add", 2, 1);
  NodePtr node = graph->AddNode(add_op_desc);
  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  node_item->input_start = 0;
  node_item->output_start = 0;
  node_item->is_dynamic = true;
  node_item->shape_inference_type = DEPEND_SHAPE_RANGE;
  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 1;

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);

  auto node_state = subgraph_context.GetNodeState(node_item);
  auto &task_ctx = *node_state->GetTaskContext();
  auto allocator = NpuMemoryAllocator::GetAllocator();
  auto tensor_buffer = TensorBuffer::Create(allocator, 100);
  auto tensor_value = TensorValue(shared_ptr<TensorBuffer>(tensor_buffer.release()));
  task_ctx.outputs_start_ = &tensor_value;

  auto work_space = allocator->Allocate(100);
  task_ctx.workspaces_.push_back(work_space);
  task_ctx.execution_context_->allocator = allocator;
  task->args_ = std::unique_ptr<uint8_t[]>(new uint8_t[sizeof(uintptr_t) * 2]);
  task->arg_base_ = reinterpret_cast<uintptr_t *>(task->args_.get());
  task->need_tiling_ = false;
  node_state->GetTaskContext()->execution_context_->trace_enabled = true;
  ASSERT_EQ(task->UpdateArgs(*node_state->GetTaskContext()), SUCCESS);
  ASSERT_NE(task->GetKeyForOpParamSize().size(), 0);
}

TEST_F(UtestAiCoreOpTask, TestUpdateTilingInfo) {
  std::unique_ptr<AiCoreOpTask> task(new AiCoreOpTask());

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr add_op_desc = CreateOpDesc("MyAdd", "MyAdd", 2, 1);
  NodePtr node = graph->AddNode(add_op_desc);
  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  node_item->input_start = 0;
  node_item->output_start = 0;
  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);

  auto node_state = subgraph_context.GetNodeState(node_item);
  auto &task_ctx = *node_state->GetTaskContext();
  task->tiling_info_ = MakeUnique<optiling::utils::OpRunInfo>();
  OpTilingFuncV2 op_tiling_func = [](const ge::Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &run_info) -> bool {
    const std::string temp("xx");
    run_info.GetAllTilingData() << temp;
    return true;
  };
  ge::AttrUtils::SetStr(add_op_desc, "compile_info_key", "op_compile_info_key");
  ge::AttrUtils::SetStr(add_op_desc, "compile_info_json", "op_compile_info_json");
  REGISTER_OP_TILING_UNIQ_V2(MyAdd, op_tiling_func, 1);
  OpTilingRegistryInterf_V2("MyAdd", op_tiling_func);

  EXPECT_EQ(task->UpdateTilingInfo(task_ctx), ACL_ERROR_GE_MEMORY_OPERATE_FAILED);
  task->args_ = std::unique_ptr<uint8_t[]>(new uint8_t[sizeof(uintptr_t) * 3 + 64]);
  task->arg_base_ = reinterpret_cast<uintptr_t *>(task->args_.get());
  task->max_tiling_size_ = 64;
  EXPECT_EQ(task->UpdateTilingInfo(task_ctx), SUCCESS);
}

TEST_F(UtestAiCoreOpTask, TestTilingkey) {
  OpTilingFuncV2 op_tiling_func = [](const ge::Operator &, const OpCompileInfoV2 &, OpRunInfoV2 &run_info) -> bool {
    run_info.SetTilingKey(0xFFFFFFFFFU);
    return true;
  };
  REGISTER_OP_TILING_UNIQ_V2(Cast, op_tiling_func, 1);
  OpTilingRegistryInterf_V2("Cast", op_tiling_func);

  OpDescPtr op_desc = CreateOpDesc("Cast", "Cast", 1, 1);
  ge::AttrUtils::SetStr(op_desc, "compile_info_key", "op_compile_info_key");
  ge::AttrUtils::SetStr(op_desc, "compile_info_json", "op_compile_info_json");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);
  auto op = OpDescUtils::CreateOperatorFromNode(node);
  optiling::utils::OpRunInfo run_info(0, true, 0);
  ASSERT_EQ(optiling::OpParaCalculateV2(op, run_info), SUCCESS);

  std::unique_ptr<AiCoreOpTask> task(new AiCoreOpTask());
  task->tiling_key_ = run_info.GetTilingKey();
  ASSERT_EQ(run_info.GetTilingKey(), 0xFFFFFFFFFU);
  ASSERT_EQ(std::to_string(task->tiling_key_), std::to_string(run_info.GetTilingKey()));
}

TEST_F(UtestAiCoreOpTask, TestUpdateBinHandle) {
  OpDescPtr op_desc = CreateOpDesc("Cast", "Cast", 1, 1);
  
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  char *handle = "a";
  NodeCompileCacheItem cci;
  ge::AttrUtils::SetStr(op_desc, "compile_info_key", "op_compile_info_key");
  ge::AttrUtils::SetStr(op_desc, "compile_info_json", "op_compile_info_json");
  ge::AttrUtils::SetStr(op_desc, "_atomic_compile_info_key", "_atomic_compile_info_key");
  ge::AttrUtils::SetStr(op_desc, "_atomic_compile_info_json", "_atomic_compile_info_key");
  auto ret = NodeCompileCacheItem::Build(kStubFunc, node, handle, cci);
  EXPECT_EQ(ret, SUCCESS);

  std::unique_ptr<AiCoreOpTask> task(new AiCoreOpTask());
  task->handle_ = nullptr;
  ret = task->UpdateBinHandle(&cci);
  EXPECT_EQ(ret, SUCCESS);  // update op task of stub_func success

  NodeCompileCacheItem cci_with_handle;
  ret = NodeCompileCacheItem::Build(kWithHandle, node, handle, cci_with_handle);
  EXPECT_EQ(ret, SUCCESS);
  ret = task->UpdateBinHandle(&cci_with_handle);
  EXPECT_EQ(ret, SUCCESS);  // update op task of stub_func with handle success

  task->handle_ = handle;  // op task of with handle
  ret = task->UpdateBinHandle(&cci_with_handle);
  EXPECT_EQ(ret, SUCCESS);  // update op task of handle with handle success;

  ret = task->UpdateBinHandle(&cci);
  EXPECT_EQ(ret, SUCCESS);  // update op task of handle with stub func success;
}

// aicore with host mem input optimization
TEST_F(UtestAiCoreOpTask, TestUpdateArgsWithHostMemInput) {
  std::unique_ptr<AiCoreOpTask> task(new AiCoreOpTask());

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr add_op_desc = CreateOpDesc("Add", "Add", 2, 1);
  NodePtr node = graph->AddNode(add_op_desc);
  std::unique_ptr<NodeItem> new_node;
  ASSERT_EQ(NodeItem::Create(node, new_node), SUCCESS);
  NodeItem *node_item = new_node.get();
  node_item->input_start = 0;
  node_item->output_start = 0;
  node_item->is_dynamic = true;
  node_item->shape_inference_type = DEPEND_SHAPE_RANGE;
  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item);
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 1;

  domi::TaskDef task_def = CreateTaskDef();

  // check, args are extended for host memory input data
  EXPECT_EQ(task->Init(node, task_def), SUCCESS);
  EXPECT_NE(task->host_mem_input_data_offset_, 0);
  EXPECT_EQ(task->args_size_ - task->host_mem_input_data_offset_, kMaxHostMemInputLen);

  GraphExecutionContext graph_context;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  uint8_t host_mem[64];
  TensorValue tensorValue(&host_mem, 64);
  subgraph_context.all_inputs_.emplace_back(tensorValue);
  subgraph_context.all_inputs_.emplace_back(tensorValue);
  EXPECT_EQ(subgraph_context.Init(), SUCCESS);

  auto node_state = subgraph_context.GetNodeState(node_item);
  auto &task_ctx = *node_state->GetTaskContext();
  auto allocator = NpuMemoryAllocator::GetAllocator();
  auto tensor_buffer = TensorBuffer::Create(allocator, 100);
  auto tensor_value = TensorValue(shared_ptr<TensorBuffer>(tensor_buffer.release()));
  task_ctx.outputs_start_ = &tensor_value;

  auto work_space = allocator->Allocate(100);
  task_ctx.workspaces_.push_back(work_space);
  task_ctx.execution_context_->allocator = allocator;
  task->need_tiling_ = false;
  task->SetNeedHostMemOpt(true);
  node_state->GetTaskContext()->execution_context_->trace_enabled = true;
  EXPECT_EQ(task->UpdateArgs(*node_state->GetTaskContext()), SUCCESS);

  // check host mem data
  ASSERT_NE(task->args_ex_.hostInputInfoPtr, nullptr);
  ASSERT_EQ(task->args_ex_.hostInputInfoNum, 2);

  uint64_t *addr = PtrToPtr<void, uint64_t>(ValueToPtr(PtrToValue(task->args_ex_.args) +
                                                       task->args_ex_.hostInputInfoPtr[0].addrOffset));
  uint64_t host_mem_data_addr = PtrToValue(task->args_ex_.args) +
                                task->args_ex_.hostInputInfoPtr[0].dataOffset;
  EXPECT_EQ(*addr, host_mem_data_addr);

  addr = PtrToPtr<void, uint64_t>(ValueToPtr(PtrToValue(task->args_ex_.args) +
                                             task->args_ex_.hostInputInfoPtr[1].addrOffset));
  host_mem_data_addr = PtrToValue(task->args_ex_.args) +
                       task->args_ex_.hostInputInfoPtr[1].dataOffset;
  EXPECT_EQ(*addr, host_mem_data_addr);
}
}  // namespace ge