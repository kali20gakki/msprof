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

#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#define protected public
#define private   public
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_cast_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_reshape_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transpose_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdata_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdatarnn_generator.h"
#include "common/configuration.h"
#include "ops_store/ops_kernel_manager.h"
#undef protected
#undef private

#include <iostream>

using namespace std;
using namespace ge;
using namespace fe;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

class UTEST_FE_TRANSOP_INSERT_RNN : public testing::Test {
protected:
  void SetUp()
  {
    std::map<std::string, std::string> options;
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo tbe_custom {
            6,
            "tbe-custom",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr_->Initialize(options);
  }

  void TearDown()
  {
    fe_ops_kernel_info_store_ptr_->Finalize();
  }

  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
protected:

};

Status QueryHighPrioOpImplTypeStubTbe_RNN(FEOpsKernelInfoStore* This, const ge::OpDescPtr& op_desc_ptr, OpImplType &impl_type) {
  impl_type = EN_IMPL_HW_TBE;
  return fe::SUCCESS;
}


/* ND(fp16) -> FORMAT_FRACTAL_ZN_RNN(fp16)
 * The Program will insert TransdataRNN ops.
 * TransdataRNN is Tbe Op
 * shape is (k, n)
 * k_num = 1 : k == hidden_size || k == input_size */
TEST_F(UTEST_FE_TRANSOP_INSERT_RNN, InsertAllTransop_01_0) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  // weight op
  OpDescPtr src_op1 = std::make_shared<OpDesc>("weight", "RNN_CONST");
  std::vector<int64_t> NDShape = {16, 1024};
  GeTensorDesc src_tensor_desc1(GeShape(NDShape), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc1.SetOriginShape(GeShape(NDShape));
  src_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  src_op1->AddOutputDesc(src_tensor_desc1);
  src_op1->AddInputDesc(src_tensor_desc1);
  auto src_node1 = graph->AddNode(src_op1);
  ge::AttrUtils::SetInt(src_op1, FE_IMPLY_TYPE, 6);
  // bias op
  OpDescPtr src_op2 = std::make_shared<OpDesc>("bias", "RNN_CONST");
  GeTensorDesc src_tensor_desc2(GeShape({1024}), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc2.SetOriginShape(GeShape({1024}));
  src_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  src_op2->AddInputDesc(src_tensor_desc2);
  src_op2->AddOutputDesc(src_tensor_desc2);
  auto src_node2 = graph->AddNode(src_op2);
  ge::AttrUtils::SetInt(src_op2, FE_IMPLY_TYPE, 6);
  // RNN op
  OpDescPtr dst_op = std::make_shared<OpDesc>("rnn", "RNN_OP");
  GeTensorDesc dst_tensor_desc1(GeShape({1,64,16,16}), ge::FORMAT_FRACTAL_ZN_RNN, ge::DT_FLOAT16);
  dst_tensor_desc1.SetOriginShape(GeShape(NDShape));
  dst_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  GeTensorDesc dst_tensor_desc2(GeShape({1024}), ge::FORMAT_ND_RNN_BIAS, ge::DT_FLOAT16);
  dst_tensor_desc2.SetOriginShape(GeShape({1024}));
  dst_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  dst_op->AddInputDesc(dst_tensor_desc1);
  dst_op->AddInputDesc(dst_tensor_desc2);
  dst_op->AddOutputDesc(dst_tensor_desc1);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetInt(dst_op, "hidden_size", 16);
  ge::AttrUtils::SetInt(dst_op, "input_size", 16);
  vector<bool> input_const_vector = {true, true};
  dst_op->SetIsInputConst(input_const_vector);

  GraphUtils::AddEdge(src_node1->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(src_node2->GetOutDataAnchor(0), dst_node->GetInDataAnchor(1));
  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));

  int count_node = 0;
  int rnn_node = 0;
  int64_t hidden_size;
  int64_t input_size;
  ASSERT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransDataRNN") {
      if (node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat() == ge::FORMAT_FRACTAL_ZN_RNN) {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 4);
        EXPECT_EQ(shape.GetDims()[0], 1);
        EXPECT_EQ(shape.GetDims()[1], 64);
        EXPECT_EQ(shape.GetDims()[2], 16);
        EXPECT_EQ(shape.GetDims()[3], 16);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 16);
        EXPECT_EQ(input_size, 16);
      } else {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 1);
        EXPECT_EQ(shape.GetDims()[0], 1024);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 16);
        EXPECT_EQ(input_size, 16);
      }
      rnn_node++;
    }
  }
  EXPECT_EQ(rnn_node, 2);
  EXPECT_EQ(count_node, 5);
}

/* ND(fp16) -> FORMAT_FRACTAL_ZN_RNN(fp16)
 * The Program will insert TransdataRNN ops.
 * TransdataRNN is Tbe Op
 * shape is (k, n)
 * k_num = 2 : k == hidden_size + input_size */
TEST_F(UTEST_FE_TRANSOP_INSERT_RNN, InsertAllTransop_01_1) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  // weight op
  OpDescPtr src_op1 = std::make_shared<OpDesc>("weight", "RNN_CONST");
  std::vector<int64_t> NDShape = {16, 1024};
  GeTensorDesc src_tensor_desc1(GeShape(NDShape), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc1.SetOriginShape(GeShape(NDShape));
  src_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  src_op1->AddOutputDesc(src_tensor_desc1);
  src_op1->AddInputDesc(src_tensor_desc1);
  auto src_node1 = graph->AddNode(src_op1);
  ge::AttrUtils::SetInt(src_op1, FE_IMPLY_TYPE, 6);
  // bias op
  OpDescPtr src_op2 = std::make_shared<OpDesc>("bias", "RNN_CONST");
  GeTensorDesc src_tensor_desc2(GeShape({1024}), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc2.SetOriginShape(GeShape({1024}));
  src_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  src_op2->AddInputDesc(src_tensor_desc2);
  src_op2->AddOutputDesc(src_tensor_desc2);
  auto src_node2 = graph->AddNode(src_op2);
  ge::AttrUtils::SetInt(src_op2, FE_IMPLY_TYPE, 6);
  // RNN op
  OpDescPtr dst_op = std::make_shared<OpDesc>("rnn", "RNN_OP");
  GeTensorDesc dst_tensor_desc1(GeShape({1,2,16,16}), ge::FORMAT_FRACTAL_ZN_RNN, ge::DT_FLOAT16);
  dst_tensor_desc1.SetOriginShape(GeShape(NDShape));
  dst_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  GeTensorDesc dst_tensor_desc2(GeShape({2048}), ge::FORMAT_ND_RNN_BIAS, ge::DT_FLOAT16);
  dst_tensor_desc2.SetOriginShape(GeShape({1024}));
  dst_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  dst_op->AddInputDesc(dst_tensor_desc1);
  dst_op->AddInputDesc(dst_tensor_desc2);
  dst_op->AddOutputDesc(dst_tensor_desc1);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetInt(dst_op, "hidden_size", 8);
  ge::AttrUtils::SetInt(dst_op, "input_size", 8);
  vector<bool> input_const_vector = {true, true};
  dst_op->SetIsInputConst(input_const_vector);

  GraphUtils::AddEdge(src_node1->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(src_node2->GetOutDataAnchor(0), dst_node->GetInDataAnchor(1));
  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));

  int count_node = 0;
  int rnn_node = 0;
  int64_t hidden_size;
  int64_t input_size;
  ASSERT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransDataRNN") {
      if (node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat() == ge::FORMAT_FRACTAL_ZN_RNN) {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 4);
        EXPECT_EQ(shape.GetDims()[0], 2);
        EXPECT_EQ(shape.GetDims()[1], 128);
        EXPECT_EQ(shape.GetDims()[2], 16);
        EXPECT_EQ(shape.GetDims()[3], 16);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 8);
        EXPECT_EQ(input_size, 8);
      } else {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 1);
        EXPECT_EQ(shape.GetDims()[0], 2048);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 8);
        EXPECT_EQ(input_size, 8);
      }
      rnn_node++;
    }
  }
  EXPECT_EQ(rnn_node, 2);
  EXPECT_EQ(count_node, 5);
}

/* ND(fp16) -> FORMAT_FRACTAL_ZN_RNN(fp16) -> ND(FP16)
 * The Program will insert TransdataRNN ops.
 * TransdataRNN is Tbe Op
 * shape is (k, n)
 * k_num = 2 : k == hidden_size + input_size */
TEST_F(UTEST_FE_TRANSOP_INSERT_RNN, InsertAllTransop_01_2) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  // weight op
  OpDescPtr src_op1 = std::make_shared<OpDesc>("weight", "RNN_CONST");
  std::vector<int64_t> NDShape = {16, 1024};
  GeTensorDesc src_tensor_desc1(GeShape(NDShape), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc1.SetOriginShape(GeShape(NDShape));
  src_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  src_op1->AddOutputDesc(src_tensor_desc1);
  src_op1->AddInputDesc(src_tensor_desc1);
  auto src_node1 = graph->AddNode(src_op1);
  ge::AttrUtils::SetInt(src_op1, FE_IMPLY_TYPE, 6);
  // bias op
  OpDescPtr src_op2 = std::make_shared<OpDesc>("bias", "RNN_CONST");
  GeTensorDesc src_tensor_desc2(GeShape({1024}), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc2.SetOriginShape(GeShape({1024}));
  src_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  src_op2->AddInputDesc(src_tensor_desc2);
  src_op2->AddOutputDesc(src_tensor_desc2);
  auto src_node2 = graph->AddNode(src_op2);
  ge::AttrUtils::SetInt(src_op2, FE_IMPLY_TYPE, 6);
  // RNN op
  OpDescPtr dst_op = std::make_shared<OpDesc>("rnn", "RNN_OP");
  GeTensorDesc dst_tensor_desc1(GeShape({1,2,16,16}), ge::FORMAT_FRACTAL_ZN_RNN, ge::DT_FLOAT16);
  dst_tensor_desc1.SetOriginShape(GeShape(NDShape));
  dst_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  GeTensorDesc dst_tensor_desc2(GeShape({2048}), ge::FORMAT_ND_RNN_BIAS, ge::DT_FLOAT16);
  dst_tensor_desc2.SetOriginShape(GeShape({1024}));
  dst_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  dst_op->AddInputDesc(dst_tensor_desc1);
  dst_op->AddInputDesc(dst_tensor_desc2);
  dst_op->AddOutputDesc(dst_tensor_desc1);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetInt(dst_op, "hidden_size", 8);
  ge::AttrUtils::SetInt(dst_op, "input_size", 8);
  vector<bool> input_const_vector = {true, true};
  dst_op->SetIsInputConst(input_const_vector);

  // out op
  OpDescPtr src_op3 = std::make_shared<OpDesc>("OUT", "RNN_CONST");
  GeTensorDesc src_tensor_desc3(GeShape({16, 1024}), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc3.SetOriginShape(GeShape({16, 1024}));
  src_tensor_desc3.SetOriginFormat(ge::FORMAT_ND);
  src_op3->AddOutputDesc(src_tensor_desc3);
  src_op3->AddInputDesc(src_tensor_desc3);
  auto src_node3 = graph->AddNode(src_op1);
  ge::AttrUtils::SetInt(src_op3, FE_IMPLY_TYPE, 6);

  GraphUtils::AddEdge(src_node1->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(src_node2->GetOutDataAnchor(0), dst_node->GetInDataAnchor(1));
  GraphUtils::AddEdge(dst_node->GetOutDataAnchor(0), src_node3->GetInDataAnchor(0));
  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));

  int count_node = 0;
  int rnn_node = 0;
  int64_t hidden_size;
  int64_t input_size;
  ASSERT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransDataRNN") {
      if (node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat() == ge::FORMAT_FRACTAL_ZN_RNN) {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 4);
        EXPECT_EQ(shape.GetDims()[0], 2);
        EXPECT_EQ(shape.GetDims()[1], 128);
        EXPECT_EQ(shape.GetDims()[2], 16);
        EXPECT_EQ(shape.GetDims()[3], 16);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 8);
        EXPECT_EQ(input_size, 8);
      } else if (node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat() == ge::FORMAT_ND_RNN_BIAS) {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 1);
        EXPECT_EQ(shape.GetDims()[0], 2048);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 8);
        EXPECT_EQ(input_size, 8);
      } else {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 2);
        EXPECT_EQ(shape.GetDims()[0], 16);
        EXPECT_EQ(shape.GetDims()[1], 1024);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 8);
        EXPECT_EQ(input_size, 8);
      }
      rnn_node++;
    }
  }
  EXPECT_EQ(rnn_node, 3);
  EXPECT_EQ(count_node, 7);
}

/* ND(fp16) -> FORMAT_FRACTAL_ZN_RNN(fp16)
 * The Program will insert TransdataRNN ops.
 * TransdataRNN is Tbe Op
 * shape is (16, k, n)
 * k_num = 2 : k == hidden_size + input_size */
TEST_F(UTEST_FE_TRANSOP_INSERT_RNN, InsertAllTransop_01_3) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  // weight op
  OpDescPtr src_op1 = std::make_shared<OpDesc>("weight", "RNN_CONST");
  std::vector<int64_t> NDShape = {16, 16, 1024};
  GeTensorDesc src_tensor_desc1(GeShape(NDShape), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc1.SetOriginShape(GeShape(NDShape));
  src_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  src_op1->AddOutputDesc(src_tensor_desc1);
  src_op1->AddInputDesc(src_tensor_desc1);
  auto src_node1 = graph->AddNode(src_op1);
  ge::AttrUtils::SetInt(src_op1, FE_IMPLY_TYPE, 6);
  // bias op
  OpDescPtr src_op2 = std::make_shared<OpDesc>("bias", "RNN_CONST");
  GeTensorDesc src_tensor_desc2(GeShape({16, 1024}), ge::FORMAT_ND, ge::DT_FLOAT16);
  src_tensor_desc2.SetOriginShape(GeShape({16, 1024}));
  src_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  src_op2->AddInputDesc(src_tensor_desc2);
  src_op2->AddOutputDesc(src_tensor_desc2);
  auto src_node2 = graph->AddNode(src_op2);
  ge::AttrUtils::SetInt(src_op2, FE_IMPLY_TYPE, 6);
  // RNN op
  OpDescPtr dst_op = std::make_shared<OpDesc>("rnn", "RNN_OP");
  GeTensorDesc dst_tensor_desc1(GeShape({16,1,2,16,16}), ge::FORMAT_FRACTAL_ZN_RNN, ge::DT_FLOAT16);
  dst_tensor_desc1.SetOriginShape(GeShape(NDShape));
  dst_tensor_desc1.SetOriginFormat(ge::FORMAT_ND);
  GeTensorDesc dst_tensor_desc2(GeShape({16, 2048}), ge::FORMAT_ND_RNN_BIAS, ge::DT_FLOAT16);
  dst_tensor_desc2.SetOriginShape(GeShape({16, 1024}));
  dst_tensor_desc2.SetOriginFormat(ge::FORMAT_ND);
  dst_op->AddInputDesc(dst_tensor_desc1);
  dst_op->AddInputDesc(dst_tensor_desc2);
  dst_op->AddOutputDesc(dst_tensor_desc1);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetInt(dst_op, "hidden_size", 8);
  ge::AttrUtils::SetInt(dst_op, "input_size", 8);
  vector<bool> input_const_vector = {true, true};
  dst_op->SetIsInputConst(input_const_vector);

  GraphUtils::AddEdge(src_node1->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(src_node2->GetOutDataAnchor(0), dst_node->GetInDataAnchor(1));
  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));

  int count_node = 0;
  int rnn_node = 0;
  int64_t hidden_size;
  int64_t input_size;
  ASSERT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransDataRNN") {
      if (node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat() == ge::FORMAT_FRACTAL_ZN_RNN) {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 5);
        EXPECT_EQ(shape.GetDims()[0], 16);
        EXPECT_EQ(shape.GetDims()[1], 2);
        EXPECT_EQ(shape.GetDims()[2], 128);
        EXPECT_EQ(shape.GetDims()[3], 16);
        EXPECT_EQ(shape.GetDims()[4], 16);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 8);
        EXPECT_EQ(input_size, 8);
      } else {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 2);
        EXPECT_EQ(shape.GetDims()[0], 16);
        EXPECT_EQ(shape.GetDims()[1], 2048);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "hidden_size", hidden_size);
        ge::AttrUtils::GetInt(node->GetOpDesc(), "input_size", input_size);
        EXPECT_EQ(hidden_size, 8);
        EXPECT_EQ(input_size, 8);
      }
      rnn_node++;
    }
  }
  EXPECT_EQ(rnn_node, 2);
  EXPECT_EQ(count_node, 5);
}
