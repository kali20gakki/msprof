#include <gtest/gtest.h>

#include "aicpu_engine/engine/aicpu_engine.h"

#include "util/util.h"
#include "config/config_file.h"
#include "stub.h"
#include "common/sgt_slice_type.h"
#include "ge/ge_api_types.h"

using namespace aicpu;
using namespace ge;
using namespace std;

TEST(AicpuKernelBuilder, Initialize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;

    GetOpsKernelBuilderObjs(opsKernelBuilders);
    ASSERT_NE(opsKernelBuilders["aicpu_ascend_builder"], nullptr);
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->Initialize(options), SUCCESS);
}

TEST(AicpuKernelBuilder, CalcOpRunningParam_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    ASSERT_NE(opsKernelInfoStores["aicpu_ascend_kernel"], nullptr);
    ASSERT_EQ(opsKernelInfoStores["aicpu_ascend_kernel"]->Initialize(options), SUCCESS);
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->Initialize(options, nullptr), SUCCESS);
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    ASSERT_NE(opsKernelBuilders["aicpu_ascend_builder"], nullptr);
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->Initialize(options), SUCCESS);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add","Add");
    AttrUtils::SetStr(opDescPtr, "resource", "RES_QUEUE");
    AttrUtils::SetStr(opDescPtr, "queue_name", "test");
    vector<int64_t> tensorShape = {1,1,3,1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x", tensor1);
    opDescPtr->AddInputDesc("y", tensor1);
    opDescPtr->AddOutputDesc("z", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");

    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->CalcOpRunningParam(*(graphPtr->AddNode(opDescPtr))), SUCCESS);
}

TEST(AicpuKernelBuilder, GenerateTask_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    vector<int64_t> tensorShape = {1,2,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "Add");
    AttrUtils::SetStr(Op1DescPtr, "resource", "RES_CHANNEL");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);

    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->CalcOpRunningParam(*node2), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->GenerateTask(*node2, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("end===================\n");
}


TEST(AicpuKernelBuilder, GenerateTask_SUCCESS_FFTS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 1;
    thread_slice_map.thread_mode = 0;
    ffts::ThreadSliceMapPtr slice_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);
    opDescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    AttrUtils::SetStr(opDescPtr, "opKernelLib", "AICPUKernel");
    AttrUtils::SetInt(opDescPtr, "_unknown_shape_type", 0);
    ge::AttrUtils::SetInt(opDescPtr, kAttrNameThreadScopeId, 1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_ffts_graph");
    Node *node = graphPtr->AddNode(opDescPtr).get();
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->CalcOpRunningParam(*node), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->GenerateTask(*node, context, tasks), SUCCESS);
    slice_ptr->thread_mode = 1;
    slice_ptr->slice_instance_num = 1;
    slice_ptr->input_tensor_slice = {{{{0, 1}}, {{0, 1}}}};
    slice_ptr->output_tensor_slice = {{{{0, 1}}}};
    opDescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->GenerateTask(*node, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("ffts task end===================\n");
}

TEST(AicpuKernelBuilder, GenerateTask_UNKNOWSHAPE_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    OpDescPtr opDescPtr = make_shared<OpDesc>("Test","Test");
    vector<int64_t> tensorShape = {1,1,3,-1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x", tensor1);
    opDescPtr->AddInputDesc("y", tensor1);
    opDescPtr->AddOutputDesc("z", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    Node *node = graphPtr->AddNode(opDescPtr).get();
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->Initialize(options, nullptr), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->CalcOpRunningParam(*node), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;
    ASSERT_EQ(opsKernelBuilders["aicpu_ascend_builder"]->GenerateTask(*node, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("end===================\n");
}