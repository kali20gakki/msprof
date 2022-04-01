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

#define private public
#define protected public
#include "graph/partition/dynamic_shape_partition.h"
#include "compute_graph.h"
#include "graph/compute_graph_impl.h"
#include "framework/common/types.h"
#include "utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "common/omg_util.h"
#include <gmock/gmock.h>
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/utils/tensor_utils.h"
#include "graph/operator_factory.h"
#include "graph/operator_reg.h"

namespace ge {
namespace {
GeTensorDescPtr CreateTensorDesc(std::initializer_list<int64_t> shape, Format format = FORMAT_NCHW,
                                 DataType data_type = DT_FLOAT) {
  GeShape ge_shape{vector<int64_t>(shape)};
  GeTensorDescPtr tensor_desc = std::make_shared<GeTensorDesc>();
  tensor_desc->SetShape(ge_shape);
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);
  return tensor_desc;
}

class NodeBuilder {
 public:
  NodeBuilder(const std::string &name, const std::string &type) { op_desc_ = std::make_shared<OpDesc>(name, type); }

  NodeBuilder &AddInputDesc(std::initializer_list<int64_t> shape = {1, 1, 224, 224}, Format format = FORMAT_NCHW,
                            DataType data_type = DT_FLOAT) {
    op_desc_->AddInputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  NodeBuilder &AddOutputDesc(std::initializer_list<int64_t> shape = {1, 1, 224, 224}, Format format = FORMAT_NCHW,
                             DataType data_type = DT_FLOAT) {
    op_desc_->AddOutputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  NodeBuilder &AddOutputDesc(GeTensorDescPtr tensor_desc) {
    op_desc_->AddOutputDesc(tensor_desc->Clone());
    return *this;
  }

  NodePtr Build(const ComputeGraphPtr &graph) {
    NodePtr node = graph->AddNode(op_desc_);
    return node;
  }

 private:
  OpDescPtr op_desc_;
};
}  // namespace

class UtestDynamicShapePartition : public testing::Test {
  protected:
    void SetUp() {}

    void TearDown() {}
};

TEST_F(UtestDynamicShapePartition, single_op_scene_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");

  NodePtr node1 =
      NodeBuilder("node1", CONSTANTOP).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr add_n_node =
      NodeBuilder("add_n_node", ADDN).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr node2 =
      NodeBuilder("node2", RELU).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), add_n_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  (void)AttrUtils::SetBool(add_n_node->GetOpDesc(), ATTR_SINGLE_OP_SCENE, true);

  DynamicShapePartitioner partitioner(graph);
  EXPECT_EQ(partitioner.Partition(), SUCCESS);
}

TEST_F(UtestDynamicShapePartition, not_single_op_scene_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");

  ComputeGraphPtr root_graph = std::make_shared<ComputeGraph>("root");
  NodePtr node1_root =
      NodeBuilder("node1", CONSTANTOP).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({-1, -1, 224, 224}).Build(root_graph);
  NodePtr add_n_node_root =
      NodeBuilder("add_n_node", ADDN).AddInputDesc({-1, -1, 224, 224}).AddOutputDesc({-1, -1, 224, 224}).Build(root_graph);
  NodePtr node2_root =
      NodeBuilder("node2", RELU).AddInputDesc({-1, -1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(root_graph);

  AttrUtils::SetStr(*root_graph, ATTR_NAME_SESSION_GRAPH_ID, "123");

  NodePtr node1 =
      NodeBuilder("node1", CONSTANTOP).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({-1, -1, 224, 224}).Build(graph);
  NodePtr add_n_node =
      NodeBuilder("add_n_node", ADDN).AddInputDesc({-1, -1, 224, 224}).AddOutputDesc({-1, -1, 224, 224}).Build(graph);
  NodePtr node2 =
      NodeBuilder("node2", RELU).AddInputDesc({-1, -1, 224, 224}).AddOutputDesc({1, 1, 224, 224}).Build(graph);
  NodePtr node4 =
      NodeBuilder("node4", CONSTANTOP).AddInputDesc({1, 1, 224, 224}).AddOutputDesc({-1, -1, 224, 224}).Build(graph);

  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), add_n_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node->GetOutControlAnchor(), node2->GetInControlAnchor());
  GraphUtils::AddEdge(add_n_node->GetOutControlAnchor(), node4->GetInControlAnchor());
  GraphUtils::AddEdge(node4->GetOutControlAnchor(), node2->GetInControlAnchor());


  GraphUtils::AddEdge(node1_root->GetOutDataAnchor(0), add_n_node_root->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node_root->GetOutDataAnchor(0), node2_root->GetInDataAnchor(0));
  GraphUtils::AddEdge(add_n_node_root->GetOutControlAnchor(), node2_root->GetInControlAnchor());

  root_graph->AddSubGraph(graph);

  DynamicShapePartitioner partitioner(root_graph);

  bool is_unknown = false;
  EXPECT_EQ(partitioner.IsUnknownShapeNode(add_n_node_root, is_unknown), SUCCESS);
  EXPECT_EQ(is_unknown, true);

  EXPECT_EQ(partitioner.Partition(), SUCCESS);
}

/*******************************************************************************
 *                |
 *              Merge1
 *      Active /      \ Active
 *            /        \.
 *           /          \.
 *        Merge2         \.
 *  Active/   \Active     \.
 *       /     \           \.
 *     Add      Sub       Relu
 *      |        |          |
 *      |        |          |
 * Switch_f2  Switch_t2     |
 *       \      /           |
 *        \    /            |
 *         Less2            |
 *           |              |
 *           |              |
 *       Switch_f      Switch_t
 *           |   \      /   |
 *           |    Active    |
 *           |       |      |
 *           |     Less1    |
 *           |     /   \    |
 *           |    /     \   |
 *            Data       Data
 ******************************************************************************/
TEST_F(UtestDynamicShapePartition, merge_control_flow_group) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");

  auto data1 = NodeBuilder("data1", DATA).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto data2 = NodeBuilder("data2", DATA).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);

  auto less1 = NodeBuilder("less1", LESS).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto active1 = NodeBuilder("active1", STREAMACTIVE).Build(graph);
  auto switch_t = NodeBuilder("switch_t", STREAMSWITCH).AddInputDesc({1}).AddInputDesc({1}).Build(graph);
  auto switch_f = NodeBuilder("switch_f", STREAMSWITCH).AddInputDesc({1}).AddInputDesc({1}).Build(graph);
  auto const_01 = NodeBuilder("const_01", CONSTANT).AddOutputDesc({1}).Build(graph);
  auto const_11 = NodeBuilder("const_11", CONSTANT).AddOutputDesc({1}).Build(graph);


  auto less2 = NodeBuilder("less2", LESS).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto active2 = NodeBuilder("active2", STREAMACTIVE).Build(graph);
  auto switch_t2 = NodeBuilder("switch_t2", STREAMSWITCH).AddInputDesc({1}).AddInputDesc({1}).Build(graph);
  auto switch_f2 = NodeBuilder("switch_f2", STREAMSWITCH).AddInputDesc({1}).AddInputDesc({1}).Build(graph);
  auto const_02 = NodeBuilder("const_02", CONSTANT).AddOutputDesc({1}).Build(graph);
  auto const_12 = NodeBuilder("const_12", CONSTANT).AddOutputDesc({1}).Build(graph);

  auto add2 = NodeBuilder("add2", ADD).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto sub2 = NodeBuilder("sub2", SUB).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto merge2 = NodeBuilder("merge2", STREAMMERGE).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto active_f2 = NodeBuilder("active_f2", STREAMACTIVE).Build(graph);
  auto active_t2 = NodeBuilder("active_t2", STREAMACTIVE).Build(graph);

  auto relu1 = NodeBuilder("relu1", RELU).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto merge1 = NodeBuilder("merge1", STREAMMERGE).AddInputDesc({1}).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto active_f1 = NodeBuilder("active_f1", STREAMACTIVE).Build(graph);
  auto active_t1 = NodeBuilder("active_t1", STREAMACTIVE).Build(graph);

  auto output1 = NodeBuilder("noutput1", NETOUTPUT).AddInputDesc({1}).Build(graph);

  GraphUtils::AddEdge(data1->GetOutDataAnchor(0), less1->GetInDataAnchor(0));
  GraphUtils::AddEdge(data2->GetOutDataAnchor(0), less1->GetInDataAnchor(1));
  GraphUtils::AddEdge(less1->GetOutDataAnchor(0), switch_t->GetInDataAnchor(0));
  GraphUtils::AddEdge(less1->GetOutDataAnchor(0), switch_f->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_01->GetOutDataAnchor(0), switch_t->GetInDataAnchor(1));
  GraphUtils::AddEdge(const_11->GetOutDataAnchor(0), switch_f->GetInDataAnchor(1));
  GraphUtils::AddEdge(less1->GetOutControlAnchor(), active1->GetInControlAnchor());
  GraphUtils::AddEdge(active1->GetOutControlAnchor(), switch_t->GetInControlAnchor());
  GraphUtils::AddEdge(active1->GetOutControlAnchor(), switch_f->GetInControlAnchor());


  GraphUtils::AddEdge(data1->GetOutDataAnchor(0), less2->GetInDataAnchor(0));
  GraphUtils::AddEdge(less1->GetOutDataAnchor(0), less2->GetInDataAnchor(1));
  GraphUtils::AddEdge(less2->GetOutDataAnchor(0), switch_t2->GetInDataAnchor(0));
  GraphUtils::AddEdge(less2->GetOutDataAnchor(0), switch_f2->GetInDataAnchor(0));
  GraphUtils::AddEdge(const_02->GetOutDataAnchor(0), switch_t2->GetInDataAnchor(1));
  GraphUtils::AddEdge(const_12->GetOutDataAnchor(0), switch_f2->GetInDataAnchor(1));
  GraphUtils::AddEdge(less2->GetOutControlAnchor(), active2->GetInControlAnchor());
  GraphUtils::AddEdge(active2->GetOutControlAnchor(), switch_t2->GetInControlAnchor());
  GraphUtils::AddEdge(active2->GetOutControlAnchor(), switch_f2->GetInControlAnchor());


  GraphUtils::AddEdge(switch_f2->GetOutControlAnchor(), add2->GetInControlAnchor());
  GraphUtils::AddEdge(less2->GetOutDataAnchor(0), add2->GetInDataAnchor(0));
  GraphUtils::AddEdge(add2->GetOutDataAnchor(0), merge2->GetInDataAnchor(0));
  GraphUtils::AddEdge(add2->GetOutControlAnchor(), active_f2->GetInControlAnchor());
  GraphUtils::AddEdge(active_f2->GetOutControlAnchor(), merge2->GetInControlAnchor());

  GraphUtils::AddEdge(switch_t2->GetOutControlAnchor(), sub2->GetInControlAnchor());
  GraphUtils::AddEdge(less2->GetOutDataAnchor(0), sub2->GetInDataAnchor(0));
  GraphUtils::AddEdge(sub2->GetOutDataAnchor(0), merge2->GetInDataAnchor(1));
  GraphUtils::AddEdge(sub2->GetOutControlAnchor(), active_t2->GetInControlAnchor());
  GraphUtils::AddEdge(active_t2->GetOutControlAnchor(), merge2->GetInControlAnchor());

  GraphUtils::AddEdge(switch_t->GetOutControlAnchor(), less2->GetInControlAnchor());
  GraphUtils::AddEdge(switch_f->GetOutControlAnchor(), relu1->GetInControlAnchor());


  GraphUtils::AddEdge(merge2->GetOutDataAnchor(0), merge1->GetInDataAnchor(0));
  GraphUtils::AddEdge(merge2->GetOutControlAnchor(), active_f1->GetInControlAnchor());
  GraphUtils::AddEdge(active_f1->GetOutControlAnchor(), merge1->GetInControlAnchor());

  GraphUtils::AddEdge(data2->GetOutDataAnchor(0), relu1->GetInDataAnchor(1));
  GraphUtils::AddEdge(relu1->GetOutDataAnchor(0), merge1->GetInDataAnchor(0));
  GraphUtils::AddEdge(relu1->GetOutControlAnchor(), active_t1->GetInControlAnchor());
  GraphUtils::AddEdge(active_t1->GetOutControlAnchor(), merge1->GetInControlAnchor());

  GraphUtils::AddEdge(merge1->GetOutDataAnchor(0), output1->GetInDataAnchor(0));

  AttrUtils::SetBool(merge2->GetOpDesc(), ATTR_NAME_FORCE_UNKNOWN_SHAPE, true);
  EXPECT_EQ(graph->TopologicalSorting(), GRAPH_SUCCESS);

  SetControlFlowGroup(merge2, merge2->GetOpDesc()->GetId());
  SetControlFlowGroup(switch_f2, merge2->GetOpDesc()->GetId());
  SetControlFlowGroup(switch_t2, merge2->GetOpDesc()->GetId());
  SetControlFlowGroup(active2, merge2->GetOpDesc()->GetId());
  SetControlFlowGroup(active_t2, merge2->GetOpDesc()->GetId());
  SetControlFlowGroup(active_f2, merge2->GetOpDesc()->GetId());

  SetControlFlowGroup(merge1, merge1->GetOpDesc()->GetId());
  SetControlFlowGroup(switch_f, merge1->GetOpDesc()->GetId());
  SetControlFlowGroup(switch_t, merge1->GetOpDesc()->GetId());
  SetControlFlowGroup(active1, merge1->GetOpDesc()->GetId());
  SetControlFlowGroup(active_f1, merge1->GetOpDesc()->GetId());
  SetControlFlowGroup(active_t1, merge1->GetOpDesc()->GetId());

  EXPECT_EQ(graph->impl_->sub_graph_.size(), 0);
  DynamicShapePartitioner partitioner(graph);
  EXPECT_EQ(partitioner.Partition(), SUCCESS);
  EXPECT_EQ(graph->impl_->sub_graph_.size(), 3);   // input  less1  uknown
}

TEST_F(UtestDynamicShapePartition, mark_unknown_shape_nodes) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");

  auto data = NodeBuilder("data", DATA).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto where = NodeBuilder("where", WHERE).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto output = NodeBuilder("output", NETOUTPUT).AddInputDesc({1}).Build(graph);

  std::vector<int64_t> known_shape = {2, 5};
  data->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(known_shape));
  where->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(known_shape));
  std::vector<int64_t> unknown_shape = {-1, 2};
  where->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(unknown_shape));
  output->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(unknown_shape));

  GraphUtils::AddEdge(data->GetOutDataAnchor(0), where->GetInDataAnchor(0));
  GraphUtils::AddEdge(where->GetOutDataAnchor(0), output->GetInDataAnchor(0));

  DynamicShapePartitioner partitioner(graph);
  EXPECT_EQ(partitioner.MarkUnknownShapeNodes(), SUCCESS);
  EXPECT_EQ(partitioner.unknown_shape_nodes_.size(), 2);
}

TEST_F(UtestDynamicShapePartition, mark_no_tiling_nodes) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");

  auto data = NodeBuilder("data", DATA).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto where = NodeBuilder("where", WHERE).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto output = NodeBuilder("output", NETOUTPUT).AddInputDesc({1}).Build(graph);

  std::vector<int64_t> known_shape = {2, 5};
  data->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(known_shape));
  where->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(known_shape));
  std::vector<int64_t> unknown_shape = {-1, 2};
  where->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(unknown_shape));
  output->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(unknown_shape));

  GraphUtils::AddEdge(data->GetOutDataAnchor(0), where->GetInDataAnchor(0));
  GraphUtils::AddEdge(where->GetOutDataAnchor(0), output->GetInDataAnchor(0));

  auto where_desc = where->GetOpDesc();
  vector<std::string> tiling_inline;
  vector<std::string> export_shape;
  AttrUtils::GetListStr(where_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
  tiling_inline.push_back("DNN_VM_AICPU");
  AttrUtils::SetListStr(where_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
  AttrUtils::GetListStr(where_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
  export_shape.push_back("DNN_VM_AICPU");
  AttrUtils::SetListStr(where_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
  where_desc->SetOpEngineName("DNN_VM_AICPU");
  AttrUtils::SetStr(where_desc, ATTR_NAME_OP_MAX_SHAPE, "10, 0");

  auto out_desc = output->GetOpDesc();

  DynamicShapePartitioner partitioner(graph);
  EXPECT_EQ(partitioner.MarkUnknownShapeNodes(), SUCCESS);
  EXPECT_EQ(partitioner.unknown_shape_nodes_.size(), 0);
  bool is_no_tiling = false;
  EXPECT_EQ(AttrUtils::GetBool(where_desc, ATTR_NAME_OP_NO_TILING, is_no_tiling), true);
  EXPECT_EQ(is_no_tiling, true);
  EXPECT_EQ(AttrUtils::GetBool(out_desc, ATTR_NAME_OP_NO_TILING, is_no_tiling), true);
  EXPECT_EQ(is_no_tiling, true);
}

TEST_F(UtestDynamicShapePartition, mark_unknown_and_no_tiling_nodes) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");

  auto data = NodeBuilder("data", DATA).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto where = NodeBuilder("where", WHERE).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto where1 = NodeBuilder("where", WHERE).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto output = NodeBuilder("output", NETOUTPUT).AddInputDesc({1}).Build(graph);

  std::vector<int64_t> known_shape = {2, 5};
  std::vector<int64_t> unknown_shape = {-1, 2};
  data->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(known_shape));
  where->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(known_shape));
  where->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(unknown_shape));
  where1->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(unknown_shape));
  where1->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(unknown_shape));
  output->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape(unknown_shape));

  GraphUtils::AddEdge(data->GetOutDataAnchor(0), where->GetInDataAnchor(0));
  GraphUtils::AddEdge(where->GetOutDataAnchor(0), where1->GetInDataAnchor(0));
  GraphUtils::AddEdge(where1->GetOutDataAnchor(0), output->GetInDataAnchor(0));

  auto where_desc = where->GetOpDesc();
  vector<std::string> tiling_inline;
  vector<std::string> export_shape;
  AttrUtils::GetListStr(where_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
  tiling_inline.push_back("DNN_VM_AICPU");
  AttrUtils::SetListStr(where_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
  AttrUtils::GetListStr(where_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
  export_shape.push_back("DNN_VM_AICPU");
  AttrUtils::SetListStr(where_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
  where_desc->SetOpEngineName("DNN_VM_AICPU");
  AttrUtils::SetStr(where_desc, ATTR_NAME_OP_MAX_SHAPE, "10, 0");

  auto where1_desc = where1->GetOpDesc();

  DynamicShapePartitioner partitioner(graph);
  EXPECT_EQ(partitioner.MarkUnknownShapeNodes(), SUCCESS);
  EXPECT_EQ(partitioner.unknown_shape_nodes_.size(), 3);
  bool is_no_tiling = false;
  EXPECT_EQ(AttrUtils::GetBool(where_desc, ATTR_NAME_OP_NO_TILING, is_no_tiling), true);
  EXPECT_EQ(is_no_tiling, false);
}

TEST_F(UtestDynamicShapePartition, test_node_support_no_tiling) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  DynamicShapePartitioner partitioner(graph);

  auto data = NodeBuilder("data", DATA).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto where = NodeBuilder("where", WHERE).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);
  auto output = NodeBuilder("output", NETOUTPUT).AddInputDesc({1}).Build(graph);

  GraphUtils::AddEdge(data->GetOutDataAnchor(0), where->GetInDataAnchor(0));
  GraphUtils::AddEdge(where->GetOutDataAnchor(0), output->GetInDataAnchor(0));

  auto where_desc = where->GetOpDesc();
  where_desc->SetOpEngineName("DNN_VM_AICPU");

  vector<std::string> tiling_inline;
  AttrUtils::GetListStr(where_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
  tiling_inline.push_back("DNN_VM_AICPU");
  AttrUtils::SetListStr(where_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);

  EXPECT_EQ(partitioner.IsNodeSupportNoTiling(where), false);

  vector<std::string> export_shape;
  AttrUtils::GetListStr(where_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
  export_shape.push_back("DNN_VM_AICPU");
  AttrUtils::SetListStr(where_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
  std::vector<int64_t> known_shape = {2, 5};
  where->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(known_shape));
  EXPECT_EQ(partitioner.IsNodeSupportNoTiling(where), true);
  
  std::vector<int64_t> unknown_shape = {-1, 2};
  where->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape(unknown_shape));
  EXPECT_EQ(partitioner.IsNodeSupportNoTiling(where), false);

  std::vector<std::pair<int64_t, int64_t>> range = {{1, -1}, {2, 2}};
  where->GetOpDesc()->MutableOutputDesc(0)->SetShapeRange(range);
  EXPECT_EQ(partitioner.IsNodeSupportNoTiling(where), false);
}

TEST_F(UtestDynamicShapePartition, test_node_support_no_tiling_01) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  DynamicShapePartitioner partitioner(graph);

  auto memcpyasync = NodeBuilder("memcpyasync", MEMCPYASYNC).AddInputDesc({1}).AddOutputDesc({1}).Build(graph);

  EXPECT_EQ(partitioner.IsNodeSupportNoTiling(memcpyasync), true);
}
TEST_F(UtestDynamicShapePartition, special_process_resource_op) {
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
  ComputeGraphPtr graph = ToComputeGraph(g1);
   for (auto &node : graph->GetAllNodes()) {
     if (node->GetName() == "stack" || node->GetName() == "stackpush" || node->GetName() == "stackpop") {
       (void)AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, 1);
     }
     if (node->GetName() == "stack1" || node->GetName() == "stackpush1" || node->GetName() == "stackpop1") {
       (void)AttrUtils::SetInt(node->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, 2);
     }

   }
  AttrUtils::SetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id");
  DynamicShapePartitioner partitioner(graph);
  EXPECT_EQ(partitioner.Partition(), SUCCESS);

  bool forced_unknown = false;
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetName() == "stack1") {
      AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_FORCE_UNKNOWN_SHAPE, forced_unknown);
    }
  }
  EXPECT_EQ(forced_unknown, true);
}
} // namespace ge