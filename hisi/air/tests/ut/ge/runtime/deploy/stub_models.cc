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

#include "stub_models.h"

#define protected public
#define private public
#include "register/ops_kernel_builder_registry.h"
#include "init/gelib.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#undef private
#undef protected

#include "graph/build/graph_builder.h"
#include "graph/passes/graph_builder_utils.h"
#include "generator/ge_generator.h"

namespace ge {
namespace {
const char *const kKernelLibName = "ops_kernel_lib";
const char *kSessionId = "123456";
OmgContext default_context;

NodePtr AddNode(ut::GraphBuilder &builder, const string &name, const string &type, int in_cnt, int out_cnt) {
  auto node = builder.AddNode(name, type, in_cnt, out_cnt);
  auto op_desc = node->GetOpDesc();
  op_desc->SetInputOffset(std::vector<int64_t>(in_cnt));
  op_desc->SetOutputOffset(std::vector<int64_t>(out_cnt));
  return node;
}

void SetSubGraph(const NodePtr &node, const string &name) {
  auto op_desc = node->GetOpDesc();
  auto builder = ut::GraphBuilder(name);

  int num_inputs = static_cast<int>(op_desc->GetInputsSize());
  int num_outputs = static_cast<int>(op_desc->GetOutputsSize());
  auto some_node = AddNode(builder, name + "_node", ADDN, num_inputs, 1);
  for (int i = 0; i < num_inputs; ++i) {
    auto data_node = AddNode(builder, name + "_data_" + std::to_string(i), DATA, 1, 1);
    AttrUtils::SetInt(data_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, i);
    builder.AddDataEdge(data_node, 0, some_node, i);
  }
  auto net_output = AddNode(builder, "Node_Output", NETOUTPUT, num_outputs, num_outputs);
  for (int i = 0; i < num_outputs; ++i) {
    AttrUtils::SetInt(net_output->GetOpDesc()->MutableInputDesc(i), ATTR_NAME_PARENT_NODE_INDEX, i);
    builder.AddDataEdge(some_node, 0, net_output, i);
  }

  auto subgraph = builder.GetGraph();
  AttrUtils::SetStr(*subgraph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  subgraph->SetParentNode(node);
  op_desc->AddSubgraphName(name);
  op_desc->SetSubgraphInstanceName(0, name);
  auto root_graph = GraphUtils::FindRootGraph(node->GetOwnerComputeGraph());
  subgraph->SetParentGraph(root_graph);
  root_graph->AddSubgraph(name, subgraph);
}

class FakeOpsKernelBuilder : public OpsKernelBuilder {
 public:
  FakeOpsKernelBuilder() = default;
 private:
  Status Initialize(const map<std::string, std::string> &options) override {
    return SUCCESS;
  };
  Status Finalize() override {
    return SUCCESS;
  };
  Status CalcOpRunningParam(Node &node) override {
    return SUCCESS;
  };
  Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) override {
    domi::TaskDef task_def;
    tasks.push_back(task_def);
    return SUCCESS;
  };
};

class FakeOpsKernelInfoStore : public OpsKernelInfoStore {
 public:
  FakeOpsKernelInfoStore() = default;

 private:
  Status Initialize(const std::map<std::string, std::string> &options) override {
    return SUCCESS;
  };
  Status Finalize() override {
    return SUCCESS;
  };
  bool CheckSupported(const OpDescPtr &op_desc, std::string &reason) const override {
    return true;
  };
  void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const override {};
};
}  // namespace

void SubModels::InitGeLib() {
  map<string, string> options;
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);
  auto instance_ptr = ge::GELib::GetInstance();
  EXPECT_NE(instance_ptr, nullptr);

  //  SchedulerConf conf;
  SchedulerConf scheduler_conf;
  scheduler_conf.name = kKernelLibName;
  scheduler_conf.cal_engines[kKernelLibName] = std::make_shared<EngineConf>();
  scheduler_conf.cal_engines[kKernelLibName]->name = kKernelLibName;
  scheduler_conf.cal_engines[kKernelLibName]->scheduler_id = kKernelLibName;
  map<string, SchedulerConf> scheduler_confs;
  scheduler_confs["scheduler"] = scheduler_conf;
  instance_ptr->DNNEngineManagerObj().schedulers_[kKernelLibName] = scheduler_conf;

  OpsKernelInfoStorePtr ops_kernel_info_store_ptr = MakeShared<FakeOpsKernelInfoStore>();
  OpsKernelManager::GetInstance().ops_kernel_store_.emplace(kKernelLibName, ops_kernel_info_store_ptr);
  OpsKernelBuilderPtr fake_builder = MakeShared<FakeOpsKernelBuilder>();
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;
}

void SubModels::FinalizeGeLib() {
  auto instance_ptr = ge::GELib::GetInstance();
  if (instance_ptr != nullptr) {
    instance_ptr->Finalize();
  }
}

ComputeGraphPtr SubModels::BuildGraphWithQueueBindings() {
  auto builder = ut::GraphBuilder("g1");
  auto data1 = AddNode(builder, "data1", DATA, 1, 1);
  AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_INDEX, 0);
  auto data2 = AddNode(builder, "data2", DATA, 1, 1);
  AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_INDEX, 1);
  auto partitioned_call_1 = AddNode(builder, "PartitionedCall1", PARTITIONEDCALL, 2, 1);
  auto partitioned_call_2 = AddNode(builder, "PartitionedCall2", PARTITIONEDCALL, 2, 1);
  auto net_output = AddNode(builder, "NetOutput", NETOUTPUT, 1, 1);

  SetSubGraph(partitioned_call_1, "subgraph-1");
  SetSubGraph(partitioned_call_2, "subgraph-2");

  builder.AddDataEdge(data1, 0, partitioned_call_1, 0);
  builder.AddDataEdge(data2, 0, partitioned_call_1, 1);
  builder.AddDataEdge(partitioned_call_1, 0, partitioned_call_2, 0);
  builder.AddDataEdge(data2, 0, partitioned_call_2, 1);
  builder.AddDataEdge(partitioned_call_2, 0, net_output, 0);
  return builder.GetGraph();
}

ComputeGraphPtr SubModels::BuildGraphWithoutNeedForBindingQueues() {
  auto builder = ut::GraphBuilder("g1");
  auto data1 = AddNode(builder, "data1", DATA, 1, 1);
  AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_INDEX, 0);
  auto data2 = AddNode(builder, "data2", DATA, 1, 1);
  AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_INDEX, 1);
  auto partitioned_call_1 = AddNode(builder, "PartitionedCall1", PARTITIONEDCALL, 1, 1);
  auto partitioned_call_2 = AddNode(builder, "PartitionedCall2", PARTITIONEDCALL, 1, 1);
  auto partitioned_call_3 = AddNode(builder, "PartitionedCall3", PARTITIONEDCALL, 2, 1);
  auto net_output = AddNode(builder, "NetOutput", NETOUTPUT, 1, 1);

  SetSubGraph(partitioned_call_1, "subgraph-1");
  SetSubGraph(partitioned_call_2, "subgraph-2");
  SetSubGraph(partitioned_call_3, "subgraph-3");

  builder.AddDataEdge(data1, 0, partitioned_call_1, 0);
  builder.AddDataEdge(data2, 0, partitioned_call_2, 0);
  builder.AddDataEdge(partitioned_call_1, 0, partitioned_call_3, 0);
  builder.AddDataEdge(partitioned_call_2, 0, partitioned_call_3, 1);
  builder.AddDataEdge(partitioned_call_3, 0, net_output, 0);

  return builder.GetGraph();
}

GeRootModelPtr SubModels::BuildRootModel(ComputeGraphPtr root_graph, bool pipeline_partitioned) {
  InitGeLib();
  SetLocalOmgContext(default_context);
  for (const auto &node : root_graph->GetAllNodes()) {
    node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
    node->GetOpDesc()->SetOpEngineName(kKernelLibName);
  }

  if (pipeline_partitioned) {
    AttrUtils::SetBool(root_graph, "_pipeline_partitioned", true);
  }
  AttrUtils::SetStr(*root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);

  ge::GraphBuilder graph_builder;
  GeRootModelPtr root_model;
  auto ret = graph_builder.Build(root_graph, root_model);
  FinalizeGeLib();
  EXPECT_EQ(ret, SUCCESS);
  if (pipeline_partitioned) {
    EXPECT_NE(root_model->GetModelRelation(), nullptr);
  }
  return root_model;
}

void SubModels::BuildModel(Graph &graph, ModelBufferData &model_buffer_data) {
  auto root_graph = GraphUtils::GetComputeGraph(graph);
  InitGeLib();
  SetLocalOmgContext(default_context);
  for (const auto &node : root_graph->GetAllNodes()) {
    node->GetOpDesc()->SetOpKernelLibName(kKernelLibName);
    node->GetOpDesc()->SetOpEngineName(kKernelLibName);
  }
  AttrUtils::SetStr(*root_graph, ATTR_NAME_SESSION_GRAPH_ID, kSessionId);
  GeGenerator ge_generator;
  std::map<std::string, std::string> options;
  options.emplace(ge::SOC_VERSION, "Ascend910");
  ge_generator.Initialize(options);
  auto ret = ge_generator.GenerateOnlineModel(graph, {}, model_buffer_data);

  //  ge::GraphBuilder graph_builder;
  //  GeRootModelPtr root_model;
  //  auto ret = graph_builder.Build(root_graph, root_model);
  EXPECT_EQ(ret, SUCCESS);

  FinalizeGeLib();
}
}  // namespace ge