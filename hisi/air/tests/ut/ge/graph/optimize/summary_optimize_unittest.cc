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
#include <gmock/gmock.h>
#include <vector>

#define private public
#define protected public
#include "graph/optimize/graph_optimize.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "framework/omg/omg_inner_types.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class UtestGraphOptimizeSummary : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
 public:
  NodePtr UtAddNode(ComputeGraphPtr &graph, std::string name, std::string type, int in_cnt, int out_cnt){
    auto tensor_desc = std::make_shared<GeTensorDesc>();
    std::vector<int64_t> shape = {1, 1, 224, 224};
    tensor_desc->SetShape(GeShape(shape));
    tensor_desc->SetFormat(FORMAT_NCHW);
    tensor_desc->SetDataType(DT_FLOAT);
    tensor_desc->SetOriginFormat(FORMAT_NCHW);
    tensor_desc->SetOriginShape(GeShape(shape));
    tensor_desc->SetOriginDataType(DT_FLOAT);
    auto op_desc = std::make_shared<OpDesc>(name, type);
    for (int i = 0; i < in_cnt; ++i) {
        op_desc->AddInputDesc(tensor_desc->Clone());
    }
    for (int i = 0; i < out_cnt; ++i) {
        op_desc->AddOutputDesc(tensor_desc->Clone());
    }
    return graph->AddNode(op_desc);
  }
};

TEST_F(UtestGraphOptimizeSummary, GraphNull) {
    ComputeGraphPtr compute_graph = nullptr;
    GraphOptimize base_optimize;
    EXPECT_EQ(base_optimize.HandleSummaryOp(compute_graph), GE_GRAPH_PARAM_NULLPTR);
}

TEST_F(UtestGraphOptimizeSummary, MaxSize) {
    ComputeGraphPtr compute_graph = nullptr;
    GraphOptimize base_optimize;
    for (int i = 1; i < 10010; i++){
        base_optimize.summary_output_indexes_[i] = std::map<std::string, size_t>();
    }
    EXPECT_EQ(base_optimize.HandleSummaryOp(compute_graph), FAILED);
}

TEST_F(UtestGraphOptimizeSummary, SimpleSuccess) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
    auto node = UtAddNode(graph, "data1", "DATA", 0, 1);
    graph->SetGraphID(1);
    GraphOptimize base_optimize;
    base_optimize.summary_output_indexes_[1] = std::map<std::string, size_t>();
    EXPECT_EQ(base_optimize.HandleSummaryOp(graph), SUCCESS);
}

TEST_F(UtestGraphOptimizeSummary, NoInData) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
    auto node = UtAddNode(graph, "data1", "Summary", 0, 1);
    GraphOptimize base_optimize;
    EXPECT_EQ(base_optimize.HandleSummaryOp(graph), GE_GRAPH_PARAM_NULLPTR);
}

TEST_F(UtestGraphOptimizeSummary, NoPeer) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
    auto node = UtAddNode(graph, "data1", "Summary", 1, 1);
    GraphOptimize base_optimize;
    EXPECT_EQ(base_optimize.HandleSummaryOp(graph), GE_GRAPH_PARAM_NULLPTR);
}

TEST_F(UtestGraphOptimizeSummary, HasPeer) {
    ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
    auto node = UtAddNode(graph, "data1", "Summary", 1, 1);
    auto node2 = UtAddNode(graph, "data2", "Data", 1, 1);
    node->GetInDataAnchor(0)->LinkFrom(node2->GetOutDataAnchor(0));
    std::vector<NodePtr> target_nodes_info;
    target_nodes_info.push_back(node);
    graph->SetGraphTargetNodesInfo(target_nodes_info);
    GraphOptimize base_optimize;
    EXPECT_EQ(base_optimize.HandleSummaryOp(graph), SUCCESS);
}



} // namespace ge