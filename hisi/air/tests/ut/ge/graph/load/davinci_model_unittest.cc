/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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
#include <gmock/gmock.h>
#include <memory>
#include <fstream>

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_local_context.h"

#define private public
#define protected public
#include "graph/utils/graph_utils.h"
#include "common/profiling/profiling_manager.h"
#include "common/dump/dump_manager.h"
#include "common/opskernel/ops_kernel_info_store.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/load/model_manager/task_info/profiler_trace_task_info.h"
#include "graph/load/model_manager/task_info/ffts_plus_task_info.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "ut/ge/ffts_plus_proto_tools.h"

using namespace std;

extern std::string g_runtime_stub_mock;

namespace ge {
class OpsKernelInfoStoreStub : public OpsKernelInfoStore {
 public:
  OpsKernelInfoStoreStub() = default;
  Status Initialize(const std::map<std::string, std::string> &options) override { return SUCCESS; }
  Status Finalize() override { return SUCCESS; }
  void GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const override {}
  bool CheckSupported(const OpDescPtr &opDescPtr, std::string &un_supported_reason) const override { return true; }
  Status UnloadTask(GETaskInfo &task) { return SUCCESS; }
};

class DModelListener : public ModelListener {
 public:
  DModelListener(){};
  uint32_t OnComputeDone(uint32_t model_id, uint32_t data_index, uint32_t result, std::vector<Tensor> &outputs) {
    return 0;
  }
};

shared_ptr<ModelListener> g_local_call_back(new DModelListener());
class UtestDavinciModel : public testing::Test {
 protected:
  void SetUp() {
    VarManager::Instance(0)->Init(0, 0, 0, 0);
    const std::map<string, string> options{ {GRAPH_MEMORY_MAX_SIZE, "1048576"}, {VARIABLE_MEMORY_MAX_SIZE, "1048576"} };
    VarManager::Instance(0)->SetMemoryMallocSize(options, 10UL * 1024UL * 1024UL);
    MemManager::Instance().Initialize({ RT_MEMORY_HBM, RT_MEMORY_P2P_DDR });
    g_runtime_stub_mock.clear();
  }

  void TearDown() {
    g_runtime_stub_mock.clear();
    MemManager::Instance().Finalize();
  }
};

int32_t MsprofReport(uint32_t moduleId, uint32_t type, void *data, uint32_t len) {
if (type == static_cast<uint32_t>(MSPROF_REPORTER_HASH)) {
    MsprofHashData *hash_data = reinterpret_cast<MsprofHashData *>(data);
    hash_data->hashId = 66U;
  }
  return 0;
}

TEST_F(UtestDavinciModel, davinci_init_success) {
  uint32_t mem_offset = 0U;
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->EDGE(0, 0)->NODE("add_n", ADDN));   // ccKernelType::TE
    CHAIN(NODE("_var_0", VARIABLE)->NODE("allreduce", HCOMALLREDUCE)->EDGE(0, 0)->NODE("relu", RELU)); // HCCL
    CHAIN(NODE("_arg_1", CONSTANTOP)->EDGE(0, 1)->NODE("add_n")->EDGE(0, 1)->NODE("relu")-> // ccKernelType::CUSTOMIZED
          NODE("square", SQUARE)->EDGE(0, 0)->      // ccKernelType::AI_CPU
          NODE("reshape", RESHAPE)->EDGE(0, 0)->    // ccKernelType::CUST_AI_CPU
          NODE("deque", FRAMEWORKOP)->EDGE(0, 0)->  // KERNEL_EX
          NODE("memcpy", MEMCPYASYNC)->EDGE(0, 0)-> // MEMCPY_ASYNC
          NODE("Node_Output", NETOUTPUT));
  };
  ComputeGraphPtr graph = ToComputeGraph(g1);
  AttrUtils::SetInt(graph, "globleworkspace_status", 1);
  AttrUtils::SetInt(graph, "globleworkspace_status_bytes", 1);
  SetKnownOpKernel(graph, mem_offset);

  ProfilingProperties::Instance().is_load_profiling_ = true;
  ProfilingManager::Instance().reporter_max_len_ = 1024;

  std::vector<uint64_t> weights_value(64, 1024);
  size_t weight_size = weights_value.size() * sizeof(uint64_t);
  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  ge_model->SetWeight(Buffer::CopyFrom((uint8_t *)weights_value.data(), weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, mem_offset));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_VAR_SIZE, 5120));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_P2P_MEMORY_SIZE, 128));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_SESSION_SCOPE_MEMORY_SIZE, 128));

  const auto model_def = MakeShared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_def);

  {
    const auto &node = graph->FindNode("_arg_0");
    const auto &op_desc = node->GetOpDesc();
    AttrUtils::SetInt(op_desc, ATTR_NAME_INDEX, 0);
  }

  {
    const auto &node = graph->FindNode("_var_0");
    const auto &op_desc = node->GetOpDesc();
    AttrUtils::SetBool(op_desc, VAR_ATTR_VAR_IS_BROADCAST, true);
    VarManager::Instance(graph->GetSessionID())->SetAllocatedGraphId(op_desc->GetName(), graph->GetGraphID());
  }

  {
    const auto &node = graph->FindNode("_arg_1");
    const auto &op_desc = node->GetOpDesc();

    std::vector<uint8_t> weights_value(64, 'A');
    GeTensorDesc data_desc = node->GetOpDesc()->GetOutputDesc(0);
    GeTensorPtr weight_value = MakeShared<GeTensor>(data_desc, weights_value.data(), weights_value.size());
    EXPECT_TRUE(AttrUtils::SetTensor(node->GetOpDesc(), ATTR_NAME_WEIGHTS, weight_value));
  }

  const std::shared_ptr<OpsKernelInfoStore> ops_kernel_store = MakeShared<OpsKernelInfoStoreStub>();
  {
    const auto &node = graph->FindNode("allreduce");
    const auto &op_desc = node->GetOpDesc();
    op_desc->SetExtAttr("OpsKernelInfoStorePtr", ops_kernel_store.get());

    auto &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_HCCL);
    task_def.set_stream_id(op_desc->GetStreamId());
    task_def.set_private_def("hccl_task"); // for GetPrivateDefByTaskDef

    auto &hccl_def = *task_def.mutable_kernel_hccl();
    hccl_def.set_op_index(op_desc->GetId());
  }

  {
    const auto &node = graph->FindNode("add_n");
    const auto &op_desc = node->GetOpDesc();

    int32_t run_mode = static_cast<uint32_t>(domi::ImplyType::TVM);
    EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, run_mode));
    std::vector<char> kernel_bin(64, '\0');
    TBEKernelPtr kernel_handle = MakeShared<OpKernelBin>(op_desc->GetName(), std::move(kernel_bin));
    EXPECT_TRUE(op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, kernel_handle));
    EXPECT_TRUE(AttrUtils::SetStr(op_desc, op_desc->GetName() + "_kernelname", op_desc->GetName()));
    EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::vector<std::string>{"dump"}));

    auto &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_KERNEL);
    task_def.set_stream_id(op_desc->GetStreamId());

    auto &kernel_def = *task_def.mutable_kernel();
    kernel_def.set_stub_func("stub_func");
    kernel_def.set_args_size(64);
    string args(64, '1');
    kernel_def.set_args(args.data(), 64);

    auto &context = *kernel_def.mutable_context();
    context.set_op_index(op_desc->GetId());
    context.set_kernel_type(static_cast<uint32_t>(ccKernelType::TE));
    uint16_t args_offset[9] = {0};
    context.set_args_offset(args_offset, 9 * sizeof(uint16_t));
  }

  {
    const auto &node = graph->FindNode("relu");
    const auto &op_desc = node->GetOpDesc();
    const char task[] = "opattr";
    AttrUtils::SetBytes(op_desc, ATTR_NAME_OPATTR, Buffer::CopyFrom((uint8_t *)task, sizeof(task)));

    auto &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_KERNEL);
    task_def.set_stream_id(op_desc->GetStreamId());

    auto &kernel_def = *task_def.mutable_kernel();
    auto &context = *kernel_def.mutable_context();

    const std::vector<uint64_t> args_info(5, 0);
    kernel_def.set_args(args_info.data(), args_info.size() * sizeof(uint64_t));
    kernel_def.set_args_size(args_info.size() * sizeof(uint64_t));

    context.set_op_index(op_desc->GetId());
    context.set_kernel_type(static_cast<uint32_t>(ccKernelType::CUSTOMIZED));
    const std::vector<uint16_t> args_offset{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8};
    context.set_args_count(args_offset.size());
    context.set_args_offset(args_offset.data(), args_offset.size() * sizeof(uint16_t));
  }

  {
    const auto &node = graph->FindNode("square");
    const auto &op_desc = node->GetOpDesc();

    int32_t run_mode = static_cast<uint32_t>(domi::ImplyType::AI_CPU);
    EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, run_mode));
    EXPECT_TRUE(AttrUtils::SetBool(op_desc, ATTR_NO_TASK_AND_DUMP_NEEDED, true));    // for IsNoTaskAndDumpNeeded
    EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::vector<std::string>{"ok"}));

    auto &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_KERNEL);
    task_def.set_stream_id(op_desc->GetStreamId());

    auto &kernel_def = *task_def.mutable_kernel();
    kernel_def.set_stub_func("stub_func");
    kernel_def.set_args_size(64);
    string args(64, '1');
    kernel_def.set_args(args.data(), 64);

    auto &context = *kernel_def.mutable_context();
    context.set_op_index(op_desc->GetId());
    context.set_kernel_type(static_cast<uint32_t>(ccKernelType::AI_CPU));
    uint16_t args_offset[9] = {0};
    context.set_args_offset(args_offset, 9 * sizeof(uint16_t));
  }

  {
    const auto &node = graph->FindNode("reshape");
    const auto &op_desc = node->GetOpDesc();

    std::vector<char> kernel_bin(128, '0');
    const auto aicpu_kernel = MakeShared<OpKernelBin>(op_desc->GetName(), std::move(kernel_bin));
    op_desc->SetExtAttr(OP_EXTATTR_CUSTAICPU_KERNEL, aicpu_kernel);

    domi::TaskDef &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_KERNEL);
    task_def.set_stream_id(op_desc->GetStreamId());

    std::vector<char> args_info(64U, '0');
    domi::KernelDef &kernel_def = *task_def.mutable_kernel();
    kernel_def.set_args_size(args_info.size());
    kernel_def.set_args(args_info.data(), args_info.size());
    kernel_def.set_so_name("libfeatures.so");
    kernel_def.set_kernel_name("features");

    domi::KernelContext &context = *kernel_def.mutable_context();
    context.set_kernel_type(static_cast<uint32_t>(ccKernelType::CUST_AI_CPU));
    context.set_op_index(op_desc->GetId());
  }

  {
    const auto &node = graph->FindNode("deque");
    const auto &op_desc = node->GetOpDesc();
    op_desc->SetWorkspace({1308});   // offset
    op_desc->SetWorkspaceBytes({120U});  // length

    domi::TaskDef &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_KERNEL_EX);
    task_def.set_stream_id(op_desc->GetStreamId());

    std::vector<uint8_t> task_info(120U, 0U);
    domi::KernelExDef &kernel_def = *task_def.mutable_kernel_ex();
    kernel_def.set_task_info(task_info.data(), task_info.size());
    kernel_def.set_task_info_size(task_info.size());
    kernel_def.set_op_index(op_desc->GetId());
  }

  {
    const auto &node = graph->FindNode("memcpy");
    const auto &op_desc = node->GetOpDesc();
    op_desc->SetOpKernelLibName(kEngineNameRts);

    auto &task_def = *model_def->add_task();
    task_def.set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
    task_def.set_stream_id(op_desc->GetStreamId());

    auto &memcpy_async = *task_def.mutable_memcpy_async();
    memcpy_async.set_src(op_desc->GetInputOffset()[0]);
    memcpy_async.set_dst(op_desc->GetOutputOffset()[0]);
    memcpy_async.set_dst_max(512);
    memcpy_async.set_count(1);
    memcpy_async.set_kind(RT_MEMCPY_DEVICE_TO_DEVICE);
    memcpy_async.set_op_index(op_desc->GetId());
  }

  {
    const auto &node = graph->FindNode("Node_Output");
    const auto &op_desc = node->GetOpDesc();

    op_desc->SetSrcName( { "memcpy" } );
    op_desc->SetSrcIndex( { 0 } );
  }

  {
    // Scene 1: Normal flow.
    DavinciModel model(0, nullptr);

    model.Assign(ge_model);
    EXPECT_EQ(model.Init(), SUCCESS);

    EXPECT_EQ(model.input_addrs_list_.size(), 1);
    EXPECT_EQ(model.output_addrs_list_.size(), 1);
    EXPECT_EQ(model.task_list_.size(), model_def->task_size());

    OutputData output_data;
    std::vector<Tensor> outputs;
    EXPECT_EQ(model.GenOutputTensorInfo(output_data, outputs), SUCCESS);
    EXPECT_EQ(output_data.blobs.size(), 1);
    EXPECT_EQ(outputs.size(), 1);
    ProfilingProperties::Instance().is_load_profiling_ = false;
  }

  EXPECT_EQ(setenv(kEnvGeuseStaticMemory, "1", true), 0);
  {
    // Scene 2: Special env.
    DavinciModel model(0, nullptr);
    model.data_dumper_.dump_properties_.is_train_op_debug_ = true;

    model.Assign(ge_model);
    EXPECT_EQ(model.Init(), SUCCESS);

    EXPECT_EQ(model.input_addrs_list_.size(), 1);
    EXPECT_EQ(model.output_addrs_list_.size(), 1);
    EXPECT_EQ(model.task_list_.size(), model_def->task_size());

    OutputData output_data;
    std::vector<Tensor> outputs;
    EXPECT_EQ(model.GenOutputTensorInfo(output_data, outputs), SUCCESS);
    EXPECT_EQ(output_data.blobs.size(), 1);
    EXPECT_EQ(outputs.size(), 1);
    ProfilingProperties::Instance().is_load_profiling_ = false;
  }
  EXPECT_EQ(unsetenv(kEnvGeuseStaticMemory), 0);

  {
    // Scene 3: MDC Queue
    DavinciModel model(0, nullptr);
    model.SetQueIds({ 1001U }, { 1004U });

    model.Assign(ge_model);
    model.input_no_tiling_flag_ = { false };
    model.output_no_tiling_flag_ = { false };
    EXPECT_EQ(model.Init(), SUCCESS);

    EXPECT_EQ(model.input_addrs_list_.size(), 1);
    EXPECT_EQ(model.output_addrs_list_.size(), 1);
    EXPECT_EQ(model.task_list_.size(), model_def->task_size());
    // AddHeadStream: +0
    // BindInputQueue: +1
    // CpuTaskModelZeroCopy: +1
    // BindOutputQueue: +0
    // CpuTaskModelZeroCopy: +0
    // CpuActiveStream: +1
    // CpuWaitEndGraph: +1
    // BindEnqueue: +1
    // CpuPostProcess: +1
    // CpuModelRepeat: +1
    EXPECT_EQ(model.cpu_task_list_.size(), 7);
  }

  {
    // Scene 4: MDC Queue
    DavinciModel model(0, nullptr);
    model.SetQueIds({ 1001U }, { 1004U });

    model.Assign(ge_model);
    model.input_no_tiling_flag_ = { true };
    model.output_no_tiling_flag_ = { true };
    model.has_no_tiling_input_ = true;
    model.has_no_tiling_output_ = true;
    EXPECT_EQ(model.Init(), SUCCESS);

    EXPECT_EQ(model.input_addrs_list_.size(), 1);
    EXPECT_EQ(model.output_addrs_list_.size(), 1);
    EXPECT_EQ(model.task_list_.size(), model_def->task_size());
    EXPECT_EQ(model.cpu_task_list_.size(), 7);
  }

  std::map<std::string, std::string> graph_options;
  graph_options[kOptionExecGeUseStaticMemory] = "2";
  GetThreadLocalContext().SetGraphOption(graph_options);
  {
    // Scene 5: extend size statis memory
    DavinciModel model1(0, nullptr);
    model1.data_dumper_.dump_properties_.is_train_op_debug_ = true;

    model1.Assign(ge_model);
    EXPECT_EQ(model1.Init(), SUCCESS);

    DavinciModel model2(0, nullptr);
    model2.data_dumper_.dump_properties_.is_train_op_debug_ = true;

    model2.Assign(ge_model);
    EXPECT_EQ(model2.Init(), SUCCESS);

    // test1, model2 reuse model1 feature map memory
    EXPECT_EQ(model1.mem_base_, model2.mem_base_);

    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 20240));
    DavinciModel model3(0, nullptr);
    model3.data_dumper_.dump_properties_.is_train_op_debug_ = true;

    // test2, model3 can not reused model1 feature map memory
    model3.Assign(ge_model);
    EXPECT_EQ(model3.Init(), ACL_ERROR_GE_MEMORY_ALLOCATION);

    EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240));
    ProfilingProperties::Instance().is_load_profiling_ = false;
  }
  graph_options[kOptionExecGeUseStaticMemory] = "";
  GetThreadLocalContext().SetGraphOption(graph_options);

}

TEST_F(UtestDavinciModel, init_data_op) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_input = CreateOpDesc("data", DATA);
  op_input->AddInputDesc(tensor);
  op_input->AddOutputDesc(tensor);
  op_input->SetInputOffset({1024});
  op_input->SetOutputOffset({1024});
  NodePtr node_input = graph->AddNode(op_input);

  OpDescPtr op_output = CreateOpDesc("output", NETOUTPUT);
  op_output->AddInputDesc(tensor);
  op_output->SetInputOffset({1024});
  op_output->SetSrcName( { "data" } );
  op_output->SetSrcIndex( { 0 } );
  NodePtr node_output = graph->AddNode(op_output);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 1);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_op_subgraph) {
  DavinciModel model(0, nullptr);
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_input = CreateOpDesc("data", DATA);
  op_input->AddInputDesc(tensor);
  op_input->AddOutputDesc(tensor);
  op_input->SetInputOffset({1024});
  op_input->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_input);

  uint32_t data_op_index = 0;
  map<uint32_t, OpDescPtr> data_by_index;
  set<const void *> input_outside_addrs;
  EXPECT_EQ(model.InitDataOp(nullptr, node, data_op_index, data_by_index, input_outside_addrs), SUCCESS);

  EXPECT_EQ(model.input_addrs_list_.size(), 0);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_EQ(data_op_index, 0);
  EXPECT_TRUE(data_by_index.empty());
}

TEST_F(UtestDavinciModel, init_netoutput_op_subgraph) {
  DavinciModel model(0, nullptr);
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_output = CreateOpDesc("output", NETOUTPUT);
  op_output->AddInputDesc(tensor);
  op_output->SetInputOffset({1024});
  op_output->SetSrcName( { "data" } );
  op_output->SetSrcIndex( { 0 } );
  NodePtr node = graph->AddNode(op_output);

  std::vector<OpDescPtr> output_op_list;
  set<const void *> output_outside_addrs;
  EXPECT_EQ(model.InitNetOutput(nullptr, node, output_op_list, output_outside_addrs), SUCCESS);

  EXPECT_EQ(model.input_addrs_list_.size(), 0);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(output_op_list.empty());
}

TEST_F(UtestDavinciModel, init_unknown) {
  DavinciModel model(0, nullptr);
  model.SetKnownNode(true);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 5120000);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_input = CreateOpDesc("data", DATA);
  op_input->AddInputDesc(tensor);
  op_input->AddOutputDesc(tensor);
  op_input->SetInputOffset({1024});
  op_input->SetOutputOffset({1024});
  NodePtr node_input = graph->AddNode(op_input);    // op_index = 0

  OpDescPtr op_kernel = CreateOpDesc("square", "Square");
  op_kernel->AddInputDesc(tensor);
  op_kernel->AddOutputDesc(tensor);
  op_kernel->SetInputOffset({1024});
  op_kernel->SetOutputOffset({1024});
  NodePtr node_kernel = graph->AddNode(op_kernel);  // op_index = 1

  OpDescPtr op_memcpy = CreateOpDesc("memcpy", MEMCPYASYNC);
  op_memcpy->AddInputDesc(tensor);
  op_memcpy->AddOutputDesc(tensor);
  op_memcpy->SetInputOffset({1024});
  op_memcpy->SetOutputOffset({5120});
  NodePtr node_memcpy = graph->AddNode(op_memcpy);  // op_index = 2

  OpDescPtr op_output = CreateOpDesc("output", NETOUTPUT);
  op_output->AddInputDesc(tensor);
  op_output->SetInputOffset({5120});
  op_output->SetSrcName( { "memcpy" } );
  op_output->SetSrcIndex( { 0 } );
  NodePtr node_output = graph->AddNode(op_output);  // op_index = 3


  domi::TaskDef *task_def1 = model_task_def->add_task();
  task_def1->set_stream_id(0);
  task_def1->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def = task_def1->mutable_kernel();
  kernel_def->set_stub_func("stub_func");
  kernel_def->set_args_size(64);
  string args(64, '1');
  kernel_def->set_args(args.data(), 64);
  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_op_index(1);
  context->set_kernel_type(2U);    // ccKernelType::TE
  uint16_t args_offset[9] = {0};
  context->set_args_offset(args_offset, 9 * sizeof(uint16_t));

  domi::TaskDef *task_def2 = model_task_def->add_task();
  task_def2->set_stream_id(0);
  task_def2->set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
  domi::MemcpyAsyncDef *memcpy_async = task_def2->mutable_memcpy_async();
  memcpy_async->set_src(1024);
  memcpy_async->set_dst(5120);
  memcpy_async->set_dst_max(512);
  memcpy_async->set_count(1);
  memcpy_async->set_kind(RT_MEMCPY_DEVICE_TO_DEVICE);
  memcpy_async->set_op_index(2);

  model.Assign(ge_model);
  ProfilingProperties::Instance().is_load_profiling_ = true;
  EXPECT_EQ(model.Init(), SUCCESS);
  ProfilingProperties::Instance().is_load_profiling_ = false;

  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 1);
  EXPECT_EQ(model.task_list_.size(), 2);

  EXPECT_EQ(model.task_list_[0]->UpdateArgs(), SUCCESS);
  EXPECT_EQ(model.task_list_[1]->UpdateArgs(), SUCCESS);

  std::vector<string> out_shape_info;
  model.GetOutputShapeInfo(out_shape_info);

  std::vector<InputOutputDescInfo> input_descs;
  std::vector<InputOutputDescInfo> output_descs;
  EXPECT_EQ(model.GetInputOutputDescInfo(input_descs, output_descs), SUCCESS);

  int32_t virtual_addr = 0;
  const std::vector<void *> inputs = { &virtual_addr };
  const std::vector<void *> outputs = { &virtual_addr  };
  EXPECT_EQ(model.UpdateKnownNodeArgs(inputs, outputs), SUCCESS);
}

TEST_F(UtestDavinciModel, Init_variable_op) {
  DavinciModel model(0, g_local_call_back);
  model.ge_model_ = MakeShared<GeModel>();
  model.runtime_param_.mem_size = 51200;
  model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(malloc(model.runtime_param_.mem_size));
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr var1 = CreateOpDesc("var1", VARIABLE);
  var1->AddInputDesc(tensor);
  var1->AddOutputDesc(tensor);
  var1->SetInputOffset({1024});
  var1->SetOutputOffset({1024});
  AttrUtils::SetBool(var1, VAR_ATTR_VAR_IS_BROADCAST, true);
  graph->AddNode(var1);

  OpDescPtr var2 = CreateOpDesc(NODE_NAME_GLOBAL_STEP, VARIABLE);
  var2->AddInputDesc(tensor);
  var2->AddOutputDesc(tensor);
  var2->SetInputOffset({1024});
  var2->SetOutputOffset({1024});
  graph->AddNode(var2);

  EXPECT_EQ(model.InitNodes(graph), SUCCESS);

  model.ReturnNoOutput(1);
  EXPECT_EQ(model.SyncVarData(), SUCCESS);

  OutputData output_data;
  EXPECT_FALSE(model.has_output_node_);
  EXPECT_EQ(model.CopyOutputData(output_data, RT_MEMCPY_DEVICE_TO_HOST), SUCCESS);

  model.ReturnResult(1, false, true, output_data);
  free(reinterpret_cast<void *>(model.runtime_param_.mem_base));
  model.runtime_param_.mem_base = 0;
}

TEST_F(UtestDavinciModel, init_stream_switchn) {
  DavinciModel model(0, g_local_call_back);
  model.ge_model_ = MakeShared<GeModel>();
  model.runtime_param_.mem_size = 51200;
  model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(malloc(model.runtime_param_.mem_size));
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("switch_n", STREAMSWITCHN);
  EXPECT_TRUE(AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, {}));
  EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_BATCH_NUM, 0));
  graph->AddNode(op_desc);

  EXPECT_EQ(model.InitNodes(graph), SUCCESS);
  free(reinterpret_cast<void *>(model.runtime_param_.mem_base));
  model.runtime_param_.mem_base = 0;
}

TEST_F(UtestDavinciModel, init_constant_op) {
  DavinciModel model(0, g_local_call_back);
  model.ge_model_ = MakeShared<GeModel>();
  model.runtime_param_.mem_size = 51200;
  model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(malloc(model.runtime_param_.mem_size));
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  OpDescPtr op_desc = CreateOpDesc("FileConstant", FILECONSTANT);
  std::vector<int64_t> shape = {2,2,2,2};

  EXPECT_TRUE(AttrUtils::SetDataType(op_desc, "dtype", DT_FLOAT));
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "file"));
  EXPECT_TRUE(AttrUtils::SetListInt(op_desc, "shape", shape));
  GeTensorDesc tensor_desc(GeShape(shape), FORMAT_ND, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 128);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetOutputOffset({0});
  graph->AddNode(op_desc);

  std::unique_ptr<float[]> float_buf(new float[16]);
  std::string file_name = "test_copy_one_weight.bin";
  std::ofstream out1("test_copy_one_weight.bin", std::ios::binary);
  if (!out1.is_open()) {
    return;
  }
  out1.write((char *)float_buf.get(), 16 * sizeof(float));
  out1.close();
  model.file_id_and_path_map_.insert(std::pair<std::string, std::string>("file", "test_copy_one_weight.bin"));
  EXPECT_EQ(model.InitNodes(graph), SUCCESS);
  free(reinterpret_cast<void *>(model.runtime_param_.mem_base));
  model.runtime_param_.mem_base = 0;
  (void)remove("test_copy_one_weight.bin");
}

TEST_F(UtestDavinciModel, output_no_tiling_data) {
  DavinciModel model(0, g_local_call_back);
  model.ge_model_ = MakeShared<GeModel>();
  model.runtime_param_.mem_size = 51200;
  model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(malloc(model.runtime_param_.mem_size));
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  OpDescPtr where = MakeShared<OpDesc>("where", "Where");
  std::vector<int64_t> shape = {-1, 2};
  GeTensorDesc tensor(GeShape(shape), FORMAT_ND, DT_INT64);
  const std::vector<std::pair<int64_t, int64_t>> range = {{1, 10}, {2, 2}};
  tensor.SetShapeRange(range);
  where->AddOutputDesc(tensor);
  where->SetOutputOffset({1024});
  AttrUtils::SetBool(tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
  AttrUtils::SetInt(tensor, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 2048);
  graph->AddNode(where);

  OpDescPtr output = MakeShared<OpDesc>("output", "NetOutput");
  output->SetInputOffset({1024});
  const std::vector<string> src_name = {"where"};
  output->SetSrcName(src_name);
  const std::vector<int64_t> src_index = {0};
  output->SetSrcIndex(src_index);
  output->AddInputDesc(tensor);
  graph->AddNode(output);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  OutputData output_data;
  RuntimeTensorDesc *addr = reinterpret_cast<RuntimeTensorDesc *>(model.output_data_info_[0].GetBasicAddr());
  addr->shape[0] = 2;
  addr->shape[1] = 5;
  addr->shape[2] = 2;
  model.ReturnResult(1, true, false, output_data);

  model.is_online_infer_dynamic_ = true;
  model.ReturnResult(1, true, false, output_data);

  free(reinterpret_cast<uint8_t *>(model.runtime_param_.mem_base));
  model.runtime_param_.mem_base = 0U;
}

TEST_F(UtestDavinciModel, copy_input_data_no_tiling) {
  DavinciModel model(0, g_local_call_back);
  model.ge_model_ = MakeShared<GeModel>();
  model.runtime_param_.mem_size = 51200;
  model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(malloc(model.runtime_param_.mem_size));
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  OpDescPtr data = CreateOpDesc("data", DATA);
  GeTensorDesc tensor_desc(GeShape({-1,-1,224,224}), FORMAT_NCHW, DT_FLOAT);
  data->SetOutputOffset({2048});
  AttrUtils::SetBool(tensor_desc, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
  AttrUtils::SetInt(tensor_desc, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 1024);
  TensorUtils::SetSize(tensor_desc, 10240);
  data->AddOutputDesc(tensor_desc);
  NodePtr data_node = graph->AddNode(data);

  uint32_t data_op_index = 0;
  set<const void *> input_outside_addrs;
  map<uint32_t, OpDescPtr> data_by_index;
  EXPECT_EQ(model.InitDataOp(graph, data_node, data_op_index, data_by_index, input_outside_addrs), SUCCESS);

  InputData input_data;
  std::vector<int64_t> shape = {4,3,224,224};
  input_data.shapes.push_back(shape);
  size_t size = 5160;
  void *data_addr = (void *)malloc(size);
  DataBuffer buffer(data_addr, size, false);
  input_data.blobs.push_back(buffer);
  EXPECT_EQ(model.CopyInputData(input_data), SUCCESS);
  free(reinterpret_cast<uint8_t *>(model.runtime_param_.mem_base));
  free(data_addr);
  model.runtime_param_.mem_base = 0U;
}

TEST_F(UtestDavinciModel, InitRealSizeAndShapeInfo_succ1) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  OpDescPtr op_output = CreateOpDesc("output_ascend_mbatch_batch_1", NETOUTPUT);
  op_output->AddInputDesc(tensor);
  op_output->SetInputOffset({1024});
  NodePtr node_output = graph->AddNode(op_output);
  EXPECT_EQ(model.InitRealSizeAndShapeInfo(graph, node_output), SUCCESS);
}

TEST_F(UtestDavinciModel, InitRealSizeAndShapeInfo_succ2) {
  DavinciModel model(0, nullptr);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test_graph");

  OpDescPtr data1 = CreateOpDesc("data1", DATA);
  GeTensorDesc shape_desc(GeShape({4,3,224,224}), FORMAT_NCHW, DT_FLOAT);
  data1->AddInputDesc(shape_desc);
  data1->AddOutputDesc(shape_desc);
  NodePtr data1_node = graph->AddNode(data1);

  OpDescPtr case_node = CreateOpDesc("case1", CASE);
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  case_node->AddInputDesc(tensor);
  case_node->AddOutputDesc(tensor);
  NodePtr case1_node = graph->AddNode(case_node);

  OpDescPtr output = CreateOpDesc("output1", NETOUTPUT);
  output->AddInputDesc(tensor);
  output->SetSrcName( { "case1" } );
  output->SetSrcIndex( { 0 } );
  NodePtr output_node = graph->AddNode(output);

  GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), case1_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(case1_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0));

  (void)AttrUtils::SetStr(output_node->GetOpDesc(), ATTR_ALL_GEARS_INFO, "1;2;4;8");
  (void)AttrUtils::SetBool(case_node, ATTR_INSERT_BY_MBATCH, true);

  model.is_getnext_sink_dynamic_ = false;
  model.is_online_infer_dynamic_ = true;
  auto ret = model.InitRealSizeAndShapeInfo(graph, output_node);
  // GetGearAndRealOutShapeInfo without ATTR_NAME_DYNAMIC_OUTPUT_DIMS
  EXPECT_EQ(ret, SUCCESS);
  std::vector<string> dynamic_output_dims = {"0,0,1,1,0,2,2,0,4,3,0,8"};
  (void)AttrUtils::SetListStr(output_node->GetOpDesc(), ATTR_NAME_DYNAMIC_OUTPUT_DIMS, dynamic_output_dims);
  ret = model.InitRealSizeAndShapeInfo(graph, output_node);
  EXPECT_EQ(ret, SUCCESS);

  EXPECT_EQ(model.InitCase(case_node), SUCCESS);
}

TEST_F(UtestDavinciModel, InitRealSizeAndShapeInfo_succ3) {
  DavinciModel model(0, nullptr);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test_graph");

  OpDescPtr data1 = CreateOpDesc("data1", DATA);
  GeTensorDesc shape_desc(GeShape({4,3,224,224}), FORMAT_NCHW, DT_FLOAT);
  data1->AddInputDesc(shape_desc);
  data1->AddOutputDesc(shape_desc);
  NodePtr data1_node = graph->AddNode(data1);

  OpDescPtr shape_node = CreateOpDesc("ascend_mbatch_get_dynamic_dims_node", GETDYNAMICDIMS);
  GeTensorDesc in_tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc out_tensor(GeShape({4,3}), FORMAT_NCHW, DT_FLOAT);
  shape_node->AddInputDesc(in_tensor);
  shape_node->AddOutputDesc(out_tensor);
  NodePtr get_dynamic_dims_node = graph->AddNode(shape_node);

  OpDescPtr output = CreateOpDesc("output1", NETOUTPUT);
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  output->AddInputDesc(tensor);
  output->SetSrcName( { "data1", "ascend_mbatch_get_dynamic_dims_node" } );
  output->SetSrcIndex( { 0, 1 } );
  NodePtr output_node = graph->AddNode(output);
  GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(get_dynamic_dims_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(1));

  (void)AttrUtils::SetStr(output_node->GetOpDesc(), ATTR_ALL_GEARS_INFO, "1,3;;4,3;,3");

  model.is_getnext_sink_dynamic_ = true;
  model.is_online_infer_dynamic_ = false;
  auto ret = model.InitRealSizeAndShapeInfo(graph, output_node);
  EXPECT_EQ(ret, SUCCESS);
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 4;
  ret = model.InitRealSizeAndShapeInfo(graph, output_node);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestDavinciModel, init_data_aipp_info) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);

  NamedAttrs aipp_attr;
  aipp_attr.SetAttr("aipp_mode", GeAttrValue::CreateFrom<int64_t>(domi::AippOpParams_AippMode_dynamic));
  aipp_attr.SetAttr("related_input_rank", GeAttrValue::CreateFrom<int64_t>(0));
  aipp_attr.SetAttr("max_src_image_size", GeAttrValue::CreateFrom<int64_t>(2048));
  aipp_attr.SetAttr("support_rotation", GeAttrValue::CreateFrom<int64_t>(1));
  EXPECT_TRUE(AttrUtils::SetNamedAttrs(op_desc, ATTR_NAME_AIPP, aipp_attr));

  AippConfigInfo aipp_info;
  EXPECT_EQ(model.GetAippInfo(0, aipp_info), ACL_ERROR_GE_AIPP_NOT_EXIST);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetAippInfo(0, aipp_info), SUCCESS);
  EXPECT_EQ(aipp_info.aipp_mode, domi::AippOpParams_AippMode_dynamic);

  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_static) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);

  AttrUtils::SetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, "static_aipp");

  InputAippType aipp_type;
  size_t aipp_index = 0;
  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), PARAM_INVALID);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), SUCCESS);
  EXPECT_EQ(aipp_type, DATA_WITH_STATIC_AIPP);
  EXPECT_EQ(aipp_index, 0xFFFFFFFFu);

  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_dynamic) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);   // op_index 0
  AttrUtils::SetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, "dynamic_aipp");
  AttrUtils::SetStr(op_desc, ATTR_DATA_AIPP_DATA_NAME_MAP, "releated_aipp");

  InputAippType aipp_type;
  size_t aipp_index = 0;
  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), PARAM_INVALID);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), SUCCESS);
  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_releated) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  {
    OpDescPtr op_desc = CreateOpDesc("data", DATA);
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({1024});
    op_desc->SetOutputOffset({1024});
    NodePtr node = graph->AddNode(op_desc);   // op_index 0
    AttrUtils::SetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, "dynamic_aipp");
    AttrUtils::SetStr(op_desc, ATTR_DATA_AIPP_DATA_NAME_MAP, "releated_aipp");
  }
  {
    OpDescPtr op_desc = CreateOpDesc("releated_aipp", DATA);
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({1024});
    op_desc->SetOutputOffset({1024});
    NodePtr node = graph->AddNode(op_desc);   // op_index 1
  }

  InputAippType aipp_type;
  size_t aipp_index = 0;
  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), PARAM_INVALID);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), SUCCESS);
  EXPECT_EQ(aipp_type, DATA_WITH_DYNAMIC_AIPP);
  EXPECT_EQ(aipp_index, 1);

  EXPECT_EQ(model.input_addrs_list_.size(), 2);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_dynamic_conf) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);   // op_index 0
  AttrUtils::SetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, "dynamic_aipp_conf");

  InputAippType aipp_type;
  size_t aipp_index = 0;
  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), PARAM_INVALID);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), SUCCESS);
  EXPECT_EQ(aipp_type, DYNAMIC_AIPP_NODE);
  EXPECT_EQ(aipp_index, 0xFFFFFFFFU);

  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_dynamic_invalid) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);   // op_index 0
  AttrUtils::SetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, "dynamic_aipp_invalid");

  InputAippType aipp_type;
  size_t aipp_index = 0;
  EXPECT_EQ(model.GetAippType(0, aipp_type, aipp_index), PARAM_INVALID);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), ACL_ERROR_GE_AIPP_MODE_INVALID);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_input_info_empty) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);   // op_index 0

  std::vector<string> inputs = {};
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = {};
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);

  OriginInputInfo orig_input_info;
  EXPECT_EQ(model.GetOrigInputInfo(0, orig_input_info), ACL_ERROR_GE_AIPP_NOT_EXIST);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetOrigInputInfo(0, orig_input_info), SUCCESS);
  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_input_info_normal) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);   // op_index 0

  std::vector<string> inputs = { "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);

  OriginInputInfo orig_input_info;
  EXPECT_EQ(model.GetOrigInputInfo(0, orig_input_info), ACL_ERROR_GE_AIPP_NOT_EXIST);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetOrigInputInfo(0, orig_input_info), SUCCESS);
  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_input_info_invalid) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);   // op_index 0

  std::vector<string> inputs = { "NCHW:DT_FLOAT:TensorName" };     // Invalid
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);

  OriginInputInfo orig_input_info;
  EXPECT_EQ(model.GetOrigInputInfo(0, orig_input_info), ACL_ERROR_GE_AIPP_NOT_EXIST);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), ACL_ERROR_GE_AIPP_MODE_INVALID);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetOrigInputInfo(0, orig_input_info), ACL_ERROR_GE_AIPP_NOT_EXIST);
  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

TEST_F(UtestDavinciModel, init_data_aipp_input_dims_normal) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();   // for CustAICPUKernelStore::GetCustAICPUKernelStore()
  model.runtime_param_.mem_base = 0x08000000;
  model.runtime_param_.mem_size = 5120000;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);   // op_index 0

  std::vector<string> inputs = { "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(op_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);

  std::vector<InputOutputDims> input_dims;
  std::vector<InputOutputDims> output_dims;
  EXPECT_EQ(model.GetAllAippInputOutputDims(0, input_dims, output_dims), ACL_ERROR_GE_AIPP_NOT_EXIST);

  std::vector<NodePtr> variable_nodes;
  EXPECT_EQ(model.InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  EXPECT_EQ(model.GetAllAippInputOutputDims(0, input_dims, output_dims), SUCCESS);
  EXPECT_EQ(input_dims.size(), 1);
  EXPECT_EQ(output_dims.size(), 1);

  EXPECT_EQ(model.input_addrs_list_.size(), 1);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_TRUE(model.op_list_.empty());
}

// test label_set_task Init
TEST_F(UtestDavinciModel, label_task_success) {
  DavinciModel model(0, nullptr);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);

  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT32);
  TensorUtils::SetSize(tensor, 64);

  {
    OpDescPtr op_desc = CreateOpDesc("label_switch", LABELSWITCHBYINDEX);
    op_desc->AddInputDesc(tensor);
    op_desc->SetInputOffset({1024});
    NodePtr node = graph->AddNode(op_desc);  // op_index = 0
    EXPECT_TRUE(AttrUtils::SetListInt(op_desc, ATTR_NAME_LABEL_SWITCH_LIST, {0, 1}));

    domi::TaskDef *task_def1 = model_task_def->add_task();
    task_def1->set_stream_id(0);
    task_def1->set_type(RT_MODEL_TASK_STREAM_LABEL_SWITCH_BY_INDEX);
    domi::LabelSwitchByIndexDef *label_task_def = task_def1->mutable_label_switch_by_index();
    label_task_def->set_op_index(op_desc->GetId());
    label_task_def->set_label_max(2);
  }

  {
    OpDescPtr op_desc = CreateOpDesc("label_then", LABELSET);
    NodePtr node = graph->AddNode(op_desc);  // op_index = 1
    EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_LABEL_SWITCH_INDEX, 1));

    domi::TaskDef *task_def1 = model_task_def->add_task();
    task_def1->set_stream_id(0);
    task_def1->set_type(RT_MODEL_TASK_LABEL_SET);
    domi::LabelSetDef *label_task_def = task_def1->mutable_label_set();
    label_task_def->set_op_index(op_desc->GetId());
  }

  {
    OpDescPtr op_desc = CreateOpDesc("label_goto", LABELGOTOEX);
    NodePtr node = graph->AddNode(op_desc);      // op_index = 2
    EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_LABEL_SWITCH_INDEX, 2));

    domi::TaskDef *task_def2 = model_task_def->add_task();
    task_def2->set_stream_id(0);
    task_def2->set_type(RT_MODEL_TASK_STREAM_LABEL_GOTO);
    domi::LabelGotoExDef *label_task_def = task_def2->mutable_label_goto_ex();
    label_task_def->set_op_index(op_desc->GetId());
  }

  {
    OpDescPtr op_desc = CreateOpDesc("label_else", LABELSET);
    NodePtr node = graph->AddNode(op_desc);  // op_index = 3
    EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_LABEL_SWITCH_INDEX, 0));

    domi::TaskDef *task_def1 = model_task_def->add_task();
    task_def1->set_stream_id(0);
    task_def1->set_type(RT_MODEL_TASK_LABEL_SET);
    domi::LabelSetDef *label_task_def = task_def1->mutable_label_set();
    label_task_def->set_op_index(op_desc->GetId());
  }

  {
    OpDescPtr op_desc = CreateOpDesc("label_leave", LABELSET);
    NodePtr node = graph->AddNode(op_desc);  // op_index = 4
    EXPECT_TRUE(AttrUtils::SetInt(op_desc, ATTR_NAME_LABEL_SWITCH_INDEX, 2));

    domi::TaskDef *task_def1 = model_task_def->add_task();
    task_def1->set_stream_id(0);
    task_def1->set_type(RT_MODEL_TASK_LABEL_SET);
    domi::LabelSetDef *label_task_def = task_def1->mutable_label_set();
    label_task_def->set_op_index(op_desc->GetId());
  }

  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_LABEL_NUM, 3));
  model.Assign(ge_model);
  EXPECT_EQ(model.Init(), SUCCESS);
  EXPECT_EQ(model.input_addrs_list_.size(), 0);
  EXPECT_EQ(model.output_addrs_list_.size(), 0);
  EXPECT_EQ(model.task_list_.size(), 5);
}

TEST_F(UtestDavinciModel, LoadWithQueue_fail_with_diff_args) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();
  model.input_queue_ids_.emplace_back(0);
  model.input_queue_ids_.emplace_back(1);
  EXPECT_EQ(model.LoadWithQueue(), ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID); // input queue size mismatch
  EXPECT_EQ(model.input_data_info_.size(), 0);
  ZeroCopyOffset zero_copy_offset;
  model.input_data_info_[0] = zero_copy_offset;
  model.input_data_info_[1] = zero_copy_offset;
  model.output_queue_ids_.emplace_back(0);
  EXPECT_EQ(model.LoadWithQueue(), ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID); // output queue size mismatch
  EXPECT_EQ(model.output_data_info_.size(), 0);
  model.output_data_info_[0] = zero_copy_offset;
  EXPECT_EQ(model.LoadWithQueue(), INTERNAL_ERROR); // AddHeadStream
  EXPECT_EQ(model.active_stream_list_.size(), 0);
}

TEST_F(UtestDavinciModel, LoadWithQueue_ControlInput) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();
  model.input_queue_ids_.emplace_back(0);
  rtStream_t active_stream = nullptr;
  rtStreamCreate(&active_stream, 0);
  model.active_stream_list_ = {active_stream};
  EXPECT_EQ(model.LoadWithQueue(), SUCCESS);
  EXPECT_TRUE(model.use_control_input_queue_);
  rtStreamDestroy(active_stream);
}

TEST_F(UtestDavinciModel, Sink_model_profile) {
  ProfilingManager::Instance().reporter_callback_ = MsprofReport;
  ProfileInfo profile;
  profile.fusion_info.op_name = "relu";

  DavinciModel model(0, nullptr);
  model.profile_list_.emplace_back(profile);
  std::map<std::string, std::pair<uint32_t, uint32_t>> op_info;
  op_info["relu"] = std::pair<uint32_t, uint32_t>(1, 1);
  EXPECT_EQ(model.ReportProfilingData(), SUCCESS);
}

TEST_F(UtestDavinciModel, Sink_time_profile) {
  ProfilingManager::Instance().reporter_callback_ = MsprofReport;
  DavinciModel model(0, nullptr);
  model.SinkTimeProfile(0, 0);
}

class ClassTest {
public:
    virtual ~ClassTest() {}

    virtual int func0() {
        return 0;
    }
    virtual int func1(int a) {
        return a;
    }
    virtual int func2(int a, int b) {
        return a + b;
    }
    virtual int func3(int a, int b) const {
        return a - b;
    }
};

class MockTest : public ClassTest {
public:
    MOCK_METHOD0(func0, int());
    MOCK_METHOD1(func1, int(int a));
    MOCK_METHOD2(func2, int(int a, int b));

    MOCK_CONST_METHOD2(func3, int(int a, int b));
};

TEST_F(UtestDavinciModel, simple_test_gmock) {
    MockTest mock_stub;

    ON_CALL(mock_stub, func0()).WillByDefault(testing::Return(250));
    EXPECT_EQ(mock_stub.func0(), 250);
    EXPECT_EQ(mock_stub.func0(), 250);
    EXPECT_EQ(mock_stub.func0(), 250);

    EXPECT_CALL(mock_stub, func1(testing::_)).Times(2).WillOnce(testing::Return(1024)).WillOnce(testing::Return(250));
    EXPECT_EQ(mock_stub.func1(1), 1024);
    EXPECT_EQ(mock_stub.func1(1), 250);

    EXPECT_CALL(mock_stub, func2(testing::_, 5)).Times(3).WillRepeatedly(testing::Return(1023));
    EXPECT_EQ(mock_stub.func2(1, 5), 1023);
    EXPECT_EQ(mock_stub.func2(2, 5), 1023);
    EXPECT_EQ(mock_stub.func2(3, 5), 1023);
}

TEST_F(UtestDavinciModel, NnExecute) {
  DavinciModel model(0, nullptr);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");
  ProfilingProperties::Instance().is_load_profiling_ = true;

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 2560);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);

  GeTensorDesc tensor(GeShape({1,4,4,8}), FORMAT_NCHW, DT_FLOAT); // sizeof(float) * 1 * 4 * 4 * 8 = 512
  TensorUtils::SetSize(tensor, 512);
  {
    OpDescPtr op_desc = CreateOpDesc("data", DATA);
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({0});
    op_desc->SetOutputOffset({0});
    NodePtr node = graph->AddNode(op_desc);    // op_index = 0
  }

  {
    OpDescPtr op_desc = CreateOpDesc("memcpy", MEMCPYASYNC);
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({512});
    op_desc->SetOutputOffset({512});
    NodePtr node = graph->AddNode(op_desc);

    domi::TaskDef *task_def = model_task_def->add_task();
    task_def->set_stream_id(0);
    task_def->set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
    domi::MemcpyAsyncDef *memcpy_async = task_def->mutable_memcpy_async();
    memcpy_async->set_src(1024);
    memcpy_async->set_dst(1024);
    memcpy_async->set_dst_max(512);
    memcpy_async->set_count(1);
    memcpy_async->set_kind(RT_MEMCPY_DEVICE_TO_DEVICE);
    memcpy_async->set_op_index(op_desc->GetId());
  }

  {
    OpDescPtr op_desc = CreateOpDesc("output", NETOUTPUT);
    op_desc->AddInputDesc(tensor);
    op_desc->SetInputOffset({2048});
    op_desc->SetSrcName( { "memcpy" } );
    op_desc->SetSrcIndex( { 0 } );
    NodePtr node = graph->AddNode(op_desc);  // op_index = 3
  }

  model.Assign(ge_model);
  EXPECT_EQ(model.Init(), SUCCESS);

  rtStream_t stream = nullptr;
  InputData input_data;
  OutputData output_data;
  std::vector<Tensor> outputs;
  EXPECT_EQ(model.GenOutputTensorInfo(output_data, outputs), SUCCESS);
  EXPECT_EQ(output_data.blobs.size(), 1);
  EXPECT_EQ(output_data.blobs[0].length, 512);
  EXPECT_EQ(outputs.size(), 1);
  input_data.blobs = output_data.blobs;
  EXPECT_EQ(input_data.blobs.size(), 1);

  ProfilingManager::Instance().reporter_callback_ = MsprofReport;
  ProfilingManager::Instance().device_id_.emplace_back(0);
  model.task_list_.resize(1);
  EXPECT_EQ(model.NnExecute(stream, false, input_data, output_data), SUCCESS);

  input_data.blobs[0].length = 128; // 128 not enough.
  EXPECT_NE(model.NnExecute(stream, false, input_data, output_data), SUCCESS);
}
TEST_F(UtestDavinciModel, update_io_addr_success) {
  DavinciModel model(0, nullptr);
  uint32_t task_id = 1;
  uint32_t stream_id = 2;
  model.fixed_mem_base_ = 0x22;
  model.mem_base_ = reinterpret_cast<uintptr_t>(&task_id);
  OpDescInfo op_desc_info;
  op_desc_info.op_name = "Save";
  op_desc_info.op_type = "Save";
  op_desc_info.task_id = 1;
  op_desc_info.stream_id = 2;
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
  model.exception_dumper_.op_desc_info_ = { op_desc_info };
  std::vector<void *> io_addr = {nullptr, nullptr};
  model.UpdateOpIOAddrs(task_id, stream_id, io_addr);
  model.mem_base_ = 0;
}
TEST_F(UtestDavinciModel, get_total_memsize_exclude_zero_copy) {
  DavinciModel model(0, nullptr);
  model.runtime_param_.mem_size = 1024;
  model.runtime_param_.zero_copy_size = 2048;
  int64_t total_useful_size = 0;
  EXPECT_EQ(model.GetTotalMemSizeExcludeZeroCopy(total_useful_size), FAILED);
  EXPECT_EQ(total_useful_size, 0);
  model.runtime_param_.zero_copy_size = 512;
  EXPECT_EQ(model.GetTotalMemSizeExcludeZeroCopy(total_useful_size), SUCCESS);
  EXPECT_EQ(total_useful_size, 512);
}

// test InitTbeHandle
TEST_F(UtestDavinciModel, init_tbe_handle) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  // !IsTbeTask, success.
  EXPECT_EQ(model.InitTbeHandle(op_desc), SUCCESS);

  // without kernel
  AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::TVM));
  EXPECT_EQ(model.InitTbeHandle(op_desc), INTERNAL_ERROR);

  std::vector<char> buffer;
  string key = op_desc->GetName();
  TBEKernelPtr tbe_kernel_ptr = MakeShared<OpKernelBin>(key, std::move(buffer));
  op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
  string attr_kernel_name = op_desc->GetName() + "_kernelname";
  string kernel_name = "kernel_name";
  AttrUtils::SetStr(op_desc, attr_kernel_name, kernel_name);
  EXPECT_EQ(model.InitTbeHandle(op_desc), SUCCESS);
}

TEST_F(UtestDavinciModel, test_update_io_task_args) {
  DavinciModel model(0, nullptr);
  map<uint32_t, ZeroCopyOffset> data_infos;
  ZeroCopyOffset data_info;
  data_info.addr_count_ = 1;
  data_info.data_count_ = 1;
  std::vector<uintptr_t> addr_offset{0x11110000U};
  uintptr_t addr = 0x12340000U;
  std::map<uintptr_t, std::vector<uintptr_t>> outside_addr{{addr, addr_offset}};
  data_info.outside_addrs_.emplace_back(outside_addr);
  uint32_t index = 0;

  data_info.data_info_.emplace_back(std::pair<int64_t, void *>(0, (void *)0x11110000));
  data_info.relative_offset_.emplace_back(0);
  data_infos[index] = data_info;

  DataBuffer data_buffer;
  uint8_t buffer_data = 67;
  data_buffer.data = &buffer_data;
  std::vector<DataBuffer> blobs = { data_buffer };
  EXPECT_EQ(blobs.size(), data_infos.size());

  for (const auto &data : data_infos) {
    EXPECT_LT(data.first, blobs.size());
    const DataBuffer &buffer = blobs[data.first];
    EXPECT_TRUE(buffer.data != nullptr);
    EXPECT_TRUE(model.CheckUserAndModelSize(buffer.length, data.second.GetDataSize(), true, true));
  }

  set<const void *> copy_only_addrs;
  model.copy_only_addrs_ = copy_only_addrs;

  addr = 0;
  set<size_t> offset_set {0, 1, 2};
  map<uintptr_t, set<size_t>> task_addr_offset;
  task_addr_offset[0] = offset_set;
  std::vector<uint8_t> args_info = {0, 1, 2};

  ZeroCopyTask task0("task0", 0, 0);
  task0.batch_label_ = "abc";
  task0.task_addr_offset_ = task_addr_offset;
  task0.args_info_ = args_info;

  ZeroCopyTask task1("task1", 0, 0);
  task1.batch_label_ = "kDefaultBatchLable";
  task0.task_addr_offset_ = task_addr_offset;
  task0.args_info_ = args_info;

  model.zero_copy_tasks_.emplace_back(task0);
  model.zero_copy_tasks_.emplace_back(task1);

  EXPECT_EQ(model.UpdateIoTaskArgs(data_infos, true, blobs, true, "batch_label"), SUCCESS);
}

TEST_F(UtestDavinciModel, run_with_task) {
  DavinciModel model(0, nullptr);
  model.SetId(1);

  InputData input_data;
  OutputData output_data;
  auto data = MakeShared<InputDataWrapper>(input_data, output_data);
  model.data_inputer_.Push(data);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_PROFILER_TRACE);
  task->stream_id_ = 0;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };
  TaskInfoPtr task_info = MakeShared<ProfilerTraceTaskInfo>();
  model.task_list_.push_back(task_info);
  model.ModelRunStart();
  sleep(5);
  model.ModelRunStop();
}

// test label_set_task Init
TEST_F(UtestDavinciModel, test_task_distribute_ffts_plus) {
  DavinciModel model(0, nullptr);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeModelPtr ge_model = MakeShared<GeModel>();
  model.ge_model_ = ge_model;
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  shared_ptr<domi::ModelTaskDef> model_task_def = MakeShared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);

  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT32);
  TensorUtils::SetSize(tensor, 64);
  OpDescPtr op_ = CreateOpDesc("ffts_plus_fun", "ssss");
  NodePtr node = graph->AddNode(op_);  // op_index = 0
  model.op_list_[op_->GetId()] = op_;
  domi::TaskDef *task_def1 = model_task_def->add_task();
  task_def1->set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def1->mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(op_->GetId());
  //std::shared_ptr<FftsPlusTaskInfo> ffts_plus_task = MakeShared<FftsPlusTaskInfo>();
  model.task_list_.resize(1);
  model.task_list_[0] = TaskInfoFactory::Instance().Create(static_cast<rtModelTaskType_t>(task_def1->type()));
  std::shared_ptr<FftsPlusTaskInfo> ffts_plus_task = std::dynamic_pointer_cast<FftsPlusTaskInfo>(model.task_list_[0]);
  ffts_plus_task->davinci_model_ = &model;
  {
    OpDescPtr op_desc = CreateOpDesc("label_then", LABELSET);
    //model.op_list_[op_desc->GetId()] = op_desc;
    NodePtr node1 = graph->AddNode(op_desc);  // op_index = 1
    domi::FftsPlusCtxDef* ctx_task_def = ffts_plus_task_def->add_ffts_plus_ctx();
    //ctx_task_def->set_op_index(op_desc->GetId());
    ctx_task_def->set_op_index(6);
    ctx_task_def->set_context_type(RT_CTX_TYPE_AICORE);
    ctx_task_def->set_context_id(0);
  }

  {
    OpDescPtr op_desc = CreateOpDesc("label_else", LABELSET);
    NodePtr node1 = graph->AddNode(op_desc);  // op_index = 1
    domi::FftsPlusCtxDef* ctx_task_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ctx_task_def->set_op_index(op_desc->GetId());
    ctx_task_def->set_context_type(RT_CTX_TYPE_MIX_AIC);
    ctx_task_def->set_context_id(1);
    model.op_list_[op_desc->GetId()] = op_desc;
  }

  {
    OpDescPtr op_desc = CreateOpDesc("label_leave", LABELSET);
    NodePtr node1 = graph->AddNode(op_desc);  // op_index = 1
    domi::FftsPlusCtxDef* ctx_task_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ctx_task_def->set_op_index(op_desc->GetId());
    ctx_task_def->set_context_type(RT_CTX_TYPE_LABEL);
    ctx_task_def->set_context_id(2);
    model.op_list_[op_desc->GetId()] = op_desc;
  }
  {
    OpDescPtr op_desc = CreateOpDesc("label_leat", LABELSET);
    NodePtr node1 = graph->AddNode(op_desc);  // op_index = 1
    domi::FftsPlusCtxDef* ctx_task_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ctx_task_def->set_op_index(op_desc->GetId());
    ctx_task_def->set_context_type(RT_CTX_TYPE_AICPU);
    ctx_task_def->set_context_id(3);
    model.op_list_[op_desc->GetId()] = op_desc;
  }

  {
    OpDescPtr op_desc = CreateOpDesc("label_leave", LABELSET);
    NodePtr node1 = graph->AddNode(op_desc);  // op_index = 1
    domi::FftsPlusCtxDef* ctx_task_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ctx_task_def->set_op_index(op_desc->GetId());
    ctx_task_def->set_context_type(RT_CTX_TYPE_AT_START);
    ctx_task_def->set_context_id(4);
    model.op_list_[op_desc->GetId()] = op_desc;
  }
  EXPECT_EQ(model.DistributeTask(*model_task_def), SUCCESS);
}
// test InitTbeHandle
TEST_F(UtestDavinciModel, init_mix_tbe_handle) {
  DavinciModel model(0, nullptr);
  model.ge_model_ = MakeShared<GeModel>();
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  // !IsTbeTask, success.
  AttrUtils::SetInt(op_desc, ATTR_NAME_IMPLY_TYPE, static_cast<uint32_t>(domi::ImplyType::TVM));
  // without kernel
  (void) AttrUtils::SetStr(op_desc, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, std::string("MIX_AIC"));
  AttrUtils::SetStr(op_desc, std::string("_mix_aic") + ATTR_NAME_TBE_KERNEL_NAME, "Conv2d");
  EXPECT_EQ(model.InitTbeHandle(op_desc), INTERNAL_ERROR);
}

TEST_F(UtestDavinciModel, TestAllocateResources) {
  DavinciModel model(0, nullptr);
  uint8_t weight[1024] {};
  model.weights_mem_base_ = reinterpret_cast<uintptr_t>(weight);
  model.runtime_param_.weight_base = reinterpret_cast<uintptr_t>(weight);
  model.runtime_param_.weight_size = 1024;

  GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_UINT32);
  TensorUtils::SetDataOffset(tensor_desc, 512);
  TensorUtils::SetSize(tensor_desc, sizeof(uint32_t));
  OpDescPtr op_desc = CreateOpDesc("Enqueue", "Enqueue");
  op_desc->AddInputDesc(tensor_desc);
  auto graph = MakeShared<ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  // no resource to create
  ASSERT_EQ(model.AllocateResource(*node), SUCCESS);

  // wrong attribute type
  AttrUtils::SetStr(op_desc, "_resource_list", "wrong-attr-type");
  ASSERT_EQ(model.AllocateResource(*node), INTERNAL_ERROR);

  // add queue resource
  std::vector<NamedAttrs> resources(1);
  NamedAttrs &queue_resource = resources.back();
  op_desc->DelAttr("_resource_list");
  AttrUtils::SetListNamedAttrs(op_desc, "_resource_list", resources);

  // no resource type
  ASSERT_EQ(model.AllocateResource(*node), PARAM_INVALID);

  // unsupported resource type
  AttrUtils::SetStr(queue_resource, "resource_type", "RES_SUPPORTED");
  AttrUtils::SetListNamedAttrs(op_desc, "_resource_list", resources);
  ASSERT_EQ(model.AllocateResource(*node), UNSUPPORTED);

  // missing queue name
  AttrUtils::SetStr(queue_resource, "resource_type", "RES_QUEUE");
  AttrUtils::SetListNamedAttrs(op_desc, "_resource_list", resources);
  ASSERT_EQ(model.AllocateResource(*node), PARAM_INVALID);

  // missing queue id idx
  AttrUtils::SetStr(queue_resource, "queue_name", "some_queue");
  AttrUtils::SetListNamedAttrs(op_desc, "_resource_list", resources);
  ASSERT_EQ(model.AllocateResource(*node), PARAM_INVALID);

  // no source node
  AttrUtils::SetInt(queue_resource, "queue_id_idx", 0);
  AttrUtils::SetListNamedAttrs(op_desc, "_resource_list", resources);
  ASSERT_EQ(model.AllocateResource(*node), PARAM_INVALID);

  // source is not a const
  OpDescPtr src_op_desc = CreateOpDesc("SrcNode", "NotConst");
  src_op_desc->AddOutputDesc(tensor_desc);
  auto src_node = graph->AddNode(src_op_desc);
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), node->GetInDataAnchor(0));
  ASSERT_EQ(model.AllocateResource(*node), PARAM_INVALID);

  // SUCCESS
  op_desc->SetIsInputConst(std::vector<bool>{true});
  src_op_desc->SetType(CONSTANT);
  ASSERT_EQ(model.AllocateResource(*node), SUCCESS);

  ASSERT_EQ(model.aicpu_resources_.aicpu_queues_.count("some_queue"), 1);
  uint32_t queue_id = model.aicpu_resources_.aicpu_queues_["some_queue"];
  uint32_t op_queue_id = *reinterpret_cast<uint32_t*>(weight + 512);
  ASSERT_EQ(queue_id, op_queue_id);

  // another node with same queue_name
  TensorUtils::SetDataOffset(*op_desc->MutableInputDesc(0), 256);
  ASSERT_EQ(model.AllocateResource(*node), SUCCESS);
  ASSERT_EQ(model.aicpu_resources_.aicpu_queues_.count("some_queue"), 1);
  op_queue_id = *reinterpret_cast<uint32_t*>(weight + 256);
  ASSERT_EQ(queue_id, op_queue_id);

  model.aicpu_resources_.ReleaseResources();
  ASSERT_TRUE(model.aicpu_resources_.aicpu_queues_.empty());
}

TEST_F(UtestDavinciModel, TestAllocateChannelResources) {
  DavinciModel model(0, nullptr);
  uint8_t weight[1024] {};
  model.weights_mem_base_ = reinterpret_cast<uintptr_t>(weight);
  model.runtime_param_.weight_base = reinterpret_cast<uintptr_t>(weight);
  model.runtime_param_.weight_size = 1024;

  GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_UINT32);
  TensorUtils::SetDataOffset(tensor_desc, 512);
  TensorUtils::SetSize(tensor_desc, sizeof(uint32_t));
  OpDescPtr op_desc = CreateOpDesc("Enqueue", "Enqueue");
  op_desc->AddInputDesc(tensor_desc);
  auto graph = MakeShared<ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);

  // add channel resource
  std::vector<NamedAttrs> resources(1);
  NamedAttrs &channel_resource = resources.back();
  AttrUtils::SetListNamedAttrs(op_desc, "_resource_list", resources);

  AttrUtils::SetStr(channel_resource, "resource_type", "RES_CHANNEL");
  AttrUtils::SetListNamedAttrs(op_desc, "_resource_list", resources);

  ASSERT_NE(model.AllocateResource(*node), SUCCESS);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };
  ASSERT_EQ(model.AllocateResource(*node), SUCCESS);
  ASSERT_EQ(model.aicpu_resources_.aicpu_channels_.size(), 1);

  // repeat execution
  rtStream_t stream2 = nullptr;
  rtStreamCreate(&stream2, 0);
  model.stream_list_ = { stream, stream2 };
  ASSERT_EQ(model.AllocateResource(*node), SUCCESS);
  ASSERT_EQ(model.aicpu_resources_.aicpu_channels_.size(), 1);

  model.aicpu_resources_.ReleaseResources();
  ASSERT_TRUE(model.aicpu_resources_.aicpu_channels_.empty());
}

TEST_F(UtestDavinciModel, save_profile_task_info) {
  DavinciModel model(0, nullptr);
  model.SetId(1);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  task->stream_id_ = 0;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task->mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(0);
  ffts_plus_task_def->set_addr_size(2);
  InitTaskSQEInfo(ffts_plus_task_def);
  domi::FftsPlusCtxDef *aicaivctx = ffts_plus_task_def->add_ffts_plus_ctx();
  aicaivctx->set_op_index(0);
  aicaivctx->set_context_type(static_cast<uint32_t>(RT_CTX_TYPE_AICORE));
  domi::FftsPlusAicAivCtxDef *aicaivdef = aicaivctx->mutable_aic_aiv_ctx();
  InitAicAivCtx(aicaivdef);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };
  std::shared_ptr<FftsPlusTaskInfo> ffts_plus_task_info = MakeShared<FftsPlusTaskInfo>();
  ffts_plus_task_info->task_id_ = 0;
  TaskInfoPtr task_info = ffts_plus_task_info;
  model.task_list_.push_back(task_info);
  OpDescPtr op_desc = CreateOpDesc("test", PARTITIONEDCALL);
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  model.op_list_[0] = op_desc;
  model.SaveProfilingTaskDescInfo(op_desc, *task_info, *task);
}

TEST_F(UtestDavinciModel, init_model_profile_ffts_plus) {
  DavinciModel model(0, nullptr);
  model.SetId(1);

  OpDescPtr op_desc = CreateOpDesc("test", PARTITIONEDCALL, 1, 1);
  EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::vector<std::string>{"1", "2"}));
  model.op_list_[op_desc->GetId()] = op_desc;

  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_FFTS_PLUS_TASK);
  task->stream_id_ = 0;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task->mutable_ffts_plus_task();
  ffts_plus_task_def->set_op_index(op_desc->GetId());
  ffts_plus_task_def->set_addr_size(2);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };
  std::shared_ptr<FftsPlusTaskInfo> ffts_plus_task_info = MakeShared<FftsPlusTaskInfo>();
  ffts_plus_task_info->task_id_ = 0;
  FusionOpInfo fusion_op_info;
  fusion_op_info.op_index = op_desc->GetId();
  fusion_op_info.stream_id = 0;
  fusion_op_info.original_op_names.push_back("conv");
  fusion_op_info.original_op_names.push_back("add");
  ffts_plus_task_info->fusion_op_info_.emplace_back(fusion_op_info);
  TaskInfoPtr task_info = ffts_plus_task_info;
  model.task_list_.push_back(task_info);

  EXPECT_EQ(model.InitFusionProfiling(fusion_op_info), SUCCESS);
}

TEST_F(UtestDavinciModel, parse_inputs_dims_data) {
  DavinciModel model(0, nullptr);

  OmeContext context;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  const auto data1 = CreateNode(*compute_graph, DATA, "data1", 1, 1);
  const auto next1 = CreateNode(*compute_graph, GETNEXT, "data1", 1, 1);

  std::vector<std::vector<int64_t>> tensor_input_dims;
  std::vector<vector<int64_t>> user_real_input_dims;

  model.SetRunContext(context);
  EXPECT_EQ(model.ParseInputsDims(tensor_input_dims, user_real_input_dims), SUCCESS);  // dynamic_node_type is empty, just return

  context.dynamic_node_type = DATA;
  model.SetRunContext(context);
  EXPECT_EQ(model.ParseInputsDims(tensor_input_dims, user_real_input_dims), SUCCESS);  // ParseInputsDimsForData

  context.getnext_nosink_nodes.emplace_back(next1);
  model.SetRunContext(context);
  EXPECT_EQ(model.ParseInputsDims(tensor_input_dims, user_real_input_dims), SUCCESS);  // ParseInputsDimsForGetNexNosinkAndData
}

TEST_F(UtestDavinciModel, parse_inputs_dims_getnext) {
  DavinciModel model(0, nullptr);

  OmeContext context;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  const auto data1 = CreateNode(*compute_graph, DATA, "data1", 1, 1);
  const auto next1 = CreateNode(*compute_graph, GETNEXT, "data1", 1, 1);

  std::vector<std::vector<int64_t>> tensor_input_dims;
  std::vector<vector<int64_t>> user_real_input_dims;

  tensor_input_dims.emplace_back(std::vector<int64_t>{});
  context.dynamic_node_type = GETNEXT;
  model.SetRunContext(context);
  EXPECT_EQ(model.ParseInputsDims(tensor_input_dims, user_real_input_dims), SUCCESS);  // just getnext_sink

  context.getnext_nosink_nodes.emplace_back(next1);
  model.SetRunContext(context);
  EXPECT_EQ(model.ParseInputsDims(tensor_input_dims, user_real_input_dims), SUCCESS);  // ParseInputsDimsForData

  context.data_nodes.emplace_back(data1);
  model.SetRunContext(context);
  EXPECT_EQ(model.ParseInputsDims(tensor_input_dims, user_real_input_dims), PARAM_INVALID);  // ParseInputsDimsForGetNexNosinkAndData
  AttrUtils::SetInt(next1->GetOpDesc(), ATTR_NAME_INDEX, 0);
  EXPECT_EQ(model.ParseInputsDims(tensor_input_dims, user_real_input_dims), SUCCESS);  // ParseInputsDimsForGetNexNosinkAndData
}

TEST_F(UtestDavinciModel, parse_queue_data) {
  DavinciModel model(0, nullptr);
  uint8_t mem[1024] {};
  model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(mem);
  model.runtime_param_.mem_size = sizeof(mem);
  std::set<const void *> input_outside_addrs;
  ASSERT_EQ(model.InitQueueDataNodes({}, 0, input_outside_addrs), SUCCESS);

  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  const auto queue_data_1 = CreateNode(*compute_graph, QUEUE_DATA, "queue_data_1", 0, 1);
  const auto queue_data_2 = CreateNode(*compute_graph, QUEUE_DATA, "queue_data_2", 0, 1);
  // multiple QueueData
  ASSERT_EQ(model.InitQueueDataNodes({queue_data_1, queue_data_2}, 0, input_outside_addrs), UNSUPPORTED);
  // not in LoadModelWithQ
  ASSERT_EQ(model.InitQueueDataNodes({queue_data_1}, 0, input_outside_addrs), UNSUPPORTED);
  model.output_queue_ids_ = {1};
  // missing attribute: queue_name
  ASSERT_EQ(model.InitQueueDataNodes({queue_data_1}, 0, input_outside_addrs), PARAM_INVALID);
  // success
  AttrUtils::SetStr(queue_data_1->GetOpDesc(), "queue_name", "some_name");
  ASSERT_EQ(model.InitQueueDataNodes({queue_data_1}, 0, input_outside_addrs), SUCCESS);
}

static NodePtr CreateNodeV2(ComputeGraph &graph, const string &name, const string &type, int in_num, int out_num) {
  OpDescPtr op_desc = MakeShared<OpDesc>(name, type);
  op_desc->SetStreamId(0);
  static int32_t index = 0;
  op_desc->SetId(index++);

  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
  TensorUtils::SetSize(tensor, 64);
  std::vector<int64_t> input_offset;
  for (int i = 0; i < in_num; i++) {
    input_offset.emplace_back(index * 64 + i * 64);
  }
  op_desc->SetInputOffset(input_offset);

  std::vector<int64_t> output_offset;
  for (int i = 0; i < out_num; i++) {
    op_desc->AddOutputDesc(tensor);
    output_offset.emplace_back(index * 64 + in_num * 64 + i * 64);
  }
  op_desc->SetOutputOffset(output_offset);
  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetOpKernelLibName("DNN_VM_RTS_OP_STORE");

  return graph.AddNode(op_desc);
}

TEST_F(UtestDavinciModel, TestIsInputOfNetoutputCanZeroCopy) {
  std::map<std::string, std::string> graph_options;
  graph_options[OPTION_EXEC_REUSE_ZERO_COPY_MEMORY] = "1";
  GetThreadLocalContext().SetGraphOption(graph_options);

  {
    uint8_t mem[1024] {};
    std::vector<OpDescPtr> output_op_list;
    std::set<const void *> output_outside_addrs;
    std::shared_ptr<ComputeGraph> graph(new (std::nothrow) ComputeGraph("graph"));
    NodePtr data1 = CreateNodeV2(*graph, "data1", DATA, 0, 1);
    NodePtr data2 = CreateNodeV2(*graph, "data2", DATA, 0, 1);
    NodePtr netoutput = CreateNodeV2(*graph, "netoutput", NETOUTPUT, 2, 0);
    data1->GetOpDesc()->MutableOutputDesc(0)->SetAttr(ATTR_IS_ZERO_COPY_BLOCK, GeAttrValue::CreateFrom<bool>(true));
    data2->GetOpDesc()->MutableOutputDesc(0)->SetAttr(ATTR_IS_ZERO_COPY_BLOCK, GeAttrValue::CreateFrom<bool>(true));
    netoutput->AddLinkFrom(data1);
    netoutput->AddLinkFrom(data2);

    DavinciModel model(0, nullptr);
    output_op_list.clear();
    output_outside_addrs.clear();
    model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(mem);
    model.runtime_param_.mem_size = sizeof(mem);

    uint32_t data_op_index = 0U;
    std::map<uint32_t, OpDescPtr> data_by_index;
    EXPECT_EQ(model.InitDataOp(graph, data1, data_op_index, data_by_index, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.InitDataOp(graph, data2, data_op_index, data_by_index, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.InitNetOutput(graph, netoutput, output_op_list, output_outside_addrs), SUCCESS);
    EXPECT_TRUE(model.copy_only_addrs_.empty()); // All outputs can zero-copy
  }

  {
    uint8_t mem[1024] {};
    std::vector<OpDescPtr> output_op_list;
    std::set<const void *> output_outside_addrs;
    std::shared_ptr<ComputeGraph> graph(new (std::nothrow) ComputeGraph("graph"));
    NodePtr data1 = CreateNodeV2(*graph, "data1", DATA, 0, 1);
    NodePtr data2 = CreateNodeV2(*graph, "data2", DATA, 0, 1);
    NodePtr netoutput = CreateNodeV2(*graph, "netoutput", NETOUTPUT, 2, 0);
    netoutput->AddLinkFrom(data1);
    netoutput->AddLinkFrom(data2);

    data1->GetOpDesc()->MutableOutputDesc(0)->SetAttr(ATTR_IS_ZERO_COPY_BLOCK, GeAttrValue::CreateFrom<bool>(true));

    DavinciModel model(0, nullptr);
    output_op_list.clear();
    output_outside_addrs.clear();
    model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(mem);
    model.runtime_param_.mem_size = sizeof(mem);

    uint32_t data_op_index = 0U;
    std::map<uint32_t, OpDescPtr> data_by_index;
    EXPECT_EQ(model.InitDataOp(graph, data1, data_op_index, data_by_index, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.InitDataOp(graph, data2, data_op_index, data_by_index, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.InitNetOutput(graph, netoutput, output_op_list, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.copy_only_addrs_.size(), 2); // one input + one output
  }

  {
    uint8_t mem[1024] {};
    std::vector<OpDescPtr> output_op_list;
    std::set<const void *> output_outside_addrs;
    std::shared_ptr<ComputeGraph> graph(new (std::nothrow) ComputeGraph("graph"));
    NodePtr data1 = CreateNodeV2(*graph, "data1", DATA, 0, 1);
    NodePtr data2 = CreateNodeV2(*graph, "data2", DATA, 0, 1);
    NodePtr netoutput = CreateNodeV2(*graph, "netoutput", NETOUTPUT, 2, 0);
    netoutput->AddLinkFrom(data1);
    netoutput->AddLinkFrom(data2);

    DavinciModel model(0, nullptr);
    output_op_list.clear();
    output_outside_addrs.clear();
    model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(mem);
    model.runtime_param_.mem_size = sizeof(mem);

    uint32_t data_op_index = 0U;
    std::map<uint32_t, OpDescPtr> data_by_index;
    EXPECT_EQ(model.InitDataOp(graph, data1, data_op_index, data_by_index, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.InitDataOp(graph, data2, data_op_index, data_by_index, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.InitNetOutput(graph, netoutput, output_op_list, output_outside_addrs), SUCCESS);
    EXPECT_EQ(model.copy_only_addrs_.size(), 4);  // two input + two output
  }

  {
    uint8_t mem[1024] {};
    std::vector<OpDescPtr> output_op_list;
    std::set<const void *> output_outside_addrs;
    std::shared_ptr<ComputeGraph> graph(new (std::nothrow) ComputeGraph("graph"));
    ge::NodePtr data1 = CreateNodeV2(*graph, "data1", DATA, 0, 1);
    ge::NodePtr netoutput = CreateNodeV2(*graph, "netoutput", NETOUTPUT, 1, 0);
    netoutput->AddLinkFrom(data1);

    DavinciModel model(0, nullptr);
    output_op_list.clear();
    output_outside_addrs.clear();
    model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(mem);
    model.runtime_param_.mem_size = sizeof(mem);
    (void)AttrUtils::SetListInt(netoutput->GetOpDesc(), "_op_max_size", {100000});

    uint32_t data_op_index = 0U;
    std::map<uint32_t, OpDescPtr> data_by_index;
    EXPECT_EQ(model.InitDataOp(graph, data1, data_op_index, data_by_index, output_outside_addrs), SUCCESS);
  }

  graph_options[OPTION_EXEC_REUSE_ZERO_COPY_MEMORY] = "";
  GetThreadLocalContext().SetGraphOption(graph_options);
}

TEST_F(UtestDavinciModel, copy_input_data_null_tensor) {
  DavinciModel model(0, nullptr);
  InputData input_data;
  std::vector<int64_t> shape = {0};
  input_data.shapes.push_back(shape);
  DataBuffer buffer(nullptr, 0, false);
  input_data.blobs.push_back(buffer);
  model.input_data_info_.emplace(0, ZeroCopyOffset());
  EXPECT_EQ(model.CopyInputData(input_data), SUCCESS);
}

TEST_F(UtestDavinciModel, prof_fusion_op_info_test) {
  OpDescPtr op_desc = CreateOpDesc("fusion_op_1", "Enqueue");
  GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_UINT32);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  const std::vector<int64_t> workspace_bytes = {1,2,3};
  op_desc->SetWorkspaceBytes(workspace_bytes);
  const std::vector<uint8_t> weights_value(64, 'A');
  GeTensorPtr weight_value = MakeShared<GeTensor>(op_desc->GetOutputDesc(0), weights_value.data(), weights_value.size());
  AttrUtils::SetTensor(*op_desc, ATTR_NAME_WEIGHTS, weight_value);
  ProfileInfo profile{};
  profile.fusion_info.original_op_names = {"op_1", "op_2", "op_3", "op_4", "op_5", "op_6","op_7","op_8","op_9"};
  profile.fusion_info.op_name = "fusion_op_1";
  ProfilingManager::Instance().reporter_callback_ = MsprofReport;
  DavinciModel davinci_model(0, nullptr);
  davinci_model.ProfFusionOpInfo(op_desc, profile.fusion_info, profile);
  EXPECT_EQ(2, profile.prof_fusion_data_lst.size());
  std::string actual_string(profile.prof_fusion_data_lst[0].fusionName.data.dataStr);
  EXPECT_EQ("fusion_op_1",  actual_string);
  EXPECT_EQ(66, profile.prof_fusion_data_lst[0].fusionOp[0]);
  EXPECT_EQ(9, profile.prof_fusion_data_lst[0].fusionOpNum);
  ProfilingManager::Instance().reporter_callback_ = nullptr;
}

TEST_F(UtestDavinciModel, GetSomeInfo) {
  DavinciModel model(0, nullptr);
  model.SetKnownNode(true);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 5120000);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_input = CreateOpDesc("data", DATA);
  op_input->AddInputDesc(tensor);
  op_input->AddOutputDesc(tensor);
  op_input->SetInputOffset({1024});
  op_input->SetOutputOffset({1024});
  NodePtr node_input = graph->AddNode(op_input);    // op_index = 0

  OpDescPtr op_output = CreateOpDesc("output", NETOUTPUT);
  op_output->AddInputDesc(tensor);
  op_output->SetInputOffset({5120});
  op_output->SetSrcName( { "memcpy" } );
  op_output->SetSrcIndex( { 0 } );
  NodePtr node_output = graph->AddNode(op_output);  // op_index = 1

  // get input output
  std::vector<InputOutputDescInfo> input_descs;
  std::vector<InputOutputDescInfo> output_descs;
  std::vector<uint32_t> input_formats;
  std::vector<uint32_t> output_formats;
   bool by_dims = false;
  auto ret = model.GetInputOutputDescInfo(input_descs, output_descs, output_formats, output_formats, by_dims);
  EXPECT_EQ(ret, SUCCESS);

  // get dynamic batch
  std::vector<std::vector<int64_t>> batch_info;
  int32_t dynamic_type;
  ret = model.GetDynamicBatchInfo(batch_info, dynamic_type);
  EXPECT_EQ(ret, SUCCESS);

  // get Combined Dynamic Dims
  model.GetCombinedDynamicDims(batch_info);
  EXPECT_EQ(batch_info.size(), 0);

  // get User Designate Shape Order
  std::vector<std::string> user_input_shape_order;
  model.GetUserDesignateShapeOrder(user_input_shape_order);
  EXPECT_EQ(user_input_shape_order.size(), 0);

  // get Flowctrl Index
  uint32_t op_index = 0;
  auto ret2 = model.GetFlowctrlIndex(op_index);
  EXPECT_EQ(ret2, 0);

  // get CurDynamic Dims
  std::vector<std::vector<int64_t>> tensor_input_dims;
  std::vector<int32_t> cur_dynamic_dims;
  auto ret3 = model.GetCurDynamicDims(tensor_input_dims, cur_dynamic_dims);
  EXPECT_EQ(ret3, INTERNAL_ERROR);
}

TEST_F(UtestDavinciModel, SetSomething) {
  DavinciModel model(0, nullptr);
  model.SetKnownNode(true);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 5120000);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  // Set EndGraphId
  uint32_t task_id = 1;
  uint32_t stream_id = 2;
  model.SetEndGraphId(task_id, stream_id);

  // Set DynamicSize
  std::vector<uint64_t> batch_num;
  int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH);
  model.SetDynamicSize(batch_num, dynamic_type);
  batch_num.emplace_back(1);
  model.SetDynamicSize(batch_num, dynamic_type);
  EXPECT_EQ(model.dynamic_type_, dynamic_type);
}

TEST_F(UtestDavinciModel, InitSomething) {
  DavinciModel model(0, nullptr);
  OpDescPtr op_desc = CreateOpDesc("data", DATA);

  // Init StreamActive
  //op_desc->SetAttr(ATTR_NAME_SWITCH_BRANCH_NODE_LABEL, 0);
  auto ret = model.InitStreamActive(op_desc);
  EXPECT_EQ(ret, SUCCESS);

  // Init StreamSwitch
  ret = model.InitStreamSwitch(op_desc);
  EXPECT_EQ(ret, INTERNAL_ERROR);
}

TEST_F(UtestDavinciModel, DavinciModel_HeadFile) {
  DavinciModel model(0, nullptr);
  std::map<int64_t, std::vector<rtStream_t>> hccl_flow;
  hccl_flow = model.GetHcclFolowStream();
  EXPECT_EQ(hccl_flow.size(), 0);

  uint32_t offset = 0;
  auto addr =  model.GetCurrentHybridArgsAddr(offset);
  string om_name = "om_abc";
  model.SetOmName(om_name);
  EXPECT_EQ(model.om_name_, om_name);
}

// for coverage all below test
TEST_F(UtestDavinciModel, IsInputOfNetoutputCanZeroCopy_fail) {
  DavinciModel *model = new DavinciModel(0, nullptr);
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");
  OpDescPtr op_input = CreateOpDesc("data", DATA);
  NodePtr node_input = graph->AddNode(op_input);

  model->DisableZeroCopyInReuseMemoryMode(node_input, 10, nullptr);

  EXPECT_EQ(model->copy_only_addrs_.size(), 1);

  model->die_id_ = 10;
  g_runtime_stub_mock = "rtSetDie";
  delete model;
}

TEST_F(UtestDavinciModel, Assign_fail) {
  DavinciModel *model = new DavinciModel(0, nullptr);

  model->Assign(nullptr);
  EXPECT_EQ(model->ge_model_, nullptr);

  void *addr1 = new uint8_t[1];
  void *addr2 = new uint8_t[1];
  model->stream_2_event_[addr1] = addr2;
  g_runtime_stub_mock = "rtEventDestroy";
  delete model;
}

TEST_F(UtestDavinciModel, InitWeightMem_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  uintptr_t mem_ptr = 0;
  uintptr_t weight_ptr = 0;
  size_t weight_size = 10;

  model.is_weight_mem_has_inited_ = true;
  EXPECT_EQ(model.InitWeightMem(mem_ptr, weight_ptr, weight_size), FAILED);

  model.is_weight_mem_has_inited_ = false;
  ge_model->weights_buffer_ = Buffer(20, 0);
  EXPECT_NE(model.InitWeightMem(mem_ptr, weight_ptr, weight_size), FAILED);   //??

  weight_size = 30;
  g_runtime_stub_mock = "rtMalloc";
  EXPECT_EQ(model.InitWeightMem(mem_ptr, weight_ptr, weight_size), FAILED);
}

TEST_F(UtestDavinciModel, InitFeatureMapAndP2PMem_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  uintptr_t mem_ptr = 10;
  size_t mem_size = 10;

  model.is_feature_map_mem_has_inited_ = false;
  std::map<std::string, std::string> param;
  param[OPTION_EXEC_REUSE_ZERO_COPY_MEMORY] = "1";
  GetThreadLocalContext().SetGlobalOption(param);

  EXPECT_NE(model.InitFeatureMapAndP2PMem(mem_ptr, mem_size), PARAM_INVALID); //??

  mem_ptr = 0;
  model.runtime_param_.mem_size = 20;
  model.runtime_param_.zero_copy_size = 10;
  g_runtime_stub_mock = "rtMalloc";

  EXPECT_NE(model.InitFeatureMapAndP2PMem(mem_ptr, mem_size), SUCCESS);
}

TEST_F(UtestDavinciModel, test_CpuModelPrepareOutput) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  EXPECT_EQ(model.CpuModelPrepareOutput(0, 0, 0), FAILED);

  void *t = new uint8_t[1];
  model.input_mbuf_list_ = std::vector<uintptr_t>({ PtrToValue(t) });
  model.output_mbuf_list_ = std::vector<uintptr_t>({ PtrToValue(t) });

  EXPECT_EQ(model.CpuModelPrepareOutput(0, 0, 0), SUCCESS);
}

TEST_F(UtestDavinciModel, test_CreateOutput) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);

  GeTensorDesc tensor(GeShape({16, 16, 16, 16}), FORMAT_FRACTAL_Z, DT_FLOAT);
  AttrUtils::SetInt(tensor, ATTR_NAME_SPECIAL_OUTPUT_SIZE, 32);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);

  InputOutputDescInfo output;
  uint32_t format_result;
  model.CreateOutput(0, op_desc, output, format_result);
  EXPECT_EQ(format_result, FORMAT_HWCN);
}

TEST_F(UtestDavinciModel, test_ReportModelExtInfo) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  domi::GetContext().is_online_model = true;

  EXPECT_EQ(model.ReportModelExtInfo(), SUCCESS);
}

TEST_F(UtestDavinciModel, test_InitConstant) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  OpDescPtr op_desc = CreateOpDesc("cosnt", CONSTANT);
  EXPECT_EQ(model.InitConstant(op_desc), PARAM_INVALID);

  const std::vector<uint8_t> weights_value(64, 'A');
  GeTensorPtr weight = MakeShared<GeTensor>(GeTensorDesc(GeShape({16}), FORMAT_NCHW, DT_STRING), weights_value.data(), weights_value.size());
  AttrUtils::SetTensor(*op_desc, ATTR_NAME_WEIGHTS, weight);

  GeTensorDesc tensor(GeShape({16}), FORMAT_NCHW, DT_FLOAT);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  model.runtime_param_.mem_size = 2000;
  model.runtime_param_.mem_base = PtrToValue(malloc(2000));

  EXPECT_EQ(model.InitConstant(op_desc), PARAM_INVALID);
}

TEST_F(UtestDavinciModel, test_InitStreamActive) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  OpDescPtr op_desc = CreateOpDesc("var", VARIABLE);
  std::vector<uint32_t> active_stream_list = {1};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list);

  EXPECT_EQ(model.InitStreamActive(op_desc), SUCCESS);
}

TEST_F(UtestDavinciModel, test_InitStreamSwitch) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  OpDescPtr op_desc = CreateOpDesc("var", VARIABLE);
  std::vector<uint32_t> active_stream_list = {1};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list);

  EXPECT_EQ(model.InitStreamSwitch(op_desc), SUCCESS);
}

TEST_F(UtestDavinciModel, test_InitCase) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  OpDescPtr op_desc = CreateOpDesc("var", VARIABLE);
  AttrUtils::SetInt(op_desc, ATTR_NAME_BATCH_NUM, 2);

  EXPECT_EQ(model.InitCase(op_desc), FAILED);
  for (uint32_t i = 0U; i < 2; ++i) {
    const std::string attr_name = ATTR_NAME_PRED_VALUE + "_" + std::to_string(i);
    std::vector<int64_t> batch_shape = {1, 2};
    AttrUtils::SetListInt(op_desc, attr_name, batch_shape);
    const std::string attr_combined_batch = ATTR_NAME_COMBINED_BATCH + "_" + std::to_string(i);
    AttrUtils::SetListInt(op_desc, attr_combined_batch, batch_shape);
  }
  EXPECT_EQ(model.InitCase(op_desc), SUCCESS);
}

TEST_F(UtestDavinciModel, test_InitEntryTask) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  model.deploy_type_ = (rtAicpuDeployType_t)0x2;

  EXPECT_EQ(model.InitEntryTask(), INTERNAL_ERROR);

  model.active_stream_list_ = std::vector<rtStream_t>({(rtStream_t)1, (rtStream_t)2});
  EXPECT_EQ(model.InitEntryTask(), SUCCESS);
}

TEST_F(UtestDavinciModel, TransAllVarData_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");
  std::vector<NodePtr> variable_nodes { nullptr };

  g_runtime_stub_mock = "rtCtxGetCurrent";
  EXPECT_EQ(model.TransAllVarData(graph, variable_nodes), FAILED); // rtCtxGetCurrent failed.
}

TEST_F(UtestDavinciModel, SetDataDumperArgs_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");

  g_runtime_stub_mock = "rtGetDevice";
  model.SetDataDumperArgs(graph, std::map<std::string, OpDescPtr>());

  EXPECT_EQ(model.data_dumper_.device_id_, 0);
}

TEST_F(UtestDavinciModel, ParseAIPPInfo_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  InputOutputDims dims_info;
  model.ParseAIPPInfo("", dims_info);
  EXPECT_EQ(dims_info.dims.size(), 0);
}

TEST_F(UtestDavinciModel, InitL1DataDumperArgs_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  model.data_dumper_.dump_properties_.AddPropertyValue("ALL_MODEL_NEED_DUMP_AND_IT_IS_NOT_A_MODEL_NAME", std::set<std::string>());
  g_runtime_stub_mock = "rtDumpAddrSet";
  EXPECT_EQ(model.InitL1DataDumperArgs(), FAILED);
}

TEST_F(UtestDavinciModel, GetEventIdForBlockingAicpuOp_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  rtStream_t stream = (rtStream_t)0;
  uint32_t event_id = 0;

  model.stream_2_event_[stream] = (rtEvent_t)2;
  g_runtime_stub_mock = "rtGetEventID";
  EXPECT_NE(model.GetEventIdForBlockingAicpuOp(op_desc, stream, event_id), SUCCESS);

  model.stream_2_event_.clear();
  EXPECT_NE(model.GetEventIdForBlockingAicpuOp(op_desc, stream, event_id), SUCCESS);

  g_runtime_stub_mock = "rtEventCreateWithFlag";
  EXPECT_EQ(model.GetEventIdForBlockingAicpuOp(op_desc, stream, event_id), SUCCESS); //??
}

TEST_F(UtestDavinciModel, UpdateOpInputValue_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);

  EXPECT_EQ(model.UpdateOpInputValue(op_desc, 10, 10), PARAM_INVALID);
}

TEST_F(UtestDavinciModel, GetCurDynamicDims_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  ComputeGraphPtr graph = MakeShared<ComputeGraph>("default");
  OpDescPtr op_input = CreateOpDesc("data", DATA);
  NodePtr node_input = graph->AddNode(op_input);

  AttrUtils::SetInt(op_input, ATTR_NAME_INDEX, 10);

  model.run_context_.data_nodes.push_back(node_input);
  model.run_context_.getnext_nosink_nodes.push_back(node_input);

  model.run_context_.dynamic_node_type = DATA;

  std::vector<std::vector<int64_t>> tensor_input_dims;
  std::vector<int32_t> cur_dynamic_dims;
  EXPECT_EQ(model.GetCurDynamicDims(tensor_input_dims, cur_dynamic_dims), INTERNAL_ERROR);

  model.run_context_.dynamic_node_type = "Add";
  model.run_context_.data_nodes.clear();
  model.run_context_.getnext_nosink_nodes.clear();
  model.run_context_.user_input_dims.push_back(std::make_pair(std::string("data"), std::vector<int64_t>()));
  EXPECT_EQ(model.GetCurDynamicDims(tensor_input_dims, cur_dynamic_dims), INTERNAL_ERROR);

  model.run_context_.dynamic_node_type = DATA;
  model.run_context_.user_input_dims.push_back(std::make_pair(std::string("data"), std::vector<int64_t>({1, 2})));

  AttrUtils::SetInt(op_input, ATTR_NAME_INDEX, 0);
  GeTensorDesc tensor(GeShape(std::vector<int64_t>({1, 2})), FORMAT_NCHW, DT_FLOAT);
  op_input->AddInputDesc(tensor);
  op_input->AddOutputDesc(tensor);

  tensor_input_dims.push_back(std::vector<int64_t>({1, 2}));
  model.run_context_.dynamic_shape_dims.push_back(std::vector<int32_t>({1, 2}));
  EXPECT_NE(model.GetCurDynamicDims(tensor_input_dims, cur_dynamic_dims), SUCCESS);  //>>

  tensor_input_dims[0].push_back(3);
  EXPECT_EQ(model.GetCurDynamicDims(tensor_input_dims, cur_dynamic_dims), INTERNAL_ERROR);
}

TEST_F(UtestDavinciModel, SuperKernelUpdateTaskId_fail) {
  DavinciModel model(0, nullptr);
  GeModelPtr ge_model = MakeShared<GeModel>();
  model.Assign(ge_model);

  uint32_t skt_task_id = 0;
  g_runtime_stub_mock = "rtModelGetTaskId";
  model.SuperKernelUpdateTaskId(skt_task_id);

  EXPECT_EQ(skt_task_id, 0);
}

TEST_F(UtestDavinciModel, CheckModelNoInputAndOutput) {
  DavinciModel model(0, nullptr);
  auto flag = model.CheckModelNoInputAndOutput();
  EXPECT_EQ(flag, true);
  model.input_queue_ids_.emplace_back(1);
  flag = model.CheckModelNoInputAndOutput();
  EXPECT_EQ(flag, false);
}

TEST_F(UtestDavinciModel, TestUpdateZeroCopyAddr) {
  DavinciModel model(0, nullptr);
  model.input_addrs_list_ = { { (void *)0x01 }, { (void *)0x02 } };
  model.output_addrs_list_ = { { (void *)0x05, (void *)0x06 } };
  EXPECT_EQ(model.GenInputOutputIndex(), SUCCESS);

  model.fixed_mem_base_ = 0;
  model.mem_base_ = 16;
  model.runtime_param_.mem_size = 8;
  std::vector<void *> total_io_addrs = { (void *)0x01, (void *)0x02, (void *)0x03,
                                         (void *)0x04, (void *)0x05, (void *)0x06 };
  for (int32_t i = 0; i < total_io_addrs.size(); ++i) {
    total_io_addrs[i] = model.GetRunAddress(total_io_addrs[i]);
  }
  EXPECT_EQ(total_io_addrs[0], (void *)0x11);
  EXPECT_EQ(total_io_addrs[1], (void *)0x12);
  EXPECT_EQ(total_io_addrs[2], (void *)0x13);
  EXPECT_EQ(total_io_addrs[3], (void *)0x14);
  EXPECT_EQ(total_io_addrs[4], (void *)0x15);
  EXPECT_EQ(total_io_addrs[5], (void *)0x16);

  vector<void *> inputs = { (void *)0x60, (void *)0x61 };
  vector<void *> outputs = { (void *)0x66, (void *)0x67 };
  EXPECT_EQ(model.UpdateZeroCopyAddr(total_io_addrs, inputs, outputs), SUCCESS);
  EXPECT_EQ(total_io_addrs[0], (void *)0x60);
  EXPECT_EQ(total_io_addrs[1], (void *)0x61);
  EXPECT_EQ(total_io_addrs[2], (void *)0x13);
  EXPECT_EQ(total_io_addrs[3], (void *)0x14);
  EXPECT_EQ(total_io_addrs[4], (void *)0x66);
  EXPECT_EQ(total_io_addrs[5], (void *)0x67);
}

TEST_F(UtestDavinciModel, TestUpdateNoncontinuousArgs) {
  DavinciModel model(0, nullptr);
  model.input_addrs_list_ = { { (void *)0x01 }, { (void *)0x02 } };
  model.output_addrs_list_ = { { (void *)0x05, (void *)0x06 } };
  EXPECT_EQ(model.GenInputOutputIndex(), SUCCESS);

  model.fixed_mem_base_ = 0;
  model.mem_base_ = 16;
  model.runtime_param_.mem_size = 8;
  std::vector<void *> total_io_addrs = { (void *)0x01, (void *)0x02, (void *)0x03,
                                         (void *)0x04, (void *)0x05, (void *)0x06 };

  model.user_input_addrs_ = { (void *)0x60, (void *)0x61 };
  model.user_output_addrs_ = { (void *)0x66, (void *)0x67 };
  EXPECT_EQ(model.UpdateRunAddr(total_io_addrs), SUCCESS);
  EXPECT_EQ(total_io_addrs[0], (void *)0x60);
  EXPECT_EQ(total_io_addrs[1], (void *)0x61);
  EXPECT_EQ(total_io_addrs[2], (void *)0x13);
  EXPECT_EQ(total_io_addrs[3], (void *)0x14);
  EXPECT_EQ(total_io_addrs[4], (void *)0x66);
  EXPECT_EQ(total_io_addrs[5], (void *)0x67);
}
}  // namespace ge
