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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/stride_hoist_pass.h"
#include "common/pass_manager.h"
#include "graph/compute_graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include "graph_optimizer/graph_fusion/graph_fusion.h"
#include "graph_optimizer/node_optimizer/split_n_optimizer.h"

#undef protected
#undef private

using namespace testing;
using namespace ge;
using namespace fe;
using namespace std;

namespace fe
{

class NODE_OPTIMIZER_UT : public testing::Test
{
public:
    ComputeGraphPtr graph;

protected:
    void SetUp()
    {
        setenv("DUMP_GE_GRAPH", "2", 2);
    }

    void TearDown()
    {
        unsetenv("DUMP_GE_GRAPH");

    }
};

TEST_F(NODE_OPTIMIZER_UT, GetLowestCommonAncestorSucc)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }

    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2a");
    ge::NodePtr ancestor = pass.GetLowestCommonAncestor(node);
    EXPECT_EQ("res2a_branch1_quant_layer", ancestor->GetName());
}

TEST_F(NODE_OPTIMIZER_UT, GetPathsFromLowestCommonAncestorFail)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2a_branch1");
    vector< vector<ge::NodePtr> > paths = pass.GetPathsFromLowestCommonAncestor(node);
    EXPECT_EQ(0, paths.size());
}

TEST_F(NODE_OPTIMIZER_UT, GetPathsFromLowestCommonAncestorSucc)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2a");
    vector< vector<ge::NodePtr> > paths = pass.GetPathsFromLowestCommonAncestor(node);
    EXPECT_EQ(2, paths.size());
    EXPECT_EQ("res2a_branch1_quant_layer", paths[0][0]->GetName());
    EXPECT_EQ(node->GetName(), paths[0][paths[0].size()-1]->GetName());
}

TEST_F(NODE_OPTIMIZER_UT, SetInputOutputShapeOfReadSelectOp)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_relu");
    ge::GeTensorDesc parent_tensor = graph->FindNode("res2b_branch2c")->GetOpDesc()->GetOutputDesc(0);

    ge::GeShape origin_shape = parent_tensor.GetOriginShape();
    int64_t h1 = origin_shape.GetDim(2);
    int64_t w1 = origin_shape.GetDim(3);
    ge::GeShape shape = parent_tensor.GetShape();
    int64_t h2 = shape.GetDim(2);
    int64_t w2 = shape.GetDim(3);

    pass.SetInputOutputShapeOfReadSelectOp(node, parent_tensor);

    ge::OpDescPtr node_opdesc = node->GetOpDesc();
    ge::GeTensorDesc tensor = node_opdesc->GetInputDesc(0);
    origin_shape = tensor.GetOriginShape();
    int64_t h5 = origin_shape.GetDim(2);
    int64_t w5 = origin_shape.GetDim(3);
    shape = tensor.GetShape();
    int64_t h6 = shape.GetDim(2);
    int64_t w6 = shape.GetDim(3);

    tensor = node_opdesc->GetOutputDesc(0);
    origin_shape = tensor.GetOriginShape();
    int64_t h7 = origin_shape.GetDim(2);
    int64_t w7 = origin_shape.GetDim(3);
    shape = tensor.GetShape();
    int64_t h8 = shape.GetDim(2);
    int64_t w8 = shape.GetDim(3);

    EXPECT_EQ(1, h1 / h5);
    EXPECT_EQ(1, w1 / w5);
    EXPECT_EQ(1, h2 / h6);
    EXPECT_EQ(1, w2 / w6);
    EXPECT_EQ(2, h1 / h7);
    EXPECT_EQ(2, w1 / w7);
    EXPECT_EQ(2, h2 / h8);
    EXPECT_EQ(2, w2 / w8);
}

TEST_F(NODE_OPTIMIZER_UT, ConnectReadSelectOp)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node1 = graph->FindNode("res2b_relu");
    ge::NodePtr node2 = graph->FindNode("res2c");
    string node_name = node2->GetName() + "_read_select";
    auto tensor_desc = std::make_shared<ge::GeTensorDesc>();
    auto op_desc = std::make_shared<ge::OpDesc>(node_name, "ReadSelect");
    op_desc->AddInputDesc(tensor_desc->Clone());
    op_desc->AddOutputDesc(tensor_desc->Clone());
    graph->AddNode(op_desc);
    ge::NodePtr rs_node = graph->FindNode(node_name);
    Status st = pass.ConnectReadSelectOp(node1, node2, rs_node);
    EXPECT_EQ(SUCCESS, st);
}

TEST_F(NODE_OPTIMIZER_UT, InsertReadSelectOp)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node1 = graph->FindNode("res2b_relu");
    ge::NodePtr node2 = graph->FindNode("res2c");
    vector<ge::NodePtr> path = {node1, node2};
    ge::NodePtr rs = pass.InsertReadSelectOp(*graph, path);
    EXPECT_EQ(node1->GetName(), rs->GetInNodes().at(0)->GetName());
    EXPECT_EQ(node2->GetName(), rs->GetOutNodes().at(0)->GetName());
}

TEST_F(NODE_OPTIMIZER_UT, HalveShape)
{
    ge::Format format;
    fe::StrideHoistingPass pass;

    ge::GeShape shape1(vector<int64_t>{8,16,56,48,16});
    format = ge::FORMAT_NCHW;
    int64_t h1 = shape1.GetDim(2);
    int64_t w1 = shape1.GetDim(3);
    pass.HalveShape(shape1, format);
    int64_t h2 = shape1.GetDim(2);
    int64_t w2 = shape1.GetDim(3);
    EXPECT_EQ(2, h1 / h2);
    EXPECT_EQ(2, w1 / w2);

    ge::GeShape shape2(vector<int64_t>{8,16,56,48,16});
    format = ge::FORMAT_NC1HWC0;
    h1 = shape2.GetDim(2);
    w1 = shape2.GetDim(3);
    pass.HalveShape(shape2, format);
    h2 = shape2.GetDim(2);
    w2 = shape2.GetDim(3);
    EXPECT_EQ(2, h1 / h2);
    EXPECT_EQ(2, w1 / w2);

    ge::GeShape shape3(vector<int64_t>{8,16,56,48,16});
    format = ge::FORMAT_NHWC;
    h1 = shape3.GetDim(1);
    w1 = shape3.GetDim(2);
    pass.HalveShape(shape3, format);
    h2 = shape3.GetDim(1);
    w2 = shape3.GetDim(2);
    EXPECT_EQ(2, h1 / h2);
    EXPECT_EQ(2, w1 / w2);

    ge::GeShape shape4(vector<int64_t>{8,16,56,48,16});
    format = ge::FORMAT_HWCN;
    h1 = shape4.GetDim(0);
    w1 = shape4.GetDim(1);
    pass.HalveShape(shape4, format);
    h2 = shape4.GetDim(0);
    w2 = shape4.GetDim(1);
    EXPECT_EQ(2, h1 / h2);
    EXPECT_EQ(2, w1 / w2);

    ge::GeShape shape5(vector<int64_t>{8,16,56,48,16});
    format = ge::FORMAT_FRACTAL_Z;
    Status st = pass.HalveShape(shape5, format);
    EXPECT_EQ(st, FAILED);
}

TEST_F(NODE_OPTIMIZER_UT, ReduceInputRelu)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_relu");

    ge::OpDescPtr node_opdesc = node->GetOpDesc();
    ge::GeTensorDesc tensor = node_opdesc->GetInputDesc(0);
    ge::GeShape origin_shape = tensor.GetOriginShape();
    int64_t h1 = origin_shape.GetDim(2);
    int64_t w1 = origin_shape.GetDim(3);

    ge::GeShape shape = tensor.GetShape();
    int64_t h2 = shape.GetDim(2);
    int64_t w2 = shape.GetDim(3);

    pass.ReduceInput(unordered_set<ge::NodePtr>{node});

    node_opdesc = node->GetOpDesc();
    tensor = node_opdesc->GetInputDesc(0);
    origin_shape = tensor.GetOriginShape();
    int64_t h3 = origin_shape.GetDim(2);
    int64_t w3 = origin_shape.GetDim(3);

    shape = tensor.GetShape();
    int64_t h4 = shape.GetDim(2);
    int64_t w4 = shape.GetDim(3);

    EXPECT_EQ(2, h1 / h3);
    EXPECT_EQ(2, w1 / w3);
    EXPECT_EQ(2, h2 / h4);
    EXPECT_EQ(2, w2 / w4);
}

TEST_F(NODE_OPTIMIZER_UT, ReduceInputEltwise)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2c");

    ge::OpDescPtr node_opdesc = node->GetOpDesc();
    ge::GeTensorDesc tensor = node_opdesc->GetInputDesc(1);
    ge::GeShape origin_shape = tensor.GetOriginShape();
    int64_t h1 = origin_shape.GetDim(2);
    int64_t w1 = origin_shape.GetDim(3);

    ge::GeShape shape = tensor.GetShape();
    int64_t h2 = shape.GetDim(2);
    int64_t w2 = shape.GetDim(3);

    pass.ReduceInput(unordered_set<ge::NodePtr>{node});

    node_opdesc = node->GetOpDesc();
    tensor = node_opdesc->GetInputDesc(1);
    origin_shape = tensor.GetOriginShape();
    int64_t h3 = origin_shape.GetDim(2);
    int64_t w3 = origin_shape.GetDim(3);

    shape = tensor.GetShape();
    int64_t h4 = shape.GetDim(2);
    int64_t w4 = shape.GetDim(3);

    EXPECT_EQ(2, h1 / h3);
    EXPECT_EQ(2, w1 / w3);
    EXPECT_EQ(2, h2 / h4);
    EXPECT_EQ(2, w2 / w4);
}

TEST_F(NODE_OPTIMIZER_UT, ReduceOutput)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_branch2c");

    ge::OpDescPtr node_opdesc = node->GetOpDesc();
    ge::GeTensorDesc tensor = node_opdesc->GetOutputDesc(0);
    ge::GeShape origin_shape = tensor.GetOriginShape();
    int64_t h1 = origin_shape.GetDim(2);
    int64_t w1 = origin_shape.GetDim(3);

    ge::GeShape shape = tensor.GetShape();
    int64_t h2 = shape.GetDim(2);
    int64_t w2 = shape.GetDim(3);

    pass.ReduceOutput(unordered_set<ge::NodePtr>{node});

    node_opdesc = node->GetOpDesc();
    tensor = node_opdesc->GetOutputDesc(0);
    origin_shape = tensor.GetOriginShape();
    int64_t h3 = origin_shape.GetDim(2);
    int64_t w3 = origin_shape.GetDim(3);

    shape = tensor.GetShape();
    int64_t h4 = shape.GetDim(2);
    int64_t w4 = shape.GetDim(3);

    EXPECT_EQ(2, h1 / h3);
    EXPECT_EQ(2, w1 / w3);
    EXPECT_EQ(2, h2 / h4);
    EXPECT_EQ(2, w2 / w4);
}

TEST_F(NODE_OPTIMIZER_UT, ReduceInputAndOutput)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_branch2c");

    ge::OpDescPtr node_opdesc = node->GetOpDesc();
    ge::GeTensorDesc tensor = node_opdesc->GetInputDesc(0);
    ge::GeShape origin_shape = tensor.GetOriginShape();
    int64_t h1 = origin_shape.GetDim(2);
    int64_t w1 = origin_shape.GetDim(3);
    ge::GeShape shape = tensor.GetShape();
    int64_t h2 = shape.GetDim(2);
    int64_t w2 = shape.GetDim(3);

    tensor = node_opdesc->GetOutputDesc(0);
    origin_shape = tensor.GetOriginShape();
    int64_t h3 = origin_shape.GetDim(2);
    int64_t w3 = origin_shape.GetDim(3);
    shape = tensor.GetShape();
    int64_t h4 = shape.GetDim(2);
    int64_t w4 = shape.GetDim(3);

    pass.ReduceInputAndOutput(unordered_set<ge::NodePtr>{node});

    node_opdesc = node->GetOpDesc();
    tensor = node_opdesc->GetInputDesc(0);
    origin_shape = tensor.GetOriginShape();
    int64_t h5 = origin_shape.GetDim(2);
    int64_t w5 = origin_shape.GetDim(3);
    shape = tensor.GetShape();
    int64_t h6 = shape.GetDim(2);
    int64_t w6 = shape.GetDim(3);

    tensor = node_opdesc->GetOutputDesc(0);
    origin_shape = tensor.GetOriginShape();
    int64_t h7 = origin_shape.GetDim(2);
    int64_t w7 = origin_shape.GetDim(3);
    shape = tensor.GetShape();
    int64_t h8 = shape.GetDim(2);
    int64_t w8 = shape.GetDim(3);

    EXPECT_EQ(2, h1 / h5);
    EXPECT_EQ(2, w1 / w5);
    EXPECT_EQ(2, h2 / h6);
    EXPECT_EQ(2, w2 / w6);
    EXPECT_EQ(2, h3 / h7);
    EXPECT_EQ(2, w3 / w7);
    EXPECT_EQ(2, h4 / h8);
    EXPECT_EQ(2, w4 / w8);
}

TEST_F(NODE_OPTIMIZER_UT, GetConvAttrs)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_branch2c");
    vector<int64_t> conv_attr = pass.GetConvAttrs(node);
    EXPECT_EQ((vector<int64_t>{1,1,1,1,0,0,1,1}), conv_attr);
}

TEST_F(NODE_OPTIMIZER_UT, GetConvAttrsTypeFail)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_branch2c_quant_layer");
    vector<int64_t> conv_attr = pass.GetConvAttrs(node);
    EXPECT_EQ(0, conv_attr.size());

    node = graph->FindNode("res2c_branch2a_dequant_layer");
    conv_attr = pass.GetConvAttrs(node);
    EXPECT_EQ(0, conv_attr.size());

    node = graph->FindNode("res2c_relu");
    conv_attr = pass.GetConvAttrs(node);
    EXPECT_EQ(0, conv_attr.size());
}

TEST_F(NODE_OPTIMIZER_UT, CheckChildrenTrue)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res3a_branch1_quant_layer");
    bool valid = pass.CheckChildren(node);
    EXPECT_TRUE(valid);
}

TEST_F(NODE_OPTIMIZER_UT, CheckChildrenFalse)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_branch2c_quant_layer");
    bool valid = pass.CheckChildren(node);
    EXPECT_FALSE(valid);

    node = graph->FindNode("res2b_branch2c");
    valid = pass.CheckChildren(node);
    EXPECT_FALSE(valid);

    node = graph->FindNode("res2b_branch2c_dequant_layer");
    valid = pass.CheckChildren(node);
    EXPECT_FALSE(valid);

    node = graph->FindNode("res2b");
    valid = pass.CheckChildren(node);
    EXPECT_FALSE(valid);

    node = graph->FindNode("res2b_relu");
    valid = pass.CheckChildren(node);
    EXPECT_FALSE(valid);
}

TEST_F(NODE_OPTIMIZER_UT, ChangeStrideConvSucc)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2c_branch2a");
    Status status = pass.ChangeStride(node, 2);
    EXPECT_EQ(SUCCESS, status);
}

TEST_F(NODE_OPTIMIZER_UT, ChangeStrideFail)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    ge::NodePtr node = graph->FindNode("res2b_branch2c_quant_layer");
    Status status = pass.ChangeStride(node, 1);
    EXPECT_EQ(FAILED, status);

    node = graph->FindNode("res2b_branch2c_dequant_layer");
    status = pass.ChangeStride(node, 1);
    EXPECT_EQ(FAILED, status);

    node = graph->FindNode("res2b");
    status = pass.ChangeStride(node, 1);
    EXPECT_EQ(FAILED, status);

    node = graph->FindNode("res2b_relu");
    status = pass.ChangeStride(node, 1);
    EXPECT_EQ(FAILED, status);

    node = graph->FindNode("pool1");
    status = pass.ChangeStride(node, 1);
    EXPECT_EQ(FAILED, status);
}

TEST_F(NODE_OPTIMIZER_UT, ConvExists)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector<vector<ge::NodePtr>> paths = {{graph->FindNode("res2b_relu"), graph->FindNode("res2c")},
        {graph->FindNode("res2b_relu"), graph->FindNode("res2c_branch2a_quant_layer"), graph->FindNode("res2c_branch2a"), 
        graph->FindNode("res2c_branch2a_dequant_layer"), graph->FindNode("res2c_branch2b_quant_layer"), graph->FindNode("res2c_branch2b"), 
        graph->FindNode("res2c_branch2b_dequant_layer"), graph->FindNode("res2c_branch2c_quant_layer"), graph->FindNode("res2c_branch2c"), 
        graph->FindNode("res2c_branch2c_dequant_layer"), graph->FindNode("res2c")}};

    bool exists = pass.ConvExists(paths);
    EXPECT_TRUE(exists);
}

TEST_F(NODE_OPTIMIZER_UT, AllPathsAreSimple)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector<vector<ge::NodePtr>> paths = {{graph->FindNode("res2b_relu"), graph->FindNode("res2c")},
        {graph->FindNode("res2b_relu"), graph->FindNode("res2c_branch2a_quant_layer"), graph->FindNode("res2c_branch2a"), 
        graph->FindNode("res2c_branch2a_dequant_layer"), graph->FindNode("res2c_branch2b_quant_layer"), graph->FindNode("res2c_branch2b"), 
        graph->FindNode("res2c_branch2b_dequant_layer"), graph->FindNode("res2c_branch2c_quant_layer"), graph->FindNode("res2c_branch2c"), 
        graph->FindNode("res2c_branch2c_dequant_layer"), graph->FindNode("res2c")}};

    bool simple = pass.AllPathsAreSimple(paths);
    EXPECT_TRUE(simple);
}

TEST_F(NODE_OPTIMIZER_UT, CheckPathsSucc)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector< vector<ge::NodePtr> > paths = {{graph->FindNode("res2b_relu"), graph->FindNode("res2c")},
        {graph->FindNode("res2b_relu"), graph->FindNode("res2c_branch2a_quant_layer"), graph->FindNode("res2c_branch2a"), 
        graph->FindNode("res2c_branch2a_dequant_layer"), graph->FindNode("res2c_branch2b_quant_layer"), graph->FindNode("res2c_branch2b"), 
        graph->FindNode("res2c_branch2b_dequant_layer"), graph->FindNode("res2c_branch2c_quant_layer"), graph->FindNode("res2c_branch2c"), 
        graph->FindNode("res2c_branch2c_dequant_layer"), graph->FindNode("res2c")}};
    
    bool valid = pass.CheckPaths(paths);
    EXPECT_TRUE(valid);
}

TEST_F(NODE_OPTIMIZER_UT, CheckPathsFalse0)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector< vector<ge::NodePtr> > paths = {{graph->FindNode("res2b_relu"), graph->FindNode("res2c")},
        {graph->FindNode("res2b_relu"), graph->FindNode("res2c_branch2a_quant_layer"), graph->FindNode("res2c_branch2a"), 
        graph->FindNode("res2c_branch2a_dequant_layer"), graph->FindNode("res2c_branch2b_quant_layer"), // graph->FindNode("res2c_branch2b"), 
        graph->FindNode("res2c_branch2b_dequant_layer"), graph->FindNode("res2c_branch2c_quant_layer"), graph->FindNode("res2c_branch2c"), 
        graph->FindNode("res2c_branch2c_dequant_layer"), graph->FindNode("res2c")}};
    
    bool valid = pass.CheckPaths(paths);
    EXPECT_FALSE(valid);
}

TEST_F(NODE_OPTIMIZER_UT, CheckPathsFalse2)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector< vector<ge::NodePtr> > paths = {{graph->FindNode("res2b_relu"), graph->FindNode("res2c")},
        {graph->FindNode("res2b_relu"), graph->FindNode("res2c_branch2a_quant_layer"), graph->FindNode("res2c_branch2a"), 
        graph->FindNode("res2c_branch2a_dequant_layer"), graph->FindNode("res2c_branch2b_quant_layer"), graph->FindNode("res2c_branch2b"), 
        graph->FindNode("res2c_branch2b_dequant_layer"), graph->FindNode("res2c_branch2c_quant_layer"), // graph->FindNode("res2c_branch2c"), 
        graph->FindNode("res2c_branch2c_dequant_layer"), graph->FindNode("res2c")}};
    
    bool valid = pass.CheckPaths(paths);
    EXPECT_FALSE(valid);
}

TEST_F(NODE_OPTIMIZER_UT, ChangeGraphSucc)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    vector< vector<ge::NodePtr> > paths = {{graph->FindNode("res2b_relu"), graph->FindNode("res2c")},
        {graph->FindNode("res2b_relu"), graph->FindNode("res2c_branch2a_quant_layer"), graph->FindNode("res2c_branch2a"), 
        graph->FindNode("res2c_branch2a_dequant_layer"), graph->FindNode("res2c_branch2b_quant_layer"), graph->FindNode("res2c_branch2b"), 
        graph->FindNode("res2c_branch2b_dequant_layer"), graph->FindNode("res2c_branch2c_quant_layer"), graph->FindNode("res2c_branch2c"), 
        graph->FindNode("res2c")}};
    vector<ge::NodePtr> new_nodes;
    pass.node_has_children_k1_s2 = graph->FindNode("res3a_branch1_quant_layer");

    Status status = pass.ChangeGraph(*graph, paths);
    EXPECT_EQ(SUCCESS, status);
}

TEST_F(NODE_OPTIMIZER_UT, FusionSucc)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    
    fe::StrideHoistingPass pass;
    
    PatternFusionBasePass::Mapping mapping;
    std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
    op_desc1->id = "Eltwise";
    NodePtr node1 = graph->FindNode("res2c");
    mapping[op_desc1] = vector<ge::NodePtr>{node1};

    std::shared_ptr<FusionPattern::OpDesc> op_desc2 = std::make_shared<FusionPattern::OpDesc>();
    op_desc2->id = "LRelu";
    NodePtr node2 = graph->FindNode("res2c_relu");
    mapping[op_desc2] = vector<ge::NodePtr>{node2};

    std::shared_ptr<FusionPattern::OpDesc> op_desc3 = std::make_shared<FusionPattern::OpDesc>();
    op_desc3->id = "Quant";
    NodePtr node3 = graph->FindNode("res3a_branch1_quant_layer");
    mapping[op_desc3] = vector<ge::NodePtr>{node3};

    // vector< vector<ge::NodePtr> > paths = {{graph->FindNode("res2b_relu"), graph->FindNode("res2c")},
    //     {graph->FindNode("res2b_relu"), graph->FindNode("res2c_branch2a_quant_layer"), graph->FindNode("res2c_branch2a"), 
    //     graph->FindNode("res2c_branch2a_dequant_layer"), graph->FindNode("res2c_branch2b_quant_layer"), graph->FindNode("res2c_branch2b"), 
    //     graph->FindNode("res2c_branch2b_dequant_layer"), graph->FindNode("res2c_branch2c_quant_layer"), graph->FindNode("res2c_branch2c"), 
    //     graph->FindNode("res2c")}};
    vector<ge::NodePtr> new_nodes;
    pass.node_has_children_k1_s2 = graph->FindNode("res3a_branch1_quant_layer");

    Status status = pass.Fusion(*graph, mapping, new_nodes);
    EXPECT_EQ(SUCCESS, status);
}

TEST_F(NODE_OPTIMIZER_UT, FusionQuantEltwiseCase)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }

    PatternFusionBasePass::Mapping mapping;
    std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
    op_desc1->id = "Eltwise";
    NodePtr node1 = graph->FindNode("res2c");
    mapping[op_desc1] = vector<ge::NodePtr>{node1};

    std::shared_ptr<FusionPattern::OpDesc> op_desc2 = std::make_shared<FusionPattern::OpDesc>();
    op_desc2->id = "LRelu";
    NodePtr node2 = graph->FindNode("res2c_relu");
    mapping[op_desc2] = vector<ge::NodePtr>{node2};

    std::shared_ptr<FusionPattern::OpDesc> op_desc3 = std::make_shared<FusionPattern::OpDesc>();
    op_desc3->id = "Quant";
    NodePtr node3 = graph->FindNode("res3a_branch1_quant_layer");
    mapping[op_desc3] = vector<ge::NodePtr>{node3};

    vector<ge::NodePtr> new_nodes;
    
    fe::StrideHoistingPass pass;
    Status status = pass.FusionEltwiseCase(*graph, mapping);
    EXPECT_EQ(SUCCESS, status);
}

TEST_F(NODE_OPTIMIZER_UT, FusionNoQuantEltwiseCase)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00017_OptimizeOriginalGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }

    PatternFusionBasePass::Mapping mapping;
    std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
    op_desc1->id = "Eltwise";
    NodePtr node1 = graph->FindNode("res2c");
    mapping[op_desc1] = vector<ge::NodePtr>{node1};

    std::shared_ptr<FusionPattern::OpDesc> op_desc2 = std::make_shared<FusionPattern::OpDesc>();
    op_desc2->id = "LRelu";
    NodePtr node2 = graph->FindNode("res2c_relu");
    mapping[op_desc2] = vector<ge::NodePtr>{node2};

    vector<ge::NodePtr> new_nodes;
    
    fe::StrideHoistingPass pass;
    Status status = pass.FusionEltwiseCase(*graph, mapping);
    EXPECT_EQ(SUCCESS, status);
}

TEST_F(NODE_OPTIMIZER_UT, FusionQuantConvCase)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00015_OptimizeQuantGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    ge::NodePtr node = graph->FindNode("res3a_branch2c");
    ge::OpDescPtr op_desc = node->GetOpDesc();
    vector<int64_t> strides;
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    strides[2] = 2;
    strides[3] = 2;
    ge::AttrUtils::SetListInt(op_desc, "strides", strides);

    PatternFusionBasePass::Mapping mapping;
    std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
    op_desc1->id = "Conv3x3";
    NodePtr node1 = graph->FindNode("res3a_branch2b");
    mapping[op_desc1] = vector<ge::NodePtr>{node1};

    std::shared_ptr<FusionPattern::OpDesc> op_desc2 = std::make_shared<FusionPattern::OpDesc>();
    op_desc2->id = "Dequant";
    NodePtr node2 = graph->FindNode("res3a_branch2b_dequant_layer");
    mapping[op_desc2] = vector<ge::NodePtr>{node2};

    std::shared_ptr<FusionPattern::OpDesc> op_desc3 = std::make_shared<FusionPattern::OpDesc>();
    op_desc3->id = "Quant";
    NodePtr node3 = graph->FindNode("res3a_branch2c_quant_layer");
    mapping[op_desc3] = vector<ge::NodePtr>{node3};

    vector<ge::NodePtr> new_nodes;
    
    fe::StrideHoistingPass pass;
    Status status = pass.FusionConvCase(*graph, mapping);
    EXPECT_EQ(SUCCESS, status);
    EXPECT_EQ(0, new_nodes.size());
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,1,1}));
    node = graph->FindNode("res3a_branch2b");
    op_desc = node->GetOpDesc();
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,2,2}));
}

TEST_F(NODE_OPTIMIZER_UT, FusionNoQuantConvCase)
{
    graph = std::make_shared<ComputeGraph>("TestGraph");
    std::string load_path("air/test/engines/nneng/ut/testcase/fusion_engine/graph_fusion/ge_proto_00017_OptimizeOriginalGraph_FeGraphFusionAfter.txt");
    bool state = GraphUtils::LoadGEGraph(load_path.c_str(), *graph);
    if (state) {
        std::cout << "========== Load Graph Success! ==========\n";
    }
    ge::NodePtr node = graph->FindNode("res3a_branch2c");
    ge::OpDescPtr op_desc = node->GetOpDesc();
    vector<int64_t> strides;
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    strides[2] = 2;
    strides[3] = 2;
    ge::AttrUtils::SetListInt(op_desc, "strides", strides);

    PatternFusionBasePass::Mapping mapping;
    std::shared_ptr<FusionPattern::OpDesc> op_desc1 = std::make_shared<FusionPattern::OpDesc>();
    op_desc1->id = "Conv3x3";
    NodePtr node1 = graph->FindNode("res3a_branch2b");
    mapping[op_desc1] = vector<ge::NodePtr>{node1};

    std::shared_ptr<FusionPattern::OpDesc> op_desc2 = std::make_shared<FusionPattern::OpDesc>();
    op_desc2->id = "LRelu";
    NodePtr node2 = graph->FindNode("res3a_branch2b_relu");
    mapping[op_desc2] = vector<ge::NodePtr>{node2};

    vector<ge::NodePtr> new_nodes;
    
    fe::StrideHoistingPass pass;
    Status status = pass.FusionConvCase(*graph, mapping);
    EXPECT_EQ(SUCCESS, status);
    EXPECT_EQ(0, new_nodes.size());
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,1,1}));
    node = graph->FindNode("res3a_branch2b");
    op_desc = node->GetOpDesc();
    ge::AttrUtils::GetListInt(op_desc, "strides", strides);
    EXPECT_EQ(strides, (vector<int64_t>{1,1,2,2}));
}

void CreateGraph(ge::ComputeGraphPtr &graph, ge::NodePtr &node, string next_op_type) {
  graph = std::make_shared<ComputeGraph>("TestGraph");

  vector<int64_t> dims{1, 2, 3, 4};
  ge::GeShape shape(dims);

  ge::GeTensorDesc tensor(shape);
  ge::OpDescPtr op = std::make_shared<OpDesc>("test", "Test");
  op->AddInputDesc(tensor);
  op->AddOutputDesc(tensor);

  ge::OpDescPtr next_op = std::make_shared<OpDesc>("next", next_op_type);
  next_op->AddInputDesc(tensor);

  node = graph->AddNode(op);
  auto next_node = graph->AddNode(next_op);

  ge::GraphUtils::AddEdge(node->GetOutDataAnchor(0), next_node->GetInDataAnchor(0));
}

TEST_F(NODE_OPTIMIZER_UT, coverage_01) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;
  ge::NodePtr node;

  CreateGraph(test_graph, node, "NetOutput");
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_UT, coverage_02) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_UT, coverage_03) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), true);
}

TEST_F(NODE_OPTIMIZER_UT, coverage_04) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);
      string fusion_virtual_op ="1";
      (void)ge::AttrUtils::SetStr(next_node->GetOpDesc(), ge::ATTR_NAME_FUSION_VIRTUAL_OP, fusion_virtual_op);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_UT, coverage_05) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);

      (void)ge::AttrUtils::SetBool(next_node->GetOpDesc(), ge::ATTR_NAME_CONTINUOUS_INPUT, true);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), false);
}

TEST_F(NODE_OPTIMIZER_UT, coverage_06) {
  SplitOptimizer so;
  ge::ComputeGraphPtr test_graph;

  ge::NodePtr node;
  CreateGraph(test_graph, node, "Sub");
  for (auto next_node : test_graph->GetDirectNode()) {
    if (next_node->GetName() == "next") {
      (void)ge::AttrUtils::SetInt(next_node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, 1);

      vector<int64_t> output_index = {1,2};
      (void)ge::AttrUtils::SetListInt(next_node->GetOpDesc(), ge::ATOMIC_ATTR_OUTPUT_INDEX, output_index);
    }
  }
  EXPECT_EQ(so.OutputCheck(node), false);
}
} // namespace fe
