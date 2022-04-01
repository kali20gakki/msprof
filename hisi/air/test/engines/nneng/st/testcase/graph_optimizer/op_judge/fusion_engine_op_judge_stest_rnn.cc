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

class STEST_fusion_engine_op_judge_unittest_Fz_RNN : public testing::Test {
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
  }

  void TearDown() {
  }
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  RefRelationsPtr reflection_builder_ptr_;
  OpFormatDtypeJudgePtr op_format_dtype_judge_ptr_;
};


TEST_F(STEST_fusion_engine_op_judge_unittest_Fz_RNN, test_01){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("rnn_op1", "RNN_OP1");
  //add descriptor
  vector<int64_t> dim_input({64, 1024});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_ND);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_ND);
  tensor_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x1", tensor_desc);

  /*1, 1, i+h, 4*h
   * h = 562, i = 672*/
  vector<int64_t> dim_weight({1024});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_ND);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_ND);
  weight_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x2", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetInt(op1, "hidden_size", 32);
  ge::AttrUtils::SetInt(op1, "input_size", 32);
  ge::NodePtr node1 = graph->AddNode(op1);


  Status ret1 = op_format_dtype_judge_ptr_->SetDtypeAndFormatByPrecisionMode(node1, "tbe-custom");
  ASSERT_EQ(ret1, fe::SUCCESS);

  vector<int64_t> dim_resultfz_rnn({4, 64, 16, 16});
  vector<int64_t> dim_result_nd_rnn_bias({1024});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_FRACTAL_ZN_RNN);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_resultfz_rnn);

  EXPECT_EQ(op1->GetInputDesc(1).GetFormat(), FORMAT_ND_RNN_BIAS);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_nd_rnn_bias);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_FRACTAL_ZN_RNN);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_resultfz_rnn);

}

TEST_F(STEST_fusion_engine_op_judge_unittest_Fz_RNN, test_02){
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr op1 = std::make_shared<OpDesc>("rnn_op1", "RNN_OP1");
  //add descriptor
  vector<int64_t> dim_input({64, 1024});
  GeShape shape(dim_input);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetOriginFormat(FORMAT_ND);
  tensor_desc.SetOriginShape(shape);
  tensor_desc.SetFormat(FORMAT_ND);
  tensor_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x1", tensor_desc);

  /*1, 1, i+h, 4*h
   * h = 562, i = 672*/
  vector<int64_t> dim_weight({1024});
  GeShape weight_shape(dim_weight);
  GeTensorDesc weight_desc(weight_shape);
  weight_desc.SetOriginFormat(FORMAT_ND);
  weight_desc.SetOriginShape(weight_shape);
  weight_desc.SetFormat(FORMAT_ND);
  weight_desc.SetDataType(DT_FLOAT);
  op1->AddInputDesc("x2", weight_desc);

  op1->AddOutputDesc("z", tensor_desc);
  ge::AttrUtils::SetInt(op1, FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetInt(op1, "hidden_size", 32);
  ge::AttrUtils::SetInt(op1, "input_size", 32);
  ge::NodePtr node1 = graph->AddNode(op1);


  Status ret1 = op_format_dtype_judge_ptr_->JudgeByNode(node1);
  ASSERT_EQ(ret1, fe::SUCCESS);

  vector<int64_t> dim_resultfz_rnn({4, 64, 16, 16});
  vector<int64_t> dim_result_nd_rnn_bias({1024});
  EXPECT_EQ(op1->GetInputDesc(0).GetFormat(), FORMAT_FRACTAL_ZN_RNN);
  EXPECT_EQ(op1->GetInputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(0).GetShape().GetDims(), dim_resultfz_rnn);

  EXPECT_EQ(op1->GetInputDesc(1).GetFormat(), FORMAT_ND_RNN_BIAS);
  EXPECT_EQ(op1->GetInputDesc(1).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetInputDesc(1).GetShape().GetDims(), dim_result_nd_rnn_bias);

  EXPECT_EQ(op1->GetOutputDesc(0).GetFormat(), FORMAT_FRACTAL_ZN_RNN);
  EXPECT_EQ(op1->GetOutputDesc(0).GetDataType(), DT_FLOAT16);
  EXPECT_EQ(op1->GetOutputDesc(0).GetShape().GetDims(), dim_resultfz_rnn);

}
