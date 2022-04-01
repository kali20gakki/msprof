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
#include "external/ge/ge_api.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/passes/set_ffts_plus_attr_pass.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "graph/build/memory/graph_mem_assigner.h"

#include "generator/ge_generator.h"
#include "graph/attr_value.h"
#include "init/gelib.h"
#include "graph/utils/tensor_utils.h"
#include "graph/operator_factory_impl.h"
#include "graph/passes/graph_builder_utils.h"
#include "graph/manager/graph_manager.h"
#include "local_engine/ops_kernel_store/ge_local_ops_kernel_builder.h"
#include "register/ops_kernel_builder_registry.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "ge_running_env/tensor_utils.h"
#include "framework/common/ge_types.h"

using namespace std;
using namespace ge;

// Temporary code that Wait for the FE formal solution to implment. 
namespace{
static void BuildFftsDynamicGraph(ComputeGraphPtr &root_graph, ComputeGraphPtr &dsp_graph, ComputeGraphPtr &ffts_graph) {
  const auto SetUnknownOpKernel = [](const ComputeGraph::Vistor<NodePtr> &all_nodes) {
    static uint32_t index = 0U;
    const static std::set<std::string> kGeLocalTypes{ DATA, CONSTANT, VARIABLE, NETOUTPUT, AIPP_DATA_TYPE };

    GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
    TensorUtils::SetSize(tensor, 64);

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      (void)AttrUtils::SetBool(op_desc, "OwnerGraphIsUnknown", true);
      std::string op_kernel_name =  (kGeLocalTypes.count(op_desc->GetType()) > 0U) ? "DNN_VM_GE_LOCAL_OP_STORE" : "DNN_VM_RTS_OP_STORE";
      op_desc->SetOpKernelLibName(op_kernel_name);

      vector<int64_t> output_offset;
      for (size_t i = 0U; i < op_desc->GetOutputsSize(); ++i) {
        op_desc->UpdateOutputDesc(i, tensor);
        output_offset.emplace_back(index * 64);
        ++index;
      }
      op_desc->SetOutputOffset(output_offset);
      op_desc->SetWorkspace({});
      op_desc->SetWorkspaceBytes({});
    }

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      vector<int64_t> input_offset;
      for (size_t i = 0U; i < op_desc->GetInputsSize(); ++i) {
        op_desc->UpdateInputDesc(i, tensor);
        if (node->GetType() == NETOUTPUT && node->GetName() != NODE_NAME_NET_OUTPUT) {
          AttrUtils::SetInt(op_desc->MutableInputDesc(i), ATTR_NAME_PARENT_NODE_INDEX, i);
        }

        const auto in_anchor = node->GetInDataAnchor(i);
        if (in_anchor == nullptr || in_anchor->GetPeerOutAnchor() == nullptr) {
          input_offset.emplace_back(-1);
          continue;
        }

        const auto out_anchor = in_anchor->GetPeerOutAnchor();
        const auto peer_node = out_anchor->GetOwnerNode();
        const vector<int64_t> output_offset = peer_node->GetOpDesc()->GetOutputOffset();
        if (static_cast<size_t>(out_anchor->GetIdx()) >= output_offset.size()) {
          input_offset.emplace_back(-1);
          continue;
        }

        input_offset.emplace_back(output_offset.at(out_anchor->GetIdx()));
      }
      op_desc->SetInputOffset(input_offset);
    }
  };

  DEF_GRAPH(g1) {
    CHAIN(NODE("_arg_0", DATA)->NODE("PartitionedCall_0", PARTITIONEDCALL)->NODE("Node_Output", NETOUTPUT));
    CHAIN(NODE("_arg_1", DATA)->NODE("PartitionedCall_0"));
  };

  root_graph = ToComputeGraph(g1);
  SetUnknownOpKernel(root_graph->GetDirectNode());
  const auto root_call_0 = root_graph->FindNode("PartitionedCall_0");
  EXPECT_NE(root_call_0, nullptr);

  auto dsp_graph_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto dsp_graph_data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
  DEF_GRAPH(g2) {
    CHAIN(NODE("dsp_graph/_arg_0", dsp_graph_data_0)->EDGE(0, 0)
              ->NODE("dsp_graph/trans_TransData_0", IDENTITY)->EDGE(0, 0)
              ->NODE("dsp_graph/PartitionedCall_0", PARTITIONEDCALL)->EDGE(0, 0)
              ->NODE("dsp_graph/trans_TransData_2", IDENTITY)->EDGE(0, 0)
              ->NODE("dsp_graph/Node_Output", NETOUTPUT)
    );
    CHAIN(NODE("dsp_graph/_arg_1", dsp_graph_data_1)->EDGE(0, 0)
              ->NODE("dsp_graph/trans_TransData_1", IDENTITY)->EDGE(0, 1)
              ->NODE("dsp_graph/PartitionedCall_0")
    );
  };

  dsp_graph = ToComputeGraph(g2);
  SetUnknownOpKernel(dsp_graph->GetDirectNode());
  dsp_graph->SetGraphUnknownFlag(true);
  dsp_graph->SetParentNode(root_call_0);
  dsp_graph->SetParentGraph(root_graph);
  root_call_0->GetOpDesc()->AddSubgraphName("f");
  root_call_0->GetOpDesc()->SetSubgraphInstanceName(0, dsp_graph->GetName());
  root_graph->AddSubgraph(dsp_graph);
  const auto dsp_graph_call_0 = dsp_graph->FindNode("dsp_graph/PartitionedCall_0");
  EXPECT_NE(dsp_graph_call_0, nullptr);
  AttrUtils::SetBool(dsp_graph_call_0->GetOpDesc(), ATTR_NAME_FFTS_PLUS_SUB_GRAPH, true);

  auto sgt_graph_data_0 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
  auto sgt_graph_data_1 = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 1);
  auto sgt_graph_conv_0 = OP_CFG(CONV2D).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC")
                                        .Attr(ATTR_NAME_IMPLY_TYPE, 1)           // domi::ImplyType::TVM
                                        .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
  auto sgt_graph_relu_0 = OP_CFG(RELU).Attr(ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIV")
                                      .Attr(ATTR_NAME_IMPLY_TYPE, 1)
                                      .Attr(TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
  DEF_GRAPH(g3) {
    CHAIN(NODE("sgt_graph/_arg_0", sgt_graph_data_0)->EDGE(0, 0)
              ->NODE("sgt_graph/Conv2D", sgt_graph_conv_0)->EDGE(0, 0)
              ->NODE("sgt_graph/Relu", sgt_graph_relu_0)->EDGE(0, 0)
              ->NODE("sgt_graph/Node_Output", NETOUTPUT)
    );
    CHAIN(NODE("sgt_graph/_arg_1", sgt_graph_data_1)->EDGE(0, 1)
              ->NODE("sgt_graph/Conv2D", sgt_graph_conv_0)
    );
  };

  ffts_graph = ToComputeGraph(g3);
  SetUnknownOpKernel(ffts_graph->GetDirectNode());
  ffts_graph->SetGraphUnknownFlag(true);
  ffts_graph->SetParentNode(dsp_graph_call_0);
  ffts_graph->SetParentGraph(dsp_graph);
  dsp_graph_call_0->GetOpDesc()->AddSubgraphName("f");
  dsp_graph_call_0->GetOpDesc()->SetSubgraphInstanceName(0, ffts_graph->GetName());
  root_graph->AddSubgraph(ffts_graph);
}

///
///      Data
///        |
///      Relu    Const
///        \      /
///         Switch
///          |   \
///          |    Relu
///          |    /
///          Merge
///         /     \
///       Relu   Relu
///         |     |
///        NetOutput
///
Graph BuildSwitchMergeGraph() {
  GeTensorDesc tensor_4_desc(ge::GeShape({2,3,4,5}), FORMAT_NCHW, DT_INT32);

  auto data1 = std::make_shared<OpDesc>("data1", DATA);
  data1->AddInputDesc(tensor_4_desc);
  data1->AddOutputDesc(tensor_4_desc);

  auto relu1 = std::make_shared<OpDesc>("relu1", RELU);
  relu1->AddInputDesc(tensor_4_desc);
  relu1->AddOutputDesc(tensor_4_desc);

  int32_t data_value_vec1[1] = {1};
  GeTensorDesc data_tensor_desc(GeShape({1}), FORMAT_ND, DT_INT32);
  GeTensorPtr data_tensor1 = make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec1, sizeof(int32_t));
  auto const1 = OP_CFG(CONSTANT).Weight(data_tensor1);

  DEF_GRAPH(g1) {
    CHAIN(NODE(data1)->NODE(relu1)->EDGE(0, 0)->NODE("switch", SWITCH)->EDGE(0, 0)->NODE("merge", MERGE)
          ->EDGE(0, 0)->NODE("relu3", RELU)->NODE("output", NETOUTPUT));
    CHAIN(NODE("switch")->EDGE(1, 0)->NODE("relu2", RELU)->EDGE(0, 1)->NODE("merge")->EDGE(1, 0)
          ->NODE("relu4", RELU)->NODE("output"));
    CHAIN(NODE("const1", const1)->EDGE(0, 1)->NODE("switch"));
  };
  return ToGeGraph(g1);
}

///
Graph BuildGraphWithHcclNode() {

  int64_t dims_size = 1;
  vector<int64_t> data_vec = {5};
  for_each(data_vec.begin(), data_vec.end(), [&](int64_t &data) { dims_size *= data; });
  vector<int32_t> data_value_vec(dims_size, 1);
  GeTensorDesc data_tensor_desc(GeShape(data_vec), FORMAT_NCHW, DT_INT32);
  GeTensorPtr data_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec.data(),
                                                       data_value_vec.size() * sizeof(int32_t));
  auto const1 = OP_CFG(CONSTANT)
      .Weight(data_tensor)
      .InCnt(0)
      .OutCnt(1)
      .Build("const1");

  // input mutable
  auto relu1 = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("relu1");
  auto relu2 = OP_CFG(RELU).InCnt(1).OutCnt(1).Build("relu2");
  auto allreduce1 = OP_CFG(HCOMALLREDUCE).InCnt(1).OutCnt(1).Attr("_input_mutable", true).Build("allreduce1");

  auto allreduce2 = OP_CFG(HCOMALLREDUCE).InCnt(1).OutCnt(1).Attr("_input_mutable", true).Build("allreduce2");

  auto var1 = OP_CFG(VARIABLE).TensorDesc(FORMAT_NCHW, DT_INT32, {2,3,4,5}).InCnt(0).OutCnt(1).Build("var1");
  auto var2 = OP_CFG(VARIABLE).TensorDesc(FORMAT_NCHW, DT_INT32, {2,3,4,5}).InCnt(0).OutCnt(1).Build("var2");

  auto data1 = OP_CFG(DATA).Build("data1");
  auto assign1 = OP_CFG(ASSIGN).TensorDesc(FORMAT_NCHW, DT_INT32, {2,3,4,5}).InCnt(2).OutCnt(1).Build("data1_Assign");
  auto broadcast1 = OP_CFG(HCOMBROADCAST).InCnt(1).OutCnt(1).Attr("_input_mutable", true).Build("broadcast1");

  auto data2 = OP_CFG(DATA).Build("data2");
  auto broadcast2 = OP_CFG(HCOMBROADCAST).InCnt(1).OutCnt(1).Attr("_input_mutable", true).Build("broadcast2");

  auto split = OP_CFG("SplitD").InCnt(2).OutCnt(2).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2,3,4,5})
      .Attr("split_dim", 0)
      .Attr("num_split", 2).Build("split");

  // input continuous
  auto data3 = OP_CFG(DATA).Build("data3");
  auto var3 = OP_CFG(VARIABLE).TensorDesc(FORMAT_NCHW, DT_INT32, {2,3,4,5}).InCnt(0).OutCnt(1).Build("var3");
  vector<int64_t> mem_type = {0x11, 0x11};
  auto broadcast3 = OP_CFG(HCOMBROADCAST).InCnt(2).OutCnt(2).
                    Attr(ATTR_NAME_CONTINUOUS_INPUT, true).
                    Attr(ATTR_NAME_INPUT_MEM_TYPE_LIST, mem_type).Build("broadcast3");

  // allgather with buffer pool
  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  auto data_tensor1 = GenerateTensor(shape);
  auto const3 = OP_CFG(CONSTANT).Weight(data_tensor1).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
                    .InCnt(0).OutCnt(1).Build("const3");
  auto w1 = OP_CFG(CONSTANT).Weight(data_tensor1).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
      .InCnt(0).OutCnt(1).Build("w1");
  auto w2 = OP_CFG(CONSTANT).Weight(data_tensor1).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
      .InCnt(0).OutCnt(1).Build("w2");
  auto w3 = OP_CFG(CONSTANT).Weight(data_tensor1).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
      .InCnt(0).OutCnt(1).Build("w3");

  auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, shape).InCnt(2).OutCnt(1).Build("add1");
  auto add2 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, shape).InCnt(2).OutCnt(1).Build("add2");
  auto add3 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, shape).InCnt(2).OutCnt(1).Build("add3");

  auto allgather1 = OP_CFG(HCOMALLGATHER).InCnt(1).OutCnt(1).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape).
    Attr(ATTR_NAME_BUFFER_POOL_ID, 0).
    Attr(ATTR_NAME_BUFFER_POOL_SIZE, 5600).Build("allgather1");
  auto allgather2 = OP_CFG(HCOMALLGATHER).InCnt(1).OutCnt(1).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape).
      Attr(ATTR_NAME_BUFFER_POOL_ID, 1).
      Attr(ATTR_NAME_BUFFER_POOL_SIZE, 2560).Build("allgather2");
  auto allgather3 = OP_CFG(HCOMALLGATHER).InCnt(1).OutCnt(1).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape).
      Attr(ATTR_NAME_BUFFER_POOL_ID, 0).
      Attr(ATTR_NAME_BUFFER_POOL_SIZE, 5600).Build("allgather3");



  DEF_GRAPH(g1) {
    CHAIN(NODE(var1)->NODE(allreduce1));
    CHAIN(NODE(var1)->EDGE(0, 0)->NODE(split));
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(split));
    CHAIN(NODE(split)->EDGE(0, 0)->NODE(relu1));
    CHAIN(NODE(split)->EDGE(0, 0)->NODE(allreduce1));

    CHAIN(NODE(var2)->EDGE(0, 0)->NODE(assign1)->NODE(broadcast1));
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(assign1));
    CHAIN(NODE(var2)->NODE(broadcast2));

    CHAIN(NODE(data3)->EDGE(0, 0)->NODE(broadcast3));
    CHAIN(NODE(var3)->EDGE(0, 1)->NODE(broadcast3));
    CHAIN(NODE(var3)->NODE(relu2));

    CHAIN(NODE(w1)->EDGE(0, 0)->NODE(allgather1)->EDGE(0, 1)->NODE(add1));
    CHAIN(NODE(w2)->EDGE(0, 0)->NODE(allgather2)->EDGE(0, 1)->NODE(add2));
    CHAIN(NODE(w3)->EDGE(0, 0)->NODE(allgather3)->EDGE(0, 1)->NODE(add3));

    CHAIN(NODE(const3)->EDGE(0, 0)->NODE(add1)->EDGE(0, 0)->NODE(add2)->EDGE(0, 0)->NODE(add3));

  };

  auto root_graph = ToComputeGraph(g1);
  const auto allgather_node1 = root_graph->FindNode("allgather1");
  const auto allgather_node2 = root_graph->FindNode("allgather2");
  const auto allgather_node3 = root_graph->FindNode("allgather3");
  EXPECT_TRUE(allgather_node1 != nullptr);
  TensorUtils::SetSize(*allgather_node1->GetOpDesc()->MutableOutputDesc(0), 1536);
  EXPECT_TRUE(allgather_node2 != nullptr);
  TensorUtils::SetSize(*allgather_node2->GetOpDesc()->MutableOutputDesc(0), 1536);
  EXPECT_TRUE(allgather_node3 != nullptr);
  TensorUtils::SetSize(*allgather_node3->GetOpDesc()->MutableOutputDesc(0), 1536);

  return ToGeGraph(g1);
}
}

class GraphCompilerTest : public testing::Test {
  void SetUp() {}
  void TearDown() {}
public:
  void SetFakerBuilder() {
    ge::OpsKernelBuilderRegistry::GetInstance().Unregister("DNN_VM_GE_LOCAL_OP_STORE");
    REGISTER_OPS_KERNEL_BUILDER("DNN_VM_GE_LOCAL_OP_STORE", FakeOpsKernelBuilder);
  }
  void SetGeLocalBuilder() {
    ge::OpsKernelBuilderRegistry::GetInstance().Unregister("DNN_VM_GE_LOCAL_OP_STORE");
    REGISTER_OPS_KERNEL_BUILDER("DNN_VM_GE_LOCAL_OP_STORE", ge::ge_local::GeLocalOpsKernelBuilder);
  }
};

///     data   data
///       \    /|
///        add  |
///          \  |
///           add
TEST_F(GraphCompilerTest, test_build_no_tiling_fail) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, -1, 224, 224})
                         .Attr(ATTR_NAME_OP_TILING_INLINE_ENGINE, engine_list)
                         .Attr(ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, engine_list);
  auto add2 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, -1, 224, 224})
                         .Attr(ATTR_NAME_OP_TILING_INLINE_ENGINE, engine_list)
                         .Attr(ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, engine_list)
                         .Attr(ATTR_NAME_OP_MAX_SHAPE, "1, 10, 224, 224");
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("add_2", add2));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_2", add2));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  node->GetOpDesc()->MutableOutputDesc(0)->SetShapeRange({{1, 1}, {1, -1}, {224, 224}, {224, 224}});

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  (void)session.BuildGraph(1, inputs);

  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_1");
    EXPECT_EQ(graph->GetAllNodesSize(), 10);
    EXPECT_EQ(graph->GetDirectNodesSize(), 4);
    for (auto node : graph->GetAllNodes()) {
      if (node->GetName() == "add_1" || node->GetName() == "add_2") {
        bool is_no_tiling = false;
        EXPECT_EQ(
          AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_OP_NO_TILING, is_no_tiling),
          true);
        EXPECT_EQ(is_no_tiling, false);
      }
    }
  };
}

///     data  data
///       \   / |
///        add  |
///          \  |
///           add
TEST_F(GraphCompilerTest, test_build_no_tiling) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, -1, 224, 224})
                         .Attr(ATTR_NAME_OP_TILING_INLINE_ENGINE, engine_list)
                         .Attr(ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, engine_list)
                         .Attr(ATTR_NAME_OP_MAX_SHAPE, "1, 10, 224, 224");
  auto add2 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, -1, 224, 224})
                         .Attr(ATTR_NAME_OP_TILING_INLINE_ENGINE, engine_list)
                         .Attr(ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, engine_list)
                         .Attr(ATTR_NAME_OP_MAX_SHAPE, "20, 10, 224, 224");
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto print = OP_CFG("Print");
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("add_2", add2));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_2", add2));
    CHAIN(NODE("add_2", add2)->EDGE(0, 0)->NODE("Print", print));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_1");
    EXPECT_EQ(graph->GetAllNodesSize(), 5);
    EXPECT_EQ(graph->GetDirectNodesSize(), 5);
    for (auto node : graph->GetAllNodes()) {
      if (node->GetName() == "add_1" || node->GetName() == "add_2") {
        bool is_no_tiling = false;
        EXPECT_EQ(
          AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_OP_NO_TILING, is_no_tiling),
          true);
        EXPECT_EQ(is_no_tiling, true);
      }
    }
  };
}

TEST_F(GraphCompilerTest, test_build_no_tiling_01) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_INT32, {}).Attr(ATTR_NAME_OP_TILING_INLINE_ENGINE, engine_list);
  auto add2 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_INT32, {}).Attr(ATTR_NAME_OP_TILING_INLINE_ENGINE, engine_list);
  auto less = OP_CFG(LESS).TensorDesc(FORMAT_NCHW, DT_BOOL, {}).Attr(ATTR_NAME_OP_TILING_INLINE_ENGINE, engine_list);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto switch1 = OP_CFG(SWITCH);
  auto switch2 = OP_CFG(SWITCH);
  auto switch3 = OP_CFG(SWITCH);
  auto identity1 = OP_CFG(IDENTITY).TensorDesc(FORMAT_NCHW, DT_BOOL, {}).InCnt(1).OutCnt(1);
  auto identity2 = OP_CFG(IDENTITY).TensorDesc(FORMAT_NCHW, DT_BOOL, {}).InCnt(1).OutCnt(1);
  GeTensor weight;
  std::vector<uint8_t> data = {1};
  weight.SetData(data);
  GeTensorDesc weight_desc;
  weight_desc.SetShape(GeShape(std::vector<int64_t>({})));
  weight.SetTensorDesc(weight_desc);
  auto constant1 = OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {}).Attr<GeTensor>(ATTR_NAME_WEIGHTS, weight);
  auto constant2 = OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {}).Attr<GeTensor>(ATTR_NAME_WEIGHTS, weight);
  auto merge = OP_CFG(MERGE).TensorDesc(FORMAT_NCHW, DT_INT32, {-2});
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", data1)->EDGE(0, 0)->NODE("less", less));
    CHAIN(NODE("data2", data2)->EDGE(0, 1)->NODE("less", less));
    CHAIN(NODE("data2", data2)->EDGE(0, 0)->NODE("switch1", switch1));
    CHAIN(NODE("less", less)->EDGE(0, 1)->NODE("switch1", switch1));
    CHAIN(NODE("data1", data1)->EDGE(0, 0)->NODE("switch2", switch2));
    CHAIN(NODE("less", less)->EDGE(0, 1)->NODE("switch2", switch2));
    CHAIN(NODE("less", less)->EDGE(0, 0)->NODE("switch3", switch3));
    CHAIN(NODE("less", less)->EDGE(0, 1)->NODE("switch3", switch3));
    CHAIN(NODE("switch3", switch3)->EDGE(0, 0)->NODE("identity1", identity1));
    CHAIN(NODE("switch3", switch3)->EDGE(1, 0)->NODE("identity2", identity2));
    CHAIN(NODE("identity1", identity1)->CTRL_EDGE()->NODE("constant1", constant1));
    CHAIN(NODE("identity2", identity2)->CTRL_EDGE()->NODE("constant2", constant2));
    CHAIN(NODE("switch1", switch1)->EDGE(0, 0)->NODE("add1", add1));
    CHAIN(NODE("constant1", constant1)->EDGE(0, 1)->NODE("add1", add1));
    CHAIN(NODE("switch2", switch2)->EDGE(0, 0)->NODE("add2", add2));
    CHAIN(NODE("constant2", constant2)->EDGE(0, 1)->NODE("add2", add2));
    CHAIN(NODE("add1", add1)->EDGE(0, 0)->NODE("merge", merge));
    CHAIN(NODE("add2", add2)->EDGE(0, 1)->NODE("merge", merge));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphCompilerTest, test_ffts_plus) {
  ComputeGraphPtr root_graph;
  ComputeGraphPtr dsp_graph;
  ComputeGraphPtr ffts_graph;
  BuildFftsDynamicGraph(root_graph, dsp_graph, ffts_graph);
  SetFftsPlusAttrPass set_ffts_plus_attr_pass;
  Status ret = set_ffts_plus_attr_pass.Run(ffts_graph);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphCompilerTest, test_build_assigner_memory) {
  ge::ComputeGraphPtr compute_graph = std::make_shared<ge::ComputeGraph>("");
  GraphMemoryAssigner graph_mem_assigner(compute_graph);
  graph_mem_assigner.AssignMemory();

  OpDescPtr op_desc_one = std::make_shared<OpDesc>("node_one", "type");
  NodePtr node_one = compute_graph->AddNode(op_desc_one);
  ge::AttrUtils::SetBool(node_one->GetOpDesc(), ATTR_NAME_CONTINUOUS_INPUT, true);
  map<uint64_t, size_t> mem_type_to_offset  = {};
  auto ret = graph_mem_assigner.ReAssignMemory(mem_type_to_offset);
  EXPECT_EQ(ret, SUCCESS);

  OpDescPtr op_desc_two = std::make_shared<OpDesc>("node_two", "type");
  NodePtr node_two = compute_graph->AddNode(op_desc_two);
  std::vector<int64_t> mem_type_list;
  mem_type_list.emplace_back(66);
  ge::AttrUtils::SetListInt(node_two->GetOpDesc(), ATTR_NAME_OUTPUT_MEM_TYPE_LIST, mem_type_list);
  ge::AttrUtils::SetBool(node_two->GetOpDesc(), ATTR_NAME_CONTINUOUS_OUTPUT, true);
  ret = graph_mem_assigner.ReAssignMemory(mem_type_to_offset);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(GraphCompilerTest, test_build_graph_accord_stage_success) {
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "Add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);

  GeTensor tensor(tensor_desc);
  const vector<GeTensor> inputs = { tensor, tensor };
  const vector<GeTensor> outputs = { tensor };

  GeGenerator generator;
  generator.Initialize({});
  ModelBufferData model_buffer;
  ComputeGraphPtr compute_graph = nullptr;

  Status ret = generator.BuildSingleOpModel(op_desc, inputs, outputs, ENGINE_SYS, false, model_buffer,
                                            GraphStage::GRAPH_STAGE_FUZZ, compute_graph);
  EXPECT_EQ(ret, SUCCESS);

  int64_t graph_stage = static_cast<int64_t>(GraphStage::GRAPH_STAGE_RESERVED);
  bool graph_has_been_added = false; 
  // test attr has been cleared
  EXPECT_EQ(AttrUtils::GetInt(compute_graph, kGraphDumpStage, graph_stage), false);
  EXPECT_EQ(AttrUtils::GetBool(compute_graph, ATTR_NAME_GRAPH_HAS_BEEN_ADDED, graph_has_been_added), false);

}

TEST_F(GraphCompilerTest, test_subgraph_multi_dims) {
  auto sub_data_1 = OP_CFG(DATA).Attr("index", 0)
                                .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 2});
  auto sub_data_2 = OP_CFG(DATA).Attr("index", 1)
                                .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, -1});
  auto slice = OP_CFG(SLICE).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 2});
  int32_t data_value_vec1[2] = {0, 0};
  int32_t data_value_vec2[2] = {2, 2};
  GeTensorDesc data_tensor_desc(GeShape({2}), FORMAT_ND, DT_INT32);
  GeTensorPtr data_tensor1 = make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec1, 2 * sizeof(int32_t));
  auto const1 = OP_CFG(CONSTANT).Weight(data_tensor1);
  GeTensorPtr data_tensor2 = make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec2, 2 * sizeof(int32_t));
  auto const2 = OP_CFG(CONSTANT).Weight(data_tensor2);
  auto sub_add = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 2});
  auto sub_net_output = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 2});
  DEF_GRAPH(sub1) {
    CHAIN(NODE("sub_data_1", sub_data_1)->EDGE(0, 0)->NODE("sub_add", sub_add));
    CHAIN(NODE("sub_data_2", sub_data_2)->EDGE(0, 0)->NODE("slice", slice));
    CHAIN(NODE("const_1", const1)->EDGE(0, 1)->NODE("slice"));
    CHAIN(NODE("const_2", const2)->EDGE(0, 2)->NODE("slice"));
    CHAIN(NODE("slice")->EDGE(0, 1)->NODE("sub_add")->EDGE(0, 0)->NODE("sub_net_output", sub_net_output));
    CHAIN(NODE("sub_data_2", sub_data_2)->EDGE(0, 1)->NODE("sub_add"));
  };
  auto partitioned_call = OP_CFG(PARTITIONEDCALL).InCnt(2).OutCnt(1)
                                                 .TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, -1})
                                                 .Attr(ATTR_NAME_SUBGRAPH_MULTI_DIMS_INDEX, 0);
  auto data_1 = OP_CFG(DATA).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, -1});
  auto data_2 = OP_CFG(DATA).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, -1});
  auto data_3 = OP_CFG(DATA).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, 2});
  auto add = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2, -1})
                        .Attr(ATTR_NAME_SUBGRAPH_MULTI_DIMS_INDEX, 0)
                        .Attr(ATTR_NAME_SUBGRAPH_MULTI_DIMS_INPUT_SHAPE, "0: 2, -1; 1: 2,-1")
                        .Attr(ATTR_NAME_SUBGRAPH_MULTI_DIMS_INPUT_DIMS, "2, 2; 4, 4; 8, 8");
  auto cast = OP_CFG(CAST).TensorDesc(FORMAT_NCHW, DT_INT32, {2, -1})
                          .Attr(ATTR_NAME_SUBGRAPH_MULTI_DIMS_INDEX, 0);
  auto net_output = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_FLOAT, {2,2});
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data_1)->EDGE(0, 0)->NODE("add", add));
    CHAIN(NODE("data_2", data_2)->EDGE(0, 1)->NODE("add"));
    CHAIN(NODE("data_3", data_3)->EDGE(0, 0)->NODE("partitioned_call", partitioned_call, sub1));
    CHAIN(NODE("add")->EDGE(0, 1)->NODE("partitioned_call")->EDGE(0, 0)->NODE("cast", cast));
    CHAIN(NODE("cast")->EDGE(0, 0)->NODE("net_output", net_output));
  };
  sub1.Layout();
  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add");
  const auto add_infer_func = [](Operator &op) {
    op.GetOutputDesc(0).SetShape(op.GetInputDesc(0).GetShape());
    return GRAPH_SUCCESS;
  };
  node->GetOpDesc()->AddInferFunc(add_infer_func);

  node = compute_graph->FindNode("cast");
  auto op_desc = node->GetOpDesc();
  (void)ge::AttrUtils::SetInt(op_desc, ATTR_INPUT_MEMORY_TYPE, RT_MEMORY_HBM);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  int data1[2][2] = {{12, 23}, {34, 45}};
  InputTensorInfo input_1 = {3, {2, 2}, data1, 16};
  int data2[2][2] = {{1, 2}, {2, 3}};
  InputTensorInfo input_2 = {3, {2, 2}, data2, 16};
  int data3[2][2] = {{3, 3}, {4, 4}};
  InputTensorInfo input_3 = {3, {2, 2}, data3, 16};
  inputs.push_back(input_1);
  inputs.push_back(input_2);
  inputs.push_back(input_3);
  auto ret = session.BuildGraph(1, inputs);

  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_1");
    EXPECT_EQ(graph->GetDirectNodesSize(), 5);
    EXPECT_EQ(graph->GetAllSubgraphs().size(), 5);
  };
}

TEST_F(GraphCompilerTest, test_dynamic_stack_handle_and_engine) {
  int32_t max_size = 100;
  GeTensorDesc tensor_desc(GeShape(), FORMAT_ND, DT_INT32);
  GeTensorPtr const_tensor =
      std::make_shared<GeTensor>(tensor_desc, reinterpret_cast<uint8_t *>(&max_size), sizeof(int32_t));
  const auto const_op = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor);
  const auto stack = OP_CFG(STACK).InCnt(1).OutCnt(1).Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true);
  const auto stack_push = OP_CFG(STACKPUSH).InCnt(2).OutCnt(1).Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true);
  const auto stack_pop = OP_CFG(STACKPOP).InCnt(1).OutCnt(1).Attr(ATTR_NAME_FORCE_UNKNOWN_SHAPE, true);
  const auto const_op_1 = OP_CFG(CONSTANT).OutCnt(1).Weight(const_tensor);

  DEF_GRAPH(g1) {
    CHAIN(NODE("const", const_op)->EDGE(0, 0)->NODE("stack", stack));

    CHAIN(NODE("stack", stack)->EDGE(0, 0)->NODE("stack_push", stack_push));
    CHAIN(NODE("const", const_op)->EDGE(0, 1)->NODE("stack_push", stack_push));

    CHAIN(NODE("stack", stack)->EDGE(0, 0)->NODE("stack_pop", stack_pop));
    CHAIN(NODE("stack_pop", stack_pop)->EDGE(0, 0)->NODE("add", ADD));
    CHAIN(NODE("const_1", const_op_1)->EDGE(0, 1)->NODE("add", ADD));
  };

  const auto graph = ToGeGraph(g1);
  const auto compute_graph = GraphUtils::GetComputeGraph(graph);
  static const std::unordered_set<std::string> kDataFlowOps = {STACK, STACKPUSH, STACKPOP, STACKCLOSE};
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (kDataFlowOps.count(node->GetType()) != 0UL) {
      node->GetOpDesc()->SetOpEngineName("DNN_VM_AICPU");
      node->GetOpDesc()->SetOpKernelLibName("DNN_VM_AICPU_ASCEND");
    }
  }

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, SUCCESS);

  // 1. check handle
  const int64_t kStackHandle = 0;
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (kDataFlowOps.count(node->GetType()) != 0UL) {
      int64_t handle = -1;
      const bool ret = AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle);
      EXPECT_TRUE(ret);
      EXPECT_EQ(handle, kStackHandle);
    }
  }

  // 2. check engine
  static const char_t *const kGeLocalEngineName = "DNN_VM_GE_LOCAL";
  static const char_t *const kGeLocalOpKernelLibName = "DNN_VM_GE_LOCAL_OP_STORE";
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (kDataFlowOps.count(node->GetType()) != 0UL) {
      EXPECT_EQ(node->GetOpDesc()->GetOpEngineName(), kGeLocalEngineName);
      EXPECT_EQ(node->GetOpDesc()->GetOpKernelLibName(), kGeLocalOpKernelLibName);
    }
  }
}

TEST_F(GraphCompilerTest, test_data_flow_process) {
 auto add_1 = OP_CFG(ADD);
  auto add_2 = OP_CFG(ADD);
  auto add_3 = OP_CFG(ADD);
  auto add_4 = OP_CFG(ADD).Attr("_force_unknown_shape", true);
  auto add_5 = OP_CFG(ADD).Attr("_force_unknown_shape", true);
  auto add_6 = OP_CFG(ADD).Attr("_force_unknown_shape", true);
  auto stack = OP_CFG("Stack");
  auto stackpush = OP_CFG("StackPush");
  auto stackpop = OP_CFG("StackPop");
  auto stack1 = OP_CFG("Stack");
  auto stackpush1 = OP_CFG("StackPush").Attr("_force_unknown_shape", true);
  auto stackpop1 = OP_CFG("StackPop").Attr("_force_unknown_shape", true);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto op_ptr = OP_CFG(DATA)
    .InCnt(1)
    .OutCnt(1)
    .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
    .Attr("compile_info_key", "ddd")
    .Attr("compile_info_json", "cccc")
    .Attr("_force_unknown_shape", true)
    .Build("data3");
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", data1)->EDGE(0, 0)->NODE("add_1", add_1)->EDGE(0, 0)
          ->NODE("add_2", add_2)->EDGE(0, 0)->NODE("add_3", add_3)
          ->NODE("add_4", add_4)->EDGE(0, 0)->NODE("add_5", add_5)
          ->NODE("add_6", add_6));

    CHAIN(NODE("data2", data2)->EDGE(0, 1)->NODE("add_1", add_1));
    CHAIN(NODE("data2", data2)->EDGE(0, 1)->NODE("add_2", add_2));
    CHAIN(NODE(op_ptr)->EDGE(0, 1)->NODE("add_4", add_4));
    CHAIN(NODE(op_ptr)->EDGE(0, 1)->NODE("add_5", add_5));

    CHAIN(NODE("stack", stack)->EDGE(0, 0)->NODE("stackpush", stackpush));
    CHAIN(NODE("stack", stack)->EDGE(0, 0)->NODE("stackpop", stackpop));
    CHAIN(NODE("add_1", add_1)->EDGE(0, 1)->NODE("stackpush", stackpush));
    CHAIN(NODE("stackpop", stackpop)->EDGE(0, 1)->NODE("add_3", add_3));
    CHAIN(NODE("stack1", stack)->EDGE(0, 0)->NODE("stackpush1", stackpush1));
    CHAIN(NODE("stack1", stack)->EDGE(0, 0)->NODE("stackpop1", stackpop1));
    CHAIN(NODE("add_4", add_4)->EDGE(0, 1)->NODE("stackpush1", stackpush1));
    CHAIN(NODE("stackpop1", stackpop1)->EDGE(0, 1)->NODE("add_6", add_6));

  };
  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  for (auto &node : compute_graph->GetAllNodes()) {
    if (node->GetName() == "stack" || node->GetName() == "stackpush" || node->GetName() == "stackpop") {
      (void)AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, 1);
    }
    if (node->GetName() == "stack1" || node->GetName() == "stackpush1" || node->GetName() == "stackpop1") {
      (void)AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, 2);
    }
  }
  auto graph_2 = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph_2, options);
  std::vector<InputTensorInfo> inputs;
  (void)session.BuildGraph(1, inputs);

}

TEST_F(GraphCompilerTest, test_switch_dead_branch_merge_pass) {
  Graph graph = BuildSwitchMergeGraph();

  // new session & add graph
  map<AscendString, AscendString> options;
  Session session(options);
  auto ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    EXPECT_EQ(graph->GetDirectNodesSize(), 7);
    auto switch1 = graph->FindNode("switch");
    EXPECT_EQ(switch1, nullptr);
    auto merge1 = graph->FindNode("merge");
    EXPECT_EQ(merge1, nullptr);
  };
}

TEST_F(GraphCompilerTest, test_hccl_node_compile) {
  Graph graph = BuildGraphWithHcclNode();

  // new session & add graph
  map<AscendString, AscendString> options;
  Session session(options);
  auto ret = session.AddGraph(0, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(0, inputs);
  EXPECT_EQ(ret, SUCCESS);

  // TODO check node
}

TEST_F(GraphCompilerTest, test_build_graph_with_maxsize_success) {
  SetGeLocalBuilder();
  DEF_GRAPH(g1) {
    auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_ND, DT_STRING, {200, 200, 3})
                           .Attr("_op_max_shape", "200,200,3");
    auto data1 = OP_CFG(DATA).TensorDesc(FORMAT_ND, DT_STRING, {0});
    auto data2 = OP_CFG(DATA).TensorDesc(FORMAT_ND, DT_STRING, {0});
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  EXPECT_EQ(ge::AttrUtils::SetListInt(node->GetOpDesc(), "_op_max_size", {1000}), true);

  auto node_data = compute_graph->FindNode("data_1");
  (void)AttrUtils::SetBool(node_data->GetOpDesc(), "OwnerGraphIsUnknown", true);
  (void) AttrUtils::SetStr(node_data->GetOpDesc(), ATTR_NAME_ENGINE_NAME_FOR_LX, "DNN_VM_GE_LOCAL");
  (void) AttrUtils::SetStr(node_data->GetOpDesc(), ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, "DNN_VM_GE_LOCAL_OP_STORE");

  node_data = compute_graph->FindNode("data_2");
  (void)AttrUtils::SetBool(node_data->GetOpDesc(), "OwnerGraphIsUnknown", true);
  (void) AttrUtils::SetStr(node_data->GetOpDesc(), ATTR_NAME_ENGINE_NAME_FOR_LX, "DNN_VM_GE_LOCAL");
  (void) AttrUtils::SetStr(node_data->GetOpDesc(), ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, "DNN_VM_GE_LOCAL_OP_STORE");

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  Status ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, FAILED);
  SetFakerBuilder();
}

///     data1
///       \
///      hcom
///        \
///      netout
TEST_F(GraphCompilerTest, test_build_memory_buffer_pool_fail) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto data1 = OP_CFG(DATA)
                    .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 32, 32})
                    .Attr(ATTR_NAME_BUFFER_POOL_ID, 1)
                    .Attr(ATTR_NAME_BUFFER_POOL_SIZE, 10240);
  auto hcom = OP_CFG(HCOMALLREDUCE)
                    .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 32, 32});
  auto netout = OP_CFG(NETOUTPUT)
                   .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 32, 32});

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("hcom_1", hcom));
    CHAIN(NODE("hcom_1", hcom)->EDGE(0, 0)->NODE("out_1", netout));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("data_1");
  TensorUtils::SetSize(*node->GetOpDesc()->MutableOutputDesc(0), 5120);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///        hcom
///       /   \
///    print  print
TEST_F(GraphCompilerTest, test_build_memory_continuous) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int64_t> memtype_list = {RT_MEMORY_HBM, RT_MEMORY_HBM};
  auto hcom = OP_CFG(HCOMALLREDUCE)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                  .Attr(ATTR_NAME_INPUT_MEM_TYPE_LIST, memtype_list)
                  .Attr(ATTR_NAME_OUTPUT_MEM_TYPE_LIST, memtype_list)
                  .Attr(ATTR_NAME_CONTINUOUS_INPUT, true)
                  .Attr(ATTR_NAME_CONTINUOUS_OUTPUT, true);

  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto data3 = OP_CFG("Print");
  auto data4 = OP_CFG("Print");

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("hcom_1", hcom));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("hcom_1", hcom));
    CHAIN(NODE("hcom_1", hcom)->EDGE(0, 0)->NODE("data_3", data3));
    CHAIN(NODE("hcom_1", hcom)->EDGE(1, 0)->NODE("data_4", data4));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("hcom_1");
  auto op_desc = node->GetOpDesc();
  op_desc->SetWorkspace({0,0});
  op_desc->SetWorkspaceBytes({32,32});

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///      broadcast
///       /   \
///    print   print
TEST_F(GraphCompilerTest, test_build_memory_continuous_broadcast) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int64_t> memtype_list = {RT_MEMORY_HBM, RT_MEMORY_HBM};
  auto hcom = OP_CFG(HCOMBROADCAST)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                  .Attr(ATTR_NAME_INPUT_MEM_TYPE_LIST, memtype_list)
                  .Attr(ATTR_NAME_OUTPUT_MEM_TYPE_LIST, memtype_list)
                  .Attr(ATTR_NAME_CONTINUOUS_INPUT, true)
                  .Attr(ATTR_NAME_CONTINUOUS_OUTPUT, true);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto print1 = OP_CFG("Print");
  auto print2 = OP_CFG("Print");

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("hcom_1", hcom));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("hcom_1", hcom));
    CHAIN(NODE("hcom_1", hcom)->EDGE(0, 0)->NODE("print_1", print1));
    CHAIN(NODE("hcom_1", hcom)->EDGE(1, 0)->NODE("print_2", print2));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///        add
///         |
///        hcom
///       /
///    netout
TEST_F(GraphCompilerTest, test_build_memory_atomic) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int32_t> input_indexes = {-1};
  auto hcom = OP_CFG(HCOMALLREDUCE).Attr(ATTR_NAME_CONTINUOUS_INPUT, true);
  auto add1 = OP_CFG(ADD).Attr(ATOMIC_ATTR_IS_ATOMIC_NODE, true);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto netout = OP_CFG(NETOUTPUT);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("hcom_1", hcom));
    CHAIN(NODE("hcom_1", hcom)->EDGE(0, 0)->NODE("out_1", netout));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("hcom_1");
  auto op_desc = node->GetOpDesc();
  (void) ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_INPUT_INDEX, input_indexes);

  node = compute_graph->FindNode("add_1");
  op_desc = node->GetOpDesc();
  std::vector<int64_t> atomic_output_index = {0};
  (void) ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///        add
///         |
///     NETOUTPUT
///
TEST_F(GraphCompilerTest, test_build_memory_atomic_netoutput) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int64_t> atomic_output_index = {0};
  auto netout = OP_CFG(NETOUTPUT);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto add = OP_CFG(ADD).Attr(ATOMIC_ATTR_IS_ATOMIC_NODE, true);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add));
    CHAIN(NODE("add_1", data2)->EDGE(0, 0)->NODE("out_1", netout));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  auto op_desc = node->GetOpDesc();
  (void)ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);
  (void)ge::AttrUtils::SetBool(op_desc, ATOMIC_ATTR_IS_FUSION_NODE, true);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///     NETOUTPUT
///
TEST_F(GraphCompilerTest, test_build_memory_atomic_netoutput_fail) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int32_t> input_indexes = {-1};
  std::vector<int64_t> atomic_output_index = {0};
  auto hcom = OP_CFG(NETOUTPUT)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                  .Attr(ATOMIC_ATTR_INPUT_INDEX, input_indexes)
                  .Attr(ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index)
                  .Attr(ATTR_NAME_CONTINUOUS_INPUT, true);
  auto data1 = OP_CFG(DATA)
                   .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                   .Attr(ATOMIC_ATTR_IS_ATOMIC_NODE, true)
                   .Attr(ATOMIC_ATTR_OUTPUT_INDEX, input_indexes);
  auto data2 = OP_CFG(DATA).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224}).Attr(ATOMIC_ATTR_IS_ATOMIC_NODE, true);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("hcom_1", hcom));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("hcom_1", hcom));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, FAILED);
}

///     data  data
///       \   /
///        add
///         |
///     NETOUTPUT
///
TEST_F(GraphCompilerTest, test_build_memory_atomic_fusion) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int64_t> atomic_output_index = {0};
  auto netout = OP_CFG(NETOUTPUT);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto add = OP_CFG(ADD).Attr(ATOMIC_ATTR_IS_ATOMIC_NODE, true);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add));
    CHAIN(NODE("add_1", data2)->EDGE(0, 0)->NODE("out_1", netout));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  auto op_desc = node->GetOpDesc();
  (void)ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);
  (void)ge::AttrUtils::SetBool(op_desc, ATOMIC_ATTR_IS_FUSION_NODE, true);
  (void)op_desc->SetExtAttr("atomic_clean_node_ptr", node);

  map<string, map<int64_t, int64_t>> workspace_info;
  workspace_info["add_1"][0] = 100;
  (void)op_desc->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspace_info);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///      assignadd
///         |
///     NETOUTPUT
TEST_F(GraphCompilerTest, test_build_memory_ref) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto add1 = OP_CFG(ASSIGNADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 32, 32});
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto netout = OP_CFG(NETOUTPUT);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("out_1", netout));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  auto op_desc = node->GetOpDesc();
  (void)ge::AttrUtils::SetBool(op_desc, ATTR_NAME_REFERENCE, true);
  op_desc->MutableAllOutputName().erase("__output0");
  op_desc->MutableAllOutputName()["__input0"] = 0;

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///      assignadd
///         |
///     NETOUTPUT
TEST_F(GraphCompilerTest, test_build_memory_atomic_ref) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto add1 =
      OP_CFG(ASSIGNADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 32, 32}).Attr(ATOMIC_ATTR_IS_ATOMIC_NODE, true);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto netout = OP_CFG(NETOUTPUT);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("out_1", netout));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  auto op_desc = node->GetOpDesc();
  (void)ge::AttrUtils::SetBool(op_desc, ATTR_NAME_REFERENCE, true);
  (void)ge::AttrUtils::SetInt(op_desc, ATTR_INPUT_MEMORY_TYPE, RT_MEMORY_HBM);
  std::vector<int64_t> atomic_output_index = {0};
  (void)ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);
  op_desc->MutableAllOutputName().erase("__output0");
  op_desc->MutableAllOutputName()["__input0"] = 0;

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data  data  data
///       \   /      \   /
///        add       add
///          \      /
///           RELU
TEST_F(GraphCompilerTest, test_build_memory_continuous_nopading) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int64_t> memtype_list = {RT_MEMORY_HBM, RT_MEMORY_HBM};
  std::vector<int32_t> input_indexes = {-1};
  auto relu = OP_CFG(RELU)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                  .Attr(ATTR_NAME_OUTPUT_REUSE_INPUT, true)
                  .Attr(ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 0)
                  .Attr(ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto data3 = OP_CFG(DATA);
  auto data4 = OP_CFG(DATA);
  auto add1 = OP_CFG(DATA);
  auto add2 = OP_CFG(DATA);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("data_3", data3)->EDGE(0, 0)->NODE("add_2", add2));
    CHAIN(NODE("data_4", data4)->EDGE(0, 1)->NODE("add_2", add2));
    CHAIN(NODE("add_1", data3)->EDGE(0, 0)->NODE("relu_1", relu));
    CHAIN(NODE("add_2", data4)->EDGE(1, 0)->NODE("relu_1", relu));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("add_1");
  auto op_desc = node->GetOpDesc();
  (void) ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_INPUT_INDEX, input_indexes);
  std::vector<int64_t> atomic_output_index = {0};
  (void) ge::AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///      data  data
///        \  /
///        add
///         |
///       SLICE
///       |   \
///    CONV2D CONV2D
TEST_F(GraphCompilerTest, test_build_memory_output_continuous_nopading_fail) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto slice = OP_CFG(SLICE)
                   .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                   .Attr(ATTR_NAME_OUTPUT_REUSE_INPUT, true)
                   .Attr(ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 0)
                   .Attr(ATTR_NAME_NOPADDING_CONTINUOUS_OUTPUT, true);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto conv1 = OP_CFG(CONV2D);
  auto conv2 = OP_CFG(CONV2D);

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", slice)->EDGE(0, 0)->NODE("slice_1", slice));
    CHAIN(NODE("slice_1", slice)->EDGE(0, 0)->NODE("conv_1", conv1));
    CHAIN(NODE("slice_1", slice)->EDGE(1, 0)->NODE("conv_2", conv2));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("slice_1");
  auto op_desc = node->GetOpDesc();
  op_desc->MutableOutputDesc(0)->MutableShape().SetDimNum(1);
  op_desc->MutableOutputDesc(0)->MutableShape().SetDim(0, 1);
  op_desc->MutableOutputDesc(1)->MutableShape().SetDimNum(1);
  op_desc->MutableOutputDesc(1)->MutableShape().SetDim(0, 1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///        add
///         |
///       RELU
///        |
///       RELU
TEST_F(GraphCompilerTest, test_build_memory_continuous_nopading_cascade) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int64_t> memtype_list = {RT_MEMORY_HBM, RT_MEMORY_HBM};
  auto relu1 = OP_CFG(RELU)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                  .Attr(ATTR_NAME_OUTPUT_REUSE_INPUT, true)
                  .Attr(ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 0)
                  .Attr(ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
  auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto relu2 = OP_CFG(RELU)
                  .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224})
                  .Attr(ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
  auto data1 = OP_CFG(DATA).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto data2 = OP_CFG(DATA).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 32, 32});

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("relu_1", relu1));
    CHAIN(NODE("relu_1", relu1)->EDGE(0, 0)->NODE("relu_2", relu2));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     data  data
///       \   /
///     NETOUTPUT
///
TEST_F(GraphCompilerTest, test_build_memory_ddr) {
  putenv("OP_NO_REUSE_MEM=data_1,data_2");
  vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int32_t> input_indexes = {-1};
  std::vector<int64_t> atomic_output_index = {};
  auto hcom = OP_CFG(NETOUTPUT);
  auto data1 = OP_CFG(DATA)
                   .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto data2 = OP_CFG(DATA)
                   .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});

  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("hcom_1", hcom));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("hcom_1", hcom));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("data_1");
  auto op_desc = node->GetOpDesc();
  std::vector<int64_t> output_memory_types = {RT_MEMORY_P2P_DDR};
  (void)ge::AttrUtils::SetListInt(op_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, output_memory_types);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     var
///       \
///      broadcast
///      /
///     print
TEST_F(GraphCompilerTest, test_build_memory_var_broadcast) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto hcom = OP_CFG(HCOMBROADCAST).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto var1 = OP_CFG(VARIABLE).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto print1 = OP_CFG(NETOUTPUT).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});

  DEF_GRAPH(g1) {
    CHAIN(NODE("var_1", var1)->EDGE(0, 0)->NODE("hcom_1", hcom));
    CHAIN(NODE("hcom_1", hcom)->EDGE(0, 0)->NODE("print_1", print1));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("hcom_1");
  TensorUtils::SetSize(*node->GetOpDesc()->MutableOutputDesc(0), 200736);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     var
///       \
///       cast
///       /   \
///     print assign
TEST_F(GraphCompilerTest, test_build_memory_var_assign) {
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto assign = OP_CFG(ASSIGN);
  auto var1 = OP_CFG(VARIABLE);
  auto cast = OP_CFG(CAST);
  auto print = OP_CFG("Print");

  DEF_GRAPH(g1) {
    CHAIN(NODE("var_1", var1)->EDGE(0, 0)->NODE("cast_1", cast));
    CHAIN(NODE("cast_1", cast)->EDGE(0, 0)->NODE("print_1", print));
    CHAIN(NODE("cast_1", cast)->EDGE(1, 0)->NODE("assign_1", assign));
  };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("assign_1");
  auto op_desc = node->GetOpDesc();
  (void)ge::AttrUtils::SetInt(op_desc, ATTR_INPUT_MEMORY_TYPE, RT_MEMORY_HBM);
  (void) AttrUtils::SetBool(op_desc, "partially_supported", true);
  (void)ge::AttrUtils::SetInt(op_desc, ATTR_NAME_FUSION_GROUP_KEY, 0);

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}


///     data1  data2
///        \   /
///        merge
///
TEST_F(GraphCompilerTest, test_update_input_output) {
  const auto data1 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {16, 16, 224, 224})
      .Build("data1");
  const auto data2 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT16, {16, 16, 224, 224})
      .Build("data2");
  const auto merge = OP_CFG(MERGE)
      .InCnt(1).OutCnt(2)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("merge1");
  const auto net_output = OP_CFG(NETOUTPUT)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("NetOutput");

  (void)AttrUtils::SetStr(data1, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  (void)AttrUtils::SetStr(data2, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  (void)AttrUtils::SetStr(data1, ATTR_ATC_USER_DEFINE_FORMAT, "NC1HWC0");
  (void)AttrUtils::SetStr(data2, ATTR_ATC_USER_DEFINE_FORMAT, "NC1HWC0");

  (void)AttrUtils::SetListStr(net_output, ATTR_ATC_USER_DEFINE_DATATYPE, {"0:DT_FLOAT16"});
  (void)AttrUtils::SetListStr(net_output, ATTR_ATC_USER_DEFINE_FORMAT, {"0:NC1HWC0"});

  DEF_GRAPH(g1) {
    CHAIN(NODE(data1)->EDGE(0, 0)->NODE(merge)->EDGE(0, 0)->NODE(net_output));
    CHAIN(NODE(data2)->EDGE(0, 1)->NODE(merge));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

///     const1  data2
///        \   /
///        merge
///
TEST_F(GraphCompilerTest, test_check_ref_input_node_failed) {
  const auto const1 = OP_CFG(CONSTANT)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {16, 16, 224, 224})
      .Build("const1");
  const auto data2 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT16, {16, 16, 224, 224})
      .Build("data2");
  const auto merge = OP_CFG(MERGE)
      .InCnt(1).OutCnt(2)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("merge1");
  const auto net_output = OP_CFG(NETOUTPUT)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("net_output1");

  (void)AttrUtils::SetStr(const1, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  (void)AttrUtils::SetStr(data2, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  (void)AttrUtils::SetStr(const1, ATTR_ATC_USER_DEFINE_FORMAT, "NC1HWC0");
  (void)AttrUtils::SetStr(data2, ATTR_ATC_USER_DEFINE_FORMAT, "NC1HWC0");

  (void)AttrUtils::SetListStr(net_output, ATTR_ATC_USER_DEFINE_DATATYPE, {"0:DT_FLOAT16"});
  (void)AttrUtils::SetListStr(net_output, ATTR_ATC_USER_DEFINE_FORMAT, {"0:NC1HWC0"});

  DEF_GRAPH(g1) {
                  CHAIN(NODE(const1)->EDGE(0, 0)->NODE(merge)->EDGE(0, 0)->NODE(net_output));
                  CHAIN(NODE(data2)->EDGE(0, 1)->NODE(merge));
                };

  auto graph = ToGeGraph(g1);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto node = compute_graph->FindNode("merge1");
  auto op_desc = node->GetOpDesc();
  op_desc->MutableAllOutputName().erase("__output0");
  op_desc->MutableAllOutputName()["__input0"] = 0;

  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, FAILED);
}

///     data1  data2
///        \   /
///         Add
///
TEST_F(GraphCompilerTest, test_storage_format) {
  const auto data1 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 16, 224, 224})
      .Build("data1");
  const auto data2 = OP_CFG(DATA)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {-1, 16, 224, 224})
      .Build("data2");
  const auto add = OP_CFG(ADD)
      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 16, 16, 224, 224})
      .Build("add1");
  const auto net_output = OP_CFG(NETOUTPUT)
      .InCnt(1)
      .TensorDesc(FORMAT_NHWC, DT_FLOAT, {16, 224, 224, 16})
      .Build("NetOutput");

  (void)AttrUtils::SetInt(data1->MutableInputDesc(0), ATTR_NAME_STORAGE_FORMAT, FORMAT_NC1HWC0);
  (void)AttrUtils::SetInt(data2->MutableInputDesc(0), ATTR_NAME_STORAGE_FORMAT, FORMAT_NC1HWC0);
  (void)AttrUtils::SetListInt(data1->MutableInputDesc(0), ATTR_NAME_STORAGE_SHAPE, {16, 1, 224, 224, 16});
  (void)AttrUtils::SetListInt(data2->MutableInputDesc(0), ATTR_NAME_STORAGE_SHAPE, {16, 1, 224, 224, 16});

  (void)AttrUtils::SetInt(net_output->MutableInputDesc(0), ATTR_NAME_STORAGE_FORMAT, FORMAT_NC1HWC0);
  (void)AttrUtils::SetListInt(net_output->MutableInputDesc(0), ATTR_NAME_STORAGE_SHAPE, {16, 1, 224, 224, 16});

  std::vector<std::pair<int64_t, int64_t>> origin_range({{1,16}, {16, 16}, {224, 224}, {224, 224}});
  data1->MutableInputDesc(0)->SetOriginShapeRange(origin_range);

  data1->SetOpEngineName(kEngineNameGeLocal);
  data1->SetOpKernelLibName(kEngineNameGeLocal);
  data2->SetOpEngineName(kEngineNameGeLocal);
  data2->SetOpKernelLibName(kEngineNameGeLocal);
  add->SetOpEngineName(kEngineNameGeLocal);
  add->SetOpKernelLibName(kEngineNameGeLocal);
  net_output->SetOpEngineName(kEngineNameGeLocal);
  net_output->SetOpKernelLibName(kEngineNameGeLocal);

  DEF_GRAPH(g1) {
    CHAIN(NODE(data1)->EDGE(0, 0)->NODE(add)->EDGE(0, 0)->NODE(net_output));
    CHAIN(NODE(data2)->EDGE(0, 1)->NODE(add));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);

  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(GraphCompilerTest, select_engine_by_attr_when_opdesc_empty) {
  std::string engine_name = "engine_name";
  std::string kernel_name = "kernel_name";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  AttrUtils::SetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, engine_name);
  AttrUtils::SetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, kernel_name);
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), SUCCESS);

  ASSERT_EQ(op_desc->GetOpEngineName(), engine_name);
  ASSERT_EQ(op_desc->GetOpKernelLibName(), kernel_name);
}

TEST_F(GraphCompilerTest, select_engine_by_when_opdesc_and_attr_empty) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), FAILED);

  ASSERT_TRUE(op_desc->GetOpEngineName().empty());
}

TEST_F(GraphCompilerTest, select_engine_when_opdesc_not_empty) {
  std::string engine_name = "engine_name";
  std::string kernel_name = "kernel_name";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  op_desc->SetOpEngineName(engine_name);
  op_desc->SetOpKernelLibName(kernel_name);
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), SUCCESS);

  std::string attr_engine_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, attr_engine_name);
  ASSERT_EQ(attr_engine_name, engine_name);
  std::string attr_kernel_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, attr_kernel_name);
  ASSERT_EQ(attr_kernel_name, kernel_name);
}

TEST_F(GraphCompilerTest, select_engine_when_opdesc_confilct_with_attr) {
  std::string op_engine_name = "op_engine_name";
  std::string op_kernel_name = "op_kernel_name";
  std::string attr_engine_name = "attr_engine_name";
  std::string attr_kernel_name = "attr_kernel_name";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  op_desc->SetOpEngineName(op_engine_name);
  op_desc->SetOpKernelLibName(op_kernel_name);
  AttrUtils::SetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, attr_engine_name);
  AttrUtils::SetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, attr_kernel_name);
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), SUCCESS);

  std::string fetched_attr_engine_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, fetched_attr_engine_name);
  ASSERT_EQ(fetched_attr_engine_name, attr_engine_name);
  std::string fetched_attr_kernel_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, fetched_attr_kernel_name);
  ASSERT_EQ(fetched_attr_kernel_name, attr_kernel_name);

  ASSERT_EQ(op_desc->GetOpEngineName(), op_engine_name);
  ASSERT_EQ(op_desc->GetOpKernelLibName(), op_kernel_name);
}
