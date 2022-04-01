/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#define private public
#define protected public

#include "inc/pass_manager.h"
#include "common/local_context.h"
#include "graph/passes/multi_batch_clone_pass.h"
#include "graph/passes/subexpression_migration_pass.h"
#include "graph/passes/unused_args_clean_pass.h"
#include "framework/omg/omg_inner_types.h"
#include "register/op_registry.h"
#include "graph/utils/graph_utils.h"

namespace ge {
/***********************************************************************************************************************
                                      +-----------+ +-----------+ +-----------+
                                      |   Const   | |   Const   | |   Const   |
                                      +-----------+ +-----------+ +-----------+
                                                \         |        /
                                                 \        |       /
+-----------+       +-----------+                   +-----------+
|   Const   |       |   Const   |                   |  BnHost   |
+-----------+       +-----------+                   +-----------+
      |                   |                        /             \
      |                   |                       /               \
+-----------+       +-----------+       +-----------+           +-----------+       +-----------+ +-----------+ +-----------+
|   Cast    |       |   Cast    |       |   Cast    |           |   Cast    |       |   Data    | |   Const   | |   Const   |
+-----------+       +-----------+       +-----------+           +-----------+       +-----------+ +-----------+ +-----------+
      |                   |                   |                       |                   |             |             |
      |                   |                   |                       |                   |             |             |
+-----------+       +-----------+       +-----------+           +-----------+       +-----------+ +-----------+ +-----------+
| TransData |       | TransData |       |  Reshape  |           |  Reshape  |       |    Cast   | |    Cast   | |    Cast   |
+-----------+       +-----------+       +-----------+           +-----------+       +-----------+ +-----------+ +-----------+
      |                   |                   |                       |                          \      |      /
      |                   |                   |                       |                           \     |     /
+-----------+       +-----------+       +-----------+           +-----------+                     +-----------+
|  Reshape  |       |  Reshape  |       | TransData |           | TransData |                     |   Conv2D  |
+-----------+       +-----------+       +-----------+           +-----------+                     +-----------+
      |                   |                          \                |                          /
      |                   |                           \               |                         /
+-----------+       +-----------+                      \        +-----------+                  /
| TransData |       | TransData |                       --------|BnInference|-----------------
+-----------+       +-----------+                               +-----------+
             \                   \                             /
              \                   \                           /
               \                   \        +-----------+    /
                ----------------------------|   Scale   |----
                                            +-----------+
                                                  |
                                                  |
                                            +-----------+
                                            | NetOutput |
                                            +-----------+
***********************************************************************************************************************/
class UTEST_UnusedArgsCleanPass : public testing::Test {
 protected:
  void SetUp() {
    SetLocalOmgContext(domi::GetContext());
    GetLocalOmgContext().dynamic_batch_size.clear();
    GetLocalOmgContext().dynamic_image_size.clear();
    GetLocalOmgContext().dynamic_dims.clear();
    GetLocalOmgContext().need_multi_batch = false;
  }

  void TearDown() {
    GetLocalOmgContext().dynamic_batch_size.clear();
    GetLocalOmgContext().dynamic_image_size.clear();
    GetLocalOmgContext().dynamic_dims.clear();
    GetLocalOmgContext().need_multi_batch = false;
  }

 public:
  NodePtr MakeNode(const ComputeGraphPtr &graph, uint32_t in_num, uint32_t out_num, string name, string type) {
    static uint32_t index = 0;
    GeTensorDesc test_desc(GeShape(), FORMAT_NCHW, DT_FLOAT);
    auto op_desc = std::make_shared<OpDesc>(name + "_" + std::to_string(index++), type);
    for (uint32_t i = 0; i < in_num; ++i) {
      op_desc->AddInputDesc(test_desc);
    }
    for (uint32_t i = 0; i < out_num; ++i) {
      op_desc->AddOutputDesc(test_desc);
    }
    return graph->AddNode(op_desc);
  }

  void MakeDataChain(const ComputeGraphPtr &graph, NodePtr &head, NodePtr &tail) {
    auto trans_0 = MakeNode(graph, 1, 1, "data",       "Data");
    auto trans_1 = MakeNode(graph, 1, 1, "trans_Cast", "Cast");

    GraphUtils::AddEdge(trans_0->GetOutDataAnchor(0), trans_1->GetInDataAnchor(0));
    head = trans_0;
    tail = trans_1;
  }

  void MakeConstChain(const ComputeGraphPtr &graph, NodePtr &head, NodePtr &tail) {
    auto trans_0 = MakeNode(graph, 1, 1, "dynamic_const", "Const");
    auto trans_1 = MakeNode(graph, 1, 1, "trans_Cast",    "Cast");

    GraphUtils::AddEdge(trans_0->GetOutDataAnchor(0), trans_1->GetInDataAnchor(0));
    head = trans_0;
    tail = trans_1;
  }

  void MakeNodeChain1(const ComputeGraphPtr &graph, NodePtr &head, NodePtr &tail) {
    auto trans_1 = MakeNode(graph, 1, 1, "trans_Cast",      "Cast");
    auto trans_2 = MakeNode(graph, 1, 1, "trans_Reshape",   "Reshape");
    auto trans_3 = MakeNode(graph, 1, 1, "trans_TransData", "TransData");

    GraphUtils::AddEdge(trans_1->GetOutDataAnchor(0), trans_2->GetInDataAnchor(0));
    GraphUtils::AddEdge(trans_2->GetOutDataAnchor(0), trans_3->GetInDataAnchor(0));
    head = trans_1;
    tail = trans_3;
  }

  void MakeNodeChain2(const ComputeGraphPtr &graph, NodePtr &head, NodePtr &tail) {
    auto const_1 = MakeNode(graph, 0, 1, "dynamic_const",   "Const");
    auto trans_1 = MakeNode(graph, 1, 1, "trans_Cast",      "Cast");
    auto trans_2 = MakeNode(graph, 1, 1, "trans_TransData", "TransData");
    auto trans_3 = MakeNode(graph, 1, 1, "trans_Reshape",   "Reshape");
    auto trans_4 = MakeNode(graph, 1, 1, "trans_TransData", "TransData");

    GraphUtils::AddEdge(const_1->GetOutDataAnchor(0), trans_1->GetInDataAnchor(0));
    GraphUtils::AddEdge(trans_1->GetOutDataAnchor(0), trans_2->GetInDataAnchor(0));
    GraphUtils::AddEdge(trans_2->GetOutDataAnchor(0), trans_3->GetInDataAnchor(0));
    GraphUtils::AddEdge(trans_3->GetOutDataAnchor(0), trans_4->GetInDataAnchor(0));
    head = const_1;
    tail = trans_4;
  }

  void make_original_graph(const ComputeGraphPtr &graph) {
    NodePtr head, tail;
    auto conv2d_node = MakeNode(graph, 3, 1, "conv1", "Conv2D");
    {
      MakeDataChain(graph, head, tail);
      GeTensorDesc tensor_desc(GeShape({-1, 3, 224, 224}), FORMAT_NCHW, DT_FLOAT);
      head->GetOpDesc()->UpdateInputDesc(0, tensor_desc);
      head->GetOpDesc()->UpdateOutputDesc(0, tensor_desc);
      AttrUtils::SetInt(head->GetOpDesc(), ATTR_NAME_INDEX, 0);
      GraphUtils::AddEdge(tail->GetOutDataAnchor(0), conv2d_node->GetInDataAnchor(0));

      MakeConstChain(graph, head, tail);
      GraphUtils::AddEdge(tail->GetOutDataAnchor(0), conv2d_node->GetInDataAnchor(1));
      MakeConstChain(graph, head, tail);
      GraphUtils::AddEdge(tail->GetOutDataAnchor(0), conv2d_node->GetInDataAnchor(2));
    }

    NodePtr bn_head_0, bn_tail_0;
    NodePtr bn_head_1, bn_tail_1;
    auto bn_host = MakeNode(graph, 3, 2, "bn_conv1_BnHost", "BnHost");
    {
      for (uint32_t i = 0; i < 3; ++i) {
        auto const_0 = MakeNode(graph, 0, 1, "dynamic_const", "Const");
        GraphUtils::AddEdge(const_0->GetOutDataAnchor(0), bn_host->GetInDataAnchor(i));
      }

      MakeNodeChain1(graph, bn_head_0, bn_tail_0);
      MakeNodeChain1(graph, bn_head_1, bn_tail_1);
      GraphUtils::AddEdge(bn_host->GetOutDataAnchor(0), bn_head_0->GetInDataAnchor(0));
      GraphUtils::AddEdge(bn_host->GetOutDataAnchor(1), bn_head_1->GetInDataAnchor(0));
    }

    auto  bn_conv1 = MakeNode(graph, 3, 1, "bn_conv1_BNInferenceD", "BNInferenceD");
    {
      GraphUtils::AddEdge(conv2d_node->GetOutDataAnchor(0),  bn_conv1->GetInDataAnchor(0));

      GraphUtils::AddEdge(bn_tail_0->GetOutDataAnchor(0),  bn_conv1->GetInDataAnchor(1));
      GraphUtils::AddEdge(bn_tail_1->GetOutDataAnchor(0),  bn_conv1->GetInDataAnchor(2));
    }

    auto scale_conv1 = MakeNode(graph, 3, 1, "scale_conv1", "Scale");
    {
      GraphUtils::AddEdge(bn_conv1->GetOutDataAnchor(0), scale_conv1->GetInDataAnchor(0));

      MakeNodeChain2(graph, head, tail);
      GraphUtils::AddEdge(tail->GetOutDataAnchor(0), scale_conv1->GetInDataAnchor(1));

      MakeNodeChain2(graph, head, tail);
      GraphUtils::AddEdge(tail->GetOutDataAnchor(0), scale_conv1->GetInDataAnchor(2));
    }

    auto output_node = MakeNode(graph, 1, 0, "output1", "NetOutput");
    GraphUtils::AddEdge(scale_conv1->GetOutDataAnchor(0), output_node->GetInDataAnchor(0));
  }
};

class FeInsertTransNodePass : public GraphPass {
 public:
  Status Run(ComputeGraphPtr graph) {
    graph->TopologicalSorting();
    for (const auto &node : graph->GetAllNodes()) {
        if (kTransOpTypes.count(node->GetType()) > 0) {
            node->SetHostNode(true);
        }
    }

    return SUCCESS;
  }

  const std::set<std::string> kTransOpTypes = { "Cast", "TransData", "Reshape", "BnHost" };
};

TEST_F(UTEST_UnusedArgsCleanPass, Run) {

  PassManager pass_manager;
  pass_manager.AddPass("MultiBatchClonePass", new (std::nothrow) MultiBatchClonePass(0));
  pass_manager.AddPass("FeInsertTransNodePass", new (std::nothrow) FeInsertTransNodePass);
  pass_manager.AddPass("SubexpressionMigrationPass", new (std::nothrow) SubexpressionMigrationPass);
  pass_manager.AddPass("UnusedArgsCleanPass", new (std::nothrow) UnusedArgsCleanPass);


  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph");
  make_original_graph(graph);
  EXPECT_EQ(graph->GetAllNodes().size(), 30);

  GetLocalOmgContext().dynamic_batch_size = "1,2,4,8";
  GetLocalOmgContext().need_multi_batch = true;

  EXPECT_EQ(pass_manager.Run(graph), SUCCESS);
  EXPECT_EQ(graph->GetAllSubgraphs().size(), 4);
  EXPECT_EQ(graph->GetDirectNode().size(), 31);

  for (const auto subgraph : graph->GetAllSubgraphs()) {
    EXPECT_EQ(subgraph->GetAllNodes().size(), 11);
  }
}

}   // namespace ge
