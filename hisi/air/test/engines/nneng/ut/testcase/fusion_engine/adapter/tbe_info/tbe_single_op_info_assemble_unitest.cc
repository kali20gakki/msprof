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
#include "common/util/op_info_util.h"
#include "ops_store/op_kernel_info.h"
#include "common/op_info_common.h"
#include "adapter/tbe_adapter/tbe_info/tbe_single_op_info_assembler.h"
#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;
using namespace te;

class UTEST_TbeSingleOpInfoAssembler : public testing::Test
{
};

TEST_F(UTEST_TbeSingleOpInfoAssembler, Test_Feed_Flag_Int64_To_Tbe_Op_Info){
  std::string op_name    = "conv";
  std::string op_module   = "";
  std::string op_type     = "tbe";
  std::string core_type   = "AIcoreEngine";

  vector<int64_t> dim_data = {1024, 3, 1024, 1024};
  GeShape shape_data(dim_data);
  GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_FLOAT);
  vector<int64_t> dim_weight = {1024, 3, 1024, 1024};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);

  vector<int64_t> dim_out = {1024, 3, 1024, 3};
  GeShape shape_out(dim_out);
  GeTensorDesc OutDesc(shape_out);

  OpDescPtr conv_op = std::make_shared<OpDesc>("Conv", "conv");
  conv_op->AddInputDesc("x", data_desc);
  conv_op->AddInputDesc("w1", weight_desc);
  conv_op->AddOutputDesc("y", OutDesc);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node_ptr = graph->AddNode(conv_op);
  Node* node = node_ptr.get();

  TbeSingleOpInfoAssembler Feed_Flag_Int64_To_Tbe_Op_Info;
  TbeOpInfo op_info(op_name, op_module, op_type, "", core_type);
  Status ret = Feed_Flag_Int64_To_Tbe_Op_Info.FeedFlagInt64ToTbeOpInfo(node, op_info);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(UTEST_TbeSingleOpInfoAssembler, Test_Parse_Input_Info_From_Op_1){
  std::string op_name    = "conv";
  std::string op_module   = "";
  std::string op_type     = "tbe";
  std::string core_type   = "AIcoreEngine";

  vector<int64_t> dim_data = {1024, 3, 1024, 1024};
  GeShape shape_data(dim_data);
  GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_INT8);
  vector<int64_t> dim_weight = {1024, 3, 1024, 1024};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);

  vector<int64_t> dim_out = {1024, 3, 1024, 3};
  GeShape shape_out(dim_out);
  GeTensorDesc OutDesc(shape_out);

  OpDescPtr conv_op = std::make_shared<OpDesc>("Conv", "conv");
  conv_op->AddInputDesc("x", data_desc);
  conv_op->AddInputDesc("w1", weight_desc);
  conv_op->AddOutputDesc("y", OutDesc);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node_ptr = graph->AddNode(conv_op);
  Node* node = node_ptr.get();

  TbeSingleOpInfoAssembler test_JudgeShapeToSetFlag;
  TbeOpInfo op_info(op_name, op_module, op_type, "", core_type);
  bool flag = false;
  auto op_desc_ptr = node->GetOpDesc();
  ge::AttrUtils::SetStr(op_desc_ptr, ge::ATTR_NAME_UNREGST_OPPATH, "../../abs.py");
  Status ret = test_JudgeShapeToSetFlag.AssembleSingleTbeInfo(node, op_info, core_type);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(UTEST_TbeSingleOpInfoAssembler, Test_Parse_Input_Info_From_Op_2){
  std::string op_name    = "conv";
  std::string op_module   = "";
  std::string op_type     = "tbe";
  std::string core_type   = "AIcoreEngine";

  vector<int64_t> dim_data = {1024, 3, 1024, 1024};
  GeShape shape_data(dim_data);
  GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_INT8);

  vector<int64_t> dim_out = {1024, 3, 1024, 3};
  GeShape shape_out(dim_out);
  GeTensorDesc OutDesc(shape_out, FORMAT_NCHW, DT_INT8);

  OpDescPtr conv_op = std::make_shared<OpDesc>("Conv", "conv");
  conv_op->AddInputDesc("x", data_desc);
  conv_op->AddOutputDesc("q", OutDesc);
  conv_op->AddOutputDesc("w", OutDesc);
  conv_op->AddOutputDesc("e", OutDesc);
  conv_op->AddOutputDesc("r", OutDesc);

  vector<uint32_t> dynamic_output_start_idx = {0,1};
  vector<uint32_t> dynamic_output_end_idx = {1};
  ge::AttrUtils::SetListInt(conv_op, "_dynamic_output_index_start", dynamic_output_start_idx);
  ge::AttrUtils::SetListInt(conv_op, "_dynamic_output_index_end", dynamic_output_end_idx);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node_ptr = graph->AddNode(conv_op);
  Node* node = node_ptr.get();

  TbeSingleOpInfoAssembler test_JudgeShapeToSetFlag;
  TbeOpInfo op_info(op_name, op_module, op_type, "", core_type);
  bool flag = false;
  auto op_desc_ptr = node->GetOpDesc();
  ge::AttrUtils::SetStr(op_desc_ptr, ge::ATTR_NAME_UNREGST_OPPATH, "../../abs.py");
  Status ret = test_JudgeShapeToSetFlag.AssembleSingleTbeInfo(node, op_info, core_type);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(UTEST_TbeSingleOpInfoAssembler, Test_Parse_Input_Info_From_Op_3){
  std::string op_name    = "conv";
  std::string op_module   = "";
  std::string op_type     = "tbe";
  std::string core_type   = "AIcoreEngine";

  vector<int64_t> dim_data = {1024, 3, 1024, 1024};
  GeShape shape_data(dim_data);
  GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_INT8);

  vector<int64_t> dim_out = {1024, 3, 1024, 3};
  GeShape shape_out(dim_out);
  GeTensorDesc OutDesc(shape_out, FORMAT_NCHW, DT_INT8);

  OpDescPtr conv_op = std::make_shared<OpDesc>("Conv", "conv");
  conv_op->AddInputDesc("x", data_desc);
  conv_op->AddOutputDesc("q", OutDesc);
  conv_op->AddOutputDesc("w", OutDesc);
  conv_op->AddOutputDesc("e", OutDesc);
  conv_op->AddOutputDesc("r", OutDesc);

  vector<uint32_t> dynamic_output_start_idx = {0,2};
  vector<uint32_t> dynamic_output_end_idx = {1,1};
  ge::AttrUtils::SetListInt(conv_op, "_dynamic_output_index_start", dynamic_output_start_idx);
  ge::AttrUtils::SetListInt(conv_op, "_dynamic_output_index_end", dynamic_output_end_idx);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node_ptr = graph->AddNode(conv_op);
  Node* node = node_ptr.get();

  TbeSingleOpInfoAssembler test_JudgeShapeToSetFlag;
  TbeOpInfo op_info(op_name, op_module, op_type, "", core_type);
  bool flag = false;
  auto op_desc_ptr = node->GetOpDesc();
  ge::AttrUtils::SetStr(op_desc_ptr, ge::ATTR_NAME_UNREGST_OPPATH, "../../abs.py");
  Status ret = test_JudgeShapeToSetFlag.AssembleSingleTbeInfo(node, op_info, core_type);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(UTEST_TbeSingleOpInfoAssembler, Test_Parse_Input_Info_From_Op_4){
  std::string op_name    = "conv";
  std::string op_module   = "";
  std::string op_type     = "tbe";
  std::string core_type   = "AIcoreEngine";

  vector<int64_t> dim_data = {1024, 3, 1024, 1024};
  GeShape shape_data(dim_data);
  GeTensorDesc data_desc(shape_data, FORMAT_NCHW, DT_INT8);

  vector<int64_t> dim_out = {1024, 3, 1024, 3};
  GeShape shape_out(dim_out);
  GeTensorDesc OutDesc(shape_out, FORMAT_NCHW, DT_INT8);

  OpDescPtr conv_op = std::make_shared<OpDesc>("Conv", "conv");
  conv_op->AddInputDesc("x", data_desc);
  conv_op->AddOutputDesc("q", OutDesc);
  conv_op->AddOutputDesc("w", OutDesc);
  conv_op->AddOutputDesc("e", OutDesc);
  conv_op->AddOutputDesc("r", OutDesc);

  vector<uint32_t> dynamic_output_start_idx = {0};
  vector<uint32_t> dynamic_output_end_idx = {2};
  ge::AttrUtils::SetListInt(conv_op, "_dynamic_output_index_start", dynamic_output_start_idx);
  ge::AttrUtils::SetListInt(conv_op, "_dynamic_output_index_end", dynamic_output_end_idx);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node_ptr = graph->AddNode(conv_op);
  Node* node = node_ptr.get();

  TbeSingleOpInfoAssembler test_JudgeShapeToSetFlag;
  TbeOpInfo op_info(op_name, op_module, op_type, "", core_type);
  bool flag = false;
  auto op_desc_ptr = node->GetOpDesc();
  ge::AttrUtils::SetStr(op_desc_ptr, ge::ATTR_NAME_UNREGST_OPPATH, "../../abs.py");
  Status ret = test_JudgeShapeToSetFlag.AssembleSingleTbeInfo(node, op_info, core_type);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(UTEST_TbeSingleOpInfoAssembler, feed_attrs_to_single_tbeopinfo_1) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("op", "Op");
  std::string op_attr_list = "x;attr:sss";
  ge::AttrUtils::SetStr(op_desc_ptr, ge::ATTR_NAME_UNREGST_ATTRLIST, op_attr_list);
  te::TbeOpInfo tbe_op_info("tbe_node", "module", "Conv2D", "Ascend310", "AICore");
  TbeSingleOpInfoAssembler singleOpInfoAssembler;
  Status ret = singleOpInfoAssembler.FeedAttrsToSingleTbeOpInfo(op_desc_ptr, tbe_op_info);
  EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_TbeSingleOpInfoAssembler, feed_attrs_to_single_tbeopinfo_2) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("op", "Op");
  std::string op_attr_list = "attr1:str;attr2:int";
  ge::AttrUtils::SetStr(op_desc_ptr, ge::ATTR_NAME_UNREGST_ATTRLIST, op_attr_list);
  te::TbeOpInfo tbe_op_info("tbe_node", "module", "Conv2D", "Ascend310", "AICore");
  TbeSingleOpInfoAssembler singleOpInfoAssembler;
  Status ret = singleOpInfoAssembler.FeedAttrsToSingleTbeOpInfo(op_desc_ptr, tbe_op_info);
  EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_TbeSingleOpInfoAssembler, feed_attrs_to_single_tbeopinfo_3) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("op", "Op");
  std::string op_attr_list = "attr1:str;attr2:int";
  ge::AttrUtils::SetStr(op_desc_ptr, ge::ATTR_NAME_UNREGST_ATTRLIST, op_attr_list);
  ge::AttrUtils::SetStr(op_desc_ptr, "attr1", "qwer");
  ge::AttrUtils::SetInt(op_desc_ptr, "attr2", 123);
  te::TbeOpInfo tbe_op_info("tbe_node", "module", "Conv2D", "Ascend310", "AICore");
  TbeSingleOpInfoAssembler singleOpInfoAssembler;
  Status ret = singleOpInfoAssembler.FeedAttrsToSingleTbeOpInfo(op_desc_ptr, tbe_op_info);
  EXPECT_EQ(ret, fe::SUCCESS);
}