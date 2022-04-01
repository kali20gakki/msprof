/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/avgpool_quant_process_fusion_pass.h"

#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

class STEST_BATCH_AVGPOOL_QUANT_PROCESS_FUSION : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown(){}
};

TEST_F(STEST_BATCH_AVGPOOL_QUANT_PROCESS_FUSION, is_global_pooling) {
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  OpDescPtr op_desc_c = std::make_shared<OpDesc>("avgpoolv2", "AvgPoolV2");

  vector<int64_t> dim{3, 2, 2, 3};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NHWC);

  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  // get stride
  vector<int64_t> strides{1, 2, 2, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "strides", strides);
  // get ksize
  vector<int64_t> ksize{1, 2, 2, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "ksize", ksize);
  vector<int64_t> pads{1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "pads", pads);
  // get data_format
  string data_format = "NHWC";
  (void)ge::AttrUtils::SetStr(op_desc_c, "data_format", data_format);
  (void)ge::AttrUtils::SetBool(op_desc_c, "global_pooling", true);

  NodePtr node_c = graph->AddNode(op_desc_c);

  AvgPoolQuantProcessFusionPass pass1;
  QuantProcessMode quant_process_mode = QuantProcessMode::QUANT_UNDIFINED;
  Status ret = pass1.SetQuantProcessModeFromStridesKsizeDataformat(quant_process_mode, node_c);
  EXPECT_EQ(true, ret);
  EXPECT_EQ(QuantProcessMode::QUANT_ROLLBACK, quant_process_mode);
}

TEST_F(STEST_BATCH_AVGPOOL_QUANT_PROCESS_FUSION, PADDING_MODE_IS_CALCULATED) {
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  OpDescPtr op_desc_c = std::make_shared<OpDesc>("avgpoolv2", "AvgPoolV2");

  vector<int64_t> dim{3, 2, 2, 3};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NHWC);

  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  // get stride
  vector<int64_t> strides{1, 2, 2, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "strides", strides);
  // get ksize
  vector<int64_t> ksize{1, 2, 2, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "ksize", ksize);
  vector<int64_t> pads{0, 0, 0, 0};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "pads", pads);
  // get data_format
  string data_format = "NHWC";
  (void)ge::AttrUtils::SetStr(op_desc_c, "data_format", data_format);
  string padding_mode = "CALCULATED";
  (void)ge::AttrUtils::SetStr(op_desc_c, "padding_mode", padding_mode);

  NodePtr node_c = graph->AddNode(op_desc_c);

  AvgPoolQuantProcessFusionPass pass1;
  QuantProcessMode quant_process_mode = QuantProcessMode::QUANT_UNDIFINED;
  Status ret = pass1.SetQuantProcessModeFromStridesKsizeDataformat(quant_process_mode, node_c);
  EXPECT_EQ(true, ret);
  EXPECT_EQ(QuantProcessMode::QUANT_ROLLBACK, quant_process_mode);
}

TEST_F(STEST_BATCH_AVGPOOL_QUANT_PROCESS_FUSION, PADDING_MODE_IS_SAME) {
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  OpDescPtr op_desc_c = std::make_shared<OpDesc>("avgpoolv2", "AvgPoolV2");

  vector<int64_t> dim{3, 2, 2, 3};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NHWC);

  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  // get stride
  vector<int64_t> strides{1, 2, 2, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "strides", strides);
  // get ksize
  vector<int64_t> ksize{1, 2, 2, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "ksize", ksize);
  vector<int64_t> pads{0, 0, 0, 0};
  (void)ge::AttrUtils::SetListInt(op_desc_c, "pads", pads);
  // get data_format
  string data_format = "NHWC";
  (void)ge::AttrUtils::SetStr(op_desc_c, "data_format", data_format);
  string padding_mode = "SAME";
  (void)ge::AttrUtils::SetStr(op_desc_c, "padding_mode", padding_mode);

  NodePtr node_c = graph->AddNode(op_desc_c);

  AvgPoolQuantProcessFusionPass pass1;
  QuantProcessMode quant_process_mode = QuantProcessMode::QUANT_UNDIFINED;
  Status ret = pass1.SetQuantProcessModeFromStridesKsizeDataformat(quant_process_mode, node_c);
  EXPECT_EQ(true, ret);
  EXPECT_EQ(QuantProcessMode::QUANT_ROLLBACK, quant_process_mode);
}

TEST_F(STEST_BATCH_AVGPOOL_QUANT_PROCESS_FUSION, JudgePadAttr) {
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data0", "Data");
  OpDescPtr op_desc_q = std::make_shared<OpDesc>("quant", "AscendQaunt");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("avgpoolv2", "AvgPoolV2");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("dequant", "AscendDeqaunt");

  vector<int64_t> dim{1, 16};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_q->AddInputDesc(out_desc);
  op_desc_q->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);
  op_desc_d->AddOutputDesc(out_desc);

  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_q = graph->AddNode(op_desc_q);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_q->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_q->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  AvgPoolQuantProcessFusionPass pass1;
  ge::NodePtr cube_node =  graph->FindNode("avgpoolv2");
  ge::NodePtr dequant_node =  graph->FindNode("dequant");
  Status ret = pass1.JudgePadAttr(cube_node, dequant_node);
  EXPECT_EQ(fe::SUCCESS, ret);
}