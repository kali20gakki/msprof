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
#include "common/configuration.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "ops_store/ops_kernel_manager.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_cast_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_reshape_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transpose_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdata_generator.h"

#undef protected
#undef private

#include <iostream>

using namespace std;
using namespace ge;
using namespace fe;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

class STEST_FE_TRANSOP_INSERT : public testing::Test {
 protected:
  void SetUp()
  {
    std::map<std::string, std::string> options;
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    fe_ops_kernel_info_store_ptr_ = make_shared<fe::FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
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
  }

  void TearDown()
  {
    fe_ops_kernel_info_store_ptr_->Finalize();

  }

  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
 protected:

};

Status QueryHighPrioOpImplTypeStubTbe(FEOpsKernelInfoStore* This, const ge::OpDescPtr& op_desc_ptr, OpImplType &impl_type) {
  impl_type = EN_IMPL_HW_TBE;
  return fe::SUCCESS;
}

Status QueryHighPrioOpImplTypeStubCce(FEOpsKernelInfoStore* This, const ge::OpDescPtr& op_desc_ptr, OpImplType &impl_type) {
  impl_type = EN_IMPL_HW_GENERAL_CCE;
  return fe::SUCCESS;
}

Status ConstructComputeGraph(ComputeGraphPtr graph, ComputeGraphPtr graph_check)
{
  return fe::SUCCESS;
}
bool CheckComputeGraph(ComputeGraphPtr graph, ComputeGraphPtr graph_check)
{
  //EXPECT_EQ(ge::GRAPH_SUCCESS, fused_graph.TopologicalSorting());
  return true;
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_01)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({100, 256, 256, 512}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
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
      EXPECT_EQ(shape.GetDims()[0], 100);
      EXPECT_EQ(shape.GetDims()[1], 256);
      EXPECT_EQ(shape.GetDims()[2], 256);
      EXPECT_EQ(shape.GetDims()[3], 512);
    }
    if (node->GetType() == "B") {
      ge::GeShape shape = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
      EXPECT_EQ(shape.GetDimNum(), 4);
      EXPECT_EQ(shape.GetDims()[0], 1);
      EXPECT_EQ(shape.GetDims()[1], 256);
      EXPECT_EQ(shape.GetDims()[2], 256);
      EXPECT_EQ(shape.GetDims()[3], 512);
    }
  }
  EXPECT_EQ(count_node, 3);

}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_02)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "B") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_05)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 32}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(ge::GeShape({123,456}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NHWC, ge::DT_FLOAT16);
  dst_tensor_desc.SetOriginShape(ge::GeShape({123,456}));
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "B") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
    if (node->GetType() == "TransData") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 5);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 512);
        EXPECT_EQ(input_vec_of_b[4], 32);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 123);
        EXPECT_EQ(input_vec_of_b[2], 456);
        EXPECT_EQ(input_vec_of_b[3], 1);
      }

    }
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_06)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 32}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(ge::GeShape({123,456}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  dst_tensor_desc.SetOriginShape(ge::GeShape({123,456}));
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "B") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
    if (node->GetType() == "TransData") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 5);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 512);
        EXPECT_EQ(input_vec_of_b[4], 32);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 123);
        EXPECT_EQ(input_vec_of_b[2], 456);
        EXPECT_EQ(input_vec_of_b[3], 1);
      }

    }
  }
  EXPECT_EQ(count_node, 3);
}


TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_07)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 32}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(GeShape({100,200,100,200}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  dst_tensor_desc.SetOriginShape(GeShape({1, 2, 3, 4}));
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "B") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
    if (node->GetType() == "TransData") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 260000);
        EXPECT_EQ(input_vec_of_b[1], 7);
        EXPECT_EQ(input_vec_of_b[2], 16);
        EXPECT_EQ(input_vec_of_b[3], 16);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 100);
        EXPECT_EQ(input_vec_of_b[1], 200);
        EXPECT_EQ(input_vec_of_b[2], 100);
        EXPECT_EQ(input_vec_of_b[3], 200);
      }

    }
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_07_1)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({100,200}), ge::FORMAT_HWCN, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(GeShape({100,200}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_HWCN);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("BB", "BB");
  GeTensorDesc dst_tensor_desc(GeShape({7, 13, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
  dst_tensor_desc.SetOriginShape(GeShape({100, 200}));
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_HWCN);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransData") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 100);
        EXPECT_EQ(input_vec_of_b[3], 200);
        EXPECT_EQ(node->GetOpDesc()->GetInputDescPtr(0)->GetFormat(), ge::FORMAT_HWCN);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 7);
        EXPECT_EQ(input_vec_of_b[1], 13);
        EXPECT_EQ(input_vec_of_b[2], 16);
        EXPECT_EQ(input_vec_of_b[3], 16);
        EXPECT_EQ(node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat(), ge::FORMAT_FRACTAL_Z);
      }

    }
  }
  EXPECT_EQ(count_node, 4);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_08)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 32}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(GeShape({100,200}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2, 3, 4}), ge::FORMAT_NHWC, ge::DT_FLOAT16);
  dst_tensor_desc.SetOriginShape(GeShape({100,200}));
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "B") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
    if (node->GetType() == "TransData") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 5);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 512);
        EXPECT_EQ(input_vec_of_b[4], 32);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 100);
        EXPECT_EQ(input_vec_of_b[2], 200);
        EXPECT_EQ(input_vec_of_b[3], 1);
      }

    }
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_09)
{
  // src:FORMAT_FRACTAL_Z, dst:FORMAT_NCHW  when C==1 && N==groups && groups > 1, insert HWCN
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  int64_t group = 2; // set groups 2  sub format
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 32}), static_cast<ge::Format>(ge::GetFormatFromSub(ge::FORMAT_FRACTAL_Z, group)), ge::DT_FLOAT);
  src_tensor_desc.SetOriginShape(GeShape({2,1,3,4}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2, 3, 4}), static_cast<ge::Format>(ge::GetFormatFromSub(ge::FORMAT_NCHW, 0)), ge::DT_FLOAT);
  dst_tensor_desc.SetOriginShape(GeShape({2,1,3,4}));
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;

  printf("ge::GetPrimaryFormat(src_op->GetInputDesc(0).GetFormat()) %d...\n", ge::GetPrimaryFormat(src_op->GetInputDesc(0).GetFormat()));
  printf("ge::GetSubFormat(op1->GetInputDesc(0).GetFormat() %d...\n", ge::GetSubFormat(src_op->GetInputDesc(0).GetFormat()));
  printf("ge::GetPrimaryFormat(dst_op->GetInputDesc(0).GetFormat()) %d...\n", ge::GetPrimaryFormat(dst_op->GetInputDesc(0).GetFormat()));
  printf("ge::GetSubFormat(dst_op->GetInputDesc(0).GetFormat() %d...\n", ge::GetSubFormat(dst_op->GetInputDesc(0).GetFormat()));
  
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    printf("countNode %d...\n", count_node);
	printf("node->GetType() %s...\n", node->GetType().c_str());

    if (node->GetType() == "A") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
		printf("InputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetInputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 5);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 512);
		EXPECT_EQ(input_vec_of_b[4], 32);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
		printf("GetOutputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 5);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 512);
		EXPECT_EQ(input_vec_of_b[4], 32);
      }
    }
	if (node->GetType() == "TransData") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
		printf("InputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetInputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 12);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 16);
        EXPECT_EQ(input_vec_of_b[3], 16);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
		printf("GetOutputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 3);
        EXPECT_EQ(input_vec_of_b[1], 4);
        EXPECT_EQ(input_vec_of_b[2], 1);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }
    }
	if (node->GetType() == "TransposeD") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
		printf("InputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetInputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 3);
        EXPECT_EQ(input_vec_of_b[1], 4);
        EXPECT_EQ(input_vec_of_b[2], 1);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
		printf("GetOutputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 2);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
    if (node->GetType() == "B") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
		printf("InputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetInputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
		printf("GetOutputDescPtr(0).GetFormat() %d...\n", node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat());
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 2);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
  }
  EXPECT_EQ(count_node, 4);
}



TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_09_1) 
{
  // src:FORMAT_NCHW, dst: FORMAT_FRACTAL_Z  when C==1 && N==groups && groups > 1, insert HWCN
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  ComputeGraphPtr graph_check =  std::make_shared<ComputeGraph>("test_graph_check");

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  int64_t group = 2; // set groups 2  sub format
  GeTensorDesc src_tensor_desc(GeShape({2,1,3,4}), static_cast<ge::Format>(ge::GetFormatFromSub(ge::FORMAT_NCHW, 0)), ge::DT_FLOAT);
  src_tensor_desc.SetOriginShape(GeShape({2,1,3,4}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_op->AddInputDesc(src_tensor_desc);
  src_op->AddOutputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);
  vector<bool> input_const_vector = {false};
  src_op->SetIsInputConst(input_const_vector);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 256, 512, 32}), static_cast<ge::Format>(ge::GetFormatFromSub(ge::FORMAT_FRACTAL_Z, group)), ge::DT_FLOAT);
  dst_tensor_desc.SetOriginShape(GeShape({2,1,3,4}));
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  dst_op->AddOutputDesc(dst_tensor_desc);
  dst_op->AddInputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;

  printf("ge::GetPrimaryFormat(src_op->GetInputDesc(0).GetFormat()) %d...\n", ge::GetPrimaryFormat(src_op->GetInputDesc(0).GetFormat()));
  printf("ge::GetSubFormat(op1->GetInputDesc(0).GetFormat() %d...\n", ge::GetSubFormat(src_op->GetInputDesc(0).GetFormat()));
  printf("ge::GetPrimaryFormat(dst_op->GetInputDesc(0).GetFormat()) %d...\n", ge::GetPrimaryFormat(dst_op->GetInputDesc(0).GetFormat()));
  printf("ge::GetSubFormat(dst_op->GetInputDesc(0).GetFormat() %d...\n", ge::GetSubFormat(dst_op->GetInputDesc(0).GetFormat()));

  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    printf("countNode %d...\n", count_node);
	printf("node->GetType() %s...\n", node->GetType().c_str());

    if (node->GetType() == "A") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 2);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 2);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
    }
	if (node->GetType() == "TransposeD") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 2);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 3);
        EXPECT_EQ(input_vec_of_b[3], 4);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 3);
        EXPECT_EQ(input_vec_of_b[1], 4);
        EXPECT_EQ(input_vec_of_b[2], 1);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }
	}
    if (node->GetType() == "TransData") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 3);
        EXPECT_EQ(input_vec_of_b[1], 4);
        EXPECT_EQ(input_vec_of_b[2], 1);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 12);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 16);
        EXPECT_EQ(input_vec_of_b[3], 16);
      }
    }
    if (node->GetType() == "B") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 5);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 512);
		EXPECT_EQ(input_vec_of_b[4], 32);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 5);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 512);
		EXPECT_EQ(input_vec_of_b[4], 32);
      }
    }
  }
  EXPECT_EQ(count_node, 4);
}


TEST_F(STEST_FE_TRANSOP_INSERT, InsertCastNode)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 1}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("C", "C");
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 256, 512, 1}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {

    ASSERT_NE(node, nullptr);
    if (node->GetType() == "A") {
      auto dim_vec = node->GetOpDesc()->GetInputDescPtr(0)->GetShape().GetDims();
      ASSERT_EQ(dim_vec.size(), 5);
      EXPECT_EQ(dim_vec[0], 1);
      EXPECT_EQ(dim_vec[1], 256);
      EXPECT_EQ(dim_vec[2], 256);
      EXPECT_EQ(dim_vec[3], 512);
      EXPECT_EQ(dim_vec[4], 1);
    }
    count_node++;
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertCastNode_not_necessary)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("C", "C");
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 256, 512, 1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
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
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
  }
  EXPECT_EQ(count_node, 2);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertPermuteNodeWithInputConst)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("C", "C");
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {

    ASSERT_NE(node, nullptr);
    if (node->GetType() == "TransposeD") {
      auto dim_vec = node->GetOpDesc()->GetInputDescPtr(0)->GetShape().GetDims();
      ASSERT_EQ(dim_vec.size(), 4);
      EXPECT_EQ(dim_vec[0], 1);
      EXPECT_EQ(dim_vec[1], 256);
      EXPECT_EQ(dim_vec[2], 256);
      EXPECT_EQ(dim_vec[3], 512);
    }
    if (node->GetType() == "A") {
      auto dim_vec = node->GetOpDesc()->GetInputDescPtr(0)->GetShape().GetDims();
      ASSERT_EQ(dim_vec.size(), 4);
      EXPECT_EQ(dim_vec[0], 1);
      EXPECT_EQ(dim_vec[1], 256);
      EXPECT_EQ(dim_vec[2], 256);
      EXPECT_EQ(dim_vec[3], 512);
    }
    count_node++;
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertPermuteNode)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("D", "D");
  GeTensorDesc dst_tensor_desc(GeShape({1, 3, 4, 2}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);

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

  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransposeD") {
      ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      auto size = shape_check.GetDimNum();
      ASSERT_EQ(size, 4);
      vector<int64_t> input_vec_of_b =shape_check.GetDims();
      EXPECT_EQ(input_vec_of_b[0], 1);
      EXPECT_EQ(input_vec_of_b[1], 256);
      EXPECT_EQ(input_vec_of_b[2], 512);
      EXPECT_EQ(input_vec_of_b[3], 1024);
    }
    if (node->GetType() == "D") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 3);
        EXPECT_EQ(input_vec_of_b[2], 4);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 3);
        EXPECT_EQ(input_vec_of_b[2], 4);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }

    }

  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertPermuteNode2)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("D", "D");
  GeTensorDesc src_tensor_desc(GeShape({1, 3, 4, 2}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);

  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);

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

  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransposeD") {
      ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      auto size = shape_check.GetDimNum();
      ASSERT_EQ(size, 4);
      vector<int64_t> input_vec_of_b =shape_check.GetDims();
      EXPECT_EQ(input_vec_of_b[0], 1);
      EXPECT_EQ(input_vec_of_b[1], 2);
      EXPECT_EQ(input_vec_of_b[2], 3);
      EXPECT_EQ(input_vec_of_b[3], 4);
      EXPECT_EQ(ge::AttrUtils::HasAttr(node->GetOpDesc(), "perm"), true);
      EXPECT_EQ(ge::AttrUtils::HasAttr(node->GetOpDesc(), "order"), true);
    }
    if (node->GetType() == "B") {
      ge::GeShape input_shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
      ge::GeShape output_shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      auto size_in = input_shape_check.GetDimNum();
      auto size_out = output_shape_check.GetDimNum();
      ASSERT_EQ(size_in, 2);
      ASSERT_EQ(size_out, 2);
      vector<int64_t> input_vec_of_b = input_shape_check.GetDims();
      vector<int64_t> output_vec_of_b = output_shape_check.GetDims();
      EXPECT_EQ(input_vec_of_b[0], 1);
      EXPECT_EQ(input_vec_of_b[1], 2);

      EXPECT_EQ(output_vec_of_b[0], 1);
      EXPECT_EQ(output_vec_of_b[1], 2);

    }
  }
  EXPECT_EQ(count_node, 3);
}



TEST_F(STEST_FE_TRANSOP_INSERT, InsertPermuteNodeWithInputConst2)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({256,512}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);

  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("C", "C");
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);

  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));
  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {

    ASSERT_NE(node, nullptr);
    if (node->GetType() == "TransposeD") {
      {
        auto dim_vec = node->GetOpDesc()->GetInputDescPtr(0)->GetShape().GetDims();
        EXPECT_EQ(node->GetOpDesc()->GetInputDescPtr(0)->GetFormat(), ge::FORMAT_NCHW);
        ASSERT_EQ(dim_vec.size(), 4);
        EXPECT_EQ(dim_vec[0], 1);
        EXPECT_EQ(dim_vec[1], 256);
        EXPECT_EQ(dim_vec[2], 512);
        EXPECT_EQ(dim_vec[3], 1);
      }
      {
        auto dim_vec = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape().GetDims();
        EXPECT_EQ(node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat(), ge::FORMAT_NHWC);
        ASSERT_EQ(dim_vec.size(), 4);
        EXPECT_EQ(dim_vec[0], 1);
        EXPECT_EQ(dim_vec[1], 512);
        EXPECT_EQ(dim_vec[2], 1);
        EXPECT_EQ(dim_vec[3], 256);
      }
    }
    if (node->GetType() == "A") {
      auto dim_vec = node->GetOpDesc()->GetInputDescPtr(0)->GetShape().GetDims();
      ASSERT_EQ(dim_vec.size(), 2);
      EXPECT_EQ(dim_vec[0], 256);
      EXPECT_EQ(dim_vec[1], 512);
    }
    if (node->GetType() == "C") {
      {
        auto dim_vec = node->GetOpDesc()->GetInputDescPtr(0)->GetShape().GetDims();
        EXPECT_EQ(node->GetOpDesc()->GetInputDescPtr(0)->GetFormat(), ge::FORMAT_NHWC);
        ASSERT_EQ(dim_vec.size(), 4);
        EXPECT_EQ(dim_vec[0], 1);
        EXPECT_EQ(dim_vec[1], 256);
        EXPECT_EQ(dim_vec[2], 256);
        EXPECT_EQ(dim_vec[3], 512);
      }
      {
        auto dim_vec = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape().GetDims();
        EXPECT_EQ(node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat(), ge::FORMAT_NHWC);
        ASSERT_EQ(dim_vec.size(), 4);
        EXPECT_EQ(dim_vec[0], 1);
        EXPECT_EQ(dim_vec[1], 256);
        EXPECT_EQ(dim_vec[2], 256);
        EXPECT_EQ(dim_vec[3], 512);
      }
    }
    count_node++;
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertPermuteNode4)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc src_tensor_desc(GeShape({1024, 256, 2}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);

  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("D", "D");
  GeTensorDesc dst_tensor_desc(GeShape({1, 3, 4, 2}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);

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

  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransposeD") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b = shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 1024);
        EXPECT_EQ(input_vec_of_b[2], 256);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b = shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 256);
        EXPECT_EQ(input_vec_of_b[2], 2);
        EXPECT_EQ(input_vec_of_b[3], 1024);
      }
    }
    if (node->GetType() == "D") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 3);
        EXPECT_EQ(input_vec_of_b[2], 4);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 3);
        EXPECT_EQ(input_vec_of_b[2], 4);
        EXPECT_EQ(input_vec_of_b[3], 2);
      }

    }

  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertPermuteNode5)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("D", "D");
  GeTensorDesc src_tensor_desc(GeShape({5}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);

  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);

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

  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransposeD") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b = shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 1);
        EXPECT_EQ(input_vec_of_b[3], 5);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b = shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 5);
        EXPECT_EQ(input_vec_of_b[2], 1);
        EXPECT_EQ(input_vec_of_b[3], 1);
      }
    }
    if (node->GetType() == "B") {
      ge::GeShape input_shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
      ge::GeShape output_shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      auto size_in = input_shape_check.GetDimNum();
      auto size_out = output_shape_check.GetDimNum();
      ASSERT_EQ(size_in, 2);
      ASSERT_EQ(size_out, 2);
      vector<int64_t> input_vec_of_b = input_shape_check.GetDims();
      vector<int64_t> output_vec_of_b = output_shape_check.GetDims();
      EXPECT_EQ(input_vec_of_b[0], 1);
      EXPECT_EQ(input_vec_of_b[1], 2);

      EXPECT_EQ(output_vec_of_b[0], 1);
      EXPECT_EQ(output_vec_of_b[1], 2);

    }
  }
  EXPECT_EQ(count_node, 3);
}


TEST_F(STEST_FE_TRANSOP_INSERT, InsertPermuteNode6)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("D", "D");
  GeTensorDesc src_tensor_desc(GeShape({5,7}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);

  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  GeTensorDesc dst_tensor_desc(GeShape({1, 2}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);

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

  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransposeD") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b = shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 5);
        EXPECT_EQ(input_vec_of_b[2], 7);
        EXPECT_EQ(input_vec_of_b[3], 1);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b = shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 1);
        EXPECT_EQ(input_vec_of_b[1], 1);
        EXPECT_EQ(input_vec_of_b[2], 5);
        EXPECT_EQ(input_vec_of_b[3], 7);
      }
    }
    if (node->GetType() == "B") {
      ge::GeShape input_shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
      ge::GeShape output_shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      auto size_in = input_shape_check.GetDimNum();
      auto size_out = output_shape_check.GetDimNum();
      ASSERT_EQ(size_in, 2);
      ASSERT_EQ(size_out, 2);
      vector<int64_t> input_vec_of_b = input_shape_check.GetDims();
      vector<int64_t> output_vec_of_b = output_shape_check.GetDims();
      EXPECT_EQ(input_vec_of_b[0], 1);
      EXPECT_EQ(input_vec_of_b[1], 2);

      EXPECT_EQ(output_vec_of_b[0], 1);
      EXPECT_EQ(output_vec_of_b[1], 2);

    }
  }
  EXPECT_EQ(count_node, 3);
}

/* 2D->NCHW->NHWC, reshape type of E is nc*/
TEST_F(STEST_FE_TRANSOP_INSERT, InsertReshapeNode) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("EE", "EE");
  GeTensorDesc src_tensor_desc(GeShape({5, 2}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);

  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("D", "D");
  GeTensorDesc dst_tensor_desc(GeShape({5, 5, 5, 5}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);

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

  ASSERT_EQ(fe::SUCCESS, status);
  int count_node = 0;
  for (auto node : graph->GetDirectNode()) {
    ASSERT_NE(node, nullptr);
    count_node++;
    if (node->GetType() == "TransposeD") {
      ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
      auto size = shape_check.GetDimNum();
      ASSERT_EQ(size, 4);
      vector<int64_t> input_vec_of_b =shape_check.GetDims();
      EXPECT_EQ(input_vec_of_b[0], 1);
      EXPECT_EQ(input_vec_of_b[1], 2);
      EXPECT_EQ(input_vec_of_b[2], 1);
      EXPECT_EQ(input_vec_of_b[3], 5);
    }
    if (node->GetType() == "D") {
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 5);
        EXPECT_EQ(input_vec_of_b[1], 5);
        EXPECT_EQ(input_vec_of_b[2], 5);
        EXPECT_EQ(input_vec_of_b[3], 5);
      }
      {
        ge::GeShape shape_check = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        auto size = shape_check.GetDimNum();
        ASSERT_EQ(size, 4);
        vector<int64_t> input_vec_of_b =shape_check.GetDims();
        EXPECT_EQ(input_vec_of_b[0], 5);
        EXPECT_EQ(input_vec_of_b[1], 5);
        EXPECT_EQ(input_vec_of_b[2], 5);
        EXPECT_EQ(input_vec_of_b[3], 5);
      }
    }
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, AddReshapeOp_01) {
  string reshape_type = "NC";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  GeTensorDesc src_tensor_desc(GeShape({1, 256}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 1, 1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
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
  trans_info_ptr->src_out_shape = GeShape({1, 256});
  trans_info_ptr->dst_in_shape = GeShape({1, 256,1,1});
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
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_FE_TRANSOP_INSERT, AddReshapeOp_02) {
  string reshape_type = "NC";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  GeTensorDesc src_tensor_desc(GeShape({1, 256}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 1, 1}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);


  TransInfoPtr trans_info_ptr = std::make_shared<TransInfo>();
  trans_info_ptr->src_out_shape = GeShape({1, 256});
  trans_info_ptr->dst_in_shape = GeShape({1, 256,1,1});
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
  /* If two anchors is not connected, it will still return success. */
  EXPECT_EQ(ret, fe::SUCCESS);
}
TEST_F(STEST_FE_TRANSOP_INSERT, MergeTwoTransDataOp) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");

  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 4}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT16);

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(src_tensor_desc);
  dst_op->AddOutputDesc(src_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  OpDescPtr dst_op_trans_data1 = std::make_shared<OpDesc>("Transdata1", "TransData");
  dst_op_trans_data1->AddInputDesc(src_tensor_desc);
  dst_op_trans_data1->AddOutputDesc(dst_tensor_desc);
  auto trandata_node1 = graph->AddNode(dst_op_trans_data1);

  OpDescPtr dst_op_trans_data2 = std::make_shared<OpDesc>("Transdata2", "TransData");
  dst_op_trans_data2->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data2->AddOutputDesc(src_tensor_desc);
  auto trandata_node2 = graph->AddNode(dst_op_trans_data2);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), trandata_node1->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node1->GetOutDataAnchor(0), trandata_node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node2->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeMerging trans_op_merger;
  trans_op_merger.MergeAllTransOps(*(graph.get()));
  uint32_t count_node = 0;
  for (auto node: graph->GetDirectNode()) {
    EXPECT_NE(node, nullptr);
    EXPECT_NE(node->GetType(), "TransData");
    count_node++;
  }
  EXPECT_EQ(count_node,2);
}

TEST_F(STEST_FE_TRANSOP_INSERT, MergeTwoTransposeOp) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");

  GeTensorDesc src_tensor_desc(GeShape({7, 3, 10, 11}), ge::FORMAT_NHWC, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({7, 11, 3, 10}), ge::FORMAT_NCHW, ge::DT_FLOAT16);

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  OpDescPtr dst_op_trans_pose1 = std::make_shared<OpDesc>("Transpose1", "TransposeD");
  dst_op_trans_pose1->AddInputDesc(src_tensor_desc);
  dst_op_trans_pose1->AddOutputDesc(dst_tensor_desc);
  auto tranpose_node1 = graph->AddNode(dst_op_trans_pose1);

  OpDescPtr dst_op_trans_pose2 = std::make_shared<OpDesc>("Transpose2", "TransposeD");
  dst_op_trans_pose2->AddInputDesc(dst_tensor_desc);
  dst_op_trans_pose2->AddOutputDesc(src_tensor_desc);
  auto tranpose_node2 = graph->AddNode(dst_op_trans_pose2);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), tranpose_node1->GetInDataAnchor(0));
  GraphUtils::AddEdge(tranpose_node1->GetOutDataAnchor(0), tranpose_node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(tranpose_node2->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeMerging trans_op_merger;
  trans_op_merger.MergeAllTransOps(*(graph.get()));
  uint32_t count_node = 0;
  for (auto node: graph->GetDirectNode()) {
    EXPECT_NE(node, nullptr);
    EXPECT_NE(node->GetType(), "TransposeD");
    count_node++;
  }
  EXPECT_EQ(count_node,2);
}

TEST_F(STEST_FE_TRANSOP_INSERT, MergeFourTransDataOp) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");

  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 4}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT16);

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(src_tensor_desc);
  dst_op->AddOutputDesc(src_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  OpDescPtr dst_op_trans_data1 = std::make_shared<OpDesc>("Transdata1", "TransData");
  dst_op_trans_data1->AddInputDesc(src_tensor_desc);
  dst_op_trans_data1->AddOutputDesc(dst_tensor_desc);
  auto trandata_node1 = graph->AddNode(dst_op_trans_data1);

  OpDescPtr dst_op_trans_data2 = std::make_shared<OpDesc>("Transdata2", "TransData");
  dst_op_trans_data2->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data2->AddOutputDesc(src_tensor_desc);
  auto trandata_node2 = graph->AddNode(dst_op_trans_data2);

  OpDescPtr dst_op_trans_data3 = std::make_shared<OpDesc>("Transdata3", "TransData");
  dst_op_trans_data3->AddInputDesc(src_tensor_desc);
  dst_op_trans_data3->AddOutputDesc(dst_tensor_desc);
  auto trandata_node3 = graph->AddNode(dst_op_trans_data3);

  OpDescPtr dst_op_trans_data4 = std::make_shared<OpDesc>("Transdata4", "TransData");
  dst_op_trans_data4->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data4->AddOutputDesc(src_tensor_desc);
  auto trandata_node4 = graph->AddNode(dst_op_trans_data4);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), trandata_node1->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node1->GetOutDataAnchor(0), trandata_node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node2->GetOutDataAnchor(0), trandata_node3->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node3->GetOutDataAnchor(0), trandata_node4->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node4->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeMerging trans_op_merger;
  trans_op_merger.MergeAllTransOps(*(graph.get()));
  uint32_t count_node = 0;
  for (auto node: graph->GetDirectNode()) {
    EXPECT_NE(node, nullptr);
    EXPECT_NE(node->GetType(), "TransData");
    count_node++;
  }
  EXPECT_EQ(count_node,2);
}

/* Two transdata nodes are different in shape, so they can not merge.
 * So, 4 nodes left and two of which are these two transdata nodes */
TEST_F(STEST_FE_TRANSOP_INSERT, MergeTwoTransDataOp_Abnomal) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");

  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 4}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc_01(GeShape({1, 1024, 256, 333}), ge::FORMAT_NCHW, ge::DT_FLOAT16);

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(src_tensor_desc);
  dst_op->AddOutputDesc(src_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  OpDescPtr dst_op_trans_data1 = std::make_shared<OpDesc>("Transdata1", "TransData");
  dst_op_trans_data1->AddInputDesc(src_tensor_desc);
  dst_op_trans_data1->AddOutputDesc(dst_tensor_desc);
  auto trandata_node1 = graph->AddNode(dst_op_trans_data1);

  OpDescPtr dst_op_trans_data2 = std::make_shared<OpDesc>("Transdata2", "TransData");
  dst_op_trans_data2->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data2->AddOutputDesc(src_tensor_desc);
  auto trandata_node2 = graph->AddNode(dst_op_trans_data2);

  OpDescPtr dst_op_trans_data3 = std::make_shared<OpDesc>("Transdata3", "TransData");
  dst_op_trans_data3->AddInputDesc(src_tensor_desc);
  dst_op_trans_data3->AddOutputDesc(dst_tensor_desc_01);
  auto trandata_node3 = graph->AddNode(dst_op_trans_data3);

  OpDescPtr dst_op_trans_data4 = std::make_shared<OpDesc>("Transdata4", "TransData");
  dst_op_trans_data4->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data4->AddOutputDesc(src_tensor_desc);
  auto trandata_node4 = graph->AddNode(dst_op_trans_data4);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), trandata_node1->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node1->GetOutDataAnchor(0), trandata_node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node2->GetOutDataAnchor(0), trandata_node3->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node3->GetOutDataAnchor(0), trandata_node4->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node4->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeMerging trans_op_merger;
  trans_op_merger.MergeAllTransOps(*(graph.get()));
  uint32_t count_node = 0;
  for (auto node: graph->GetDirectNode()) {
    EXPECT_NE(node, nullptr);
    if(node->GetType() == "TransData") {
      EXPECT_NE(node->GetName(), "Transdata1");
      EXPECT_NE(node->GetName(), "Transdata2");
    }
    count_node++;
  }
  EXPECT_EQ(count_node,4);
}

/* One transdata node can not find its lover, so it can not be merged.
 * So, 3 nodes left and one of which is the transdata nodes. */
TEST_F(STEST_FE_TRANSOP_INSERT, MergeThreeTransDataOp_Abnomal) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");

  GeTensorDesc src_tensor_desc(GeShape({1, 256, 256, 512, 4}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT16);

  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(src_tensor_desc);
  dst_op->AddOutputDesc(src_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  OpDescPtr dst_op_trans_data1 = std::make_shared<OpDesc>("Transdata1", "TransData");
  dst_op_trans_data1->AddInputDesc(src_tensor_desc);
  dst_op_trans_data1->AddOutputDesc(dst_tensor_desc);
  auto trandata_node1 = graph->AddNode(dst_op_trans_data1);

  OpDescPtr dst_op_trans_data2 = std::make_shared<OpDesc>("Transdata2", "TransData");
  dst_op_trans_data2->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data2->AddOutputDesc(src_tensor_desc);
  auto trandata_node2 = graph->AddNode(dst_op_trans_data2);

  OpDescPtr dst_op_trans_data3 = std::make_shared<OpDesc>("Transdata3", "TransData");
  dst_op_trans_data3->AddInputDesc(src_tensor_desc);
  dst_op_trans_data3->AddOutputDesc(src_tensor_desc);
  auto trandata_node3 = graph->AddNode(dst_op_trans_data3);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), trandata_node1->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node1->GetOutDataAnchor(0), trandata_node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node2->GetOutDataAnchor(0), trandata_node3->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node3->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeMerging trans_op_merger;
  trans_op_merger.MergeAllTransOps(*(graph.get()));
  uint32_t count_node = 0;
  for (auto node: graph->GetDirectNode()) {
    EXPECT_NE(node, nullptr);
    if(node->GetType() == "TransData") {
      EXPECT_EQ(node->GetName(), "Transdata3");
    }
    count_node++;
  }
  EXPECT_EQ(count_node,3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, MergeTwoTransDataAndTwoCastOp) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");

  GeTensorDesc src_tensor_desc(GeShape({128, 256, 256, 512, 4}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({128, 1024, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc_cast(GeShape({128, 256, 512, 1024}), ge::FORMAT_NHWC, ge::DT_FLOAT16);
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(src_tensor_desc);
  dst_op->AddOutputDesc(src_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  OpDescPtr dst_op_trans_data1 = std::make_shared<OpDesc>("Transdata1", "TransData");
  dst_op_trans_data1->AddInputDesc(src_tensor_desc);
  dst_op_trans_data1->AddOutputDesc(dst_tensor_desc);
  auto trandata_node1 = graph->AddNode(dst_op_trans_data1);

  OpDescPtr dst_op_trans_data2 = std::make_shared<OpDesc>("Cast1", "Cast");
  dst_op_trans_data2->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data2->AddOutputDesc(dst_tensor_desc_cast);
  auto trandata_node2 = graph->AddNode(dst_op_trans_data2);

  OpDescPtr dst_op_trans_data3 = std::make_shared<OpDesc>("Cast2", "Cast");
  dst_op_trans_data3->AddInputDesc(dst_tensor_desc_cast);
  dst_op_trans_data3->AddOutputDesc(dst_tensor_desc);
  auto trandata_node3 = graph->AddNode(dst_op_trans_data3);

  OpDescPtr dst_op_trans_data4 = std::make_shared<OpDesc>("Transdata4", "TransData");
  dst_op_trans_data4->AddInputDesc(dst_tensor_desc);
  dst_op_trans_data4->AddOutputDesc(src_tensor_desc);
  auto trandata_node4 = graph->AddNode(dst_op_trans_data4);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), trandata_node1->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node1->GetOutDataAnchor(0), trandata_node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node2->GetOutDataAnchor(0), trandata_node3->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node3->GetOutDataAnchor(0), trandata_node4->GetInDataAnchor(0));
  GraphUtils::AddEdge(trandata_node4->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeMerging trans_op_merger;
  trans_op_merger.MergeAllTransOps(*(graph.get()));
  uint32_t count_node = 0;
  for (auto node: graph->GetDirectNode()) {
    EXPECT_NE(node, nullptr);
    EXPECT_NE(node->GetType(), "TransData");
    count_node++;
  }
  EXPECT_EQ(count_node,2);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertCastAfterPlaceHolder) {
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");

  OpDescPtr src_op = std::make_shared<OpDesc>("PlaceHolder", "PlaceHolder");
  GeTensorDesc src_tensor_desc(GeShape({1, 1024, 256, 512}), ge::FORMAT_NHWC, ge::DT_FLOAT16);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("D", "D");
  GeTensorDesc dst_tensor_desc(GeShape({1, 3, 4, 2}), ge::FORMAT_NHWC, ge::DT_FLOAT);
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 4);
  vector<bool> input_const_vector = {false};
  dst_op->SetIsInputConst(input_const_vector);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  trans_op_insert.Initialize();
  Status status = trans_op_insert.InsertAndMergeTransNodes(*(graph.get()));
  uint32_t count_node = 0;
  uint32_t count_cast_node = 0;
  for (auto node: graph->GetDirectNode()) {
    EXPECT_NE(node, nullptr);
    if (node->GetType() == "Cast") {
      count_cast_node++;
    }
    count_node++;
  }
  EXPECT_EQ(count_node,3);
  EXPECT_EQ(count_cast_node,1);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNode_ES)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", OP_TYPE_PLACE_HOLDER);
  GeTensorDesc src_tensor_desc(GeShape({100, 256, 256, 512}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", OP_TYPE_END);
  GeTensorDesc dst_tensor_desc(GeShape({1, 256, 256, 512}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  vector<bool> input_const_vector = {false};
  dst_op->SetIsInputConst(input_const_vector);
  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransNodeManager trans_op_insert(fe_ops_kernel_info_store_ptr_);
  Configuration &configure = Configuration::Instance(fe::AI_CORE_NAME);
  configure.soc_version_ = "Hi3796CV300ES";
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
      EXPECT_EQ(shape.GetDims()[0], 100);
      EXPECT_EQ(shape.GetDims()[1], 256);
      EXPECT_EQ(shape.GetDims()[2], 256);
      EXPECT_EQ(shape.GetDims()[3], 512);
    }
    if (node->GetType() == OP_TYPE_END) {
      ge::GeShape shape = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
      EXPECT_EQ(shape.GetDimNum(), 4);
      EXPECT_EQ(shape.GetDims()[0], 1);
      EXPECT_EQ(shape.GetDims()[1], 256);
      EXPECT_EQ(shape.GetDims()[2], 256);
      EXPECT_EQ(shape.GetDims()[3], 512);
    }
  }
  EXPECT_EQ(count_node, 3);
}

TEST_F(STEST_FE_TRANSOP_INSERT, AddReduceReshapeOp_01) {
  string reshape_type = "NC";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  GeTensorDesc src_tensor_desc(GeShape({18, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({16, 32, 3, 3}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  OpDescPtr src_op = std::make_shared<OpDesc>("reduce1", "ReduceOp");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransInfoPtr trans_info_ptr = std::make_shared<TransInfo>();
  trans_info_ptr->src_out_shape = GeShape({2, 3, 3, 1, 16, 16});
  trans_info_ptr->dst_in_shape = GeShape({16, 32, 3, 3});
  trans_info_ptr->src_reshape_type = reshape_type;
  trans_info_ptr->dst_reshape_type = reshape_type;
  trans_info_ptr->src_out_data_type = ge::DT_FLOAT16;
  trans_info_ptr->dst_in_data_type = ge::DT_FLOAT16;
  trans_info_ptr->src_op_pattern = OP_PATTERN_REDUCE;
  trans_info_ptr->src_out_primary_format = ge::FORMAT_FRACTAL_Z;
  trans_info_ptr->dst_in_primary_format = ge::FORMAT_NCHW;

  trans_info_ptr->src_op_desc = src_op;
  trans_info_ptr->dst_op_desc = dst_op;
  trans_info_ptr->src_node_ptr = src_node;
  trans_info_ptr->dst_node_ptr = dst_node;
  trans_info_ptr->src_anchor = src_node->GetOutDataAnchor(0);
  trans_info_ptr->dst_anchor = dst_node->GetInDataAnchor(0);

  TransNodeReshapeGenerator trans_op_insert(fe_ops_kernel_info_store_ptr_, trans_info_ptr);
  Status ret = trans_op_insert.AddTransNode(*graph.get(), trans_info_ptr);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_FE_TRANSOP_INSERT, AddReduceReshapeOp_02) {
  string reshape_type = "NC";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  GeTensorDesc src_tensor_desc(GeShape({16, 32, 3, 3}), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  GeTensorDesc dst_tensor_desc(GeShape({2, 3, 3, 1, 16, 16}), ge::FORMAT_FRACTAL_Z, ge::DT_FLOAT16);
  OpDescPtr src_op = std::make_shared<OpDesc>("B", "B");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  OpDescPtr dst_op = std::make_shared<OpDesc>("reduce2", "ReduceOp");
  dst_op->AddInputDesc(dst_tensor_desc);
  dst_op->AddOutputDesc(dst_tensor_desc);
  auto dst_node = graph->AddNode(dst_op);

  GraphUtils::AddEdge(src_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(0));

  TransInfoPtr trans_info_ptr = std::make_shared<TransInfo>();
  trans_info_ptr->src_out_shape = GeShape({16, 32, 3, 3});
  trans_info_ptr->dst_in_shape = GeShape({2, 3, 3, 1, 16, 16});
  trans_info_ptr->src_reshape_type = reshape_type;
  trans_info_ptr->dst_reshape_type = reshape_type;
  trans_info_ptr->src_out_data_type = ge::DT_FLOAT16;
  trans_info_ptr->dst_in_data_type = ge::DT_FLOAT16;
  trans_info_ptr->dst_op_pattern = OP_PATTERN_REDUCE;
  trans_info_ptr->dst_in_primary_format = ge::FORMAT_FRACTAL_Z;
  trans_info_ptr->src_out_primary_format = ge::FORMAT_NCHW;

  trans_info_ptr->src_op_desc = src_op;
  trans_info_ptr->dst_op_desc = dst_op;
  trans_info_ptr->src_node_ptr = src_node;
  trans_info_ptr->dst_node_ptr = dst_node;
  trans_info_ptr->src_anchor = src_node->GetOutDataAnchor(0);
  trans_info_ptr->dst_anchor = dst_node->GetInDataAnchor(0);

  TransNodeReshapeGenerator trans_op_insert(fe_ops_kernel_info_store_ptr_, trans_info_ptr);
  Status ret = trans_op_insert.AddTransNode(*graph.get(), trans_info_ptr);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNodeBeforeEndOfNetOutput_01)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  vector<int64_t> nchw_dims = {1, 256, 256, 512};
  vector<int64_t> nc1hwc0_dims = {100, 256, 256, 512, 16};
  GeTensorDesc src_tensor_desc(GeShape({100, 256, 256, 512, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_tensor_desc.SetOriginShape(GeShape(nchw_dims));
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", OP_TYPE_END);

  GeTensorDesc dst_tensor_desc(GeShape(nchw_dims), ge::FORMAT_NCHW, ge::DT_FLOAT);
  ge::AttrUtils::SetStr(dst_op, PARENT_OP_TYPE, fe::NETOUTPUT);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  dst_tensor_desc.SetOriginShape(GeShape(nchw_dims));
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
      {
        ge::GeShape shape = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 5);
        EXPECT_EQ(shape.GetDims(), nc1hwc0_dims);
        ge::DataType data_type = node->GetOpDesc()->GetInputDescPtr(
            0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetInputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT16);
        EXPECT_EQ(format, ge::FORMAT_NC1HWC0);
      }
      {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 4);
        EXPECT_EQ(shape.GetDims(), nchw_dims);
        ge::DataType data_type = node->GetOpDesc()->GetOutputDescPtr(
            0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT16);
        EXPECT_EQ(format, ge::FORMAT_NCHW);
      }
    }
    if (node->GetType() == "Cast") {
      {
        ge::GeShape shape = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 4);
        EXPECT_EQ(shape.GetDims(), nchw_dims);
        ge::DataType data_type = node->GetOpDesc()->GetInputDescPtr(0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetInputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT16);
        EXPECT_EQ(format, ge::FORMAT_NCHW);
      }
      {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 4);
        EXPECT_EQ(shape.GetDims(), nchw_dims);
        ge::DataType data_type = node->GetOpDesc()->GetOutputDescPtr(0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT);
        EXPECT_EQ(format, ge::FORMAT_NCHW);
      }
    }
  }
  EXPECT_EQ(count_node, 4);
}


TEST_F(STEST_FE_TRANSOP_INSERT, InsertTransDataNodeBeforeEndOfNetOutput_02)
{
  // src:cce op, dst:cce op
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", OP_TYPE_PLACE_HOLDER);
  vector<int64_t> nchw_dims = {1, 256, 256, 512};
  vector<int64_t> nc1hwc0_dims = {100, 256, 256, 512, 16};
  GeTensorDesc src_tensor_desc(GeShape({100, 256, 256, 512, 16}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  src_tensor_desc.SetOriginShape(GeShape(nchw_dims));
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::AttrUtils::SetInt(src_op, FE_IMPLY_TYPE, 6);

  OpDescPtr dst_op = std::make_shared<OpDesc>("B", "B");

  GeTensorDesc dst_tensor_desc(GeShape(nchw_dims), ge::FORMAT_NCHW, ge::DT_FLOAT16);
  ge::AttrUtils::SetStr(dst_op, PARENT_OP_TYPE, fe::NETOUTPUT);
  ge::AttrUtils::SetInt(dst_op, FE_IMPLY_TYPE, 6);
  dst_tensor_desc.SetOriginFormat(ge::FORMAT_NCHW);
  dst_tensor_desc.SetOriginShape(GeShape(nchw_dims));
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

    if (node->GetType() == "Cast") {
      {
        ge::GeShape shape = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 5);
        EXPECT_EQ(shape.GetDims(), nc1hwc0_dims);
        ge::DataType data_type = node->GetOpDesc()->GetInputDescPtr(0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetInputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT);
        EXPECT_EQ(format, ge::FORMAT_NC1HWC0);
      }
      {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 5);
        EXPECT_EQ(shape.GetDims(), nc1hwc0_dims);
        ge::DataType data_type = node->GetOpDesc()->GetOutputDescPtr(0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT16);
        EXPECT_EQ(format, ge::FORMAT_NC1HWC0);
      }
    }
    if (node->GetType() == "TransData") {
      {
        ge::GeShape shape = node->GetOpDesc()->GetInputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 5);
        EXPECT_EQ(shape.GetDims(), nc1hwc0_dims);
        ge::DataType data_type = node->GetOpDesc()->GetInputDescPtr(
            0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetInputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT16);
        EXPECT_EQ(format, ge::FORMAT_NC1HWC0);
      }
      {
        ge::GeShape shape = node->GetOpDesc()->GetOutputDescPtr(0)->GetShape();
        EXPECT_EQ(shape.GetDimNum(), 4);
        EXPECT_EQ(shape.GetDims(), nchw_dims);
        ge::DataType data_type = node->GetOpDesc()->GetOutputDescPtr(
            0)->GetDataType();
        ge::Format format = node->GetOpDesc()->GetOutputDescPtr(0)->GetFormat();
        EXPECT_EQ(data_type, ge::DT_FLOAT16);
        EXPECT_EQ(format, ge::FORMAT_NCHW);
      }
    }
  }
  EXPECT_EQ(count_node, 4);
}
