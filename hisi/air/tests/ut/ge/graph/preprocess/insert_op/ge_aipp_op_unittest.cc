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

#include "common/ge_inner_error_codes.h"
#include "common/types.h"
#include "common/util.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/passes/graph_builder_utils.h"
#include "framework/omg/omg_inner_types.h"
#include "common/local_context.h"

#define private public
#define protected public
#include "graph/preprocess/insert_op/ge_aipp_op.h"
#undef private
#undef protected

using namespace std;
namespace ge {
class UtestGeAipp : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};
class NodeBuilder {
 public:
  NodeBuilder(const std::string &name, const std::string &type) { op_desc_ = std::make_shared<OpDesc>(name, type); }

  NodeBuilder &AddInputDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                            ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddInputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  NodeBuilder &AddOutputDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                             ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddOutputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  ge::NodePtr Build(const ge::ComputeGraphPtr &graph) { return graph->AddNode(op_desc_); }

 private:
  ge::GeTensorDescPtr CreateTensorDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                                       ge::DataType data_type = DT_FLOAT) {
    GeShape ge_shape{std::vector<int64_t>(shape)};
    ge::GeTensorDescPtr tensor_desc = std::make_shared<ge::GeTensorDesc>();
    tensor_desc->SetShape(ge_shape);
    tensor_desc->SetFormat(format);
    tensor_desc->SetDataType(data_type);
    return tensor_desc;
  }

  ge::OpDescPtr op_desc_;
};

ComputeGraphPtr BuildGraph() {
  auto builder = ut::GraphBuilder("mbatch_Case");

  auto data1 = builder.AddNode("data1", DATA, 1, 1);
  auto data_desc = data1->GetOpDesc();
  AttrUtils::SetStr(data_desc, ATTR_ATC_USER_DEFINE_DATATYPE, "DT_FLOAT16");
  AttrUtils::SetStr(data_desc, "mbatch-switch-name", "case1");
  AttrUtils::SetInt(data_desc, ATTR_NAME_INDEX, 0);

  auto mapindex1 = builder.AddNode("mapindex1", "MapIndex", 0, 1);
  auto case1 = builder.AddNode("case1", CASE, 2, 1);
  auto netoutput1 = builder.AddNode("netoutput1", NETOUTPUT, 1, 0);

  builder.AddDataEdge(mapindex1, 0, case1, 0);
  builder.AddDataEdge(data1, 0, case1, 1);
  builder.AddDataEdge(case1, 0, netoutput1, 0);

  return builder.GetGraph();
}

TEST_F(UtestGeAipp, test_Cinvert_related_input_name_to_rank) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  domi::AippOpParams params;
  params.set_related_input_name("data0");
  AippOp aipp_op;
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op.ConvertRelatedInputNameToRank();
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(aipp_op.aipp_params_->related_input_rank(), 0);

  domi::AippOpParams params1;
  params1.set_related_input_name("data1");
  AippOp aipp_op1;
  auto ret1 = aipp_op1.Init(&params1);
  ASSERT_EQ(ret1, SUCCESS);
  ret1 = aipp_op1.ConvertRelatedInputNameToRank();
  EXPECT_EQ(ret1, PARAM_INVALID);
  EXPECT_EQ(aipp_op1.aipp_params_->related_input_rank(), 0);
  // Reset global context
  domi::GetContext().data_tensor_names = data_tensor_names_old;

  NodePtr aipp_node = nullptr;
  ret = aipp_op.AddNodeToGraph(aipp_node, 0);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGeAipp, test_Insert_Aipp_ToGraph) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  const ge::NodePtr data1 = NodeBuilder("data1", DATA)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .Build(graph);
  AippOp aipp_op;
  domi::AippOpParams params;
  std::string aippConfigPath;
  const uint32_t index = 1;
  params.set_related_input_name("data1");
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  aipp_op.InsertAippToGraph(graph, aippConfigPath, index);

  AippOp aipp_op1;
  domi::AippOpParams params1;
  std::string aippConfigPath1;
  const uint32_t index1 = 1;
  params1.set_related_input_name("data0");
  ret = aipp_op.Init(&params1);
  ASSERT_EQ(ret, SUCCESS);
  aipp_op1.InsertAippToGraph(graph, aippConfigPath1, index1);

  // Reset global context
  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Validate_Params) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  AippOp aipp_op;
  domi::AippOpParams params;
  params.set_aipp_mode(domi::AippOpParams::undefined);
  auto ret = aipp_op.ValidateParams();
  ASSERT_EQ(ret, PARAM_INVALID);

  AippOp aipp_op1;
  params.set_aipp_mode(domi::AippOpParams::dynamic);
  ret = aipp_op1.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op1.ValidateParams();
  ASSERT_EQ(ret, PARAM_INVALID);

  AippOp aipp_op2;
  params.set_aipp_mode(domi::AippOpParams::static_);
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  ret = aipp_op2.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op2.ValidateParams();
  ASSERT_EQ(ret, SUCCESS);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Set_Csc_Default_Value) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  AippOp aipp_op;
  domi::AippOpParams params;
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  aipp_op.SetCscDefaultValue();

  AippOp aipp_op1;
  params.set_input_format(domi::AippOpParams::XRGB8888_U8);
  ret = aipp_op1.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  aipp_op1.SetCscDefaultValue();

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Set_Dtc_Default_Value) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  AippOp aipp_op;
  domi::AippOpParams params;
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  aipp_op.SetDtcDefaultValue();

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Convert_Param_To_Attr) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  AippOp aipp_op;
  domi::AippOpParams params;
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  params.set_aipp_mode(domi::AippOpParams::static_);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ge::NamedAttrs aipp_attrs;
  aipp_op.ConvertParamToAttr(aipp_attrs);

  AippOp aipp_op1;
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  params.set_aipp_mode(domi::AippOpParams::dynamic);
  ret = aipp_op1.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  aipp_op1.ConvertParamToAttr(aipp_attrs);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Set_Default_Params) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  AippOp aipp_op;
  domi::AippOpParams params;
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  params.set_aipp_mode(domi::AippOpParams::static_);
  params.set_csc_switch(true);
  params.set_crop(false);
  params.set_resize(false);
  params.set_padding(false);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ge::NamedAttrs aipp_attrs;
  aipp_op.ConvertParamToAttr(aipp_attrs);
  aipp_op.SetDefaultParams();
  ASSERT_EQ(ret, SUCCESS);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Generate_OpDesc) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  AippOp aipp_op;
  domi::AippOpParams params;
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  OpDescPtr desc_ptr = std::make_shared<OpDesc>("name1", "type1");
  aipp_op.GenerateOpDesc(desc_ptr);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Add_Attr_To_AippData) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  AippOp aipp_op;
  domi::AippOpParams params;
  params.set_input_format(domi::AippOpParams::YUV420SP_U8);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  OpDescPtr desc_ptr = std::make_shared<OpDesc>("name1", "type1");
  aipp_op.AddAttrToAippData(desc_ptr);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Find_Data_By_Index) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  const ge::NodePtr data1 = NodeBuilder("data1", DATA)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .Build(graph);
  AippOp aipp_op;
  domi::AippOpParams params;
  std::string aippConfigPath;
  const uint32_t index = 1;
  params.set_related_input_name("data1");
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op.InsertAippToGraph(graph, aippConfigPath, index);
  ge::NodePtr data2 = nullptr;
  data2 = aipp_op.FindDataByIndex(graph, 0);
  if (data2 != nullptr) {
      ret = SUCCESS;
      ASSERT_EQ(ret, SUCCESS);
  }

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Add_Aipp_Attrbutes) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  const ge::NodePtr data1 = NodeBuilder("data1", DATA)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .Build(graph);
  AippOp aipp_op;
  domi::AippOpParams params;
  std::string aippConfigPath;
  const uint32_t index = 0;
  params.set_related_input_name("data1");
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  OpDescPtr desc_ptr = std::make_shared<OpDesc>("name1", "type1");
  const std::string aipp_cfg_path = "/root/";
  ret = aipp_op.AddAippAttrbutes(desc_ptr, aippConfigPath, index);
  ASSERT_EQ(ret, SUCCESS);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Get_Target_Position) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::NodePtr data1 = NodeBuilder("data1", DATA)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .Build(graph);
  AippOp aipp_op;
  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_related_input_name("data1");
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op.GetTargetPosition(graph, data1, target_edges);
  ASSERT_EQ(ret, SUCCESS);

  ge::NodePtr data2 = NodeBuilder("case1", CASE)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .Build(graph);
  AippOp aipp_op1;
  domi::AippOpParams params1;
  params1.set_related_input_name("data1");
  ret = aipp_op1.Init(&params1);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op1.GetTargetPosition(graph, data2, target_edges);
  ASSERT_EQ(ret, SUCCESS);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Get_Static_TargetNode) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::NodePtr data1 = NodeBuilder("data1", DATA)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .Build(graph);
  ge::NodePtr data2 = NodeBuilder("case1", CASE)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .Build(graph);
  AippOp aipp_op;
  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::static_);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op.GetStaticTargetNode(data1, data2);
  ASSERT_EQ(ret, SUCCESS);

  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Insert_Aipp_ToGraph1) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  const ge::NodePtr data1 = NodeBuilder("data1", DATA)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                          .Build(graph);
  AippOp aipp_op;
  domi::AippOpParams params;
  std::string aippConfigPath;
  const uint32_t index = 1;
  params.set_related_input_name("data0");
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  ret = aipp_op.ConvertRelatedInputNameToRank();
  EXPECT_EQ(ret, SUCCESS);
  aipp_op.InsertAippToGraph(graph, aippConfigPath, index);
  // Reset global context
  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_Create_Aipp) {
  // Set global context
  auto data_tensor_names_old = domi::GetContext().data_tensor_names;
  domi::GetContext().data_tensor_names.push_back("data0");

  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::NodePtr aipp1 = NodeBuilder("aipp1", AIPP)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .AddOutputDesc({1}, FORMAT_NCHW, DT_FLOAT)
                    .Build(graph);

  AippOp aipp_op;
  domi::AippOpParams params;
  std::string aippConfigPath;
  uint32_t index;
  OutDataAnchorPtr outptr = std::make_shared<OutDataAnchor>(aipp1, 0);
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::static_);
  auto ret = aipp_op.Init(&params);
  ASSERT_EQ(ret, SUCCESS);
  aipp_op.CreateAipp(outptr, aippConfigPath, index);
  domi::GetContext().data_tensor_names = data_tensor_names_old;
}

TEST_F(UtestGeAipp, test_CreateAippData5) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{224,224,3}), FORMAT_NHWC, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{224,224,3}), FORMAT_ND, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NHWC, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_ND, DT_INT64));
  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::static_);
  auto ret = aipp_op.Init(&params);
  int32_t rank = 0;
  NodePtr target;
  std::set<uint32_t> edge_indexes;
  aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes);
  EXPECT_EQ(aipp_op.CreateAippData(aipp), PARAM_INVALID);
}

TEST_F(UtestGeAipp, test_CreateAippData1) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  GetLocalOmgContext().format = domi::DOMI_TENSOR_NHWC;
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{224,224,3}), FORMAT_NHWC, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{224,224,3}), FORMAT_ND, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NHWC, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_ND, DT_INT64));
  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::static_);
  auto ret = aipp_op.Init(&params);
  int32_t rank = 0;
  NodePtr target;
  std::set<uint32_t> edge_indexes;
  aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes);
  EXPECT_EQ(aipp_op.CreateAippData(aipp), INTERNAL_ERROR);
}

TEST_F(UtestGeAipp, test_CreateAippData2) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{224}), FORMAT_ND, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{224}), FORMAT_ND, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_ND, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_ND, DT_INT64));
  std::vector<int64_t> origin_input_dims = {224};
  AttrUtils::SetListInt(data1->GetOpDesc(), ATTR_MBATCH_ORIGIN_INPUT_DIMS, origin_input_dims);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  GetLocalOmgContext().format = domi::DOMI_TENSOR_NCHW;

  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::dynamic);
  auto ret = aipp_op.Init(&params);
  int32_t rank = 0;
  NodePtr target;
  std::set<uint32_t> edge_indexes;
  aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes);
  EXPECT_EQ(aipp_op.CreateAippData(aipp), PARAM_INVALID);
}

TEST_F(UtestGeAipp, test_CreateAippData3) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{-1,3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{-1,3,224,224}), FORMAT_ND, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_ND, DT_INT64));
  std::vector<int64_t> origin_input_dims = {-1,3,224,224};
  AttrUtils::SetListInt(data1->GetOpDesc(), ATTR_MBATCH_ORIGIN_INPUT_DIMS, origin_input_dims);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  GetLocalOmgContext().format = domi::DOMI_TENSOR_NCHW;

  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::static_);
  auto ret = aipp_op.Init(&params);
  int32_t rank = 0;
  NodePtr target;
  std::set<uint32_t> edge_indexes;
  aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes);
  EXPECT_EQ(aipp_op.CreateAippData(aipp), PARAM_INVALID);
}

TEST_F(UtestGeAipp, test_CreateAippData4) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  GetLocalOmgContext().format = domi::DOMI_TENSOR_NCHW;
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2,3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2,3,224,224}), FORMAT_ND, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_ND, DT_INT64));
  std::vector<int64_t> origin_input_dims = {2,3,224,224};
  AttrUtils::SetListInt(data1->GetOpDesc(), ATTR_MBATCH_ORIGIN_INPUT_DIMS, origin_input_dims);

  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::static_);
  auto ret = aipp_op.Init(&params);
  int32_t rank = 0;
  NodePtr target;
  std::set<uint32_t> edge_indexes;
  aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes);
  EXPECT_EQ(aipp_op.CreateAippData(aipp), INTERNAL_ERROR);
}

TEST_F(UtestGeAipp, test_GetStaticTargetNode1) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));
  std::vector<int64_t> origin_input_dims = {3,224,224};
  AttrUtils::SetStr(data1->GetOpDesc(), "mbatch-switch-name", "");

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  GetLocalOmgContext().format = domi::DOMI_TENSOR_NCHW;

  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::static_);
  EXPECT_EQ(aipp_op.Init(&params), SUCCESS);
  NodePtr target;
  EXPECT_EQ(aipp_op.GetStaticTargetNode(data1, target), SUCCESS);
}

TEST_F(UtestGeAipp, test_GetAndCheckTarget1) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Const", 1, 1);

  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  int32_t rank = 0;
  NodePtr target;
  std::set<uint32_t> edge_indexes;
  EXPECT_EQ(aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes), PARAM_INVALID);
}

TEST_F(UtestGeAipp, test_GetAndCheckTarget2) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));
  AttrUtils::SetBool(data1->GetOpDesc(), "mbatch-inserted-node", false);
  AttrUtils::SetBool(data2->GetOpDesc(), "mbatch-inserted-node", true);
  std::string test = "test";
  AttrUtils::SetStr(data1->GetOpDesc(), "_user_defined_data_type", test);
  AttrUtils::SetStr(data2->GetOpDesc(), "_user_defined_data_type", test);

  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  GetLocalOmgContext().format = domi::DOMI_TENSOR_NCHW;

  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::dynamic);
  auto ret = aipp_op.Init(&params);
  NodePtr target;
  int32_t rank = 0;
  std::set<uint32_t> edge_indexes;
  EXPECT_EQ(aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes), PARAM_INVALID);
}

TEST_F(UtestGeAipp, test_GetAndCheckTarget3) {
  ge::ut::GraphBuilder builder("graph");
  auto data1 = builder.AddNode("data1", "Data", 1, 1);
  auto data2 = builder.AddNode("data2", "Data", 1, 1);
  auto aipp = builder.AddNode("aipp", "aipp", 2, 1);
  auto netoutput = builder.AddNode("Node_Output", "NetOutput", 1, 0);

  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddInputDesc(ge::GeTensorDesc());
  aipp->GetOpDesc()->AddOutputDesc(ge::GeTensorDesc());
  data1->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data1->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{3,224,224}), FORMAT_NCHW, DT_FLOAT));
  data2->GetOpDesc()->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));
  data2->GetOpDesc()->UpdateOutputDesc(0, GeTensorDesc(GeShape(std::vector<int64_t>{2}), FORMAT_NCHW, DT_INT64));


  builder.AddDataEdge(data1, 0, aipp, 0);
  builder.AddDataEdge(data2, 0, aipp, 1);
  builder.AddDataEdge(aipp, 0, netoutput, 0);
  ComputeGraphPtr computeGraph = builder.GetGraph();
  AippOp aipp_op;
  GetLocalOmgContext().format = domi::DOMI_TENSOR_NCHW;

  domi::AippOpParams params;
  std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> target_edges;
  params.set_aipp_mode(domi::AippOpParams::dynamic);
  auto ret = aipp_op.Init(&params);
  NodePtr target;
  int32_t rank = 0;
  std::set<uint32_t> edge_indexes;
  EXPECT_EQ(aipp_op.GetAndCheckTarget(computeGraph, rank, target, edge_indexes), SUCCESS);
}

}