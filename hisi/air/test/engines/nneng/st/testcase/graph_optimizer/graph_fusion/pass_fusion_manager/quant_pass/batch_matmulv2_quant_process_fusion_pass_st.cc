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

#define protected public
#define private public
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/batch_matmulv2_quant_process_fusion_pass.h"

#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

class STEST_BATCH_MATMULV2_QUANT_PROCESS_FUSION : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown(){}
};

/*
 *  data0   data1
 *    |       |
 *   batchmatMul
 */
TEST_F(STEST_BATCH_MATMULV2_QUANT_PROCESS_FUSION, GET_CO_VALUE_SUCCESS) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data0", "Data");
  OpDescPtr op_desc_1 = std::make_shared<OpDesc>("data1", "Data");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("batchmatMul", "BatchMatMul");

  //add descriptor
  vector<int64_t> dim{1, 16, 3, 3};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_1->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_1 = graph->AddNode(op_desc_1);
  NodePtr node_c = graph->AddNode(op_desc_c);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_1->GetOutDataAnchor(0), node_c->GetInDataAnchor(1));

  BatchMatmulV2QuantProcessFusionPass pass1;
  ge::NodePtr cube_node = graph->FindNode("batchmatMul");
  int64_t co;
  Status ret = pass1.GetCoValue(cube_node, co);
  EXPECT_EQ(fe::SUCCESS, ret);
}

/*
 *  data0
 *    |
 *  quant   data1
 *    |       |
 *   batchmatMul
 */
TEST_F(STEST_BATCH_MATMULV2_QUANT_PROCESS_FUSION, GET_QUANT_PROCESS_MODE_SUCCESS) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data0", "Data");
  OpDescPtr op_desc_1 = std::make_shared<OpDesc>("data1", "Data");
  OpDescPtr op_desc_q = std::make_shared<OpDesc>("quant", "AscendQaunt");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("batchmatMul", "BatchMatMul");

  //add descriptor
  vector<int64_t> dim{1, 16};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_1->AddOutputDesc(out_desc);
  op_desc_q->AddInputDesc(out_desc);
  op_desc_q->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_1 = graph->AddNode(op_desc_1);
  NodePtr node_q = graph->AddNode(op_desc_q);
  NodePtr node_c = graph->AddNode(op_desc_c);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_q->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_q->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_1->GetOutDataAnchor(0), node_c->GetInDataAnchor(1));

  BatchMatmulV2QuantProcessFusionPass pass1;
  ge::NodePtr quant_node = graph->FindNode("quant");
  ge::NodePtr cube_node =  graph->FindNode("batchmatMul");
  QuantProcessMode quant_process_mode = QuantProcessMode::BIAS_OPTIMIZE;
  Status ret = pass1.GetQuantProcessMode(quant_node, cube_node, quant_process_mode);
  EXPECT_EQ(fe::SUCCESS, ret);
}