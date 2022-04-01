#include <gtest/gtest.h>

#define protected public
#define private public
#include "tf_engine/engine/tf_engine.h"
#include "tf_optimizer/tf_optimizer.h"
#include "tf_optimizer/tf_optimizer_utils.h"
#undef private
#undef protected

#include "util/tf_util.h"
#include "config/config_file.h"
#include "graph/debug/ge_attr_define.h"
#include "ge/ge_api_types.h"

using namespace aicpu;
using namespace ge;
using namespace std;

TEST(TfGraphOptimizer, Initialize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    string kernelConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPUOpsKernel", kernelConfig), true);
    ASSERT_EQ(kernelConfig, "TFKernel");
    string optimizerConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPUGraphOptimizer", optimizerConfig), true);
    ASSERT_EQ(optimizerConfig, "TFOptimizer");
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);

    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->Initialize(options, nullptr), SUCCESS);
}

TEST(TfGraphOptimizer, OptimizeOriginalGraphJudgeInsert_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);

    OpDescPtr opDescPtr = make_shared<OpDesc>("ResizeArea","ResizeArea");
    vector<int64_t> tensorShape1 = {1,5,5,3};
    GeTensorDesc tensor1(GeShape(tensorShape1), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("images", tensor1);
    vector<int64_t> tensorShape2 = {2};
    GeTensorDesc tensor2(GeShape(tensorShape2), FORMAT_ND, DT_INT32);
    opDescPtr->AddInputDesc("size", tensor2);
    vector<int64_t> tensorShape3 = {1,4,4,3};
    GeTensorDesc tensor3(GeShape(tensorShape3), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddOutputDesc("y", tensor3);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    graphPtr->AddNode(opDescPtr);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeOriginalGraphJudgeInsert(*(graphPtr.get())), SUCCESS);
    ASSERT_EQ(opDescPtr->GetInputDesc("images").GetFormat(), FORMAT_NHWC);
    ASSERT_EQ(opDescPtr->GetOutputDesc("y").GetFormat(), FORMAT_NHWC);
}

TEST(TfGraphOptimizer, OptimizeFusedGraph_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    vector<int64_t> tensorShape = {1,2,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    tensor1.SetOriginFormat(FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "Add");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(TfGraphOptimizer, OptimizeCacheGraph_SUCCESS)
{
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    // create placehoder1 node
    OpDescPtr placeholderOpDescPtr1 = make_shared<OpDesc>("placeholder1", "PlaceHolder");
    AttrUtils::SetStr(placeholderOpDescPtr1, ATTR_NAME_PLD_FRONT_NODE_ENGINE_NAME, "AIcoreEngine");
    NodePtr placeholderNodePtr1 = make_shared<Node>(placeholderOpDescPtr1, graphPtr);
    placeholderNodePtr1->UpdateOpDesc(placeholderOpDescPtr1);
    vector<int64_t> tensorShape = {1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_ND, DT_INT32);
    placeholderOpDescPtr1->AddOutputDesc("x", tensor1);
    placeholderNodePtr1->Init();
    graphPtr->AddNode(placeholderNodePtr1);

    // create cast node
    OpDescPtr castOpDescPtr = make_shared<OpDesc>("cast", "Cast");
    NodePtr castNodePtr = make_shared<Node>(castOpDescPtr, graphPtr);
    castNodePtr->UpdateOpDesc(castOpDescPtr);
    castOpDescPtr->AddOutputDesc("y", tensor1);
    castNodePtr->Init();
    castNodePtr->AddLinkFrom(placeholderNodePtr1);
    graphPtr->AddNode(castNodePtr);

    OpDescPtr endOpDescPtr = make_shared<OpDesc>("end", "End");
    AttrUtils::SetStr(endOpDescPtr, ATTR_NAME_END_REAR_NODE_ENGINE_NAME, "AIcoreEngine");
    AttrUtils::SetStr(endOpDescPtr, "parentOpType", "NetOutput");
    NodePtr endNodePtr = make_shared<Node>(endOpDescPtr, graphPtr);
    endNodePtr->UpdateOpDesc(endOpDescPtr);
    endOpDescPtr->AddOutputDesc("output", tensor1);
    endNodePtr->Init();
    endNodePtr->AddLinkFrom(castNodePtr);
    graphPtr->AddNode(endNodePtr);

    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);
    map<string, string> options;
    options[ge::SOC_VERSION] = "Hi3796es";
    ASSERT_EQ(Initialize(options), SUCCESS);
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->Initialize(options, nullptr), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(TfGraphOptimizer, OptimizeIsRefTensor_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    vector<int64_t> tensorShape = {1,2,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    tensor1.SetOriginFormat(FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "Add");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), 20000046);
}


TEST(TfGraphOptimizer, OptimizeOriginalGraph_SUCCESS)
{
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);
    map<string, string> options;
    options[ge::SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->Initialize(options, nullptr), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeOriginalGraph(*(graphPtr.get())), SUCCESS);
}

TEST(TfGraphOptimizer, OptimizeWholeGraph_SUCCESS)
{
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);
    map<string, string> options;
    options[ge::SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->Initialize(options, nullptr), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeWholeGraph(*(graphPtr.get())), SUCCESS);
}

TEST(TfGraphOptimizer, RefCreateAndInsert_Succeed)
{
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    vector<int64_t> tensorShape = {1,2,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    tensor1.SetOriginFormat(FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("cast", "Cast");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("end", "End");

    Op1DescPtr->AddOutputDesc("z",tensor1);

    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);


    map<string, string> options;
    options[ge::SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->Initialize(options, nullptr), SUCCESS);

    ComputeGraph &graph = *(graphPtr.get());

    OutDataAnchorPtr srcAnchor1 = node1->GetOutDataAnchor(0);
    InDataAnchorPtr dstAnchor1 = node2->GetInDataAnchor(0);
    NodePtr variableNode;
    NodePtr assignNode;
    NodePtr identityNode;

    Status ret = TfVariableGraph::CreateAndInsertVariable(srcAnchor1, dstAnchor1, variableNode, graph);

    ASSERT_EQ(SUCCESS, ret);

    ret = TfVariableGraph::CreateAndInsertAssign(srcAnchor1, dstAnchor1, variableNode, graph);

    ASSERT_EQ(SUCCESS, ret);

    OutDataAnchorPtr srcAnchor3 = node2->GetOutDataAnchor(0);
    InDataAnchorPtr dstAnchor3 = node3->GetInDataAnchor(0);

    ret = TfVariableGraph::CreateAndInsertIdentity(srcAnchor3, dstAnchor3, graph);

    ASSERT_EQ(SUCCESS, ret);
}

TEST(TfGraphOptimizer, RefInsert_Succeed)
{
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    vector<int64_t> tensorShape = {1,2,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    tensor1.SetOriginFormat(FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("cast", "Cast");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("end", "End");

    Op1DescPtr->AddOutputDesc("z",tensor1);

    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);


    map<string, string> options;
    options[ge::SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->Initialize(options, nullptr), SUCCESS);

    ComputeGraph &graph = *(graphPtr.get());

    OutDataAnchorPtr srcAnchor1 = node1->GetOutDataAnchor(0);
    InDataAnchorPtr dstAnchor1 = node2->GetInDataAnchor(0);
    NodePtr variableNode;
    NodePtr assignNode;
    NodePtr identityNode;
    GeTensorDesc tensor2(GeShape(tensorShape), FORMAT_ND, DT_INT32);
    GeTensorDesc tensor3(GeShape(tensorShape), FORMAT_ND, DT_INT32);
    GeTensorDesc tensor4(GeShape(tensorShape), FORMAT_ND, DT_INT32);

    Status ret = TfVariableGraph::CreateVariable(tensor2, graph, variableNode);
    ret = TfVariableGraph::InsertVariable(srcAnchor1, dstAnchor1, variableNode);

    ASSERT_EQ(SUCCESS, ret);

    ret = TfVariableGraph::CreateAssign(tensor2, tensor3, tensor4, graph, assignNode);
    ret = TfVariableGraph::InsertAssign(srcAnchor1, dstAnchor1, variableNode, assignNode);

    ASSERT_EQ(SUCCESS, ret);

    OutDataAnchorPtr srcAnchor3 = node2->GetOutDataAnchor(0);
    InDataAnchorPtr dstAnchor3 = node3->GetInDataAnchor(0);

    ret = TfVariableGraph::CreateIdentity(tensor2, tensor3, graph, identityNode);
    ret = TfVariableGraph::InsertIdentity(srcAnchor3, dstAnchor3, identityNode);

    ASSERT_EQ(SUCCESS, ret);
}

TEST(TfGraphOptimizer, CheckSubGraphSupportFuse_SUCCESS)
{
    TfOptimizer TfOptimizer;
    ge::ComputeGraphPtr sub_graph = std::make_shared<ge::ComputeGraph>("subGraph");
    SubGraphInfo sub_graph_info;
    std::unordered_map<std::string, ge::NodePtr> tf_isolated_node_map;
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");

    vector<int64_t> tensorShape = {2, 2, 3, 4};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    tensor1.SetOriginFormat(FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "AddTest_REF");
    Op1DescPtr->AddOutputDesc("z", tensor1);
    auto node1 = graphPtr->AddNode(Op1DescPtr);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "AddTest_REF");
    Op2DescPtr->AddOutputDesc("z", tensor1);
    NodePtr node2 = make_shared<Node>(Op2DescPtr, graphPtr);
    node2->Init();
    graphPtr->AddNode(node2);
    node2->AddLinkFrom(node1);

    OpDescPtr Op3DescPtr = make_shared<OpDesc>("add3", "AddTest_REF");
    Op3DescPtr->AddOutputDesc("z", tensor1);
    NodePtr node3 = make_shared<Node>(Op3DescPtr, graphPtr);
    graphPtr->AddNode(node3);
    node3->AddLinkFrom(node2);

    std::vector<ge::NodePtr> nodeList;
    nodeList.push_back(node2);
    TfOptimizer.InsertNodesToSubGraph(sub_graph, nodeList, sub_graph_info);
    ASSERT_EQ(TfOptimizer.CheckSubGraphSupportFuse(nodeList, sub_graph_info, tf_isolated_node_map), true);
}
TEST(TfGraphOptimizer, OptimizeFusedGraphFFTSAutoMode_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);

    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 2;
    thread_slice_map.thread_mode = 0;
    ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    vector<int64_t> tensorShape = {1, 2, 3, 4};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    AttrUtils::SetListInt(tensor1, "ref_port_index", {1, 1, 1});
    tensor1.SetOriginFormat(FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    OpDescPtr opDescPtr0 = make_shared<OpDesc>("add0", "Add");
    opDescPtr0->AddOutputDesc("z", tensor1);
    opDescPtr0->SetExtAttr("_sgt_struct_info", tsmp_ptr);
    ge::AttrUtils::SetInt(opDescPtr0, "_thread_scope_id", 1);
    ge::AttrUtils::SetBool(opDescPtr0, "_aicpu_private", true);
    auto node1 = graphPtr->AddNode(opDescPtr0);

    OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "Add");
    Op1DescPtr->AddOutputDesc("z", tensor1);
    Op1DescPtr->SetExtAttr("_sgt_struct_info", tsmp_ptr);
    ge::AttrUtils::SetInt(Op1DescPtr, "_thread_scope_id", 1);
    ge::AttrUtils::SetBool(Op1DescPtr, "_aicpu_private", true);
    auto node2 = graphPtr->AddNode(Op1DescPtr);
    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");
    Op2DescPtr->AddOutputDesc("z", tensor1);
    Op2DescPtr->SetExtAttr("_sgt_struct_info", tsmp_ptr);
    ge::AttrUtils::SetInt(Op2DescPtr, "_thread_scope_id", 1);
    ge::AttrUtils::SetBool(Op2DescPtr, "_aicpu_private", true);
    auto node3 = graphPtr->AddNode(Op2DescPtr);
    node3->AddLinkFrom(node2);

    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(TfGraphOptimizer, OptimizeFusedGraphFFTSManualMode_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_tf_optimizer"], nullptr);

    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 2;
    thread_slice_map.thread_mode = 1;
    ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    vector<int64_t> tensorShape = {1, 2, 3, 4};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    AttrUtils::SetListInt(tensor1, "ref_port_index", {1, 1, 1});
    tensor1.SetOriginFormat(FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    OpDescPtr opDescPtr0 = make_shared<OpDesc>("add0", "Add");
    opDescPtr0->AddOutputDesc("z", tensor1);
    opDescPtr0->SetExtAttr("_sgt_struct_info", tsmp_ptr);
    ge::AttrUtils::SetInt(opDescPtr0, "_thread_scope_id", 1);
    ge::AttrUtils::SetBool(opDescPtr0, "_aicpu_private", true);
    auto node1 = graphPtr->AddNode(opDescPtr0);

    OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "Add");
    Op1DescPtr->AddOutputDesc("z", tensor1);
    Op1DescPtr->SetExtAttr("_sgt_struct_info", tsmp_ptr);
    ge::AttrUtils::SetInt(Op1DescPtr, "_thread_scope_id", 1);
    ge::AttrUtils::SetBool(Op1DescPtr, "_aicpu_private", true);
    auto node2 = graphPtr->AddNode(Op1DescPtr);
    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");
    Op2DescPtr->AddOutputDesc("z", tensor1);
    Op2DescPtr->SetExtAttr("_sgt_struct_info", tsmp_ptr);
    ge::AttrUtils::SetInt(Op2DescPtr, "_thread_scope_id", 1);
    ge::AttrUtils::SetBool(Op2DescPtr, "_aicpu_private", false);
    auto node3 = graphPtr->AddNode(Op2DescPtr);
    node3->AddLinkFrom(node2);

    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}