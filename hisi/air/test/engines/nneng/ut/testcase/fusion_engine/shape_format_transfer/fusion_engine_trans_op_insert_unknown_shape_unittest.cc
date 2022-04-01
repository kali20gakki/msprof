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
#include "common/aicore_util_attr_define.h"
#include "common/unknown_shape_util.h"

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
#include "common/configuration.h"
#include "ops_store/ops_kernel_manager.h"
#undef protected
#undef private

#include <iostream>

using namespace std;
using namespace ge;
using namespace fe;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

class UTEST_FE_TRANSOP_INSERT_UNKNOWN_SHAPE : public testing::Test {
 protected:
  void SetUp()
  {
    std::map<std::string, std::string> options;
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    FEOpsStoreInfo tbe_custom {
            2,
            "tbe-custom",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            ""};
    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

    fe_ops_kernel_info_store_ptr_->Initialize(options);
    Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  }

    void TearDown()
  {
    fe_ops_kernel_info_store_ptr_->Finalize();

  }

  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
 protected:

};

TEST_F(UTEST_FE_TRANSOP_INSERT_UNKNOWN_SHAPE, InsertTransDataNode_01)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  vector<std::pair<int64_t, int64_t>> range({{1, 100}, {1, 256}, {256, 256}, {512, 512}});
  vector<std::pair<int64_t, int64_t>> range2({{1, 100}, {16, 256}, {256, 256}, {512, 512}});
  GeTensorDesc src_tensor_desc(GeShape({-1, -1, 256, 512}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetShapeRange(range);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  vector<std::pair<int64_t, int64_t>> range1({{1, 1}, {16, 256}, {256, 256}, {1, 512}});
  GeTensorDesc dst_tensor_desc(GeShape({1, -1, 256, -1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  dst_tensor_desc.SetShapeRange(range1);
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  vector<bool> input_const_vector = {false};
  dst_op->SetIsInputConst(input_const_vector);
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));
  int count_node = 0;
  ASSERT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransData") {
      ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      EXPECT_EQ(shape.GetDimNum(), 4);
      EXPECT_EQ(shape.GetDims()[0], -1);
      EXPECT_EQ(shape.GetDims()[1], -16);
      EXPECT_EQ(shape.GetDims()[2], 256);
      EXPECT_EQ(shape.GetDims()[3], 512);
      EXPECT_EQ(GetShapeRange(*node->GetOpDesc()->GetOutputDescPtr(0)), range2);
    }
    if (node->GetType() == "B") {
      ge::GeShape shape = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
      EXPECT_EQ(shape.GetDimNum(), 4);
      EXPECT_EQ(shape.GetDims()[0], 1);
      EXPECT_EQ(shape.GetDims()[1], -1);
      EXPECT_EQ(shape.GetDims()[2], 256);
      EXPECT_EQ(shape.GetDims()[3], -1);
      EXPECT_EQ(GetShapeRange(*node->GetOpDesc()->GetInputDescPtr(0)), range1);
    }
  }
  EXPECT_EQ(count_node, 3);

}

TEST_F(UTEST_FE_TRANSOP_INSERT_UNKNOWN_SHAPE, InsertTransDataNode_Unknown_Shape)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({-2, 9}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({-2, 9}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  vector<bool> input_const_vector = {false};
  dst_op->SetIsInputConst(input_const_vector);
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));
  int count_node = 0;
  ASSERT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransData") {
      ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      EXPECT_EQ(shape.GetDims().size(), 2);
      EXPECT_EQ(shape.GetDims()[0], -2);
      ge::GeShape shape1 = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
      EXPECT_EQ(shape1.GetDims().size(), 2);
      EXPECT_EQ(shape1.GetDims()[0], -2);
      EXPECT_EQ(shape1.GetDims()[1], 9);
    }
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(UTEST_FE_TRANSOP_INSERT_UNKNOWN_SHAPE, AddReshapeOp_01) {
  string reshape_type = "NC";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  vector<std::pair<int64_t, int64_t>> range({{1, 256}});
  vector<std::pair<int64_t, int64_t>> range1({{1, 256}});
  GeTensorDesc src_tensor_desc(GeShape({1, -1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({1, -1, 1, 1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransInfoPtr trans_info_ptr = std::make_shared<TransInfo>();
  trans_info_ptr->src_out_shape = GeShape({1, -1});
  trans_info_ptr->dst_in_shape = GeShape({1, -1,1,1});
  vector<std::pair<int64_t, int64_t>> range2({{1, 1}, {1, 256}});
  vector<std::pair<int64_t, int64_t>> range3({{1, 1}, {1, 256}, {1, 1}, {1, 1}});
  trans_info_ptr->src_out_range = range2;
  trans_info_ptr->dst_in_range = range3;
  trans_info_ptr->src_reshape_type = reshape_type;
  trans_info_ptr->dst_reshape_type = reshape_type;
  trans_info_ptr->src_out_data_type = ge::DT_FLOAT16;
  trans_info_ptr->dst_in_data_type = ge::DT_FLOAT16;

  trans_info_ptr->src_op_desc = src_op;
  trans_info_ptr->dst_op_desc = dst_op;
  trans_info_ptr->src_node_ptr = src_node;
  trans_info_ptr->dst_node_ptr = dst_node;
  trans_info_ptr->src_anchor = src_node->GetOutDataAnchor(0);
  trans_info_ptr->dst_anchor = dst_node->GetInDataAnchor(0);


  TransNodeReshapeGenerator trans_op_insert(fe_ops_kernel_info_store_ptr_, trans_info_ptr);
  Status ret = trans_op_insert.AddTransNode(*graph.get(), trans_info_ptr);
  for (auto node: graph->GetDirectNode()) {
    if (node->GetType() == "Reshape") {
      auto input_desc = node->GetOpDesc()->GetInputDescPtr(0);
      EXPECT_EQ(GetShapeRange(*input_desc), range2);
      auto output_desc = node->GetOpDesc()->GetOutputDescPtr(0);
      EXPECT_EQ(GetShapeRange(*output_desc), range3);
    }
  }
  EXPECT_EQ(ret, fe::SUCCESS);
}