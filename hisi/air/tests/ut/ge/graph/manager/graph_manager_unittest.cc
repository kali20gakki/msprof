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

#include <memory>
#include <stdlib.h>
#include <pthread.h>
#include <algorithm>
#include <future>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <future>
#include <utility>

#define protected public
#define private public
#include "graph/manager/graph_manager.h"
#include "init/gelib.h"
#include "plugin/engine/engine_manage.h"

#include "external/graph/graph.h"
#include "common/math/math_util.h"
#include "common/thread_pool.h"
#include "common/dump/dump_manager.h"
#include "analyzer/analyzer.h"
#include "common/ge_call_wrapper.h"
#include "common/local_context.h"
#include "common/transop_util.h"
#include "graph/ge_context.h"
#include "graph/ge_global_options.h"
#include "graph/manager/util/rt_context_util.h"
#include "graph/partition/dynamic_shape_partition.h"
#include "graph/passes/enter_pass.h"
#include "graph/partition/stage_partition.h"
#include "graph/passes/addn_pass.h"
#include "graph/passes/bitcast_pass.h"
#include "graph/passes/assign_remove_pass.h"
#include "graph/passes/inplace_support_check_pass.h"
#include "graph/passes/atomic_addr_clean_pass.h"
#include "graph/passes/attach_stream_label_pass.h"
#include "graph/passes/cast_remove_pass.h"
#include "graph/passes/common_subexpression_elimination_pass.h"
#include "graph/passes/compile_nodes_pass.h"
#include "graph/passes/cond_remove_pass.h"
#include "graph/passes/constant_folding_pass.h"
#include "graph/passes/constant_fuse_same_pass.h"
#include "graph/passes/control_trigger_pass.h"
#include "graph/passes/ctrl_edge_transfer_pass.h"
#include "graph/passes/dimension_adjust_pass.h"
#include "graph/passes/dimension_compute_pass.h"
#include "graph/passes/flow_ctrl_pass.h"
#include "graph/passes/fuse_data_nodes_with_common_input_pass.h"
#include "graph/passes/identity_pass.h"
#include "graph/passes/input_output_connection_identify_pass.h"
#include "graph/passes/iterator_op_pass.h"
#include "graph/passes/link_gen_mask_nodes_pass.h"
#include "graph/passes/mark_graph_unknown_status_pass.h"
#include "graph/passes/merge_pass.h"
#include "graph/passes/merge_input_memcpy_pass.h"
#include "graph/passes/merge_to_stream_merge_pass.h"
#include "graph/passes/multi_batch_pass.h"
#include "graph/passes/next_iteration_pass.h"
#include "graph/passes/permute_pass.h"
#include "graph/passes/prune_pass.h"
#include "graph/passes/ref_identity_delete_op_pass.h"
#include "graph/passes/remove_same_const_pass.h"
#include "graph/passes/reshape_recovery_pass.h"
#include "graph/passes/reshape_remove_pass.h"
#include "graph/passes/same_transdata_breadth_fusion_pass.h"
#include "graph/passes/subgraph_pass.h"
#include "graph/passes/switch_data_edges_bypass.h"
#include "graph/passes/switch_dead_branch_elimination.h"
#include "graph/passes/switch_logic_remove_pass.h"
#include "graph/passes/switch_to_stream_switch_pass.h"
#include "graph/passes/transop_breadth_fusion_pass.h"
#include "graph/passes/transop_nearby_allreduce_fusion_pass.h"
#include "graph/passes/transop_symmetry_elimination_pass.h"
#include "graph/passes/transop_without_reshape_fusion_pass.h"
#include "graph/passes/transpose_transdata_pass.h"
#include "graph/passes/useless_control_out_remove_pass.h"
#include "graph/passes/variable_op_pass.h"
#include "graph/passes/variable_ref_delete_op_pass.h"
#include "graph/passes/variable_ref_useless_control_out_delete_pass.h"
#include "graph/passes/end_of_sequence_add_control_pass.h"
#include "graph/passes/subexpression_migration_pass.h"
#include "graph/passes/subgraph_const_migration_pass.h"
#include "graph/passes/unused_args_clean_pass.h"
#include "graph/passes/global_step_insert_pass.h"
#include "graph/passes/memcpy_addr_async_pass.h"
#include "graph/passes/hccl_continuous_memcpy_pass.h"
#include "graph/build/label_allocator.h"
#include "graph/utils/tensor_adapter.h"
#include "inc/pass_manager.h"
#include "ir_build/option_utils.h"
#include "common/local_context.h"
#include "common/omg_util.h"
#include "common/formats/utils/formats_trans_utils.h"
#include "../passes/graph_builder_utils.h"
#include "register/custom_pass_helper.h"
#include "graph/ops_stub.h"
#include "ge_attr_value.h"
#include "graph/manager/graph_context.h"
#include "graph/optimize/graph_optimize.h"
#include "generator/ge_generator.h"
#include "graph/utils/tensor_utils.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "graph/execute/model_executor.h"
using namespace std;
using namespace testing;
using namespace domi;

namespace {
const uint32_t kNotAdded = 0;
const uint32_t kStartAdd = 1;
const uint32_t kDoneAdded = 2;
}

namespace ge {
namespace graph_manager_ut {
class GraphBuilder {
 public:
  explicit GraphBuilder(const std::string &name) { graph_ = std::make_shared<ComputeGraph>(name); }
  NodePtr AddNode(const std::string &name, const std::string &type, int in_cnt, int out_cnt,
                  Format format = FORMAT_NCHW, DataType data_type = DT_FLOAT,
                  std::vector<int64_t> shape = {1, 1, 224, 224});
  NodePtr AddNode(const std::string &name, const std::string &type,
                  std::initializer_list<std::string> input_names,
                  std::initializer_list<std::string> output_names,
                  Format format = FORMAT_NCHW, DataType data_type = DT_FLOAT,
                  std::vector<int64_t> shape = {1, 1, 224, 224});
  void AddDataEdge(const NodePtr &src_node, int src_idx, const NodePtr &dst_node, int dst_idx);
  void AddControlEdge(const NodePtr &src_node, const NodePtr &dst_node);
  ComputeGraphPtr GetGraph() {
    graph_->TopologicalSorting();
    return graph_;
  }

 private:
  ComputeGraphPtr graph_;
};

NodePtr GraphBuilder::AddNode(const std::string &name, const std::string &type, int in_cnt, int out_cnt, Format format,
                              DataType data_type, std::vector<int64_t> shape) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape(shape));
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);
  tensor_desc->SetOriginFormat(format);
  tensor_desc->SetOriginShape(GeShape(shape));
  tensor_desc->SetOriginDataType(data_type);

  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (int i = 0; i < in_cnt; ++i) {
    op_desc->AddInputDesc(tensor_desc->Clone());
  }
  for (int i = 0; i < out_cnt; ++i) {
    op_desc->AddOutputDesc(tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc);
}
void GraphBuilder::AddDataEdge(const NodePtr &src_node, int src_idx, const NodePtr &dst_node, int dst_idx) {
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(src_idx), dst_node->GetInDataAnchor(dst_idx));
}
void GraphBuilder::AddControlEdge(const NodePtr &src_node, const NodePtr &dst_node) {
  GraphUtils::AddEdge(src_node->GetOutControlAnchor(), dst_node->GetInControlAnchor());
}
NodePtr GraphBuilder::AddNode(const string &name, const string &type, std::initializer_list<std::string> input_names,
                              std::initializer_list<std::string> output_names, Format format, DataType data_type,
                              std::vector<int64_t> shape) {
  auto tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(GeShape(shape));
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);
  tensor_desc->SetOriginFormat(format);
  tensor_desc->SetOriginShape(GeShape(shape));
  tensor_desc->SetOriginDataType(data_type);

  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (auto &input_name : input_names) {
    op_desc->AddInputDesc(input_name, tensor_desc->Clone());
  }
  for (auto &output_name :output_names) {
    op_desc->AddOutputDesc(output_name, tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc);
}

/*                                  -------------------------
*                                  |  partitioncall_0_const1* |
*     partitioncall_0--------------|             |           |
*           |                      |          netoutput      |
*           |                      --------------------------
*           |                       ------------------         -------------
*           |                      |        data      |       |    data     |
*           |                      |          |       |       |     |       |
*     partitioncall_1--------------|        case -----|-------|   squeeze*  |
*                                  |          |       |       |     |       |
*                                  |      netoutput   |       |  netoutput  |
*                                   ------------------         -------------
*/
ComputeGraphPtr BuildGraphPartitionCall() {
  auto root_builder = graph_manager_ut::GraphBuilder("root");
  const auto &partitioncall_0 = root_builder.AddNode("partitioncall_0", PARTITIONEDCALL, 0, 1);
  const auto &partitioncall_1 = root_builder.AddNode("partitioncall_1", PARTITIONEDCALL, 1, 1);
  root_builder.AddDataEdge(partitioncall_0, 0, partitioncall_1, 0);
  const auto &root_graph = root_builder.GetGraph();

  // 1.build partitioncall_0 sub graph
  auto p1_sub_builder = graph_manager_ut::GraphBuilder("partitioncall_0_sub");
  const auto &partitioncall_0_const1 = p1_sub_builder.AddNode("partitioncall_0_const1", CONSTANT, 0, 1);
  const auto &partitioncall_0_netoutput = p1_sub_builder.AddNode("partitioncall_0_netoutput", NETOUTPUT, 1, 1);
  AttrUtils::SetInt(partitioncall_0_netoutput->GetOpDesc()->MutableInputDesc(0), "_parent_node_index", 0);
  p1_sub_builder.AddDataEdge(partitioncall_0_const1, 0, partitioncall_0_netoutput, 0);
  const auto &sub_graph = p1_sub_builder.GetGraph();
  sub_graph->SetParentNode(partitioncall_0);
  sub_graph->SetParentGraph(root_graph);
  partitioncall_0->GetOpDesc()->AddSubgraphName("f");
  partitioncall_0->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_0_sub");

  // 2.build partitioncall_1 sub graph
  auto p2_sub_builder = graph_manager_ut::GraphBuilder("partitioncall_1_sub");
  const auto &partitioncall_1_data = p2_sub_builder.AddNode("partitioncall_1_data", DATA, 0, 1);
  AttrUtils::SetInt(partitioncall_1_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &partitioncall_1_case = p2_sub_builder.AddNode("partitioncall_1_case", "Case", 1, 1);
  const auto &partitioncall_1_netoutput = p2_sub_builder.AddNode("partitioncall_1_netoutput", NETOUTPUT, 1, 1);
  p2_sub_builder.AddDataEdge(partitioncall_1_data, 0, partitioncall_1_case, 0);
  p2_sub_builder.AddDataEdge(partitioncall_1_case, 0, partitioncall_1_netoutput, 0);
  const auto &sub_graph2 = p2_sub_builder.GetGraph();
  sub_graph2->SetParentNode(partitioncall_1);
  sub_graph2->SetParentGraph(root_graph);
  partitioncall_1->GetOpDesc()->AddSubgraphName("f");
  partitioncall_1->GetOpDesc()->SetSubgraphInstanceName(0, "partitioncall_1_sub");

  // 2.1 build case sub graph
  auto case_sub_builder = graph_manager_ut::GraphBuilder("case_sub");
  const auto &case_data = case_sub_builder.AddNode("case_data", DATA, 0, 1);
  AttrUtils::SetInt(case_data->GetOpDesc(), "_parent_node_index", 0);
  const auto &case_squeeze = case_sub_builder.AddNode("case_squeeze", SQUEEZE, 1, 1);
  const auto &case_netoutput = case_sub_builder.AddNode("case_netoutput", NETOUTPUT, 1, 1);
  case_sub_builder.AddDataEdge(case_data, 0, case_squeeze, 0);
  case_sub_builder.AddDataEdge(case_squeeze, 0, case_netoutput, 0);
  const auto &case_sub_graph = case_sub_builder.GetGraph();
  case_sub_graph->SetParentNode(partitioncall_1_case);
  case_sub_graph->SetParentGraph(sub_graph2);
  partitioncall_1_case->GetOpDesc()->AddSubgraphName("branches");
  partitioncall_1_case->GetOpDesc()->SetSubgraphInstanceName(0, "case_sub");

  root_graph->AddSubgraph(case_sub_graph->GetName(), case_sub_graph);
  root_graph->AddSubgraph(sub_graph->GetName(), sub_graph);
  root_graph->AddSubgraph(sub_graph2->GetName(), sub_graph2);
  return root_graph;
}

}  // namespace graph_manager_ut

using namespace graph_manager_ut;
const char *const kKernelLibName = "DNN_VM_GE_LOCAL";
class UtestGraphManagerTest : public testing::Test {
 protected:
  void SetUp() {
    SetLocalOmgContext(domi::GetContext());
  }
  void TearDown() {}

  class FakeOpsKernelInfoStore : public OpsKernelInfoStore {
   public:
    FakeOpsKernelInfoStore(){supported_ = true;};
    bool supported_;

   private:
    Status Initialize(const std::map<std::string, std::string> &options) override {
      return SUCCESS;
    };
    Status Finalize() override {
      return SUCCESS;
    };
    bool CheckSupported(const OpDescPtr &op_desc, std::string &reason) const override {
      return supported_;
    };
    void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const override {};
  };

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
  void InitGeLib() {
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

    OpsKernelInfoStorePtr ops_kernel_info_store_ptr = std::make_shared<FakeOpsKernelInfoStore>();
    OpsKernelManager::GetInstance().ops_kernel_store_.emplace(kKernelLibName, ops_kernel_info_store_ptr);
    OpsKernelBuilderPtr fake_builder = std::make_shared<FakeOpsKernelBuilder>();
    OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;
    OpInfo op_info;
    op_info.engine = kKernelLibName;
    op_info.opKernelLib = kKernelLibName;
    OpsKernelManager &ops_kernel_manager = instance_ptr->OpsKernelManagerObj();
    ops_kernel_manager.ops_kernel_info_[DATA].emplace_back(op_info);
    ops_kernel_manager.ops_kernel_info_[ADD].emplace_back(op_info);
    ops_kernel_manager.ops_kernel_info_[NETOUTPUT].emplace_back(op_info);
  }

  void FinalizeGeLib() {
    auto instance_ptr = ge::GELib::GetInstance();
    if (instance_ptr != nullptr) {
      instance_ptr->Finalize();
    }
  }
};

class StubExecutor : public Executor {
 public:
  Status LoadGraph(const FlowModelPtr &ge_root_model, const GraphNodePtr &graph_node) override {
    return SUCCESS;
  }

  Status UnloadGraph(const FlowModelPtr &ge_root_model, const uint32_t graph_id) override {
    return SUCCESS;
  }

  Status PushRunArgs(const RunArgs &args) override {
    return SUCCESS;
  }

  Status RunGraph(const GraphNodePtr &graph_node, const GraphId graph_id,
                  const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs) override {
    return SUCCESS;
  }

  Status RunGraphWithStream(const GraphNodePtr &graph_node, const GraphId graph_id, const rtStream_t stream,
                            const std::vector<GeTensor> &inputs, const std::vector<GeTensor> &outputs) override {
    return SUCCESS;
  }
};

class StubExecutorFail : public Executor {
 public:
  Status LoadGraph(const FlowModelPtr &ge_root_model, const GraphNodePtr &graph_node) override {
    return FAILED;
  }

  Status UnloadGraph(const FlowModelPtr &ge_root_model, const uint32_t graph_id) override {
    return FAILED;
  }

  Status PushRunArgs(const RunArgs &args) override {
    return FAILED;
  }

  Status RunGraph(const GraphNodePtr &graph_node, const GraphId graph_id,
                  const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs) override {
    return FAILED;
  }

  Status RunGraphWithStream(const GraphNodePtr &graph_node, const GraphId graph_id, const rtStream_t stream,
                            const std::vector<GeTensor> &inputs, const std::vector<GeTensor> &outputs) override {
    return FAILED;
  }
};

void SetSubGraph(ComputeGraphPtr &root_graph, NodePtr &parent_node, const std::string &name) {
  auto subgraph = std::make_shared<ComputeGraph>(name);
  subgraph->SetParentGraph(root_graph);
  subgraph->SetParentNode(parent_node);
  auto op_desc = parent_node->GetOpDesc();
  op_desc->AddSubgraphName(name);
  op_desc->SetSubgraphInstanceName(0, name);
  root_graph->AddSubgraph(name, subgraph);
}

void CreateGraph(Graph &graph) {
  TensorDesc desc(ge::Shape({1, 3, 224, 224}));
  uint32_t size = desc.GetShape().GetShapeSize();
  desc.SetSize(size);
  auto data = op::Data("Data").set_attr_index(0);
  data.update_input_desc_data(desc);
  data.update_output_desc_out(desc);

  auto flatten = op::Flatten("Flatten").set_input_x(data, data.name_out_out());

  std::vector<Operator> inputs{data};
  std::vector<Operator> outputs{flatten};
  std::vector<Operator> targets{flatten};
  // Graph graph("test_graph");
  graph.SetInputs(inputs).SetOutputs(outputs).SetTargets(targets);
}
/*      Data
 *       |
 *      Relu       Const
 *       |
 *    Netoutput
 */

ge::ComputeGraphPtr CreateGraphWithIsolatedConst() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data", "Data", 1, 1);
  auto relu = builder.AddNode("addn1", "Relu", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);
  auto const1 = builder.AddNode("const1", "Const", 0, 1);

  builder.AddDataEdge(data, 0, relu, 0);
  builder.AddDataEdge(relu, 0, netoutput, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithFrameworkOp() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data1", "FrameworkOp", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, netoutput, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data1", "Variable", 1, 1);
  data->GetOpDesc()->SetAttr("CheckPointGraphForGetVar", GeAttrValue::CreateFrom<int64_t>(6));
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, netoutput, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput1() {
  ge::ut::GraphBuilder builder("graph1");
  auto data1 = builder.AddNode("data1", "Variable", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto relu = builder.AddNode("addn1", "Relu", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data2, 0, relu, 0);
  builder.AddDataEdge(data1, 0, netoutput, 0);
  builder.AddDataEdge(relu, 0, netoutput, 1);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput2() {
  ge::ut::GraphBuilder builder("graph1");
  auto data = builder.AddNode("data", "Data", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, netoutput, 0);
  ComputeGraphPtr compute_graph = builder.GetGraph();
  compute_graph->RemoveNode(data);
  return compute_graph;
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput3() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data1", "Variable", 1, 1);
  auto hcom = builder.AddNode("hcom", "HcomBroadcast", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, hcom, 0);
  builder.AddDataEdge(hcom, 0, netoutput, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput4() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data1", "Variable", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto hcom = builder.AddNode("hcom", "HcomBroadcast", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, hcom, 0);
  builder.AddDataEdge(data2, 0, hcom, 1);
  builder.AddDataEdge(hcom, 0, netoutput, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput5() {
  ge::ut::GraphBuilder builder("graph");
  auto assign = builder.AddNode("assign", "Assign", 1, 1);
  auto data = builder.AddNode("data1", "Variable", 1, 1);
  auto hcom = builder.AddNode("hcom", "HcomBroadcast", 1, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, assign, 0);
  builder.AddDataEdge(assign, 0, hcom, 0);
  builder.AddDataEdge(hcom, 0, netoutput, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput6() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data1", "Variable", 1, 1);
  auto cast = builder.AddNode("cast", "Cast", 1, 1);
  data->GetOpDesc()->SetAttr("CheckPointGraphForGetVar", GeAttrValue::CreateFrom<int64_t>(6));
  builder.AddDataEdge(data, 0, cast, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput7() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("const", "Constant", 1, 1);
  auto relu = builder.AddNode("relu", "Relu", 1, 1);
  builder.AddDataEdge(data, 0, relu, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphWithVariableOutput8() {
  ge::ut::GraphBuilder builder("graph");
  auto cast = builder.AddNode("cast", "Cast", 1, 1);
  auto relu = builder.AddNode("relu", "Relu", 1, 1);
  builder.AddDataEdge(cast, 0, relu, 0);
  return builder.GetGraph();
}

ge::ComputeGraphPtr CreateGraphNoOutput() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data1", "Data", 1, 1);
  auto relu = builder.AddNode("relu", "Relu", 1, 1);
  builder.AddDataEdge(data, 0, relu, 0);
  return builder.GetGraph();
}

ge::Status callbackFunc1(uint32_t a, const std::map<std::string, ge::Tensor> b)
{
  return SUCCESS;
}

ge::Status callbackFunc2(uint32_t a, const std::map<AscendString, ge::Tensor> b)
{
  return SUCCESS;
}

ge::ComputeGraphPtr CreateGraphPipelineParitioned() {
  ge::ut::GraphBuilder builder("root_graph");
  auto data1 = builder.AddNode("data1", DATA, 0, 1);
  auto data2 = builder.AddNode("data2", DATA, 0, 1);
  auto partitioned_call_1 = builder.AddNode("PartitionedCall1", PARTITIONEDCALL, 2, 1);
  auto partitioned_call_2 = builder.AddNode("PartitionedCall2", PARTITIONEDCALL, 2, 1);
  auto netoutput = builder.AddNode("NetOutput", NETOUTPUT, 1, 1);
  builder.AddDataEdge(data1, 0, partitioned_call_1, 0);
  builder.AddDataEdge(data2, 0, partitioned_call_1, 1);
  builder.AddDataEdge(partitioned_call_1, 0, partitioned_call_2, 0);
  builder.AddDataEdge(data2, 0, partitioned_call_2, 1);
  builder.AddDataEdge(partitioned_call_2, 0, netoutput, 0);
  auto root_graph = builder.GetGraph();
  SetSubGraph(root_graph, partitioned_call_1, "subgraph1");
  SetSubGraph(root_graph, partitioned_call_2, "subgraph2");
  return root_graph;
}

TEST_F(UtestGraphManagerTest, test_add_graph_sub) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  // create graph
  Graph graph("test_graph");
  CreateGraph(graph);
  ComputeGraphPtr compute_graph = std::shared_ptr<ge::ComputeGraph>(GraphUtils::GetComputeGraph(graph));

  Graph root_graph("root_graph");
  CreateGraph(root_graph);
  ComputeGraphPtr root_compute_graph = std::shared_ptr<ge::ComputeGraph>(GraphUtils::GetComputeGraph(root_graph));

  root_compute_graph->AddSubGraph(compute_graph);


  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, root_graph, options, context);
  EXPECT_EQ(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_CheckEngineName) {
  GraphManager graph_manager;
  auto p1 = std::make_shared<GELib>();

  Status status = graph_manager.CheckEngineName("test", "test", std::map<std::string, int>({{"test",1}}));
  EXPECT_EQ(status, ge::PARAM_INVALID);
  status = graph_manager.CheckEngineName("", "test", std::map<std::string, int>());
  EXPECT_EQ(status, ge::GE_GRAPH_OPTIONS_INVALID);
  status = graph_manager.CheckEngineName("test", "test", std::map<std::string, int>());
  EXPECT_EQ(status, ge::PARAM_INVALID);
}

TEST_F(UtestGraphManagerTest, test_ParseParallelNum) {
  GraphManager graph_manager;
  const std::string paralled_num1 = "8";
  const std::string paralled_num2 = "0";
  const std::string paralled_num3 = "";
  const std::string paralled_num4 = " ";
  const std::string key = "a";
  int32_t num;
  Status status = graph_manager.ParseParallelNum(paralled_num1, key, num);
  EXPECT_EQ(status, ge::SUCCESS);
  status = graph_manager.ParseParallelNum(paralled_num2, key, num);
  EXPECT_EQ(status, ge::GE_GRAPH_OPTIONS_INVALID);
  status = graph_manager.ParseParallelNum(paralled_num3, key, num);
  EXPECT_EQ(status, ge::GE_GRAPH_OPTIONS_INVALID);
  status = graph_manager.ParseParallelNum(paralled_num4, key, num);
  EXPECT_EQ(status, ge::GE_GRAPH_OPTIONS_INVALID);
}

TEST_F(UtestGraphManagerTest, test_SetAttrForHcomBroadCastOp) {
  GraphManager graph_manager;

  Graph graph("test_graph");
  CreateGraph(graph);

  ComputeGraphPtr compute_graph = std::shared_ptr<ge::ComputeGraph>(GraphUtils::GetComputeGraph(graph));

  graph_manager.SetAttrForHcomBroadCastOp(compute_graph);
}

TEST_F(UtestGraphManagerTest, test_SubgraphPartitionAndOptimization) {
  // need build while buildflag is true, var format changed
  GraphId graph_id = 1;
  GraphManager graph_manager;
  Graph graph("test_graph");
  CreateGraph(graph);

  Graph root_graph("root_graph");
  CreateGraph(root_graph);

  GraphUtils::GetComputeGraph(root_graph)->AddSubGraph(GraphUtils::GetComputeGraph(graph));
  ComputeGraphPtr compute_graph = std::shared_ptr<ge::ComputeGraph>(GraphUtils::GetComputeGraph(root_graph));
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);

  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);

  OpsKernelManager::GetInstance().composite_engines_["test"] = std::set<string>({"test"});

  Status status = graph_manager.SubgraphPartitionAndOptimization(graph_node, compute_graph, 1, GraphPartitioner::kCompositeEnginePartitioning);
  EXPECT_EQ(status, ge::FAILED);
}

TEST_F(UtestGraphManagerTest, set_and_get_add_graph_flag) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.SetAddGraphCondition(graph_id, 1);
  uint32_t res = graph_manager.GetAddGraphCondition(graph_id);
  EXPECT_EQ(res, 1);
}

TEST_F(UtestGraphManagerTest, test_add_graph_1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  // create graph
  Graph graph("test_graph");
  CreateGraph(graph);

  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  EXPECT_EQ(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_add_graph_2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_manager.AddGraphNode(graph_id, graph_node);
  graph_manager.SetAddGraphCondition(graph_id, kDoneAdded);
  Graph graph("test_graph");
  CreateGraph(graph);
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  EXPECT_EQ(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_add_graph_3) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  Graph graph("test_graph");
  CreateGraph(graph);

  std::map<std::string, std::string> options;
  OmgContext context;

  std::future<Status> fut1 = std::async(std::launch::async,
      &GraphManager::AddGraph, &graph_manager, graph_id, graph, options, context);
  std::future<Status> fut2 = std::async(std::launch::async,
      &GraphManager::AddGraph, &graph_manager, graph_id, graph, options, context);
  fut1.wait();
  fut2.wait();
  Status status1 = fut1.get();
  Status status2 = fut2.get();
  EXPECT_EQ(status1, ge::SUCCESS);
  EXPECT_EQ(status2, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_add_graph_4) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  std::map<std::string, std::string> options;
  ModelExecutor executor;
  graph_manager.Initialize(options, &executor);

  // create graph
  Graph graph("test_graph");
  CreateGraph(graph);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  (void)AttrUtils::SetBool(*compute_graph, ATTR_NAME_GRAPH_HAS_BEEN_ADDED, true);

  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  EXPECT_NE(status, ge::SUCCESS);

  Tensor value;
  const std::string test_str = "test";
  const std::string empty_str = "";
  status = graph_manager.GetVariable(empty_str, value);
  EXPECT_EQ(status, ge::GE_GRAPH_EMPTY_STRING_NAME);
  status = graph_manager.GetVariable(test_str, value);
  EXPECT_EQ(status, ge::GE_GRAPH_EMPTY_VARIABLE_TENSOR_TABLE);
  EXPECT_EQ(graph_manager.Finalize(), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_add_graph_5) {
  Graph graph("test_graph");
  auto data = op::Data("Data").set_attr_index(1);
  auto flatten = op::Flatten("Flatten").set_input_x(data, data.name_out_out());
  std::vector<Operator> inputs{data};
  std::vector<Operator> outputs{flatten};
  graph.SetInputs(inputs).SetOutputs(outputs);

  std::map<std::string, std::string> options = {{"ge.exec.dataInputsShapeRange", "0:[-1]"}};
  OmgContext context;
  GraphId graph_id = 1;
  GraphManager graph_manager;
  EXPECT_EQ(graph_manager.AddGraph(graph_id, graph, options, context), GRAPH_PARAM_INVALID);
}

TEST_F(UtestGraphManagerTest, test_add_graph_with_copy_1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;

  // create graph
  Graph graph("test_graph");
  CreateGraph(graph);
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  graph_manager.graph_map_.insert({1, graph_node});

  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraphWithCopy(graph_id, graph, options, context);
  EXPECT_NE(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_remove_graph_1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  Status status = graph_manager.RemoveGraph(graph_id);
  EXPECT_EQ(status, ge::GE_GRAPH_GRAPH_NOT_EXIST);
  graph_manager.AddGraphNode(graph_id, graph_node);
  graph_node->SetRunFlag(true);
  status = graph_manager.RemoveGraph(graph_id);
  EXPECT_EQ(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_remove_graph_2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor stub_executor;
  graph_manager.executor_ = &stub_executor;

  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  Graph graph("test_graph");
  CreateGraph(graph);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  ge_root_model->SetModelId(1);
  ge_root_model->SetModelId(2);
  graph_node->SetFlowModel(ge_root_model);
  graph_node->SetLoadFlag(true);
  graph_manager.AddGraphNode(graph_id, graph_node);
  EXPECT_EQ(graph_manager.RemoveGraph(graph_id), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_decrease_graph_count) {
  GraphId graph_id = 1;
  GraphId graph_id1 = 2;
  GraphManager graph_manager;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  Graph graph("test_graph");
  CreateGraph(graph);
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  graph_manager.DecreaseGraphCount(graph_id);
  graph_manager.DecreaseGraphCount(graph_id1);
}

TEST_F(UtestGraphManagerTest, test_save_checkpoint1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  // Graph graph("test_graph");
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  std::vector<Tensor> outputs;
  std::vector<Tensor> outputs1;
  Tensor te;
  outputs.push_back(te);
  std::map<std::string, Tensor> var_results = {{"data1", te},};
  std::map<std::string, Tensor> var_results1;
  EXPECT_EQ(graph_manager.SaveCheckPointResult(graph, outputs1, var_results1), FAILED);
  EXPECT_EQ(graph_manager.SaveCheckPointResult(graph, outputs, var_results), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_save_checkpoint2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput1();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  // Graph graph("test_graph");
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  std::vector<Tensor> outputs;
  Tensor te;
  outputs.push_back(te);
  std::map<std::string, Tensor> var_results = {{"data1", te},};
  EXPECT_EQ(graph_manager.SaveCheckPointResult(graph, outputs, var_results), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_save_checkpoint3) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput4();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  // Graph graph("test_graph");
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  std::vector<Tensor> outputs;
  Tensor te;
  outputs.push_back(te);
  std::map<std::string, Tensor> var_results = {{"data1", te},};
  EXPECT_EQ(graph_manager.SaveCheckPointResult(graph, outputs, var_results), FAILED);
}

TEST_F(UtestGraphManagerTest, test_save_variable1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  std::vector<std::string> names;
  std::string str = "test";
  names.push_back(str);
  std::vector<Tensor> outputs;
  Tensor te;
  outputs.push_back(te);
  std::vector<Tensor> var_results;
  EXPECT_EQ(graph_manager.SaveVariables(graph, names, outputs, var_results), FAILED);
}

TEST_F(UtestGraphManagerTest, test_save_variable2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  std::vector<std::string> names;
  std::vector<std::string> names1;
  std::string str = "data1";
  names.push_back(str);
  std::vector<Tensor> outputs;
  Tensor te;
  outputs.push_back(te);
  std::vector<Tensor> var_results;
  std::vector<Tensor> var_results1;
  EXPECT_EQ(graph_manager.SaveVariables(graph, names1, outputs, var_results1), SUCCESS);
  EXPECT_EQ(graph_manager.SaveVariables(graph, names, outputs, var_results), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_is_checkpoint_graph1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  ComputeGraphPtr compute_graph = nullptr;
  auto compute_graph1 = CreateGraphWithVariableOutput();
  auto compute_graph2 = CreateGraphWithVariableOutput1();
  auto compute_graph3 = CreateGraphWithVariableOutput2();
  EXPECT_FALSE(graph_manager.IsCheckpointGraph(compute_graph));
  EXPECT_TRUE(graph_manager.IsCheckpointGraph(compute_graph1));
  EXPECT_FALSE(graph_manager.IsCheckpointGraph(compute_graph2));
  EXPECT_FALSE(graph_manager.IsCheckpointGraph(compute_graph3));
}

TEST_F(UtestGraphManagerTest, test_is_checkpoint_graph2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput6();
  auto compute_graph1 = CreateGraphWithVariableOutput7();
  auto compute_graph2 = CreateGraphWithVariableOutput8();
  EXPECT_TRUE(graph_manager.IsCheckpointGraph(compute_graph));
  EXPECT_FALSE(graph_manager.IsCheckpointGraph(compute_graph1));
  EXPECT_FALSE(graph_manager.IsCheckpointGraph(compute_graph2));
}

TEST_F(UtestGraphManagerTest, test_get_graph_options) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  GraphNodePtr graph_node = nullptr;
  graph_manager.AddGraphNode(graph_id, graph_node);
  auto map = graph_manager.GetGraphOptions(graph_id);
}

TEST_F(UtestGraphManagerTest, test_push_savedate2me) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  Tensor te;
  std::map<std::string, Tensor> save_data = {{"data1", te},};
  const std::string summary = "Save";
  uint32_t a;
  const std::map<std::string, ge::Tensor> b;
  EXPECT_EQ(graph_manager.PushSaveData2ME(graph_id, save_data), FAILED);
  graph_manager.RegisterCallBackFunc(summary, callbackFunc2);
  EXPECT_EQ(graph_manager.PushSaveData2ME(graph_id, save_data), SUCCESS);
  graph_manager.RegisterCallBackFunc(summary, callbackFunc1);
  EXPECT_EQ(graph_manager.PushSaveData2ME(graph_id, save_data), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_push_summarydate2me) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  Tensor te;
  std::map<std::string, Tensor> summary_data = {{"data1", te},};
  const std::string summary = "Summary";
  uint32_t a;
  const std::map<std::string, ge::Tensor> b;
  EXPECT_EQ(graph_manager.PushSummaryData2ME(graph_id, summary_data), FAILED);
  graph_manager.RegisterCallBackFunc(summary, callbackFunc2);
  EXPECT_EQ(graph_manager.PushSummaryData2ME(graph_id, summary_data), SUCCESS);
  graph_manager.RegisterCallBackFunc(summary, callbackFunc1);
  EXPECT_EQ(graph_manager.PushSummaryData2ME(graph_id, summary_data), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_checkpoint_handle) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph1 = CreateGraphWithVariableOutput();
  Graph graph1 = GraphUtils::CreateGraphFromComputeGraph(compute_graph1);
  std::vector<GeTensor> outputs;
  EXPECT_EQ(graph_manager.CheckpointHandle(graph_id, compute_graph1, outputs), FAILED);
  auto compute_graph2 = CreateGraphNoOutput();
  Graph graph2 = GraphUtils::CreateGraphFromComputeGraph(compute_graph2);
  outputs.clear();
  EXPECT_EQ(graph_manager.CheckpointHandle(graph_id, compute_graph2, outputs), FAILED);
  auto compute_graph3 = CreateGraphWithVariableOutput1();
  Graph graph3 = GraphUtils::CreateGraphFromComputeGraph(compute_graph3);
  outputs.clear();
  EXPECT_EQ(graph_manager.CheckpointHandle(graph_id, compute_graph3, outputs), FAILED);
}

TEST_F(UtestGraphManagerTest, test_summary_handle) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  std::vector<GeTensor> outputs;
  EXPECT_EQ(graph_manager.SummaryHandle(graph_id, outputs), FAILED);
}

TEST_F(UtestGraphManagerTest, test_setattr_hcombroadcast1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput3();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  graph_manager.SetAttrForHcomBroadCastOp(compute_graph);
}

TEST_F(UtestGraphManagerTest, test_setattr_hcombroadcast2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;
  auto compute_graph = CreateGraphWithVariableOutput5();
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  graph_manager.SetAttrForHcomBroadCastOp(compute_graph);
}

TEST_F(UtestGraphManagerTest, test_graph_context) {
  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  graph_node->SetGraph(graph_ptr);
  GraphContext graph_context(graph_node);
  EXPECT_EQ(graph_context.SetComputeGraph(graph_node), SUCCESS);
  GraphNodePtr graph_node2 = nullptr;
  EXPECT_EQ(graph_context.SetComputeGraph(graph_node2), GE_GRAPH_PARAM_NULLPTR);
  GraphId graph_id3 = 3;
  GraphNodePtr graph_node3 = MakeShared<ge::GraphNode>(graph_id3);
  ComputeGraphPtr compute_graph3 = MakeShared<ComputeGraph>("test_graph");
  graph_node3->SetComputeGraph(compute_graph3);
  EXPECT_EQ(graph_context.SetComputeGraph(graph_node3), SUCCESS);
  GraphId graph_id4 = 4;
  GraphNodePtr graph_node4 = MakeShared<ge::GraphNode>(graph_id4);
  EXPECT_EQ(graph_context.SetComputeGraph(graph_node4), GE_GRAPH_OPTIMIZE_COMPUTE_GRAPH_NULL);
  GradOpList a;
  VariableRecord b("test", a, 0);
  GeTensor c;
  std::pair<VariableRecord, GeTensor> d(b, c);
  graph_context.GetVarNodeTensorTable().push_back(d);
  const std::string var_data_name = "aaa";
  GeTensor res;
  EXPECT_EQ(graph_context.GetVariableTensor(var_data_name, res), GE_GRAPH_VARIABLE_DOES_NOT_EXIST);
}

TEST_F(UtestGraphManagerTest, test_IsGraphNeedRebuild1) {
  GraphId graph_id = 1;
  GraphId graph_id2 = 2;
  GraphManager graph_manager;
  auto compute_graph = CreateGraphWithVariableOutput();
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_manager.AddGraphNode(graph_id, graph_node);
  EXPECT_EQ(graph_manager.IsGraphNeedRebuild(graph_id2), true);
}

TEST_F(UtestGraphManagerTest, test_IsGraphNeedRebuild2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  auto compute_graph = CreateGraphWithVariableOutput();
  GraphNodePtr graph_node = nullptr;
  graph_manager.AddGraphNode(graph_id, graph_node);
  EXPECT_EQ(graph_manager.IsGraphNeedRebuild(graph_id), true);
}

TEST_F(UtestGraphManagerTest, test_AdjustAssignOpData) {
  GraphManager graph_manager;
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data", "Data", 1, 1);
  graph_manager.AdjustAssignOpData(data);
}

TEST_F(UtestGraphManagerTest, test_pre_run_thread) {

  GraphManager graph_manager;
  graph_manager.thread_run_flag_ = true;

  GraphId graph_id = 1;
  std::vector<ge::Tensor> input_tensor;
  uint64_t session_id = 0;
  error_message::Context error_context;
  GEThreadLocalContext context;
  RunAsyncCallback callback;
  // PreRunArgs args{graph_id, input_tensor, session_id, error_context, context, callback};
  bool ret = graph_manager.prerun_args_q_.Push({graph_id, input_tensor, session_id, error_context, context, callback});
  EXPECT_EQ(ret, true);

  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_manager.AddGraphNode(graph_id, graph_node);
  graph_manager.PreRunThread();
  // end with failed
}

TEST_F(UtestGraphManagerTest, test_pre_run_thread_2) {

  GraphManager graph_manager;
  graph_manager.thread_run_flag_ = true;

  GraphId graph_id = 1;
  GraphNodePtr graph_node_1 = MakeShared<ge::GraphNode>(graph_id);
  graph_manager.AddGraphNode(graph_id, graph_node_1);
  graph_manager.IncreaseGraphCount(graph_id);
  graph_manager.IncreaseGraphCount(graph_id);
  graph_node_1->SetBuildFlag(true);
  std::vector<ge::Tensor> input_tensor;
  uint64_t session_id = 0;
  error_message::Context error_context;
  GEThreadLocalContext context;
  RunAsyncCallback callback;
  // PreRunArgs args{graph_id, input_tensor, session_id, error_context, context, callback};
  bool ret = graph_manager.prerun_args_q_.Push({graph_id, input_tensor, session_id, error_context, context, callback});
  EXPECT_EQ(ret, true);
  graph_id = 2;
  GraphNodePtr graph_node_2 = MakeShared<ge::GraphNode>(graph_id);
  graph_manager.AddGraphNode(graph_id, graph_node_2);
  ret = graph_manager.prerun_args_q_.Push({graph_id, input_tensor, session_id, error_context, context, callback});
  EXPECT_EQ(ret, true);
  graph_manager.PreRunThread();
  // end with failed
}

TEST_F(UtestGraphManagerTest, test_pre_run_thread_3) {
  GraphId graph_id1 = 1;
  GraphId graph_id2 = 1;
  GraphManager graph_manager;
  GraphNodePtr graph_node1 = MakeShared<ge::GraphNode>(graph_id1);
  GraphNodePtr graph_node2 = MakeShared<ge::GraphNode>(graph_id2);
  graph_manager.AddGraphNode(graph_id1, graph_node1);
  graph_manager.AddGraphNode(graph_id2, graph_node2);
  graph_manager.SetAddGraphCondition(graph_id1, kDoneAdded);
  Graph graph("test_graph");
  CreateGraph(graph);
  std::map<std::string, std::string> options = {
    {"ge.exec.isTailingOptimization", "1"},
    {"ge.streamMaxParallelNum", "a"},
    };
  OmgContext context;
  EXPECT_EQ(graph_manager.Initialize(options), GE_GRAPH_OPTIONS_INVALID);
  std::map<std::string, std::string> options1 = {
    {"ge.exec.isTailingOptimization", "1"},
    {"ge.streamMaxParallelNum", "8"},
    {"ge.streamNum", "0"},
    };
  EXPECT_EQ(graph_manager.Initialize(options1), GE_GRAPH_OPTIONS_INVALID);
  std::map<std::string, std::string> options2 = {
    {"ge.exec.isTailingOptimization", "1"},
    {"ge.streamMaxParallelNum", "8"},
    {"ge.streamNum", "1"},
    {"ge.perfLevel", "-2"},
    };
  EXPECT_EQ(graph_manager.Initialize(options2), GE_GRAPH_OPTIONS_INVALID);
  Status status = graph_manager.AddGraph(graph_id1, graph, options1, context);
  EXPECT_EQ(status, ge::SUCCESS);
  EXPECT_EQ(graph_manager.RemoveGraph(graph_id1), SUCCESS);
  graph_node2->SetRunFlag(true);
  EXPECT_EQ(graph_manager.Finalize(), SUCCESS);
  graph_node2->SetRunFlag(false);
  EXPECT_EQ(graph_manager.Finalize(), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_ParseOption1) {
  GraphManager graph_manager;
  std::map<std::string, std::string> options = {
    {"test0", "0"},
    {"test1", "1"},
    {"test2", "2"},
  };
  bool option;
  EXPECT_EQ(graph_manager.ParseOption(options, "test0", option), SUCCESS);
  EXPECT_EQ(graph_manager.ParseOption(options, "test1", option), SUCCESS);
  EXPECT_EQ(graph_manager.ParseOption(options, "test2", option), GE_GRAPH_OPTIONS_INVALID);
}

TEST_F(UtestGraphManagerTest, test_ParseOption2) {
  GraphManager graph_manager;
  std::map<std::string, std::string> options = {
    {"test0", "a"},
    {"test1", "1"},
  };
  int32_t option;
  EXPECT_EQ(graph_manager.ParseOption(options, "test0", option), GE_GRAPH_OPTIONS_INVALID);
  EXPECT_EQ(graph_manager.ParseOption(options, "test1", option), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_initialize1) {
  auto p1 = std::make_shared<EngineManager>();
  std::map<std::string, DNNEnginePtr> engines_map;
  GetDNNEngineObjs(engines_map);
  GraphId graph_id1 = 1;
  GraphManager graph_manager;
  std::map<std::string, std::string> options = {
    {"ge.streamMaxParallelNum", "AIcoreEngine:8"},
  };
  EXPECT_EQ(graph_manager.Initialize(options), GE_GRAPH_OPTIONS_INVALID);
}

TEST_F(UtestGraphManagerTest, test_initialize2) {
  GraphManager graph_manager;
  std::map<std::string, std::string> options = {};
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  GraphId graph_id1 = 1;
  GraphNodePtr graph_node1 = MakeShared<ge::GraphNode>(graph_id1);
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  graph_node1->SetGraph(graph_ptr);
  graph_node1->SetRunFlag(true);
  graph_manager.AddGraphNode(graph_id1, graph_node1);
  EXPECT_EQ(graph_manager.Finalize(), GE_GRAPH_GRAPH_IS_RUNNING);
}

TEST_F(UtestGraphManagerTest, test_initialize3) {
  GraphManager graph_manager;
  std::map<std::string, std::string> options = {};
  StubExecutor executor;
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  graph_manager.executor_ = &executor;
  GraphId graph_id2 = 1;
  GraphNodePtr graph_node2 = MakeShared<ge::GraphNode>(graph_id2);
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  //ComputeGraphPtr compute_graph = CreateGraphWithVariableOutput();
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  ge_root_model->SetModelId(1);
  graph_node2->SetFlowModel(ge_root_model);
  graph_node2->SetLoadFlag(true);
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  graph_node2->SetGraph(graph_ptr);
  graph_manager.AddGraphNode(graph_id2, graph_node2);
  EXPECT_EQ(graph_manager.Finalize(), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_initialize4) {
  GraphManager graph_manager;
  std::map<std::string, std::string> options = {};
  StubExecutorFail executor;
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  graph_manager.executor_ = &executor;
  GraphId graph_id2 = 1;
  GraphNodePtr graph_node2 = MakeShared<ge::GraphNode>(graph_id2);
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  //ComputeGraphPtr compute_graph = CreateGraphWithVariableOutput();
  auto flow_model = MakeShared<FlowModel>(compute_graph);
  auto ge_root_model = MakeShared<GeRootModel>(compute_graph);
  (void)flow_model->AddSubModel(ge_root_model);
  flow_model->SetModelId(1);
  graph_node2->SetFlowModel(flow_model);
  graph_node2->SetLoadFlag(true);
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  graph_node2->SetGraph(graph_ptr);
  graph_manager.AddGraphNode(graph_id2, graph_node2);
  EXPECT_EQ(graph_manager.Finalize(), FAILED);
}

TEST_F(UtestGraphManagerTest, test_InitDynamicParams1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  auto compute_graph = CreateGraphWithFrameworkOp();
  EXPECT_EQ(graph_manager.InitDynamicParams(compute_graph), FAILED);
}

TEST_F(UtestGraphManagerTest, test_InitDynamicParams2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  auto compute_graph = CreateGraphWithVariableOutput();
  graph_manager.options_.dynamic_node_type = 0;
  EXPECT_EQ(graph_manager.InitDynamicParams(compute_graph), SUCCESS);
  graph_manager.options_.dynamic_node_type = 1;
  EXPECT_EQ(graph_manager.InitDynamicParams(compute_graph), SUCCESS);
  graph_manager.options_.input_shape = {1,2,3,4};
  graph_manager.options_.dynamic_dims = {0};
  EXPECT_EQ(graph_manager.InitDynamicParams(compute_graph), GRAPH_PARAM_INVALID);
}

TEST_F(UtestGraphManagerTest, test_CheckRepeatAdd1) {
  GraphId graph_id = 1;
  bool is_added = false;
  GraphManager graph_manager;
  EXPECT_EQ(graph_manager.CheckRepeatAdd(graph_id, is_added), INTERNAL_ERROR);
}

TEST_F(UtestGraphManagerTest, test_CheckRepeatAdd2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_manager.AddGraphNode(graph_id, graph_node);
  graph_manager.SetAddGraphCondition(graph_id, kDoneAdded);
  bool is_added = false;
  EXPECT_EQ(graph_manager.CheckRepeatAdd(graph_id, is_added), INTERNAL_ERROR);
}

TEST_F(UtestGraphManagerTest, test_NotifyWaittingGraph) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  EXPECT_EQ(graph_manager.NotifyWaittingGraph(graph_id), INTERNAL_ERROR);
}

TEST_F(UtestGraphManagerTest, test_CheckGraphAdded) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  Graph graph;
  EXPECT_EQ(graph_manager.CheckGraphAdded(graph_id, graph), FAILED);
}

TEST_F(UtestGraphManagerTest, test_StartForRunGraph) {
  GraphManager graph_manager;
  std::map<std::string, std::string> options = {};
  StubExecutorFail executor;
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  EXPECT_EQ(graph_manager.Initialize(options), SUCCESS);
  graph_manager.executor_ = &executor;
  GraphId graph_id2 = 1;
  const GraphNodePtr graph_node2 = MakeShared<ge::GraphNode>(graph_id2);
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  //ComputeGraphPtr compute_graph = CreateGraphWithVariableOutput();
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  FlowModelPtr flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetModelId(1);
  graph_node2->SetFlowModel(flow_root_model);
  graph_node2->SetBuildFlag(true);
  graph_node2->SetLoadFlag(false);
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  graph_node2->SetGraph(graph_ptr);
  graph_manager.AddGraphNode(graph_id2, graph_node2);
  const std::vector<GeTensor> inputs;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.StartForRunGraph(graph_node2, inputs, flow_root_model, session_id), FAILED);
  EXPECT_EQ(graph_manager.Finalize(), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_LoadGraph) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr flow_root_model = MakeShared<FlowModel>(compute_graph);
  graph_manager.options_.run_graph_flag = false;
  EXPECT_EQ(graph_manager.LoadGraph(flow_root_model, graph_node), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_InnerRunGraphWithStream) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  const std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  rtStream_t stream;
  EXPECT_EQ(graph_manager.InnerRunGraphWithStream(graph_node, graph_id, stream, inputs, outputs), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_InnerRunGraph) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  const std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  EXPECT_EQ(graph_manager.InnerRunGraph(graph_node, graph_id, inputs, outputs), SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_RunGraphWithStreamAsync1) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  GraphNodePtr graph_node = nullptr;
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  rtStream_t stream;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.RunGraphWithStreamAsync(graph_id, stream, session_id, inputs, outputs), GE_GRAPH_GRAPH_NODE_NULL);
}

TEST_F(UtestGraphManagerTest, test_RunGraphWithStreamAsync2) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetRunFlag(true);
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  rtStream_t stream;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.RunGraphWithStreamAsync(graph_id, stream, session_id, inputs, outputs), GE_GRAPH_ALREADY_RUNNING);
}

TEST_F(UtestGraphManagerTest, test_RunGraph1) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  const std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.RunGraph(graph_id, inputs, outputs, session_id), GE_GRAPH_GRAPH_NOT_EXIST);
}

TEST_F(UtestGraphManagerTest, test_RunGraph2) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  GraphNodePtr graph_node = nullptr;
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.RunGraph(graph_id, inputs, outputs, session_id), GE_GRAPH_GRAPH_NODE_NULL);
}

TEST_F(UtestGraphManagerTest, test_RunGraph3) {
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetRunFlag(true);
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  uint64_t session_id = 1;
  EXPECT_EQ(graph_manager.RunGraph(graph_id, inputs, outputs, session_id), GE_GRAPH_ALREADY_RUNNING);
}

TEST_F(UtestGraphManagerTest, test_GenerateInfershapeGraph1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  EXPECT_EQ(graph_manager.GenerateInfershapeGraph(graph_id), GE_GRAPH_GRAPH_NOT_EXIST);
}

TEST_F(UtestGraphManagerTest, test_GenerateInfershapeGraph2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphNodePtr graph_node = nullptr;
  graph_manager.AddGraphNode(graph_id, graph_node);
  EXPECT_EQ(graph_manager.GenerateInfershapeGraph(graph_id), GE_GRAPH_GRAPH_NODE_NULL);
}

TEST_F(UtestGraphManagerTest, test_GenerateInfershapeGraph3) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_manager.AddGraphNode(graph_id, graph_node);
  EXPECT_EQ(graph_manager.GenerateInfershapeGraph(graph_id), GE_GRAPH_NULL_INPUT);
}

TEST_F(UtestGraphManagerTest, test_BuildGraphForUnregisteredOp1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  const std::vector<GeTensor> inputs;
  uint64_t session_id = 1;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  EXPECT_EQ(graph_manager.BuildGraphForUnregisteredOp(graph_id, inputs, ge_root_model, session_id), GE_GRAPH_GRAPH_NOT_EXIST);
}

TEST_F(UtestGraphManagerTest, test_BuildGraphForUnregisteredOp2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphNodePtr graph_node = nullptr;
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  uint64_t session_id = 1;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  EXPECT_EQ(graph_manager.BuildGraphForUnregisteredOp(graph_id, inputs, ge_root_model, session_id), GE_GRAPH_GRAPH_NODE_NULL);
}

TEST_F(UtestGraphManagerTest, test_BuildGraphForUnregisteredOp3) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  graph_node->SetGraph(graph_ptr);
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  uint64_t session_id = 1;
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  EXPECT_EQ(graph_manager.BuildGraphForUnregisteredOp(graph_id, inputs, ge_root_model, session_id), GE_GRAPH_INIT_FAILED);
}

TEST_F(UtestGraphManagerTest, test_BuildGraph1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  const std::vector<GeTensor> inputs;
  uint64_t session_id = 1;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  bool async = false;
  EXPECT_EQ(graph_manager.BuildGraph(graph_id, inputs, ge_root_model, session_id, async), GE_GRAPH_GRAPH_NOT_EXIST);
}

TEST_F(UtestGraphManagerTest, test_BuildGraph2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphNodePtr graph_node = nullptr;
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  uint64_t session_id = 1;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  bool async = false;
  EXPECT_EQ(graph_manager.BuildGraph(graph_id, inputs, ge_root_model, session_id, async), GE_GRAPH_GRAPH_NODE_NULL);
}

TEST_F(UtestGraphManagerTest, test_BuildGraph3) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  StubExecutor executor;
  graph_manager.executor_ = &executor;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetRunFlag(true);
  graph_manager.AddGraphNode(graph_id, graph_node);
  const std::vector<GeTensor> inputs;
  uint64_t session_id = 1;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  bool async = false;
  EXPECT_EQ(graph_manager.BuildGraph(graph_id, inputs, ge_root_model, session_id, async), GE_GRAPH_ALREADY_RUNNING);
}

TEST_F(UtestGraphManagerTest, test_check_incre_build_and_pre_run_1) {
  // no need to build
  GraphId graph_id = 1;
  GraphManager graph_manager;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  GraphManager::PreRunArgs arg;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetBuildFlag(true);
  Status status = graph_manager.CheckIncreBuildAndPreRun(arg, graph_node, ge_root_model);
  EXPECT_EQ(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_check_incre_build_and_pre_run_2) {
  // need build while buildflag is true, var format changed
  GraphId graph_id = 1;
  GraphManager graph_manager;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  GraphManager::PreRunArgs arg;
  arg.callback = [](Status, std::vector<ge::Tensor> &) {};
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetBuildFlag(true);
  graph_node->Lock();
  graph_manager.graph_rebuild_state_ctrl_.graph_ids_need_rebuild_.insert(graph_id);
  Status status = graph_manager.CheckIncreBuildAndPreRun(arg, graph_node, ge_root_model);
  EXPECT_EQ(status, ge::PARAM_INVALID);
}

TEST_F(UtestGraphManagerTest, test_check_incre_build_and_pre_run_3) {
  // need build while buildflag is false, var format unchanged
  GraphId graph_id = 1;
  GraphManager graph_manager;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  FlowModelPtr ge_root_model = MakeShared<FlowModel>(compute_graph);
  GraphManager::PreRunArgs arg;
  arg.callback = [](Status, std::vector<ge::Tensor> &) {};
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetBuildFlag(false);
  graph_node->Lock();
  Status status = graph_manager.CheckIncreBuildAndPreRun(arg, graph_node, ge_root_model);
  EXPECT_NE(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_add_graph_with_copy_success) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  // create graph
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraphWithCopy(graph_id, graph, options, context);
  EXPECT_EQ(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_add_graph_with_copy_fail) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  // create graph
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

  std::map<std::string, std::string> options;
  OmgContext context;
  Status status = graph_manager.AddGraph(graph_id, graph, options, context);
  EXPECT_EQ(status, ge::SUCCESS);
  status = graph_manager.RemoveGraph(graph_id);
  EXPECT_EQ(status, ge::SUCCESS);
  status = graph_manager.AddGraphWithCopy(graph_id, graph, options, context);
  EXPECT_NE(status, ge::SUCCESS);
}

TEST_F(UtestGraphManagerTest, test_prerunthread_failed_1) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.thread_run_flag_ = true;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  GraphManager::PreRunArgs args;
  error_message::Context error_ctx{1, "1st_stage", "2nd_stage", "log_header"};
  Status st = 0;
  args.callback = [&st](Status st_return, std::vector<ge::Tensor> &) { st = st_return; };
  args.graph_id = graph_id;
  args.session_id = 1;
  args.error_context = error_ctx;
  args.context = GetThreadLocalContext();
  // create graph
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetGraph(graph_ptr);

  graph_manager.options_.local_fmk_op_flag = false;
  // need build while buildflag is true, var format changed
  graph_node->SetBuildFlag(true);
  graph_manager.graph_rebuild_state_ctrl_.graph_ids_need_rebuild_.insert(graph_id);

  graph_manager.graph_map_.insert({graph_id, graph_node});
  graph_manager.graph_count_.insert({graph_id, 1});
  graph_node->SetRunFlag(false);
  // function return.
  graph_manager.prerun_args_q_.Push(args);
  auto t1 = std::thread(&GraphManager::PreRunThread, &graph_manager);
  if (t1.joinable()) {
    t1.join();
  }
  EXPECT_EQ(st, ge::PARAM_INVALID);
}

TEST_F(UtestGraphManagerTest, test_prerunthread_failed_2) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.thread_run_flag_ = true;
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  GraphManager::PreRunArgs args;
  error_message::Context error_ctx{1, "1st_stage", "2nd_stage", "log_header"};
  Status st;
  args.callback = [&st, &graph_manager](Status st_return, std::vector<ge::Tensor> &) { st = st_return;
      graph_manager.thread_run_flag_ = false;};
  args.graph_id = graph_id;
  args.session_id = 1;
  args.error_context = error_ctx;
  args.context = GetThreadLocalContext();
  // create graph
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::shared_ptr<Graph> graph_ptr = MakeShared<ge::Graph>(graph);
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetGraph(graph_ptr);

  graph_manager.options_.local_fmk_op_flag = false;
  // need build while buildflag is true, var format changed
  graph_node->SetBuildFlag(true);
  graph_manager.graph_rebuild_state_ctrl_.graph_ids_need_rebuild_.insert(graph_id);

  graph_manager.graph_map_.insert({graph_id, graph_node});
  graph_manager.graph_count_.insert({graph_id, 1});
  graph_node->SetRunFlag(false);
  // function continue
  int ret = setenv("ENABLE_NETWORK_ANALYSIS_DEBUG", "1", 1);
  EXPECT_EQ(ret, 0);
  graph_manager.prerun_args_q_.Push(args);
  auto t1 = std::thread(&GraphManager::PreRunThread, &graph_manager);
  if (t1.joinable()) {
    t1.join();
  }
  EXPECT_EQ(st, ge::PARAM_INVALID);
}
// TEST_F(UtestGraphManagerTest, ParseInputsDimsForGetNexNosinkAndData_success) {
//   GraphManager graph_manager;

//   ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

//   // save1
//   ge::OpDescPtr save_op = std::make_shared<ge::OpDesc>();
//   save_op->SetType("Save");
//   save_op->SetName("Save1");
//   save_op->AddInputDesc(ge::GeTensorDesc());
//   save_op->AddOutputDesc(ge::GeTensorDesc());
//   AttrUtils::SetInt(save_op, ATTR_NAME_INDEX, 1);
//   ge::NodePtr save_node = graph->AddNode(save_op);

//   std::vector<NodePtr> nodes;
//   nodes.emplace_back(save_node);
//   ge::Tensor tensor;
//   std::vector<Tensor> input_tensors;
//   input_tensors.emplace_back(tensor);
//   auto ret = graph_manager.ParseInputsDimsForGetNexNosinkAndData(nodes, input_tensors);
//   EXPECT_EQ(ret, ge::SUCCESS);
// }

TEST_F(UtestGraphManagerTest, ChangeAndDeleteConst_success) {
  GraphId graph_id = 1;
  GraphManager graph_manager;
  graph_manager.options_.train_graph_flag = true;

  auto graph = CreateGraphWithIsolatedConst();
  graph_manager.ChangeConstTypeWhenTraining(graph);
  auto const1 = graph->FindFirstNodeMatchType("Const");
  EXPECT_EQ(const1, nullptr);

  Status status = graph_manager.RemoveIsolatedConstInThisGraph(graph);
  EXPECT_EQ(status, ge::SUCCESS);
  auto all_nodes = graph->GetDirectNode();
  EXPECT_EQ(all_nodes.size(), 3);
}

TEST_F(UtestGraphManagerTest, test_set_run_context) {
  GraphNodePtr graph_node = MakeShared<GraphNode>(0);
  GraphManager graph_manager;

  GetLocalOmgContext().dynamic_dims = "1;4;8;16";
  EXPECT_EQ(graph_manager.SetRunContext(graph_node), SUCCESS);
  EXPECT_EQ(graph_node->context_.dynamic_shape_dims.size(), 4);
  EXPECT_EQ(graph_node->context_.dynamic_shape_dims[0], std::vector<int32_t>{1});
  EXPECT_EQ(graph_node->context_.dynamic_shape_dims[1], std::vector<int32_t>{4});
  EXPECT_EQ(graph_node->context_.dynamic_shape_dims[2], std::vector<int32_t>{8});
  EXPECT_EQ(graph_node->context_.dynamic_shape_dims[3], std::vector<int32_t>{16});
}

TEST_F(UtestGraphManagerTest, GraphContext) {
  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);

  GraphContext gctx(nullptr);
  GraphContextPtr gcptr = MakeShared<ge::GraphContext>(graph_node);
  EXPECT_NE(gcptr, nullptr);
}

TEST_F(UtestGraphManagerTest, CheckEngineName) {
  InitGeLib();
  GraphManager graph_manager;
  std::string engine_name = "engine";
  std::string key = "key";
  std::map<std::string, int32_t> option;
  EXPECT_EQ(graph_manager.CheckEngineName(engine_name, key, option), SUCCESS);
  option["engine"] = 1;
  EXPECT_EQ(graph_manager.CheckEngineName(engine_name, key, option), GE_GRAPH_OPTIONS_INVALID);
  FinalizeGeLib();
}

TEST_F(UtestGraphManagerTest, ParseParallelNum) {
  GraphManager graph_manager;
  std::string parallel_num = "111111111111";
  std::string key;
  int32_t num = 0;
  EXPECT_EQ(graph_manager.ParseParallelNum(parallel_num, key, num), FAILED);
}

TEST_F(UtestGraphManagerTest, OptimizeSubGraphWithMultiThreads) {
  GraphManager graph_manager;
  ComputeGraphPtr compute_graph = BuildGraphPartitionCall();
  ComputeGraphPtr subgraph = compute_graph->GetSubgraph("case_sub");
  Graph2SubGraphInfoList sub_graph_map;
  std::vector<SubGraphInfoPtr> sgi1;
  sub_graph_map[compute_graph] = sgi1;
  std::vector<SubGraphInfoPtr> sgi2;
  auto p = std::make_shared<SubGraphInfo>();
  p->SetSubGraph(subgraph);
  sgi2.push_back(p);
  sub_graph_map[subgraph] = sgi2;
  uint64_t session_id = 0;
  AttrUtils::SetStr(compute_graph, "_op_compile_strategy", "op_compile_strategy");
  EXPECT_EQ(graph_manager.OptimizeSubGraphWithMultiThreads(compute_graph, sub_graph_map, session_id), SUCCESS);
}

} // namespace ge
