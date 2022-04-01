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


#define protected public
#define private public
#include "common/util/op_info_util.h"
#include "common/configuration.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/pooling_quant_process_fusion_pass.h"
#include "common/pass_manager.h"
#include "common/configuration.h"
#include "common/fe_log.h"

#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

namespace fe {
const std::string DATA_TYPE = "Data";

class UTEST_pooling_bias_optimize_quant_rollback : public testing::Test {

protected:
  void SetUp() {}
  void TearDown()
  {

  }

  void InitDequantOp(NodePtr node)
  {
    //初始化Scale算子OpDef
    uint64_t sample_deq_scale[1] = {0x00000000392BCD31};
    vector<GeTensorPtr> scale_weights = OpDescUtils::MutableWeights(node);

    vector<int64_t> dim{1};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
    TensorUtils::SetDataOffset(out_desc, 0);

    GeTensorPtr scale_weight = std::make_shared<ge::GeTensor>(out_desc, (uint8_t *)sample_deq_scale, 1 * sizeof(uint64_t));

    scale_weights.push_back(scale_weight);
    OpDescUtils::SetWeights(node, scale_weights);

  }

  static void DumpGraph(const ge::ComputeGraphPtr graph, string graph_name)
  {
    printf("start to dump graph %s...\n", graph_name.c_str());
    for(ge::NodePtr node : graph->GetAllNodes()) {
      printf("node name = %s.\n", node->GetName().c_str());
      for (ge::OutDataAnchorPtr anchor : node->GetAllOutDataAnchors()) {
        for (ge::InDataAnchorPtr peer_in_anchor : anchor->GetPeerInDataAnchors()) {
          printf("    node name = %s[%d], out data node name = %s[%d].\n",
                 node->GetName().c_str(),
                 anchor->GetIdx(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str(),
                 peer_in_anchor->GetIdx());
        }
      }
      if (node->GetOutControlAnchor() != nullptr) {
        for (ge::InControlAnchorPtr peer_in_anchor : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
          printf("    node name = %s, out control node name = %s.\n", node->GetName().c_str(),
                 peer_in_anchor->GetOwnerNode()->GetName().c_str());
        }
      }
    }

    return;
  }
};

TEST_F(UTEST_pooling_bias_optimize_quant_rollback, bias_optimize_success) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim{3, 32, 9, 9};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init avgpool node
  vector<int64_t> window = {3, 3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);

  InitDequantOp(node_c);
  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  DumpGraph(graph, "before_quant_rollback");

  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  (void)graph->TopologicalSorting();
  DumpGraph(graph, "after_quant_rollback");

  int32_t op_num = graph->GetDirectNode().size();
  EXPECT_EQ(op_num, 6);

}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, bias_optimize_success1) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim{3, 32, 9, 9};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init avgpool node
  vector<int64_t> window = {3, 3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 1;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);

  InitDequantOp(node_c);
  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  DumpGraph(graph, "before_quant_rollback");

  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  (void)graph->TopologicalSorting();
  DumpGraph(graph, "after_quant_rollback");

  int32_t op_num = graph->GetDirectNode().size();
  EXPECT_EQ(op_num, 6);

}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, quant_rollback_pattern_success) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  
  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");
  
  //add descriptor
  vector<int64_t> dim{3, 32, 1, 1};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  
  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);
  
  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init pooling node
  vector<int64_t> window = {3, 3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);
  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
  InitDequantOp(node_c);
  DumpGraph(graph, "before_quant_rollback");
  
  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  (void)graph->TopologicalSorting();
  DumpGraph(graph, "after_quant_rollback");
  
  int32_t op_num = graph->GetDirectNode().size();
  EXPECT_EQ(op_num, 3);
}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, get_quant_mode_fail1) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim{3, 32, 1, 1};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init pooling node
  vector<int64_t> window = {3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  DumpGraph(graph, "before_quant_rollback");

  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::FAILED, status);
}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, get_quant_mode_fail2) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim{3, 32, 1, 1};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init pooling node
  vector<int64_t> window = {3, 3};
  vector<int64_t> stride = {3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  DumpGraph(graph, "before_quant_rollback");

  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::FAILED, status);
}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, get_quant_mode_fail3) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim{3, 32, 1, 1};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init pooling node
  vector<int64_t> window = {3, 3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  DumpGraph(graph, "before_quant_rollback");

  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::FAILED, status);
}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, get_quant_mode_fail4) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim{3, 32};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init pooling node
  vector<int64_t> window = {3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  DumpGraph(graph, "before_quant_rollback");

  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::FAILED, status);
}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, get_quant_mode_fail5) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim{3, 32};
  GeShape shape(dim);
  GeTensorDesc in_desc(shape, ge::FORMAT_NCHW);
  vector<int64_t> dim1{3, 32, 1, 1};
  GeShape shape1(dim1);
  GeTensorDesc out_desc(shape1, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(in_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(in_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(in_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(in_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  // init pooling node
  vector<int64_t> window = {3, 3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  DumpGraph(graph, "before_quant_rollback");

  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::FAILED, status);
}
TEST_F(UTEST_pooling_bias_optimize_quant_rollback, quant_rollback_pattern_success1) {
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  
  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Pooling");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");
  OpDescPtr op_desc_e = std::make_shared<OpDesc>("E", "Conv2D");
  
  //add descriptor
  vector<int64_t> dim{3, 32, 1, 1};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
  
  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);
  op_desc_e->AddInputDesc(out_desc);
  
  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);
  NodePtr node_e = graph->AddNode(op_desc_e);

  // init pooling node
  vector<int64_t> window = {3, 3};
  vector<int64_t> stride = {3, 3};
  int64_t ceil_mode = 0;
  vector<int64_t> pad = {1, 1, 1, 1};
  (void)ge::AttrUtils::SetListInt(op_desc_b, "window", window);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "stride", stride);
  (void)ge::AttrUtils::SetInt(op_desc_b, "ceil_mode", ceil_mode);
  (void)ge::AttrUtils::SetListInt(op_desc_b, "pad", pad);
  (void)ge::AttrUtils::SetInt(op_desc_b, "mode", 1);
  
  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
  InitDequantOp(node_c);
  DumpGraph(graph, "before_quant_rollback");
  
  //执行融合
  PoolingQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  (void)graph->TopologicalSorting();
  DumpGraph(graph, "after_quant_rollback");
  
  int32_t op_num = graph->GetDirectNode().size();
  EXPECT_EQ(op_num, 6);
}

}
