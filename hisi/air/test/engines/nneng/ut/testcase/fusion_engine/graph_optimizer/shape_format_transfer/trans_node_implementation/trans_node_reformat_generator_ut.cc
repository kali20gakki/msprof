/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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
#include <iostream>
#include <string>
#include <vector>

#define protected public
#define private public
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_reformat_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

using TransNodeReformatGeneratorPtr = shared_ptr<TransNodeReformatGenerator>;
using TransNodeInsertionPtr = shared_ptr<TransNodeInsertion>;

class TRANS_NODE_REFORMAT_GENERATOR_UTEST: public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(TRANS_NODE_REFORMAT_GENERATOR_UTEST, AddTransNode_suc) {
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr = make_shared<OpStoreAdapterManager>();
  FEOpsKernelInfoStorePtr fe_ops_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr, fe::AI_CORE_NAME);
  TransInfoPtr trans_info_ptr = make_shared<TransInfo>();
  TransNodeReformatGeneratorPtr trans_node_reformat_generator = make_shared<TransNodeReformatGenerator>(fe_ops_store_ptr, trans_info_ptr);

  ComputeGraphPtr fused_graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  OpDescPtr op_desc_1 = std::make_shared<OpDesc>("relu", "relu");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddInputDesc(out_desc);
  op_desc_0->AddOutputDesc(out_desc);
  op_desc_1->AddInputDesc(out_desc);
  op_desc_1->AddOutputDesc(out_desc);
  NodePtr node_add = fused_graph->AddNode(op_desc_0);
  NodePtr node_relu = fused_graph->AddNode(op_desc_1);
  GraphUtils::AddEdge(node_add->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));

  trans_info_ptr->src_op_desc = op_desc_0;
  trans_info_ptr->dst_op_desc = op_desc_1;
  trans_info_ptr->src_anchor = node_add->GetOutDataAnchor(0);
  trans_info_ptr->dst_anchor = node_relu->GetInDataAnchor(0);
  Status ret = trans_node_reformat_generator->AddTransNode(*fused_graph, trans_info_ptr);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(TRANS_NODE_REFORMAT_GENERATOR_UTEST, generate_strategy_by_orignal_format_test) {
  std::vector<std::vector<uint32_t>> strategy_vector_combination = {{1}, {2}, {2, 1}};
  std::vector<TransInfoPtr> trans_info_front_and_end_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr = make_shared<OpStoreAdapterManager>();
  FEOpsKernelInfoStorePtr fe_ops_store_ptr = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr, fe::AI_CORE_NAME);
  TransNodeInsertionPtr trans_node_instertion_ptr = make_shared<TransNodeInsertion>(fe_ops_store_ptr);
  Status ret = trans_node_instertion_ptr->GenerateStrategyByOrignalFormat(strategy_vector_combination);
  EXPECT_EQ(ret, fe::FAILED);
}