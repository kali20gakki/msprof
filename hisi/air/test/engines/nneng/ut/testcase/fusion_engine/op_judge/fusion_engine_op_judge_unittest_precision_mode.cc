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

#include "common/util/op_info_util.h"
#include "graph/ge_context.h"

#define private public
#define protected public
#include "ops_store/ops_kernel_manager.h"
#include "ops_store/sub_op_info_store.h"
#include "ops_store/op_kernel_info.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "graph_optimizer/op_judge/imply_type/op_impl_type_judge.h"
#include "graph_optimizer/op_judge/format_and_dtype/op_format_dtype_judge.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_rise_matcher.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/format/op_format_matcher.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include "graph/debug/ge_attr_define.h"
#include "common/configuration.h"
using namespace std;
using namespace ge;
using namespace fe;
#define DIMENSION_4 (4)
#define DIMENSION_1 (1)
using OpImplTypeJudgePtr = std::shared_ptr<OpImplTypeJudge>;
using OpFormatDtypeJudgePtr = std::shared_ptr<OpFormatDtypeJudge>;
using OpDtypeRiseMatcherPtr = std::shared_ptr<OpDtypeRiseMatcher>;
using OpFormatMatcherPtr = std::shared_ptr<OpFormatMatcher>;
using OpFormatDtypeStrategyManagerPtr = std::shared_ptr<OpFormatDtypeStrategyManager>;
using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using TransNodeManagerPtr = std::shared_ptr<TransNodeManager>;


class UTEST_fusion_engine_op_judge_precision_mode : public testing::Test
{
 protected:
  void SetUp()
  {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));

    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo tbe_custom {
        6,
        "tbe-custom",
        EN_IMPL_HW_TBE,
        "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
        "",
        false,
        false};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr_->Initialize(options);
    reflection_builder_ptr_ = std::make_shared<ge::RefRelations>();

    op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
    op_format_dtype_judge_ptr_->Initialize();

    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    OpsKernelManager::Instance(AI_CORE_NAME).Initialize();

  }

  void TearDown()
  {

  }

  static void CreateUnknownShapeGraph(ComputeGraphPtr graph, ge::Format d_format, ge::Format format, ge::GeShape unknown_shape) {
    OpDescPtr g_op = std::make_shared<OpDesc>("Data", fe::DATA);
    OpDescPtr h_op = std::make_shared<OpDesc>("UnknownShape1", "UnknownShape");

    //add descriptor
    vector<int64_t> dim({4, 33, 12, 16, 16});
    GeShape shape(dim);
    GeTensorDesc tensor_desc(shape);
    tensor_desc.SetOriginFormat(FORMAT_NCHW);
    tensor_desc.SetFormat(d_format);
    tensor_desc.SetDataType(DT_FLOAT16);
    g_op->AddInputDesc("x", tensor_desc);
    g_op->AddOutputDesc("z", tensor_desc);
    ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
    ge::NodePtr g_node = graph->AddNode(g_op);

    GeTensorDesc tensor_desc_h(unknown_shape);
    tensor_desc_h.SetOriginFormat(format);
    tensor_desc_h.SetFormat(format);
    tensor_desc_h.SetDataType(DT_FLOAT16);
    h_op->AddInputDesc("x", tensor_desc_h);
    h_op->AddOutputDesc("z", tensor_desc_h);
    ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
    ge::NodePtr h_node = graph->AddNode(h_op);
    GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));
  }
  static void CreateTwoInputOneOutputNode(OpDescPtr node_op, vector<int64_t> dim, ge::DataType Dtype) {
    GeShape shape(dim);
    GeTensorDesc tensor_desc(shape);
    tensor_desc.SetOriginFormat(FORMAT_NCHW);
    tensor_desc.SetFormat(FORMAT_NCHW);
    tensor_desc.SetDataType(Dtype);
    node_op->AddInputDesc("x", tensor_desc);
    node_op->AddInputDesc("y", tensor_desc);
    node_op->AddOutputDesc("z", tensor_desc);
    ge::AttrUtils::SetInt(node_op, FE_IMPLY_TYPE, 6);
  }
  static void CreateSetOneNode(OpDescPtr node_op, vector<int64_t> dim, ge::DataType Dtype) {
    GeShape shape(dim);
    GeTensorDesc tensor_desc(shape);
    tensor_desc.SetOriginFormat(FORMAT_NCHW);
    tensor_desc.SetFormat(FORMAT_NCHW);
    tensor_desc.SetDataType(Dtype);
    node_op->AddInputDesc("x", tensor_desc);
    node_op->AddOutputDesc("z", tensor_desc);
    ge::AttrUtils::SetInt(node_op, FE_IMPLY_TYPE, 6);
  }

  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  RefRelationsPtr reflection_builder_ptr_;
  OpFormatDtypeJudgePtr op_format_dtype_judge_ptr_;
};

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_force_fp16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NHWC);
  tensor_desc_h.SetFormat(FORMAT_NHWC);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nhw_c_to_NC1_hw_c0({1, 1, 2, 3, 32});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);
}


TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_force_fp16_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("BM", "BatchMatMul");

  //add descriptor
  vector<int64_t> dim1({32, 1, 2});
  vector<int64_t> dim2({32, 2, 2});
  vector<int64_t> dim_o({32, 1, 2});
  GeShape shape1(dim1);
  GeShape shape2(dim2);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginFormat(FORMAT_NHWC);
  tensor_desc1.SetFormat(FORMAT_NHWC);
  tensor_desc1.SetDataType(DT_FLOAT);
  tensor_desc1.SetOriginDataType(DT_FLOAT);

  GeTensorDesc tensor_desc2(shape2);
  tensor_desc2.SetOriginFormat(FORMAT_NHWC);
  tensor_desc2.SetFormat(FORMAT_NHWC);
  tensor_desc2.SetDataType(DT_FLOAT);
  tensor_desc2.SetOriginDataType(DT_FLOAT);

  g_op->AddInputDesc("x1", tensor_desc2);
  g_op->AddInputDesc("x2", tensor_desc2);
  g_op->AddOutputDesc("y", tensor_desc2);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);

  vector<int64_t> dim_result1({32, 1, 1, 16, 16});
  vector<int64_t> dim_result2({32, 1, 1, 16, 16});
  vector<int64_t> dim_result_o({32, 1, 1, 16, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result1);

  EXPECT_EQ(g_op->GetInputDesc(1).GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(g_op->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(1).GetShape().GetDims(), dim_result2);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_o);
}

/* Original format is consecutive */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_force_fp16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 32});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}
/* Original format is consecutive */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_force_fp32_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP32;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));


  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 32});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}
/* Original format is consecutive */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_force_fp32_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP32;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G2");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G2");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT32);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));


  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT64);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT64);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_auto_mixed_precision)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NHWC);
  tensor_desc_h.SetFormat(FORMAT_NHWC);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nhw_c_to_NC1_hw_c0({1, 1, 2, 3, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);
}

/* Original format is consecutive, Double can be the higher precision version
 * of float16 */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GGray");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT32);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_h_NHWC({1, 3, 4, 2});
  vector<int64_t> dim_h_NCHW({1, 2, 3, 4});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_h_NCHW);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NHWC);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_h_NHWC);
}

/* G2 is in Gray list, we can decrease the precision for it. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_3)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GGray");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_UINT8);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT32);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 2, 12, 16, 32});
  vector<int64_t> dim_h_NHWC({1, 3, 4, 2});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_UINT8);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_UINT8);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_h);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NHWC);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_h_NHWC);
}


/* G2 is in Black list, it must select the original dtype */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_4)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GBlack");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_DOUBLE);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT32);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::FAILED);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT32);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_h);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT32);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_h);
}

/* G2 is in the white list and we must select fp16 if the original data type is
 * fp32/fp16. And select the original dtype in default mode when dtype is not
 * fp32 or fp16. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_5)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GWhite");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_DOUBLE);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT32);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::FAILED);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT32);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_h);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT32);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_h);
}

/* G2 is in the white list and we must select fp16 if the original data type is
 * fp32/fp16. And select the original dtype in default mode when dtype is not
 * fp32 or fp16. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_6)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GWhite");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_DOUBLE);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_FLOAT);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}

/* G(fp16) -> Cast(fp16 to fp32) -> G, if Cast is not in Black list,
 * we will jump over cast and select fp16 for the second G op. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_7)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr cast_op = std::make_shared<OpDesc>("Cast", "Cast");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GWhite");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_cast({4, 33, 12, 16});
  GeShape shape_cast(dim);
  GeTensorDesc tensor_desc_cast(shape_cast);
  tensor_desc_cast.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_cast.SetFormat(FORMAT_NCHW);
  tensor_desc_cast.SetDataType(DT_FLOAT);
  cast_op->AddInputDesc("x", tensor_desc); // Float 16
  cast_op->AddOutputDesc("z", tensor_desc_cast); // Float 32
  ge::AttrUtils::SetInt(cast_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr cast_node = graph->AddNode(cast_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_FLOAT);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), cast_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(cast_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(cast_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(cast_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(cast_op->GetInputDesc(0).GetShape().GetDims(), dim_cast);

  EXPECT_EQ(cast_op->GetOutputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(cast_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(cast_op->GetOutputDesc(0).GetShape().GetDims(), dim_cast);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}

/* G(fp16) -> Cast(fp16 to fp32) -> G, if Cast is not in Black list,
 * we will jump over cast and select fp16 for the second G op. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_8)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  /* Stub Cast in Black list */
  SubOpInfoStorePtr subOpInfoStorePtr = OpsKernelManager::Instance(AI_CORE_NAME).GetSubOpsKernelByStoreName("tbe-custom");
  OpKernelInfoPtr opKernelInfoPtr = subOpInfoStorePtr->GetOpKernelByOpType(fe::CAST);
  opKernelInfoPtr->op_store_info_.precision_policy = BLACK;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr cast_op = std::make_shared<OpDesc>("Cast", fe::CAST);
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GGray");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_cast({4, 33, 12, 16});
  GeShape shape_cast(dim);
  GeTensorDesc tensor_desc_cast(shape_cast);
  tensor_desc_cast.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_cast.SetFormat(FORMAT_NCHW);
  tensor_desc_cast.SetDataType(DT_FLOAT);
  cast_op->AddInputDesc("x", tensor_desc);
  cast_op->AddOutputDesc("z", tensor_desc_cast);
  ge::AttrUtils::SetInt(cast_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr cast_node = graph->AddNode(cast_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_FLOAT);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), cast_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(cast_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_h_NHWC({1, 3, 4, 2});
  vector<int64_t> dim_h_NCHW({1, 2, 3, 4});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(cast_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(cast_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(cast_op->GetInputDesc(0).GetShape().GetDims(), dim_cast);

  EXPECT_EQ(cast_op->GetOutputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(cast_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(cast_op->GetOutputDesc(0).GetShape().GetDims(), dim_cast);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_h_NCHW);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NHWC);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_h_NHWC);
  /* restore Cast back to Gray list */
  opKernelInfoPtr->op_store_info_.precision_policy = BLACK;
}

/* Op GBlackOnlyFp16 is in blacklist but it does not support fp32 and it's original data type is fp32,
 * we return failed and tell the user this op should not be configured in black list. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_auto_mixed_precision_9)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G", "GBlackOnlyFp16");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::FAILED);
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_auto_mixed_precision_new)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  ge::AttrUtils::SetInt(tensor_desc, FORMAT_CONTINUOUS, 1);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NHWC);
  tensor_desc_h.SetFormat(FORMAT_NHWC);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nhw_c_to_NC1_hw_c0({1, 1, 2, 3, 32});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_default_mode)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = MUST_KEEP_ORIGIN_DTYPE;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NHWC);
  tensor_desc_h.SetFormat(FORMAT_NHWC);
  tensor_desc_h.SetDataType(DT_INT32);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::FAILED);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NHWC);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT32);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_h);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NHWC);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT32);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_h);
}

/* Original format is consecutive */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_default_mode_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = MUST_KEEP_ORIGIN_DTYPE;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 32});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}

/* When precisoin mode is must_keep_origin_dtype, we do
 * not allow precision reduce in CheckSupported. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, check_support_with_must_keep_orignal_dtype)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = MUST_KEEP_ORIGIN_DTYPE;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "GBlackOnlyFp16");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  string reason;
  bool result = fe_ops_kernel_info_store_ptr_->CheckSupported(g_node->GetOpDesc(),  reason);
  ASSERT_EQ(result, false);
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_allow_fp32_to_fp16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_DOUBLE);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NHWC);
  tensor_desc_h.SetFormat(FORMAT_NHWC);
  tensor_desc_h.SetDataType(DT_FLOAT16);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nhw_c_to_NC1_hw_c0({1, 1, 2, 3, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_DOUBLE);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nhw_c_to_NC1_hw_c0);
}

/* Original format is consecutive */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_allow_fp32_to_fp16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("conv1", "ConvTemp");
  OpDescPtr h_op = std::make_shared<OpDesc>("conv2", "ConvTemp");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 32});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_INT8);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}


/* Original format is consecutive,
 * and the second op is GE op, it use mixed precision as default strategy. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode,
       set_two_nodes_format_dtype_allow_fp32_to_fp16_3)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("conv1", "ConvTemp");
  OpDescPtr h_op = std::make_shared<OpDesc>("conv2", "ConvTemp_Ge");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_INT8);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));

  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 16});
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}

/* For pytorch, Data's format is FRACTAL_NZ. */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, pytorch_set_two_nodes_format_dtype_allow_fp32_to_fp16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr data_op = std::make_shared<OpDesc>("Data", fe::DATA);
  OpDescPtr h_op = std::make_shared<OpDesc>("bm", "BatchMatMul2");

  //add descriptor
  vector<int64_t> dim({4, 33, 2, 3, 16, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_FRACTAL_NZ);
  tensor_desc.SetDataType(DT_FLOAT);
  data_op->AddInputDesc("x", tensor_desc);
  data_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(data_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr data_node = graph->AddNode(data_op);

  vector<int64_t> dim_h({4, 33, 48, 32});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_FLOAT);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);
  GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));
  
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret2, fe::SUCCESS);
  EXPECT_EQ(data_op->GetInputDesc(0).GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(data_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(data_op->GetInputDesc(0).GetShape().GetDims(), dim);

  EXPECT_EQ(data_op->GetOutputDesc(0).GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(data_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(data_op->GetOutputDesc(0).GetShape().GetDims(), dim);

  EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), dim);

  EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), FORMAT_FRACTAL_NZ);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), dim);
}

//TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_unknown_shape_c)
//{
//  map<ge::Format, vector<int64_t>> format_shape_map = {
//    {FORMAT_NCHW, {1, -1, 3, 4}},
//    {FORMAT_NHWC, {1, 2, 3, -1}},
//    {FORMAT_HWCN, {1, 2, -1, 4}},
//    {FORMAT_CHWN, {-1, 2, 3, 4}},
//    {FORMAT_NDHWC, {1, 2, 3, 4, -1}},
//    {FORMAT_DHWCN, {1, 2, 3, -1, 5}},
//    {FORMAT_DHWNC, {1, -1, 3, 4, -1}},
//  };
//  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
//  for (auto format_shape : format_shape_map) {
//    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
//    GeShape shape_h(format_shape.second);
//    CreateUnknownShapeGraph(graph, FORMAT_NC1HWC0, format_shape.first, shape_h);
//
//    for (ge::NodePtr node : graph->GetDirectNode()) {
//      if (node->GetType() != "UnknownShape") {
//        continue;
//      }
//      
//      Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node,  "tbe-custom");
//      ASSERT_EQ(ret2, fe::SUCCESS);
//      OpDescPtr h_op = node->GetOpDesc();
//      vector<int64_t> dim_result_nhw_c_to_NC1_hw_c0({1, 1, 2, 3, 16});
//      vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
//
//      EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), format_shape.first);
//      EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
//      EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), shape_h.GetDims());
//
//      EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), format_shape.first);
//      EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
//      EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), shape_h.GetDims());
//    }
//  }
//}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_two_nodes_format_dtype_unknown_shape_last_two)
{
  map<ge::Format, vector<int64_t>> format_shape_map = {
          {FORMAT_NCHW, {1, 2, -1, 4}},
          {FORMAT_NHWC, {1, 2, 3, -1}},
          {FORMAT_HWCN, {1, 2, -1, 4}},
          {FORMAT_CHWN, {1, 2, 3, -1}},
          {FORMAT_NDHWC, {1, 2, 3, 4, -1}},
          {FORMAT_DHWCN, {1, 2, 3, -1, 5}},
          {FORMAT_DHWNC, {1, -1, 3, 4, -1}},
  };
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  for (auto format_shape : format_shape_map) {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
    GeShape shape_h(format_shape.second);
    CreateUnknownShapeGraph(graph, FORMAT_FRACTAL_NZ, format_shape.first, shape_h);

    for (ge::NodePtr node : graph->GetDirectNode()) {
      if (node->GetType() != "UnknownShape") {
        continue;
      }
      
      Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node,  "tbe-custom");
      ASSERT_EQ(ret2, fe::SUCCESS);
      OpDescPtr h_op = node->GetOpDesc();
      vector<int64_t> dim_result_nhw_c_to_NC1_hw_c0({1, 1, 2, 3, 16});
      vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});

      EXPECT_EQ(h_op->GetInputDesc(0).GetFormat(), format_shape.first);
      EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
      EXPECT_EQ(h_op->GetInputDesc(0).GetShape().GetDims(), shape_h.GetDims());

      EXPECT_EQ(h_op->GetOutputDesc(0).GetFormat(), format_shape.first);
      EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
      EXPECT_EQ(h_op->GetOutputDesc(0).GetShape().GetDims(), shape_h.GetDims());
    }
  }
}


/* Original format is consecutive */
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, keep_dtype_01)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "G");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_FLOAT);
  g1_op->AddInputDesc("x", tensor_desc);
  g1_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g1_op, FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetInt(g1_op, KEEP_DTYPE, 1);
  ge::NodePtr g_node = graph->AddNode(g1_op);


  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_FLOAT);
  g2_op->AddInputDesc("x", tensor_desc_h);
  g2_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(g2_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));


  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0({4, 3, 12, 16, 16});
  vector<int64_t> dim_result_nch_w_to_NC1_hw_c0_2({1, 1, 3, 4, 16});
  EXPECT_EQ(g1_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g1_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g1_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g1_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0);

  EXPECT_EQ(g2_op->GetInputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);

  EXPECT_EQ(g2_op->GetOutputDesc(0).GetFormat(), FORMAT_NC1HWC0);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetShape().GetDims(), dim_result_nch_w_to_NC1_hw_c0_2);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_cast_nodes_format_dtype_force_fp16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("Cast1", CAST);

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc float_tensor_desc(shape);
  float_tensor_desc.SetOriginFormat(FORMAT_NCHW);
  float_tensor_desc.SetFormat(FORMAT_NCHW);
  float_tensor_desc.SetDataType(DT_FLOAT);
  g_op->AddInputDesc("x", float_tensor_desc);

  GeTensorDesc int16_tensor_desc(shape);
  int16_tensor_desc.SetOriginFormat(FORMAT_NCHW);
  int16_tensor_desc.SetFormat(FORMAT_NCHW);
  int16_tensor_desc.SetDataType(DT_INT16);

  g_op->AddOutputDesc("z", int16_tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  EXPECT_EQ(g_op->GetInputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_FLOAT);
  EXPECT_EQ(g_op->GetInputDesc(0).GetShape().GetDims(), dim);

  EXPECT_EQ(g_op->GetOutputDesc(0).GetFormat(), FORMAT_NCHW);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_INT16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetShape().GetDims(), dim);
}
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_fp16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  //G2 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};

  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_fp16_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  //G2 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_fp16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
   //G2 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16,};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16,};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_bf16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  //G3 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_bf16_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  //G3 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_bf16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  //G3 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_lowerprecision)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_LOWERPRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_lowerprecision_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_LOWERPRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_lowerprecision_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_LOWERPRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_fp16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  //G2 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_fp16_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  //G2 check no support dtypes. so return config context 1
  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_fp16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_bf16_3)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GFp16Fp1632Fp1632");
  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateTwoInputOneOutputNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT16, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};

  EXPECT_EQ(g1_op->GetInputDesc(1).GetDataType(), result_input[0]);
  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_input[0]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_bf16_4)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GFp16Fp1632Fp32");
  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateTwoInputOneOutputNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT16, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};

  EXPECT_EQ(g1_op->GetInputDesc(1).GetDataType(), result_input[0]);
  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_input[4]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_bf16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_UINT8, DT_FLOAT, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_UINT8, DT_FLOAT, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_bf16_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_bf16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_UINT8, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_to_lowprecision)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_LOWPRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}


TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_to_lowprecision_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_LOWPRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_allow_fp32_to_lowprecision_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_FP32_TO_LOWPRECISION;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_float_force_fp32)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = FORCE_FP32;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_keep_oringin)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = MUST_KEEP_ORIGIN_DTYPE;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::FAILED);
  ASSERT_EQ(ret3, fe::FAILED);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::FAILED);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_FLOAT, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  //EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  //EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  //EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  //EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  //EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  //EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_keep_oringin_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = MUST_KEEP_ORIGIN_DTYPE;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::FAILED);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  //EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  //EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_keep_oringin_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = MUST_KEEP_ORIGIN_DTYPE;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GSupportFp32Bf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GSupportfp32Fp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GSupportBf16Fp16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  GraphUtils::AddEdge(g4_node->GetOutDataAnchor(0), g5_node->GetInDataAnchor(0));
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  GraphUtils::AddEdge(g5_node->GetOutDataAnchor(0), g6_node->GetInDataAnchor(0));
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  GraphUtils::AddEdge(g6_node->GetOutDataAnchor(0), g7_node->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_FLOAT16, DT_BF16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
}


TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_fp16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GBlackOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GWhiteOnlyFp16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GOnlyFp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GBlackSupportFp32Bf16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "GWhiteSupportFp32Bf16");
  OpDescPtr g8_op = std::make_shared<OpDesc>("G8", "GSupportFp32Bf16");
  OpDescPtr g9_op = std::make_shared<OpDesc>("G9", "GBlackSupportfp32Fp16");
  OpDescPtr g10_op = std::make_shared<OpDesc>("G10", "GWhiteSupportfp32Fp16");
  OpDescPtr g11_op = std::make_shared<OpDesc>("G11", "GSupportfp32Fp16");
  OpDescPtr g12_op = std::make_shared<OpDesc>("G12", "GBlackSupportBf16Fp16");
  OpDescPtr g13_op = std::make_shared<OpDesc>("G13", "GWhiteSupportBf16Fp16");
  OpDescPtr g14_op = std::make_shared<OpDesc>("G14", "GSupportBf16Fp16");
  OpDescPtr g15_op = std::make_shared<OpDesc>("G15", "GBlack");
  OpDescPtr g16_op = std::make_shared<OpDesc>("G16", "GWhite");
  OpDescPtr g17_op = std::make_shared<OpDesc>("G17", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  CreateSetOneNode(g8_op, dim, dtype);
  ge::NodePtr g8_node = graph->AddNode(g8_op);
  CreateSetOneNode(g9_op, dim, dtype);
  ge::NodePtr g9_node = graph->AddNode(g9_op);
  CreateSetOneNode(g10_op, dim, dtype);
  ge::NodePtr g10_node = graph->AddNode(g10_op);
  CreateSetOneNode(g11_op, dim, dtype);
  ge::NodePtr g11_node = graph->AddNode(g11_op);
  CreateSetOneNode(g12_op, dim, dtype);
  ge::NodePtr g12_node = graph->AddNode(g12_op);
  CreateSetOneNode(g13_op, dim, dtype);
  ge::NodePtr g13_node = graph->AddNode(g13_op);
  CreateSetOneNode(g14_op, dim, dtype);
  ge::NodePtr g14_node = graph->AddNode(g14_op);
  CreateSetOneNode(g15_op, dim, dtype);
  ge::NodePtr g15_node = graph->AddNode(g15_op);
  CreateSetOneNode(g16_op, dim, dtype);
  ge::NodePtr g16_node = graph->AddNode(g16_op);
  CreateSetOneNode(g17_op, dim, dtype);
  ge::NodePtr g17_node = graph->AddNode(g17_op);
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");
  Status ret8 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g8_node,  "tbe-custom");
  Status ret9 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g9_node,  "tbe-custom");
  Status ret10 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g10_node,  "tbe-custom");
  Status ret11 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g11_node,  "tbe-custom");
  Status ret12 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g12_node,  "tbe-custom");
  Status ret13 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g13_node,  "tbe-custom");
  Status ret14 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g14_node,  "tbe-custom");
  Status ret15 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g15_node,  "tbe-custom");
  Status ret16 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g16_node,  "tbe-custom");
  Status ret17 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g17_node,  "tbe-custom");


  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::FAILED);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  ASSERT_EQ(ret8, fe::SUCCESS);
  ASSERT_EQ(ret9, fe::SUCCESS);
  ASSERT_EQ(ret10, fe::SUCCESS);
  ASSERT_EQ(ret11, fe::SUCCESS);
  ASSERT_EQ(ret12, fe::FAILED);
  ASSERT_EQ(ret13, fe::SUCCESS);
  ASSERT_EQ(ret14, fe::SUCCESS);
  ASSERT_EQ(ret15, fe::SUCCESS);
  ASSERT_EQ(ret16, fe::SUCCESS);
  ASSERT_EQ(ret17, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_MAX, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT, DT_MAX, DT_FLOAT16, DT_FLOAT16, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_MAX, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT, DT_MAX, DT_FLOAT16, DT_FLOAT16, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  //EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);
  EXPECT_EQ(g8_op->GetInputDesc(0).GetDataType(), result_input[7]);
  EXPECT_EQ(g9_op->GetInputDesc(0).GetDataType(), result_input[8]);
  EXPECT_EQ(g10_op->GetInputDesc(0).GetDataType(), result_input[9]);
  EXPECT_EQ(g11_op->GetInputDesc(0).GetDataType(), result_input[10]);
  //EXPECT_EQ(g12_op->GetInputDesc(0).GetDataType(), result_input[11]);
  EXPECT_EQ(g13_op->GetInputDesc(0).GetDataType(), result_input[12]);
  EXPECT_EQ(g14_op->GetInputDesc(0).GetDataType(), result_input[13]);
  EXPECT_EQ(g15_op->GetInputDesc(0).GetDataType(), result_input[14]);
  EXPECT_EQ(g16_op->GetInputDesc(0).GetDataType(), result_input[15]);
  EXPECT_EQ(g17_op->GetInputDesc(0).GetDataType(), result_input[16]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  //EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
  EXPECT_EQ(g8_op->GetOutputDesc(0).GetDataType(), result_output[7]);
  EXPECT_EQ(g9_op->GetOutputDesc(0).GetDataType(), result_output[8]);
  EXPECT_EQ(g10_op->GetOutputDesc(0).GetDataType(), result_output[9]);
  EXPECT_EQ(g11_op->GetOutputDesc(0).GetDataType(), result_output[10]);
  //EXPECT_EQ(g12_op->GetOutputDesc(0).GetDataType(), result_output[11]);
  EXPECT_EQ(g13_op->GetOutputDesc(0).GetDataType(), result_output[12]);
  EXPECT_EQ(g14_op->GetOutputDesc(0).GetDataType(), result_output[13]);
  EXPECT_EQ(g15_op->GetOutputDesc(0).GetDataType(), result_output[14]);
  EXPECT_EQ(g16_op->GetOutputDesc(0).GetDataType(), result_output[15]);
  EXPECT_EQ(g17_op->GetOutputDesc(0).GetDataType(), result_output[16]);
}


TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_fp16_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GBlackOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GWhiteOnlyFp16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GOnlyFp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GBlackSupportFp32Bf16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "GWhiteSupportFp32Bf16");
  OpDescPtr g8_op = std::make_shared<OpDesc>("G8", "GSupportFp32Bf16");
  OpDescPtr g9_op = std::make_shared<OpDesc>("G9", "GBlackSupportfp32Fp16");
  OpDescPtr g10_op = std::make_shared<OpDesc>("G10", "GWhiteSupportfp32Fp16");
  OpDescPtr g11_op = std::make_shared<OpDesc>("G11", "GSupportfp32Fp16");
  OpDescPtr g12_op = std::make_shared<OpDesc>("G12", "GBlackSupportBf16Fp16");
  OpDescPtr g13_op = std::make_shared<OpDesc>("G13", "GWhiteSupportBf16Fp16");
  OpDescPtr g14_op = std::make_shared<OpDesc>("G14", "GSupportBf16Fp16");
  OpDescPtr g15_op = std::make_shared<OpDesc>("G15", "GBlack");
  OpDescPtr g16_op = std::make_shared<OpDesc>("G16", "GWhite");
  OpDescPtr g17_op = std::make_shared<OpDesc>("G17", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  CreateSetOneNode(g8_op, dim, dtype);
  ge::NodePtr g8_node = graph->AddNode(g8_op);
  CreateSetOneNode(g9_op, dim, dtype);
  ge::NodePtr g9_node = graph->AddNode(g9_op);
  CreateSetOneNode(g10_op, dim, dtype);
  ge::NodePtr g10_node = graph->AddNode(g10_op);
  CreateSetOneNode(g11_op, dim, dtype);
  ge::NodePtr g11_node = graph->AddNode(g11_op);
  CreateSetOneNode(g12_op, dim, dtype);
  ge::NodePtr g12_node = graph->AddNode(g12_op);
  CreateSetOneNode(g13_op, dim, dtype);
  ge::NodePtr g13_node = graph->AddNode(g13_op);
  CreateSetOneNode(g14_op, dim, dtype);
  ge::NodePtr g14_node = graph->AddNode(g14_op);
  CreateSetOneNode(g15_op, dim, dtype);
  ge::NodePtr g15_node = graph->AddNode(g15_op);
  CreateSetOneNode(g16_op, dim, dtype);
  ge::NodePtr g16_node = graph->AddNode(g16_op);
  CreateSetOneNode(g17_op, dim, dtype);
  ge::NodePtr g17_node = graph->AddNode(g17_op);
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");
  Status ret8 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g8_node,  "tbe-custom");
  Status ret9 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g9_node,  "tbe-custom");
  Status ret10 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g10_node,  "tbe-custom");
  Status ret11 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g11_node,  "tbe-custom");
  Status ret12 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g12_node,  "tbe-custom");
  Status ret13 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g13_node,  "tbe-custom");
  Status ret14 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g14_node,  "tbe-custom");
  Status ret15 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g15_node,  "tbe-custom");
  Status ret16 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g16_node,  "tbe-custom");
  Status ret17 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g17_node,  "tbe-custom");


  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::FAILED);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  ASSERT_EQ(ret8, fe::SUCCESS);
  ASSERT_EQ(ret9, fe::SUCCESS);
  ASSERT_EQ(ret10, fe::SUCCESS);
  ASSERT_EQ(ret11, fe::SUCCESS);
  ASSERT_EQ(ret12, fe::FAILED);
  ASSERT_EQ(ret13, fe::SUCCESS);
  ASSERT_EQ(ret14, fe::SUCCESS);
  ASSERT_EQ(ret15, fe::SUCCESS);
  ASSERT_EQ(ret16, fe::SUCCESS);
  ASSERT_EQ(ret17, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_MAX, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT16, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_MAX, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT, DT_FLOAT, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT16, DT_FLOAT,
                                       DT_FLOAT16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  //EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);
  EXPECT_EQ(g8_op->GetInputDesc(0).GetDataType(), result_input[7]);
  EXPECT_EQ(g9_op->GetInputDesc(0).GetDataType(), result_input[8]);
  EXPECT_EQ(g10_op->GetInputDesc(0).GetDataType(), result_input[9]);
  EXPECT_EQ(g11_op->GetInputDesc(0).GetDataType(), result_input[10]);
  //EXPECT_EQ(g12_op->GetInputDesc(0).GetDataType(), result_input[11]);
  EXPECT_EQ(g13_op->GetInputDesc(0).GetDataType(), result_input[12]);
  EXPECT_EQ(g14_op->GetInputDesc(0).GetDataType(), result_input[13]);
  EXPECT_EQ(g15_op->GetInputDesc(0).GetDataType(), result_input[14]);
  EXPECT_EQ(g16_op->GetInputDesc(0).GetDataType(), result_input[15]);
  EXPECT_EQ(g17_op->GetInputDesc(0).GetDataType(), result_input[16]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  //EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
  EXPECT_EQ(g8_op->GetOutputDesc(0).GetDataType(), result_output[7]);
  EXPECT_EQ(g9_op->GetOutputDesc(0).GetDataType(), result_output[8]);
  EXPECT_EQ(g10_op->GetOutputDesc(0).GetDataType(), result_output[9]);
  EXPECT_EQ(g11_op->GetOutputDesc(0).GetDataType(), result_output[10]);
  //EXPECT_EQ(g12_op->GetOutputDesc(0).GetDataType(), result_output[11]);
  EXPECT_EQ(g13_op->GetOutputDesc(0).GetDataType(), result_output[12]);
  EXPECT_EQ(g14_op->GetOutputDesc(0).GetDataType(), result_output[13]);
  EXPECT_EQ(g15_op->GetOutputDesc(0).GetDataType(), result_output[14]);
  EXPECT_EQ(g16_op->GetOutputDesc(0).GetDataType(), result_output[15]);
  EXPECT_EQ(g17_op->GetOutputDesc(0).GetDataType(), result_output[16]);
}


TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_fp16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GBlackOnlyFp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GWhiteOnlyFp16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GOnlyFp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GBlackSupportFp32Bf16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "GWhiteSupportFp32Bf16");
  OpDescPtr g8_op = std::make_shared<OpDesc>("G8", "GSupportFp32Bf16");
  OpDescPtr g9_op = std::make_shared<OpDesc>("G9", "GBlackSupportfp32Fp16");
  OpDescPtr g10_op = std::make_shared<OpDesc>("G10", "GWhiteSupportfp32Fp16");
  OpDescPtr g11_op = std::make_shared<OpDesc>("G11", "GSupportfp32Fp16");
  OpDescPtr g12_op = std::make_shared<OpDesc>("G12", "GBlackSupportBf16Fp16");
  OpDescPtr g13_op = std::make_shared<OpDesc>("G13", "GWhiteSupportBf16Fp16");
  OpDescPtr g14_op = std::make_shared<OpDesc>("G14", "GSupportBf16Fp16");
  OpDescPtr g15_op = std::make_shared<OpDesc>("G15", "GBlack");
  OpDescPtr g16_op = std::make_shared<OpDesc>("G16", "GWhite");
  OpDescPtr g17_op = std::make_shared<OpDesc>("G17", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  CreateSetOneNode(g8_op, dim, dtype);
  ge::NodePtr g8_node = graph->AddNode(g8_op);
  CreateSetOneNode(g9_op, dim, dtype);
  ge::NodePtr g9_node = graph->AddNode(g9_op);
  CreateSetOneNode(g10_op, dim, dtype);
  ge::NodePtr g10_node = graph->AddNode(g10_op);
  CreateSetOneNode(g11_op, dim, dtype);
  ge::NodePtr g11_node = graph->AddNode(g11_op);
  CreateSetOneNode(g12_op, dim, dtype);
  ge::NodePtr g12_node = graph->AddNode(g12_op);
  CreateSetOneNode(g13_op, dim, dtype);
  ge::NodePtr g13_node = graph->AddNode(g13_op);
  CreateSetOneNode(g14_op, dim, dtype);
  ge::NodePtr g14_node = graph->AddNode(g14_op);
  CreateSetOneNode(g15_op, dim, dtype);
  ge::NodePtr g15_node = graph->AddNode(g15_op);
  CreateSetOneNode(g16_op, dim, dtype);
  ge::NodePtr g16_node = graph->AddNode(g16_op);
  CreateSetOneNode(g17_op, dim, dtype);
  ge::NodePtr g17_node = graph->AddNode(g17_op);
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");
  Status ret8 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g8_node,  "tbe-custom");
  Status ret9 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g9_node,  "tbe-custom");
  Status ret10 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g10_node,  "tbe-custom");
  Status ret11 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g11_node,  "tbe-custom");
  Status ret12 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g12_node,  "tbe-custom");
  Status ret13 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g13_node,  "tbe-custom");
  Status ret14 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g14_node,  "tbe-custom");
  Status ret15 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g15_node,  "tbe-custom");
  Status ret16 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g16_node,  "tbe-custom");
  Status ret17 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g17_node,  "tbe-custom");


  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  ASSERT_EQ(ret8, fe::SUCCESS);
  ASSERT_EQ(ret9, fe::SUCCESS);
  ASSERT_EQ(ret10, fe::SUCCESS);
  ASSERT_EQ(ret11, fe::SUCCESS);
  ASSERT_EQ(ret12, fe::SUCCESS);
  ASSERT_EQ(ret13, fe::SUCCESS);
  ASSERT_EQ(ret14, fe::SUCCESS);
  ASSERT_EQ(ret15, fe::SUCCESS);
  ASSERT_EQ(ret16, fe::SUCCESS);
  ASSERT_EQ(ret17, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16, DT_FLOAT16, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_UINT8, DT_FLOAT16, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT, DT_UINT8, DT_FLOAT, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16, DT_FLOAT16, DT_FLOAT16, DT_FLOAT16,
                                       DT_FLOAT16, DT_FLOAT16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);
  EXPECT_EQ(g8_op->GetInputDesc(0).GetDataType(), result_input[7]);
  EXPECT_EQ(g9_op->GetInputDesc(0).GetDataType(), result_input[8]);
  EXPECT_EQ(g10_op->GetInputDesc(0).GetDataType(), result_input[9]);
  EXPECT_EQ(g11_op->GetInputDesc(0).GetDataType(), result_input[10]);
  EXPECT_EQ(g12_op->GetInputDesc(0).GetDataType(), result_input[11]);
  EXPECT_EQ(g13_op->GetInputDesc(0).GetDataType(), result_input[12]);
  EXPECT_EQ(g14_op->GetInputDesc(0).GetDataType(), result_input[13]);
  EXPECT_EQ(g15_op->GetInputDesc(0).GetDataType(), result_input[14]);
  EXPECT_EQ(g16_op->GetInputDesc(0).GetDataType(), result_input[15]);
  EXPECT_EQ(g17_op->GetInputDesc(0).GetDataType(), result_input[16]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
  EXPECT_EQ(g8_op->GetOutputDesc(0).GetDataType(), result_output[7]);
  EXPECT_EQ(g9_op->GetOutputDesc(0).GetDataType(), result_output[8]);
  EXPECT_EQ(g10_op->GetOutputDesc(0).GetDataType(), result_output[9]);
  EXPECT_EQ(g11_op->GetOutputDesc(0).GetDataType(), result_output[10]);
  EXPECT_EQ(g12_op->GetOutputDesc(0).GetDataType(), result_output[11]);
  EXPECT_EQ(g13_op->GetOutputDesc(0).GetDataType(), result_output[12]);
  EXPECT_EQ(g14_op->GetOutputDesc(0).GetDataType(), result_output[13]);
  EXPECT_EQ(g15_op->GetOutputDesc(0).GetDataType(), result_output[14]);
  EXPECT_EQ(g16_op->GetOutputDesc(0).GetDataType(), result_output[15]);
  EXPECT_EQ(g17_op->GetOutputDesc(0).GetDataType(), result_output[16]);
}

// test for net in gray, input fp32-> GWhite -> GBlackSupportFp32Bf16 -> GWhiteSupportBf16Fp16 -> G
// the fllows output is        fp32 -> fp16 ->  fp32                  -> fp16                  -> fp16
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_fp16_3)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_FP16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GWhite");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GBlackSupportFp32Bf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GWhiteSupportBf16Fp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
   
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);


  vector<ge::DataType> result_input = { DT_FLOAT16, DT_FLOAT, DT_FLOAT16, DT_FLOAT16 };
  vector<ge::DataType> result_output = { DT_FLOAT16, DT_FLOAT, DT_FLOAT16, DT_FLOAT16 };
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);

  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
}

TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_bf16)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GBlackOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GWhiteOnlyBf16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GOnlyBf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GOnlyFp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GBlackSupportFp32Bf16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "GWhiteSupportFp32Bf16");
  OpDescPtr g8_op = std::make_shared<OpDesc>("G8", "GSupportFp32Bf16");
  OpDescPtr g9_op = std::make_shared<OpDesc>("G9", "GBlackSupportfp32Fp16");
  OpDescPtr g10_op = std::make_shared<OpDesc>("G10", "GWhiteSupportfp32Fp16");
  OpDescPtr g11_op = std::make_shared<OpDesc>("G11", "GSupportfp32Fp16");
  OpDescPtr g12_op = std::make_shared<OpDesc>("G12", "GBlackSupportBf16Fp16");
  OpDescPtr g13_op = std::make_shared<OpDesc>("G13", "GWhiteSupportBf16Fp16");
  OpDescPtr g14_op = std::make_shared<OpDesc>("G14", "GSupportBf16Fp16");
  OpDescPtr g15_op = std::make_shared<OpDesc>("G15", "GBlack");
  OpDescPtr g16_op = std::make_shared<OpDesc>("G16", "GWhite");
  OpDescPtr g17_op = std::make_shared<OpDesc>("G17", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  CreateSetOneNode(g8_op, dim, dtype);
  ge::NodePtr g8_node = graph->AddNode(g8_op);
  CreateSetOneNode(g9_op, dim, dtype);
  ge::NodePtr g9_node = graph->AddNode(g9_op);
  CreateSetOneNode(g10_op, dim, dtype);
  ge::NodePtr g10_node = graph->AddNode(g10_op);
  CreateSetOneNode(g11_op, dim, dtype);
  ge::NodePtr g11_node = graph->AddNode(g11_op);
  CreateSetOneNode(g12_op, dim, dtype);
  ge::NodePtr g12_node = graph->AddNode(g12_op);
  CreateSetOneNode(g13_op, dim, dtype);
  ge::NodePtr g13_node = graph->AddNode(g13_op);
  CreateSetOneNode(g14_op, dim, dtype);
  ge::NodePtr g14_node = graph->AddNode(g14_op);
  CreateSetOneNode(g15_op, dim, dtype);
  ge::NodePtr g15_node = graph->AddNode(g15_op);
  CreateSetOneNode(g16_op, dim, dtype);
  ge::NodePtr g16_node = graph->AddNode(g16_op);
  CreateSetOneNode(g17_op, dim, dtype);
  ge::NodePtr g17_node = graph->AddNode(g17_op);
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");
  Status ret8 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g8_node,  "tbe-custom");
  Status ret9 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g9_node,  "tbe-custom");
  Status ret10 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g10_node,  "tbe-custom");
  Status ret11 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g11_node,  "tbe-custom");
  Status ret12 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g12_node,  "tbe-custom");
  Status ret13 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g13_node,  "tbe-custom");
  Status ret14 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g14_node,  "tbe-custom");
  Status ret15 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g15_node,  "tbe-custom");
  Status ret16 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g16_node,  "tbe-custom");
  Status ret17 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g17_node,  "tbe-custom");


  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::FAILED);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  ASSERT_EQ(ret8, fe::SUCCESS);
  ASSERT_EQ(ret9, fe::SUCCESS);
  ASSERT_EQ(ret10, fe::SUCCESS);
  ASSERT_EQ(ret11, fe::SUCCESS);
  ASSERT_EQ(ret12, fe::FAILED);
  ASSERT_EQ(ret13, fe::SUCCESS);
  ASSERT_EQ(ret14, fe::SUCCESS);
  ASSERT_EQ(ret15, fe::SUCCESS);
  ASSERT_EQ(ret16, fe::SUCCESS);
  ASSERT_EQ(ret17, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_MAX, DT_BF16, DT_BF16, DT_UINT8,
                                       DT_FLOAT, DT_BF16, DT_FLOAT, DT_FLOAT, DT_UINT8,
                                       DT_FLOAT, DT_MAX, DT_BF16, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_MAX, DT_BF16, DT_BF16, DT_UINT8,
                                       DT_FLOAT, DT_BF16, DT_FLOAT, DT_FLOAT, DT_UINT8,
                                       DT_FLOAT, DT_MAX, DT_BF16, DT_BF16, DT_FLOAT,
                                       DT_BF16, DT_FLOAT};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  //EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);
  EXPECT_EQ(g8_op->GetInputDesc(0).GetDataType(), result_input[7]);
  EXPECT_EQ(g9_op->GetInputDesc(0).GetDataType(), result_input[8]);
  EXPECT_EQ(g10_op->GetInputDesc(0).GetDataType(), result_input[9]);
  EXPECT_EQ(g11_op->GetInputDesc(0).GetDataType(), result_input[10]);
  //EXPECT_EQ(g12_op->GetInputDesc(0).GetDataType(), result_input[11]);
  EXPECT_EQ(g13_op->GetInputDesc(0).GetDataType(), result_input[12]);
  EXPECT_EQ(g14_op->GetInputDesc(0).GetDataType(), result_input[13]);
  EXPECT_EQ(g15_op->GetInputDesc(0).GetDataType(), result_input[14]);
  EXPECT_EQ(g16_op->GetInputDesc(0).GetDataType(), result_input[15]);
  EXPECT_EQ(g17_op->GetInputDesc(0).GetDataType(), result_input[16]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  //EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
  EXPECT_EQ(g8_op->GetOutputDesc(0).GetDataType(), result_output[7]);
  EXPECT_EQ(g9_op->GetOutputDesc(0).GetDataType(), result_output[8]);
  EXPECT_EQ(g10_op->GetOutputDesc(0).GetDataType(), result_output[9]);
  EXPECT_EQ(g11_op->GetOutputDesc(0).GetDataType(), result_output[10]);
  //EXPECT_EQ(g12_op->GetOutputDesc(0).GetDataType(), result_output[11]);
  EXPECT_EQ(g13_op->GetOutputDesc(0).GetDataType(), result_output[12]);
  EXPECT_EQ(g14_op->GetOutputDesc(0).GetDataType(), result_output[13]);
  EXPECT_EQ(g15_op->GetOutputDesc(0).GetDataType(), result_output[14]);
  EXPECT_EQ(g16_op->GetOutputDesc(0).GetDataType(), result_output[15]);
  EXPECT_EQ(g17_op->GetOutputDesc(0).GetDataType(), result_output[16]);
}


TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_bf16_1)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GBlackOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GWhiteOnlyBf16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GOnlyBf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GOnlyFp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GBlackSupportFp32Bf16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "GWhiteSupportFp32Bf16");
  OpDescPtr g8_op = std::make_shared<OpDesc>("G8", "GSupportFp32Bf16");
  OpDescPtr g9_op = std::make_shared<OpDesc>("G9", "GBlackSupportfp32Fp16");
  OpDescPtr g10_op = std::make_shared<OpDesc>("G10", "GWhiteSupportfp32Fp16");
  OpDescPtr g11_op = std::make_shared<OpDesc>("G11", "GSupportfp32Fp16");
  OpDescPtr g12_op = std::make_shared<OpDesc>("G12", "GBlackSupportBf16Fp16");
  OpDescPtr g13_op = std::make_shared<OpDesc>("G13", "GWhiteSupportBf16Fp16");
  OpDescPtr g14_op = std::make_shared<OpDesc>("G14", "GSupportBf16Fp16");
  OpDescPtr g15_op = std::make_shared<OpDesc>("G15", "GBlack");
  OpDescPtr g16_op = std::make_shared<OpDesc>("G16", "GWhite");
  OpDescPtr g17_op = std::make_shared<OpDesc>("G17", "G");
  //test for int32 input
  OpDescPtr g18_op = std::make_shared<OpDesc>("G18", "GWhite");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_BF16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  CreateSetOneNode(g8_op, dim, dtype);
  ge::NodePtr g8_node = graph->AddNode(g8_op);
  CreateSetOneNode(g9_op, dim, dtype);
  ge::NodePtr g9_node = graph->AddNode(g9_op);
  CreateSetOneNode(g10_op, dim, dtype);
  ge::NodePtr g10_node = graph->AddNode(g10_op);
  CreateSetOneNode(g11_op, dim, dtype);
  ge::NodePtr g11_node = graph->AddNode(g11_op);
  CreateSetOneNode(g12_op, dim, dtype);
  ge::NodePtr g12_node = graph->AddNode(g12_op);
  CreateSetOneNode(g13_op, dim, dtype);
  ge::NodePtr g13_node = graph->AddNode(g13_op);
  CreateSetOneNode(g14_op, dim, dtype);
  ge::NodePtr g14_node = graph->AddNode(g14_op);
  CreateSetOneNode(g15_op, dim, dtype);
  ge::NodePtr g15_node = graph->AddNode(g15_op);
  CreateSetOneNode(g16_op, dim, dtype);
  ge::NodePtr g16_node = graph->AddNode(g16_op);
  CreateSetOneNode(g17_op, dim, dtype);
  ge::NodePtr g17_node = graph->AddNode(g17_op);
  CreateSetOneNode(g18_op, dim, DT_INT8);
  ge::NodePtr g18_node = graph->AddNode(g18_op);
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");
  Status ret8 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g8_node,  "tbe-custom");
  Status ret9 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g9_node,  "tbe-custom");
  Status ret10 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g10_node,  "tbe-custom");
  Status ret11 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g11_node,  "tbe-custom");
  Status ret12 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g12_node,  "tbe-custom");
  Status ret13 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g13_node,  "tbe-custom");
  Status ret14 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g14_node,  "tbe-custom");
  Status ret15 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g15_node,  "tbe-custom");
  Status ret16 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g16_node,  "tbe-custom");
  Status ret17 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g17_node,  "tbe-custom");
  Status ret18 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g18_node,  "tbe-custom");


  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  ASSERT_EQ(ret8, fe::SUCCESS);
  ASSERT_EQ(ret9, fe::SUCCESS);
  ASSERT_EQ(ret10, fe::SUCCESS);
  ASSERT_EQ(ret11, fe::SUCCESS);
  ASSERT_EQ(ret12, fe::SUCCESS);
  ASSERT_EQ(ret13, fe::SUCCESS);
  ASSERT_EQ(ret14, fe::SUCCESS);
  ASSERT_EQ(ret15, fe::SUCCESS);
  ASSERT_EQ(ret16, fe::SUCCESS);
  ASSERT_EQ(ret17, fe::SUCCESS);
  ASSERT_EQ(ret18, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_UINT8,
                                       DT_BF16, DT_BF16, DT_BF16, DT_FLOAT, DT_UINT8,
                                       DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_BF16,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_UINT8,
                                       DT_BF16, DT_BF16, DT_BF16, DT_FLOAT, DT_UINT8,
                                       DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_BF16,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);
  EXPECT_EQ(g8_op->GetInputDesc(0).GetDataType(), result_input[7]);
  EXPECT_EQ(g9_op->GetInputDesc(0).GetDataType(), result_input[8]);
  EXPECT_EQ(g10_op->GetInputDesc(0).GetDataType(), result_input[9]);
  EXPECT_EQ(g11_op->GetInputDesc(0).GetDataType(), result_input[10]);
  EXPECT_EQ(g12_op->GetInputDesc(0).GetDataType(), result_input[11]);
  EXPECT_EQ(g13_op->GetInputDesc(0).GetDataType(), result_input[12]);
  EXPECT_EQ(g14_op->GetInputDesc(0).GetDataType(), result_input[13]);
  EXPECT_EQ(g15_op->GetInputDesc(0).GetDataType(), result_input[14]);
  EXPECT_EQ(g16_op->GetInputDesc(0).GetDataType(), result_input[15]);
  EXPECT_EQ(g17_op->GetInputDesc(0).GetDataType(), result_input[16]);
  EXPECT_EQ(g18_op->GetInputDesc(0).GetDataType(), DT_INT8);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
  EXPECT_EQ(g8_op->GetOutputDesc(0).GetDataType(), result_output[7]);
  EXPECT_EQ(g9_op->GetOutputDesc(0).GetDataType(), result_output[8]);
  EXPECT_EQ(g10_op->GetOutputDesc(0).GetDataType(), result_output[9]);
  EXPECT_EQ(g11_op->GetOutputDesc(0).GetDataType(), result_output[10]);
  EXPECT_EQ(g12_op->GetOutputDesc(0).GetDataType(), result_output[11]);
  EXPECT_EQ(g13_op->GetOutputDesc(0).GetDataType(), result_output[12]);
  EXPECT_EQ(g14_op->GetOutputDesc(0).GetDataType(), result_output[13]);
  EXPECT_EQ(g15_op->GetOutputDesc(0).GetDataType(), result_output[14]);
  EXPECT_EQ(g16_op->GetOutputDesc(0).GetDataType(), result_output[15]);
  EXPECT_EQ(g17_op->GetOutputDesc(0).GetDataType(), result_output[16]);
  EXPECT_EQ(g18_op->GetInputDesc(0).GetDataType(), DT_INT8);
}


TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_bf16_2)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GOnlyfp32");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GBlackOnlyBf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GWhiteOnlyBf16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "GOnlyBf16");
  OpDescPtr g5_op = std::make_shared<OpDesc>("G5", "GOnlyFp16");
  OpDescPtr g6_op = std::make_shared<OpDesc>("G6", "GBlackSupportFp32Bf16");
  OpDescPtr g7_op = std::make_shared<OpDesc>("G7", "GWhiteSupportFp32Bf16");
  OpDescPtr g8_op = std::make_shared<OpDesc>("G8", "GSupportFp32Bf16");
  OpDescPtr g9_op = std::make_shared<OpDesc>("G9", "GBlackSupportfp32Fp16");
  OpDescPtr g10_op = std::make_shared<OpDesc>("G10", "GWhiteSupportfp32Fp16");
  OpDescPtr g11_op = std::make_shared<OpDesc>("G11", "GSupportfp32Fp16");
  OpDescPtr g12_op = std::make_shared<OpDesc>("G12", "GBlackSupportBf16Fp16");
  OpDescPtr g13_op = std::make_shared<OpDesc>("G13", "GWhiteSupportBf16Fp16");
  OpDescPtr g14_op = std::make_shared<OpDesc>("G14", "GSupportBf16Fp16");
  OpDescPtr g15_op = std::make_shared<OpDesc>("G15", "GBlack");
  OpDescPtr g16_op = std::make_shared<OpDesc>("G16", "GWhite");
  OpDescPtr g17_op = std::make_shared<OpDesc>("G17", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT16;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  CreateSetOneNode(g5_op, dim, dtype);
  ge::NodePtr g5_node = graph->AddNode(g5_op);
  CreateSetOneNode(g6_op, dim, dtype);
  ge::NodePtr g6_node = graph->AddNode(g6_op);
  CreateSetOneNode(g7_op, dim, dtype);
  ge::NodePtr g7_node = graph->AddNode(g7_op);
  CreateSetOneNode(g8_op, dim, dtype);
  ge::NodePtr g8_node = graph->AddNode(g8_op);
  CreateSetOneNode(g9_op, dim, dtype);
  ge::NodePtr g9_node = graph->AddNode(g9_op);
  CreateSetOneNode(g10_op, dim, dtype);
  ge::NodePtr g10_node = graph->AddNode(g10_op);
  CreateSetOneNode(g11_op, dim, dtype);
  ge::NodePtr g11_node = graph->AddNode(g11_op);
  CreateSetOneNode(g12_op, dim, dtype);
  ge::NodePtr g12_node = graph->AddNode(g12_op);
  CreateSetOneNode(g13_op, dim, dtype);
  ge::NodePtr g13_node = graph->AddNode(g13_op);
  CreateSetOneNode(g14_op, dim, dtype);
  ge::NodePtr g14_node = graph->AddNode(g14_op);
  CreateSetOneNode(g15_op, dim, dtype);
  ge::NodePtr g15_node = graph->AddNode(g15_op);
  CreateSetOneNode(g16_op, dim, dtype);
  ge::NodePtr g16_node = graph->AddNode(g16_op);
  CreateSetOneNode(g17_op, dim, dtype);
  ge::NodePtr g17_node = graph->AddNode(g17_op);
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");
  Status ret5 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g5_node,  "tbe-custom");
  Status ret6 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g6_node,  "tbe-custom");
  Status ret7 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g7_node,  "tbe-custom");
  Status ret8 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g8_node,  "tbe-custom");
  Status ret9 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g9_node,  "tbe-custom");
  Status ret10 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g10_node,  "tbe-custom");
  Status ret11 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g11_node,  "tbe-custom");
  Status ret12 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g12_node,  "tbe-custom");
  Status ret13 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g13_node,  "tbe-custom");
  Status ret14 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g14_node,  "tbe-custom");
  Status ret15 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g15_node,  "tbe-custom");
  Status ret16 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g16_node,  "tbe-custom");
  Status ret17 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g17_node,  "tbe-custom");


  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);
  ASSERT_EQ(ret5, fe::SUCCESS);
  ASSERT_EQ(ret6, fe::SUCCESS);
  ASSERT_EQ(ret7, fe::SUCCESS);
  ASSERT_EQ(ret8, fe::SUCCESS);
  ASSERT_EQ(ret9, fe::SUCCESS);
  ASSERT_EQ(ret10, fe::SUCCESS);
  ASSERT_EQ(ret11, fe::SUCCESS);
  ASSERT_EQ(ret12, fe::SUCCESS);
  ASSERT_EQ(ret13, fe::SUCCESS);
  ASSERT_EQ(ret14, fe::SUCCESS);
  ASSERT_EQ(ret15, fe::SUCCESS);
  ASSERT_EQ(ret16, fe::SUCCESS);
  ASSERT_EQ(ret17, fe::SUCCESS);

  vector<ge::DataType> result_input = {DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_UINT8,
                                       DT_BF16, DT_BF16, DT_BF16, DT_FLOAT, DT_UINT8,
                                       DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_BF16,
                                       DT_BF16, DT_BF16};
  vector<ge::DataType> result_output = {DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_UINT8,
                                       DT_BF16, DT_BF16, DT_BF16, DT_FLOAT, DT_UINT8,
                                       DT_FLOAT, DT_BF16, DT_BF16, DT_BF16, DT_BF16,
                                       DT_BF16, DT_BF16};
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);
  EXPECT_EQ(g5_op->GetInputDesc(0).GetDataType(), result_input[4]);
  EXPECT_EQ(g6_op->GetInputDesc(0).GetDataType(), result_input[5]);
  EXPECT_EQ(g7_op->GetInputDesc(0).GetDataType(), result_input[6]);
  EXPECT_EQ(g8_op->GetInputDesc(0).GetDataType(), result_input[7]);
  EXPECT_EQ(g9_op->GetInputDesc(0).GetDataType(), result_input[8]);
  EXPECT_EQ(g10_op->GetInputDesc(0).GetDataType(), result_input[9]);
  EXPECT_EQ(g11_op->GetInputDesc(0).GetDataType(), result_input[10]);
  EXPECT_EQ(g12_op->GetInputDesc(0).GetDataType(), result_input[11]);
  EXPECT_EQ(g13_op->GetInputDesc(0).GetDataType(), result_input[12]);
  EXPECT_EQ(g14_op->GetInputDesc(0).GetDataType(), result_input[13]);
  EXPECT_EQ(g15_op->GetInputDesc(0).GetDataType(), result_input[14]);
  EXPECT_EQ(g16_op->GetInputDesc(0).GetDataType(), result_input[15]);
  EXPECT_EQ(g17_op->GetInputDesc(0).GetDataType(), result_input[16]);


  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
  EXPECT_EQ(g5_op->GetOutputDesc(0).GetDataType(), result_output[4]);
  EXPECT_EQ(g6_op->GetOutputDesc(0).GetDataType(), result_output[5]);
  EXPECT_EQ(g7_op->GetOutputDesc(0).GetDataType(), result_output[6]);
  EXPECT_EQ(g8_op->GetOutputDesc(0).GetDataType(), result_output[7]);
  EXPECT_EQ(g9_op->GetOutputDesc(0).GetDataType(), result_output[8]);
  EXPECT_EQ(g10_op->GetOutputDesc(0).GetDataType(), result_output[9]);
  EXPECT_EQ(g11_op->GetOutputDesc(0).GetDataType(), result_output[10]);
  EXPECT_EQ(g12_op->GetOutputDesc(0).GetDataType(), result_output[11]);
  EXPECT_EQ(g13_op->GetOutputDesc(0).GetDataType(), result_output[12]);
  EXPECT_EQ(g14_op->GetOutputDesc(0).GetDataType(), result_output[13]);
  EXPECT_EQ(g15_op->GetOutputDesc(0).GetDataType(), result_output[14]);
  EXPECT_EQ(g16_op->GetOutputDesc(0).GetDataType(), result_output[15]);
  EXPECT_EQ(g17_op->GetOutputDesc(0).GetDataType(), result_output[16]);
}

// test for net in gray, input fp32-> GWhite -> GBlackSupportFp32Bf16 -> GWhiteSupportBf16Fp16 -> G
// the fllows output is        fp32 -> bf16 ->  fp32                  -> bf16                  -> bf16
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_bf16_3)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g1_op = std::make_shared<OpDesc>("G1", "GWhite");
  OpDescPtr g2_op = std::make_shared<OpDesc>("G2", "GBlackSupportFp32Bf16");
  OpDescPtr g3_op = std::make_shared<OpDesc>("G3", "GWhiteSupportBf16Fp16");
  OpDescPtr g4_op = std::make_shared<OpDesc>("G4", "G");

  vector<int64_t> dim({4, 33, 12, 16});
  ge::DataType dtype = DT_FLOAT;
  //add descriptor
  CreateSetOneNode(g1_op, dim, dtype);
  ge::NodePtr g1_node = graph->AddNode(g1_op);
  CreateSetOneNode(g2_op, dim, dtype);
  ge::NodePtr g2_node = graph->AddNode(g2_op);
  GraphUtils::AddEdge(g1_node->GetOutDataAnchor(0), g2_node->GetInDataAnchor(0));
  CreateSetOneNode(g3_op, dim, dtype);
  ge::NodePtr g3_node = graph->AddNode(g3_op);
  GraphUtils::AddEdge(g2_node->GetOutDataAnchor(0), g3_node->GetInDataAnchor(0));
  CreateSetOneNode(g4_op, dim, dtype);
  ge::NodePtr g4_node = graph->AddNode(g4_op);
  GraphUtils::AddEdge(g3_node->GetOutDataAnchor(0), g4_node->GetInDataAnchor(0));
   
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g1_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g2_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g3_node,  "tbe-custom");
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g4_node,  "tbe-custom");

  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);
  ASSERT_EQ(ret4, fe::SUCCESS);


  vector<ge::DataType> result_input = { DT_BF16, DT_FLOAT, DT_BF16, DT_BF16 };
  vector<ge::DataType> result_output = { DT_BF16, DT_FLOAT, DT_BF16, DT_BF16 };
  EXPECT_EQ(g1_op->GetInputDesc(0).GetDataType(), result_input[0]);
  EXPECT_EQ(g2_op->GetInputDesc(0).GetDataType(), result_input[1]);
  EXPECT_EQ(g3_op->GetInputDesc(0).GetDataType(), result_input[2]);
  EXPECT_EQ(g4_op->GetInputDesc(0).GetDataType(), result_input[3]);

  EXPECT_EQ(g1_op->GetOutputDesc(0).GetDataType(), result_output[0]);
  EXPECT_EQ(g2_op->GetOutputDesc(0).GetDataType(), result_output[1]);
  EXPECT_EQ(g3_op->GetOutputDesc(0).GetDataType(), result_output[2]);
  EXPECT_EQ(g4_op->GetOutputDesc(0).GetDataType(), result_output[3]);
}

// test for net in gray, input fp32-> G     -> Cast -> GWhite -> Cast1 -> GWhite
// the fllows output is        fp32 -> bf16 ->  fp32                  -> bf16                  -> bf16
TEST_F(UTEST_fusion_engine_op_judge_precision_mode, set_nodes_IO_dtypes_mix_bf16_4)
{
  Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION_BF16;
  op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME, op_store_adapter_manager_ptr_, reflection_builder_ptr_);
  op_format_dtype_judge_ptr_->Initialize();
  /* Stub Cast in Black list */
  SubOpInfoStorePtr subOpInfoStorePtr = OpsKernelManager::Instance(AI_CORE_NAME).GetSubOpsKernelByStoreName("tbe-custom");
  OpKernelInfoPtr opKernelInfoPtr = subOpInfoStorePtr->GetOpKernelByOpType(fe::CAST);
  opKernelInfoPtr->op_store_info_.precision_policy = BLACK;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr g_op = std::make_shared<OpDesc>("G1", "G");
  OpDescPtr cast_op = std::make_shared<OpDesc>("Cast", "Cast");
  OpDescPtr h_op = std::make_shared<OpDesc>("G2", "GWhite");
  OpDescPtr cast1_op = std::make_shared<OpDesc>("Cast1", "Cast");
  OpDescPtr u_op = std::make_shared<OpDesc>("G3", "GWhite");

  //add descriptor
  vector<int64_t> dim({4, 33, 12, 16});
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCHW);
  tensor_desc.SetFormat(FORMAT_NCHW);
  tensor_desc.SetDataType(DT_BF16);
  g_op->AddInputDesc("x", tensor_desc);
  g_op->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(g_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr g_node = graph->AddNode(g_op);

  vector<int64_t> dim_cast({4, 33, 12, 16});
  GeShape shape_cast(dim);
  GeTensorDesc tensor_desc_cast(shape_cast);
  tensor_desc_cast.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_cast.SetFormat(FORMAT_NCHW);
  tensor_desc_cast.SetDataType(DT_FLOAT);
  cast_op->AddInputDesc("x", tensor_desc);
  cast_op->AddOutputDesc("z", tensor_desc_cast);
  ge::AttrUtils::SetInt(cast_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr cast_node = graph->AddNode(cast_op);

  vector<int64_t> dim_h({1, 2, 3, 4});
  GeShape shape_h(dim_h);
  GeTensorDesc tensor_desc_h(shape_h);
  tensor_desc_h.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_h.SetFormat(FORMAT_NCHW);
  tensor_desc_h.SetDataType(DT_FLOAT);
  h_op->AddInputDesc("x", tensor_desc_h);
  h_op->AddOutputDesc("z", tensor_desc_h);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr h_node = graph->AddNode(h_op);

  vector<int64_t> dim_cast1({1, 2, 3, 4});
  GeShape shape_cast1(dim_cast1);
  GeTensorDesc tensor_desc_cast1(shape_cast1);
  tensor_desc_cast1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_cast1.SetFormat(FORMAT_NCHW);
  tensor_desc_cast1.SetDataType(DT_BF16);
  cast1_op->AddInputDesc("x", tensor_desc_cast1);
  cast1_op->AddOutputDesc("z", tensor_desc_cast1);
  ge::AttrUtils::SetInt(h_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr cast1_node = graph->AddNode(cast1_op);

  vector<int64_t> dim_u({1, 2, 3, 4});
  GeShape shape_u(dim_u);
  GeTensorDesc tensor_desc_u(shape_u);
  tensor_desc_u.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_u.SetFormat(FORMAT_NCHW);
  tensor_desc_u.SetDataType(DT_FLOAT);
  u_op->AddInputDesc("x", tensor_desc_u);
  u_op->AddOutputDesc("z", tensor_desc_u);
  ge::AttrUtils::SetInt(u_op, FE_IMPLY_TYPE, 6);
  ge::NodePtr u_node = graph->AddNode(u_op);
  GraphUtils::AddEdge(g_node->GetOutDataAnchor(0), cast_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(cast_node->GetOutDataAnchor(0), h_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(h_node->GetOutDataAnchor(0), cast1_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(cast1_node->GetOutDataAnchor(0), u_node->GetInDataAnchor(0));
  

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(g_node,  "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(h_node,  "tbe-custom");
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(u_node,  "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);
  ASSERT_EQ(ret3, fe::SUCCESS);

  EXPECT_EQ(g_op->GetInputDesc(0).GetDataType(), DT_BF16);
  EXPECT_EQ(g_op->GetOutputDesc(0).GetDataType(), DT_BF16);

  EXPECT_EQ(cast_op->GetInputDesc(0).GetDataType(), DT_BF16);
  EXPECT_EQ(cast_op->GetOutputDesc(0).GetDataType(), DT_FLOAT);

  EXPECT_EQ(h_op->GetInputDesc(0).GetDataType(), DT_BF16);
  EXPECT_EQ(h_op->GetOutputDesc(0).GetDataType(), DT_BF16);

  EXPECT_EQ(cast1_op->GetInputDesc(0).GetDataType(), DT_BF16);
  EXPECT_EQ(cast1_op->GetOutputDesc(0).GetDataType(), DT_BF16);

  EXPECT_EQ(u_op->GetInputDesc(0).GetDataType(), DT_BF16);
  EXPECT_EQ(u_op->GetOutputDesc(0).GetDataType(), DT_BF16);
}


