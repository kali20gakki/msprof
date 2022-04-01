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
#include "common/aicore_util_attr_define.h"
#include "common/unknown_shape_util.h"

#define private public
#define protected public
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
#include "ops_store/ops_kernel_manager.h"
using namespace std;
using namespace ge;
using namespace fe;
#define DIMENSION_4 (4)
#define DIMENSION_1 (1)
using OpImplTypeJudgePtr = std::shared_ptr<OpImplTypeJudge>;
using OpFormatDtypeJudgePtr = std::shared_ptr<OpFormatDtypeJudge>;
using OpDtypeRiseMatcherPtr = std::shared_ptr<OpDtypeRiseMatcher>;
using OpFormatMatcherPtr = std::shared_ptr<OpFormatMatcher>;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using TransNodeManagerPtr = std::shared_ptr<TransNodeManager>;

class UTEST_fusion_engine_op_judge_unittest_conv3d : public testing::Test {
 protected:

  void SetUp() {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(
        std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(
        op_store_adapter_manager_ptr_);

    FEOpsStoreInfo tbe_custom{
        6,
        "tbe-custom",
        EN_IMPL_HW_TBE,
        "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/conv3d",
        ""};
    vector<FEOpsStoreInfo> store_info;

    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    Configuration::Instance(fe::AI_CORE_NAME).precision_mode_ = ALLOW_MIX_PRECISION;
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr_->Initialize(options);
    reflection_builder_ptr_ = std::make_shared<ge::RefRelations>();
    op_format_dtype_judge_ptr_ = std::make_shared<OpFormatDtypeJudge>(AI_CORE_NAME,op_store_adapter_manager_ptr_,reflection_builder_ptr_);
    op_format_dtype_judge_ptr_->Initialize();
  }

  void TearDown() {

  }
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  RefRelationsPtr reflection_builder_ptr_;
  OpFormatDtypeJudgePtr op_format_dtype_judge_ptr_;
};


TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_01){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, 33, 12, 16, 64});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, 3, 3, 4, 5});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_NDHWC);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_NDHWC);
  weight_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, 33, 4, 12, 16, 16});
  vector<int64_t> dim_result_fz({36, 1, 16, 16});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);

  EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_02){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3DTranspose", "Conv3DTranspose");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, 33, 12, 16, 64});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, 3, 3, 4, 5});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_DHWNC);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_DHWNC);
  weight_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, 33, 4, 12, 16, 16});
  vector<int64_t> dim_result_fz({27, 1, 16, 16});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);

  EXPECT_EQ(op1->GetInputDesc(1).GetFormat(), FORMAT_FRACTAL_Z_3D_TRANSPOSE);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
}


TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_03){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, 33, 12, 16, 64});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, 3, 3, 4, 5});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_DHWCN);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_DHWCN);
  weight_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, 33, 4, 12, 16, 16});
  vector<int64_t> dim_result_fz({27, 1, 16, 16});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);

  EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
}

// x: NCDHW, filter:NCDHW, y:NCDHW
TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_04){
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
    OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
    OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
    //add descriptor
    vector<int64_t> dim_input({4, 33, 12, 16, 64});
    GeShape shape(dim_input);
    GeTensorDesc tensor_desc(shape);
    tensor_desc.SetOriginFormat(FORMAT_NCDHW);
    tensor_desc.SetOriginShape(shape);
    tensor_desc.SetFormat(FORMAT_NCDHW);
    tensor_desc.SetDataType(DT_FLOAT);
    op1->AddInputDesc("x", tensor_desc);

    vector<int64_t> dim_weight({3, 3, 3, 4, 5});
    GeShape weight_shape(dim_weight);
    GeTensorDesc weight_desc(weight_shape);
    weight_desc.SetOriginFormat(FORMAT_NCDHW);
    weight_desc.SetOriginShape(weight_shape);
    weight_desc.SetFormat(FORMAT_NCDHW);
    weight_desc.SetDataType(DT_FLOAT);
    op1->AddInputDesc("y", weight_desc);

    op1->AddOutputDesc("z", tensor_desc);
    ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
    ge::NodePtr node1 = graph->AddNode(op1);

    op2->AddInputDesc("x", tensor_desc);
    op2->AddOutputDesc("z", tensor_desc);
    ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
    ge::NodePtr node2 = graph->AddNode(op2);
    GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

    Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
    Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
    ASSERT_EQ(ret1, fe::SUCCESS);
    ASSERT_EQ(ret2, fe::SUCCESS);

    vector<int64_t> dim_result6_d({4, 12, 3, 16, 64, 16});
    vector<int64_t> dim_result_fz({60, 1, 16, 16});
    EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
    EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);

    EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
    EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);

    EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
    EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);

    EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NCDHW);
    EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
    EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NCDHW);
    EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
}

// x: NDCHW, filter:NDCHW, y:NDCHW
TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_05){
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
    OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
    OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
    //add descriptor
    vector<int64_t> dim_input({4, 33, 12, 16, 64});
    GeShape shape(dim_input);
    GeTensorDesc tensor_desc(shape);
    tensor_desc.SetOriginFormat(FORMAT_NDHWC);
    tensor_desc.SetOriginShape(shape);
    tensor_desc.SetFormat(FORMAT_NDHWC);
    tensor_desc.SetDataType(DT_FLOAT);
    op1->AddInputDesc("x", tensor_desc);

    vector<int64_t> dim_weight({3, 3, 3, 4, 5});
    GeShape weight_shape(dim_weight);
    GeTensorDesc weight_desc(weight_shape);
    weight_desc.SetOriginFormat(FORMAT_NDHWC);
    weight_desc.SetOriginShape(weight_shape);
    weight_desc.SetFormat(FORMAT_NDHWC);
    weight_desc.SetDataType(DT_FLOAT);
    op1->AddInputDesc("y", weight_desc);

    op1->AddOutputDesc("z", tensor_desc);
    ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
    ge::NodePtr node1 = graph->AddNode(op1);

    op2->AddInputDesc("x", tensor_desc);
    op2->AddOutputDesc("z", tensor_desc);
    ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
    ge::NodePtr node2 = graph->AddNode(op2);
    GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

    Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
    Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
    ASSERT_EQ(ret1, fe::SUCCESS);
    ASSERT_EQ(ret2, fe::SUCCESS);

    vector<int64_t> dim_result6_d({4, 33, 4, 12, 16, 16});
    vector<int64_t> dim_result_fz({36, 1, 16, 16});
    EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
    EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);

    EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
    EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);

    EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
    EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);

    EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
    EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
    EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
    EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
    EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_expand_dim_NDHWC){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("b1", "B");
  OpDescPtr op2 = std::make_shared<OpDesc>("b2", "B");
  OpDescPtr op3 = std::make_shared<OpDesc>("b3", "B");
  OpDescPtr op4 = std::make_shared<OpDesc>("b4", "B");

  vector<int64_t> dim_input({44});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x", tensor_desc);
  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_1 = graph->AddNode(op1);

  vector<int64_t> dim_input_2({44, 55});
  GeShape shape_2(dim_input_2);
  GeTensorDesc tensor_desc_2(shape_2);
  tensor_desc_2.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc_2.SetOriginShape(shape);
  tensor_desc_2.SetFormat(FORMAT_NDHWC);
  tensor_desc_2.SetDataType(DT_FLOAT);
  op2->AddInputDesc("x", tensor_desc_2);
  op2->AddOutputDesc("z", tensor_desc_2);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_2 = graph->AddNode(op2);

  vector<int64_t> dim_input_3({44, 55, 66});
  GeShape shape_3(dim_input_3);
  GeTensorDesc tensor_desc_3(shape_3);
  tensor_desc_3.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc_3.SetOriginShape(shape);
  tensor_desc_3.SetFormat(FORMAT_NDHWC);
  tensor_desc_3.SetDataType(DT_FLOAT);
  op3->AddInputDesc("x", tensor_desc_3);
  op3->AddOutputDesc("z", tensor_desc_3);
  ge::AttrUtils::SetInt(op3, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_3 = graph->AddNode(op3);

  vector<int64_t> dim_input_4({44, 55, 66, 77});
  GeShape shape_4(dim_input_4);
  GeTensorDesc tensor_desc_4(shape_4);
  tensor_desc_4.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc_4.SetOriginShape(shape);
  tensor_desc_4.SetFormat(FORMAT_NDHWC);
  tensor_desc_4.SetDataType(DT_FLOAT);
  op4->AddInputDesc("x", tensor_desc_4);
  op4->AddOutputDesc("z", tensor_desc_4);
  ge::AttrUtils::SetInt(op4, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_4 = graph->AddNode(op4);

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_1, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_3, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_4, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);

  vector<int64_t> dim_result6_d_1({1, 1, 3, 1, 1, 16});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_1);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_1);

  vector<int64_t> dim_result6_d_2({1, 1, 4, 1, 44, 16});
  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_2);

  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_2);

  vector<int64_t> dim_result6_d_3({1, 1, 5, 44, 55, 16});
  EXPECT_EQ(op3->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op3->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op3->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_3);

  EXPECT_EQ(op3->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op3->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op3->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_3);

  vector<int64_t> dim_result6_d_4({1, 44, 5, 55, 66, 16});
  EXPECT_EQ(op4->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op4->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op4->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_4);

  EXPECT_EQ(op4->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op4->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op4->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_4);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_expand_dim_NCDHW){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("b1", "B");
  OpDescPtr op2 = std::make_shared<OpDesc>("b2", "B");
  OpDescPtr op3 = std::make_shared<OpDesc>("b3", "B");
  OpDescPtr op4 = std::make_shared<OpDesc>("b4", "B");

  vector<int64_t> dim_input({44});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCDHW);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NCDHW);
  tensor_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x", tensor_desc);
  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_1 = graph->AddNode(op1);

  vector<int64_t> dim_input_2({44, 55});
  GeShape shape_2(dim_input_2);
  GeTensorDesc tensor_desc_2(shape_2);
  tensor_desc_2.SetOriginFormat(FORMAT_NCDHW);
  tensor_desc_2.SetOriginShape(shape);
  tensor_desc_2.SetFormat(FORMAT_NCDHW);
  tensor_desc_2.SetDataType(DT_FLOAT);
  op2->AddInputDesc("x", tensor_desc_2);
  op2->AddOutputDesc("z", tensor_desc_2);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_2 = graph->AddNode(op2);

  vector<int64_t> dim_input_3({44, 55, 66});
  GeShape shape_3(dim_input_3);
  GeTensorDesc tensor_desc_3(shape_3);
  tensor_desc_3.SetOriginFormat(FORMAT_NCDHW);
  tensor_desc_3.SetOriginShape(shape);
  tensor_desc_3.SetFormat(FORMAT_NCDHW);
  tensor_desc_3.SetDataType(DT_FLOAT);
  op3->AddInputDesc("x", tensor_desc_3);
  op3->AddOutputDesc("z", tensor_desc_3);
  ge::AttrUtils::SetInt(op3, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_3 = graph->AddNode(op3);

  vector<int64_t> dim_input_4({44, 55, 66, 77});
  GeShape shape_4(dim_input_4);
  GeTensorDesc tensor_desc_4(shape_4);
  tensor_desc_4.SetOriginFormat(FORMAT_NCDHW);
  tensor_desc_4.SetOriginShape(shape);
  tensor_desc_4.SetFormat(FORMAT_NCDHW);
  tensor_desc_4.SetDataType(DT_FLOAT);
  op4->AddInputDesc("x", tensor_desc_4);
  op4->AddOutputDesc("z", tensor_desc_4);
  ge::AttrUtils::SetInt(op4, FE_IMPLY_TYPE, 6);
  ge::NodePtr node_4 = graph->AddNode(op4);

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_1, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  Status ret3 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_3, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  Status ret4 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node_4, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);

  vector<int64_t> dim_result6_d_1({1, 1, 3, 1, 1, 16});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_1);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_1);

  vector<int64_t> dim_result6_d_2({1, 1, 1, 44, 55, 16});
  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_2);

  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_2);

  vector<int64_t> dim_result6_d_3({1, 44, 1, 55, 66, 16});
  EXPECT_EQ(op3->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op3->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op3->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_3);

  EXPECT_EQ(op3->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op3->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op3->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_3);

  vector<int64_t> dim_result6_d_4({1, 55, 3, 66, 77, 16});
  EXPECT_EQ(op4->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op4->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op4->GetInputDesc(0).GetShape().GetDims(), dim_result6_d_4);

  EXPECT_EQ(op4->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op4->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op4->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d_4);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_unknown_shape_01){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range1({{1, 33}, {1, 12}, {1, 16}, {1, 64}});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  tensor_desc.SetShapeRange(range1);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range2({{1, 3}, {1, 3}, {1, 4}, {1, 5}});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_NDHWC);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_NDHWC);
  weight_desc.SetDataType(DT_FLOAT);
  weight_desc.SetShapeRange(range2);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, -1, -1, -1, -1, 16});
  vector<int64_t> dim_result_fz({-1, 1, 16, 16});
  vector<std::pair<int64_t, int64_t>> range_ex1({{4, 4}, {1, 33}, {1, 4}, {1, 12}, {1, 16}, {16, 16}});
  vector<std::pair<int64_t, int64_t>> range_ex2({{1, 36}, {1, 1}, {16, 16}, {16, 16}});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(0)), range_ex1);

  EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(1)), range_ex2);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetOutputDesc(0)), range_ex1);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetInputDesc(0)), range1);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetOutputDesc(0)), range1);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_unknown_shape_02){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3DTranspose", "Conv3DTranspose");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range1({{1, 33}, {1, 12}, {1, 16}, {1, 64}});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  tensor_desc.SetShapeRange(range1);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range2({{1, 3}, {1, 3}, {1, 4}, {1, 5}});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_DHWNC);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_DHWNC);
  weight_desc.SetDataType(DT_FLOAT);
  weight_desc.SetShapeRange(range2);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, -1, -1, -1, -1, 16});
  vector<int64_t> dim_result_fz({-1, -1, 16, 16});
  vector<std::pair<int64_t, int64_t>> range_ex1({{4, 4}, {1, 33}, {1, 4}, {1, 12}, {1, 16}, {16, 16}});
  vector<std::pair<int64_t, int64_t>> range_ex2({{3, 27}, {1, 1}, {16, 16}, {16, 16}});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(0)), range_ex1);

  EXPECT_EQ(op1->GetInputDesc(1).GetFormat(), FORMAT_FRACTAL_Z_3D_TRANSPOSE);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(1)), range_ex2);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetOutputDesc(0)), range_ex1);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetInputDesc(0)), range1);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetOutputDesc(0)), range1);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_unknown_shape_03){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range1({{1, 33}, {1, 12}, {1, 16}, {1, 64}});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  tensor_desc.SetShapeRange(range1);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range2({{1, 3}, {1, 3}, {1, 4}, {1, 5}});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_DHWCN);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_DHWCN);
  weight_desc.SetDataType(DT_FLOAT);
  weight_desc.SetShapeRange(range2);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, -1, -1, -1, -1, 16});
  vector<int64_t> dim_result_fz({-1, -1, 16, 16});
  vector<std::pair<int64_t, int64_t>> range_ex1({{4, 4}, {1, 33}, {1, 4}, {1, 12}, {1, 16}, {16, 16}});
  vector<std::pair<int64_t, int64_t>> range_ex2({{3, 27}, {1, 1}, {16, 16}, {16, 16}});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(0)), range_ex1);

  EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(1)), range_ex2);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetOutputDesc(0)), range_ex1);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetInputDesc(0)), range1);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetOutputDesc(0)), range1);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_unknown_shape_04){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range1({{1, 33}, {1, 12}, {1, 16}, {1, 64}});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NCDHW);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NCDHW);
  tensor_desc.SetDataType(DT_FLOAT);
  tensor_desc.SetShapeRange(range1);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range2({{1, 3}, {1, 3}, {1, 4}, {1, 5}});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_NCDHW);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_NCDHW);
  weight_desc.SetDataType(DT_FLOAT);
  weight_desc.SetShapeRange(range2);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  
  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, -1, -1, -1, -1, 16});
  vector<int64_t> dim_result_fz({-1, 1, 16, 16});
  vector<std::pair<int64_t, int64_t>> range_ex1({{4, 4}, {1, 12}, {1, 3}, {1, 16}, {1, 64}, {16, 16}});
  vector<std::pair<int64_t, int64_t>> range_ex2({{1, 60}, {1, 1}, {16, 16}, {16, 16}});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(0)), range_ex1);

  EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(1)), range_ex2);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetOutputDesc(0)), range_ex1);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NCDHW);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetInputDesc(0)), range1);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NCDHW);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetOutputDesc(0)), range1);
}

TEST_F(UTEST_fusion_engine_op_judge_unittest_conv3d, test_unknown_shape_05){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("Conv3D", "Conv3D");
  OpDescPtr op2 = std::make_shared<OpDesc>("A1", "A");
  //add descriptor
  vector<int64_t> dim_input({4, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range1({{1, 33}, {1, 12}, {1, 16}, {1, 64}});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_NDHWC);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_NDHWC);
  tensor_desc.SetDataType(DT_FLOAT);
  tensor_desc.SetShapeRange(range1);
  op1->AddInputDesc("x", tensor_desc);

  vector<int64_t> dim_weight({3, -1, -1, -1, -1});
  vector<std::pair<int64_t, int64_t>> range2({{1, 3}, {1, 3}, {1, 4}, {1, 5}});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_NDHWC);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_NDHWC);
  weight_desc.SetDataType(DT_FLOAT);
  weight_desc.SetShapeRange(range2);
  op1->AddInputDesc("y", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::NodePtr node1 = graph->AddNode(op1);

  op2->AddInputDesc("x", tensor_desc);
  op2->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op2, FE_IMPLY_TYPE, 6);
  ge::NodePtr node2 = graph->AddNode(op2);
  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));

  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  Status ret2 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node2, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);
  ASSERT_EQ(ret2, fe::SUCCESS);

  vector<int64_t> dim_result6_d({4, -1, -1, -1, -1, 16});
  vector<int64_t> dim_result_fz({-1, 1, 16, 16});
  vector<std::pair<int64_t, int64_t>> range_ex1({{4, 4}, {1, 33}, {1, 4}, {1, 12}, {1, 16}, {16, 16}});
  vector<std::pair<int64_t, int64_t>> range_ex2({{1, 36}, {1, 1}, {16, 16}, {16, 16}});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(0)), range_ex1);

  EXPECT_EQ(ge::GetPrimaryFormat(op1->GetInputDesc(1).GetFormat()), FORMAT_FRACTAL_Z_3D);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_fz);
  EXPECT_EQ(GetShapeRange(op1->GetInputDesc(1)), range_ex2);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_NDC1HWC0);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_result6_d);
  EXPECT_EQ(GetShapeRange(op1->GetOutputDesc(0)), range_ex1);

  EXPECT_EQ(op2->GetInputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetInputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetInputDesc(0)), range1);
  EXPECT_EQ(op2->GetOutputDesc(0).GetFormat(), FORMAT_NDHWC);
  EXPECT_EQ(op2->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op2->GetOutputDesc(0).GetShape().GetDims(), dim_input);
  EXPECT_EQ(GetShapeRange(op2->GetOutputDesc(0)), range1);
}
