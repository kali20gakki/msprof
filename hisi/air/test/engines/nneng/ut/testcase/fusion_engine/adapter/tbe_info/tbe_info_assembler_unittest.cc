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
#include "graph/node.h"
#include "graph/op_desc.h"
#include <string>
#define protected public
#define private public
#include "common/configuration.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "ge/ge_api_types.h"
#include "common/util/op_info_util.h"
#include "ops_store/op_kernel_info.h"
#include "common/op_info_common.h"
#include "common/fe_inner_attr_define.h"
#include "adapter/tbe_adapter/tbe_info/tbe_info_assembler.h"
#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;
using namespace te;

class UTEST_TbeInfoAssembler : public testing::Test
{
protected:
  static NodePtr CreateConstDependNode(ComputeGraphPtr graph, ge::DataType dType) {
    vector<int64_t> dim = {1,2,3,4};
    ge::GeShape shape(dim);
    ge::GeTensorDesc tensorDesc(shape, ge::FORMAT_NCHW, dType);
    OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr const_desc = std::make_shared<OpDesc>("const", "Const");

    op_desc->AddInputDesc("x", tensorDesc);
    op_desc->AddInputDesc("y", tensorDesc);
    op_desc->AddOutputDesc("z", tensorDesc);
    const_desc->AddOutputDesc("z", tensorDesc);

    ge::GeTensorPtr constTensor = std::make_shared<ge::GeTensor>(tensorDesc);
    if (dType == ge::DT_INT32) {
      vector<int32_t> data_value(24, 4);
      constTensor->SetData(reinterpret_cast<uint8_t *>(data_value.data()), data_value.size() * sizeof(int32_t));
    } else if (dType == ge::DT_FLOAT16) {
      vector<int16_t> data_value(24, 4);
      constTensor->SetData(reinterpret_cast<uint8_t *>(data_value.data()), data_value.size() * sizeof(int16_t));
    } else {
      vector<float> data_value(24, 4.3);
      constTensor->SetData(reinterpret_cast<uint8_t *>(data_value.data()), data_value.size() * sizeof(float));
    }
    OpDescUtils::SetWeights(const_desc, constTensor);

    NodePtr relu_node = graph->AddNode(op_desc);
    NodePtr const_node = graph->AddNode(const_desc);
    ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(1));
    return relu_node;
  }
};

TEST_F(UTEST_TbeInfoAssembler, Test_JudgeShapeToSetFlag){
  std::string op_name    = "conv";
  std::string op_module   = "";
  std::string op_type     = "tbe";
  std::string core_type   = "AIcoreEngine";

  vector<int64_t> dim_data = {1024, 2, 1024, 1024};
  GeShape shape_data(dim_data);
  GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_INT2);
  vector<int64_t> dim_weight = {1024, 2, 1024, 1024};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);

  vector<int64_t> dim_out = {1024, 2, 1024, 2};
  GeShape shape_out(dim_out);
  GeTensorDesc OutDesc(shape_out);

  OpDescPtr conv_op = std::make_shared<OpDesc>("Conv", "conv");
  conv_op->AddInputDesc("x", data_desc);
  conv_op->AddInputDesc("w1", weight_desc);
  conv_op->AddOutputDesc("y", OutDesc);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node_ptr = graph->AddNode(conv_op);
  Node* node = node_ptr.get();

  TbeInfoAssembler test_JudgeShapeToSetFlag;
  TbeOpInfo op_info(op_name, op_module, op_type, "", core_type);
  bool flag = false;
  auto op_desc_ptr = node->GetOpDesc();
  Status ret = test_JudgeShapeToSetFlag.JudgeShapeToSetFlag(op_desc_ptr, op_info, flag, "in");
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(UTEST_TbeInfoAssembler, UpdateOpPrecisionMode_1){
  map<std::string, std::string> options;
  options.emplace(ge::OP_SELECT_IMPL_MODE, "high_precision");
  options.emplace(ge::OPTYPELIST_FOR_IMPLMODE, "A,B,C");
  std::string op_precision_mode_str;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.GetOpPrecisionMode(options, op_precision_mode_str);
  EXPECT_EQ(op_precision_mode_str, "A:high_precision,B:high_precision,C:high_precision");
}

TEST_F(UTEST_TbeInfoAssembler, UpdateOpPrecisionMode_2){
  map<std::string, std::string> options;
  options.emplace(ge::OP_SELECT_IMPL_MODE, "high_performance");
  options.emplace(ge::OPTYPELIST_FOR_IMPLMODE, "A,B,C");
  std::string op_precision_mode_str;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.GetOpPrecisionMode(options, op_precision_mode_str);
  EXPECT_EQ(op_precision_mode_str, "A:high_performance,B:high_performance,C:high_performance");
}

TEST_F(UTEST_TbeInfoAssembler, UpdateOpPrecisionMode_3){
  map<std::string, std::string> options;
  options.emplace(ge::OP_SELECT_IMPL_MODE, "high_performance");
  std::string op_precision_mode_str;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.GetOpPrecisionMode(options, op_precision_mode_str);
  EXPECT_EQ(op_precision_mode_str.empty(), true);
}

TEST_F(UTEST_TbeInfoAssembler, UpdateOpPrecisionMode_4){
  map<std::string, std::string> options;
  options.emplace(ge::OP_SELECT_IMPL_MODE, "");
  options.emplace(ge::OPTYPELIST_FOR_IMPLMODE, "A,B,C");
  std::string op_precision_mode_str;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.GetOpPrecisionMode(options, op_precision_mode_str);
  EXPECT_EQ(op_precision_mode_str.empty(), true);
}

TEST_F(UTEST_TbeInfoAssembler, UpdateOpPrecisionMode_5){
  map<std::string, std::string> options;
  options.emplace(ge::OP_SELECT_IMPL_MODE, "high_performance_for_all");
  options.emplace(ge::OPTYPELIST_FOR_IMPLMODE, "A,B,C");
  std::string op_precision_mode_str;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.GetOpPrecisionMode(options, op_precision_mode_str);
  EXPECT_EQ(op_precision_mode_str.empty(), true);
}

TEST_F(UTEST_TbeInfoAssembler, get_options_01) {
  TbeInfoAssembler tbe_info_assembler;
  putenv("NPU_COLLECT_PATH=x");
  auto options = tbe_info_assembler.GetAllOptionsForTBE("AiCoreEngine");
  EXPECT_EQ(options[ge::OP_DEBUG_LEVEL], "2");

  putenv("NPU_COLLECT_PATH=");
  options = tbe_info_assembler.GetAllOptionsForTBE("AiCoreEngine");
  EXPECT_NE(options[ge::OP_DEBUG_LEVEL], "2");
}

TEST_F(UTEST_TbeInfoAssembler, set_tensor_constvalue_1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = CreateConstDependNode(graph, ge::DT_FLOAT);
  InputOrOutputInfoPtr tensor_info_ptr = std::make_shared<InputOrOutputInfo>("input");
  tensor_info_ptr->op_const_value_depend_ = CONST_REQUIRED;
  std::vector<int64_t> shape = {1,2,3,4};
  te::TbeOpTensor op_tensor("x", shape, "float", "NCHW");
  TbeInfoAssembler tbe_info_assembler;
  Status ret = tbe_info_assembler.SetTensorConstValue(node.get(), 0, tensor_info_ptr, op_tensor);
  EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_TbeInfoAssembler, set_tensor_constvalue_2) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = CreateConstDependNode(graph, ge::DT_FLOAT);
  InputOrOutputInfoPtr tensor_info_ptr = std::make_shared<InputOrOutputInfo>("input");
  tensor_info_ptr->op_const_value_depend_ = CONST_OPTIONAL;
  std::vector<int64_t> shape = {1,2,3,4};
  te::TbeOpTensor op_tensor("x", shape, "float", "NCHW");
  TbeInfoAssembler tbe_info_assembler;
  Status ret = tbe_info_assembler.SetTensorConstValue(node.get(), 0, tensor_info_ptr, op_tensor);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_TbeInfoAssembler, set_tensor_constvalue_3) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = CreateConstDependNode(graph, ge::DT_FLOAT);
  InputOrOutputInfoPtr tensor_info_ptr = std::make_shared<InputOrOutputInfo>("input");
  tensor_info_ptr->op_const_value_depend_ = CONST_REQUIRED;
  std::vector<int64_t> shape = {1,2,3,4};
  te::TbeOpTensor op_tensor("x", shape, "float", "NCHW");
  TbeInfoAssembler tbe_info_assembler;
  Status ret = tbe_info_assembler.SetTensorConstValue(node.get(), 1, tensor_info_ptr, op_tensor);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_TbeInfoAssembler, set_tensor_constvalue_4) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = CreateConstDependNode(graph, ge::DT_FLOAT16);
  InputOrOutputInfoPtr tensor_info_ptr = std::make_shared<InputOrOutputInfo>("input");
  tensor_info_ptr->op_const_value_depend_ = CONST_REQUIRED;
  std::vector<int64_t> shape = {1,2,3,4};
  te::TbeOpTensor op_tensor("x", shape, "float16", "NCHW");
  TbeInfoAssembler tbe_info_assembler;
  Status ret = tbe_info_assembler.SetTensorConstValue(node.get(), 1, tensor_info_ptr, op_tensor);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_TbeInfoAssembler, set_tensor_constvalue_5) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = CreateConstDependNode(graph, ge::DT_INT32);
  InputOrOutputInfoPtr tensor_info_ptr = std::make_shared<InputOrOutputInfo>("input");
  tensor_info_ptr->op_const_value_depend_ = CONST_REQUIRED;
  std::vector<int64_t> shape = {1,2,3,4};
  te::TbeOpTensor op_tensor("x", shape, "int32", "NCHW");
  TbeInfoAssembler tbe_info_assembler;
  Status ret = tbe_info_assembler.SetTensorConstValue(node.get(), 1, tensor_info_ptr, op_tensor);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_TbeInfoAssembler, set_tensor_constvalue_6) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  ge::GeTensorDesc tensorDesc(shape, ge::FORMAT_NCHW, ge::DT_INT64);
  op_desc->AddInputDesc(tensorDesc);
  op_desc->AddOutputDesc(tensorDesc);

  ge::GeTensorPtr constTensor = std::make_shared<ge::GeTensor>(tensorDesc);
  vector<int64_t> data_value(24, 64);
  constTensor->SetData(reinterpret_cast<uint8_t *>(data_value.data()), data_value.size() * sizeof(int64_t));
  ge::AttrUtils::SetTensor(op_desc->MutableInputDesc(0), ge::ATTR_NAME_VALUE, constTensor);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  InputOrOutputInfoPtr tensor_info_ptr = std::make_shared<InputOrOutputInfo>("input");
  tensor_info_ptr->op_const_value_depend_ = CONST_REQUIRED;
  std::vector<int64_t> dim_vec = {1,2,3,4};
  te::TbeOpTensor op_tensor("x", dim_vec, "int64", "NCHW");
  TbeInfoAssembler tbe_info_assembler;
  Status ret = tbe_info_assembler.SetTensorConstValue(node.get(), 0, tensor_info_ptr, op_tensor);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_TbeInfoAssembler, feed_l1_input_tensor1) {
  ToOpStructPtr l1_info = std::make_shared<ToOpStruct>();
  l1_info->slice_output_shape = {{2,2,2,2}};
  l1_info->slice_input_offset = {{2,2,2,2}};
  l1_info->slice_output_offset = {{2,2,2,2}};
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  op_desc->AddInputDesc(out_desc);
  op_desc->AddOutputDesc(out_desc);

  IndexNameMap input_idx_name_map;
  uint32_t index_in_opdesc;
  te::TbeOpTensor input_tensor;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.FeedL1InputTensor(l1_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_l1_input_tensor2) {
  ToOpStructPtr l1_info = std::make_shared<ToOpStruct>();
  l1_info->slice_input_shape = {{2,2,2,2}};
  l1_info->slice_output_shape = {{2,2,2,2}};
  l1_info->slice_output_offset = {{2,2,2,2}};
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  op_desc->AddInputDesc(out_desc);
  op_desc->AddOutputDesc(out_desc);
  IndexNameMap input_idx_name_map;
  uint32_t index_in_opdesc;
  te::TbeOpTensor input_tensor;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.FeedL1InputTensor(l1_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_l1_input_tensor3) {
  ToOpStructPtr l1_info = std::make_shared<ToOpStruct>();
  l1_info->slice_input_shape = {{2,2,2,2}};
  l1_info->slice_output_shape = {{2,2,2,2}};
  l1_info->slice_input_offset = {{2,2,2,2}};
  l1_info->slice_output_offset = {{2,2,2,2}};
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  op_desc->AddInputDesc(out_desc);
  op_desc->AddOutputDesc(out_desc);
  vector<int64_t> inputOffset = {1};
  op_desc->SetInputOffset(inputOffset);
  IndexNameMap input_idx_name_map;
  uint32_t index_in_opdesc = 1;
  te::TbeOpTensor input_tensor;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.FeedL1InputTensor(l1_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_l2_input_tensor1) {
  ToOpStructPtr l2_info = std::make_shared<ToOpStruct>();
  l2_info->slice_output_shape = {{2,2,2,2}};
  l2_info->slice_input_offset = {{2,2,2,2}};
  l2_info->slice_output_offset = {{2,2,2,2}};
  ge::OpDescPtr op_desc;
  IndexNameMap input_idx_name_map;
  uint32_t index_in_opdesc;
  te::TbeOpTensor input_tensor;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.FeedL2InputTensor(l2_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_l2_input_tensor2) {
 ToOpStructPtr l2_info = std::make_shared<ToOpStruct>();
  l2_info->slice_input_shape = {{2,2,2,2}};
  l2_info->slice_output_shape = {{2,2,2,2}};
  l2_info->slice_output_offset = {{2,2,2,2}};
  ge::OpDescPtr op_desc;
  IndexNameMap input_idx_name_map;
  uint32_t index_in_opdesc;
  te::TbeOpTensor input_tensor;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.FeedL2InputTensor(l2_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_l2_input_tensor3) {
  ToOpStructPtr l2_info = std::make_shared<ToOpStruct>();
  l2_info->slice_input_shape = {{2,2,2,2}};
  l2_info->slice_output_shape = {{2,2,2,2}};
  l2_info->slice_input_offset = {{2,2,2,2}};
  l2_info->slice_output_offset = {{2,2,2,2}};
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  op_desc->AddInputDesc(out_desc);
  op_desc->AddOutputDesc(out_desc);
  vector<int64_t> inputOffset = {1};
  op_desc->SetInputOffset(inputOffset);
  uint32_t index_in_opdesc = 2;
  IndexNameMap input_idx_name_map;
  te::TbeOpTensor input_tensor;
  TbeInfoAssembler tbe_info_assembler;
  tbe_info_assembler.FeedL2InputTensor(l2_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_fusion_output_tensor1) {
  ToOpStructPtr fusion_info = std::make_shared<ToOpStruct>();
  fusion_info->slice_input_shape = {{2,2,2,2}};
  fusion_info->slice_input_offset = {{2,2,2,2}};
  fusion_info->slice_output_offset = {{2,2,2,2}};
  uint32_t index_in_opdesc;
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  op_desc->AddInputDesc(out_desc);
  op_desc->AddOutputDesc(out_desc);
  vector<int64_t> inputOffset = {1};
  op_desc->SetInputOffset(inputOffset);
  IndexNameMap output_idx_name_map;
  TbeInfoAssembler tbe_info_assembler;
  te::TbeOpTensor output_tensor;
  tbe_info_assembler.FeedFusionOutputTensor(fusion_info, op_desc, output_idx_name_map, index_in_opdesc, output_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_fusion_output_tensor2) {
  ToOpStructPtr fusion_info = std::make_shared<ToOpStruct>();
  fusion_info->slice_input_shape = {{2,2,2,2}};
  fusion_info->slice_output_shape = {{2,2,2,2}};
  fusion_info->slice_input_offset = {{2,2,2,2}};
  uint32_t index_in_opdesc;
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  op_desc->AddInputDesc(out_desc);
  op_desc->AddOutputDesc(out_desc);
  vector<int64_t> inputOffset = {1};
  op_desc->SetInputOffset(inputOffset);
  IndexNameMap output_idx_name_map;
  TbeInfoAssembler tbe_info_assembler;
  te::TbeOpTensor output_tensor;
  tbe_info_assembler.FeedFusionOutputTensor(fusion_info, op_desc, output_idx_name_map, index_in_opdesc, output_tensor);
}

TEST_F(UTEST_TbeInfoAssembler, feed_fusion_output_tensor3) {
  ToOpStructPtr fusion_info = std::make_shared<ToOpStruct>();
  fusion_info->slice_input_shape = {{2,2,2,2}};
  fusion_info->slice_output_shape = {{2,2,2,2}};
  fusion_info->slice_input_offset = {{2,2,2,2}};
  fusion_info->slice_output_offset = {{2,2,2,2}};
  uint32_t index_in_opdesc = 1;
  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {1,2,3,4};
  ge::GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  op_desc->AddInputDesc(out_desc);
  op_desc->AddOutputDesc(out_desc);
  vector<int64_t> outputOffset = {1};
  op_desc->SetOutputOffset(outputOffset);
  IndexNameMap output_idx_name_map;
  TbeInfoAssembler tbe_info_assembler;
  te::TbeOpTensor output_tensor;
  tbe_info_assembler.FeedFusionOutputTensor(fusion_info, op_desc, output_idx_name_map, index_in_opdesc, output_tensor);
}

