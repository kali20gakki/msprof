#include <gtest/gtest.h>

#define protected public
#define private public
#include "tf_engine/engine/tf_engine.h"
#include "tf_kernel_builder/tf_kernel_builder.h"
#undef private
#undef protected

#include "util/tf_util.h"
#include "config/config_file.h"
#include "stub.h"

#include "util/util_stub.h"
#include "ge/ge_api_types.h"
#include "config/ops_json_file.h"
#include "graph/utils/tensor_utils.h"
#include "runtime/context.h"
#include "runtime/stream.h"
#include "runtime/rt_model.h"
#include "runtime/kernel.h"
#include "runtime/mem.h"
#include "common/sgt_slice_type.h"

using namespace aicpu;
using namespace ge;
using namespace std;

TEST(TfKernelBuilder, Initialize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;

    GetOpsKernelBuilderObjs(opsKernelBuilders);
    ASSERT_NE(opsKernelBuilders["aicpu_tf_builder"], nullptr);
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->Initialize(options), SUCCESS);
}

TEST(TfKernelBuilder, CalcOpRunningParam_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*(graphPtr->AddNode(opDescPtr))), SUCCESS);
}

TEST(TfKernelBuilder, CalcOpRunningParamWithWorkspaceSize_SUCCESS)
{

    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);

    vector<int64_t> workspaceBase = {int64_t(457568634557)};
    opDescPtr->SetWorkspace(workspaceBase);
    opDescPtr->SetWorkspaceBytes({10000});
    vector<int64_t> inputOffset = {};
    vector<int64_t> outputOffset = {};
    inputOffset.push_back(568679);
    inputOffset.push_back(56868);
    outputOffset.push_back(457568);
    opDescPtr->SetInputOffset(inputOffset);
    opDescPtr->SetOutputOffset(outputOffset);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");

    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*(graphPtr->AddNode(opDescPtr))), SUCCESS);
}

TEST(TfKernelBuilder, CalcOpRunningParamFMK_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    opDescPtr->SetType("FrameworkOp");
    AttrUtils::SetStr(opDescPtr, "original_type", "Add");
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*(graphPtr->AddNode(opDescPtr))), SUCCESS);
}

TEST(TfKernelBuilder, CalcOpRunningParamFMK_FAILED)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    opDescPtr->SetType("FrameworkOp");
    AttrUtils::SetStr(opDescPtr, "original_type", "");
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*(graphPtr->AddNode(opDescPtr))), STR_IS_EMPTY);
}


TEST(TfKernelBuilder, GenerateTask_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    AttrUtils::SetInt(opDescPtr, "_unknown_shape_type", 1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    Node *node = graphPtr->AddNode(opDescPtr).get();
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*node), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->GenerateTask(*node, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("end===================\n");
}

TEST(TfKernelBuilder, GenerateTask_SUCCESS_FFTS)
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
    ge::AttrUtils::SetInt(opDescPtr, kAttrNameThreadScopeId, 1);
    AttrUtils::SetInt(opDescPtr, "_unknown_shape_type", 0);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    Node *node = graphPtr->AddNode(opDescPtr).get();
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*node), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->GenerateTask(*node, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("ffts task end===================\n");
}

TEST(TfKernelBuilder, GenerateTaskFMK_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    AttrUtils::SetInt(opDescPtr, "_unknown_shape_type", 1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    opDescPtr->SetType("FrameworkOp");
    AttrUtils::SetStr(opDescPtr, "original_type", "Add");
    Node *node = graphPtr->AddNode(opDescPtr).get();
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*node), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->GenerateTask(*node, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("end===================\n");
}

TEST(TfKernelBuilder, GenSingleOpRunTask_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    NodePtr nodePtr = make_shared<Node>(opDescPtr, graphPtr);
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    nodePtr->Init();

    vector<int64_t> workspaceBase = {int64_t(457568634557)};
    opDescPtr->SetWorkspace(workspaceBase);
    opDescPtr->SetWorkspaceBytes({10000});
    vector<int64_t> inputOffset = {};
    vector<int64_t> outputOffset = {};
    inputOffset.push_back(568679);
    inputOffset.push_back(56868);
    outputOffset.push_back(457568);
    opDescPtr->SetInputOffset(inputOffset);
    opDescPtr->SetOutputOffset(outputOffset);

    STR_FWK_OP_KERNEL task;
    std::string task_info;
    ASSERT_EQ(SUCCESS, opsKernelBuilders["aicpu_tf_builder"]->GenSingleOpRunTask(nodePtr, task, task_info));
}

TEST(TfKernelBuilder, GenSingleOpRunTaskFunctionDef_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    NodePtr nodePtr = make_shared<Node>(opDescPtr, graphPtr);
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    nodePtr->Init();
    vector<int64_t> workspaceBase = {int64_t(457568634557)};
    opDescPtr->SetWorkspace(workspaceBase);
    opDescPtr->SetWorkspaceBytes({10000});
    string funcDefStr = "AddOpFunctionDef";
    const uint8_t *funcDefData = reinterpret_cast<const uint8_t*>(funcDefStr.data());
    AttrUtils::SetZeroCopyBytes(*(opDescPtr.get()), "func_def", Buffer::CopyFrom(funcDefData, funcDefStr.length()));
    vector<int64_t> inputOffset = {};
    vector<int64_t> outputOffset = {};
    inputOffset.push_back(568679);
    inputOffset.push_back(56868);
    outputOffset.push_back(457568);
    opDescPtr->SetInputOffset(inputOffset);
    opDescPtr->SetOutputOffset(outputOffset);

    STR_FWK_OP_KERNEL task;
    std::string task_info;
    ASSERT_EQ(SUCCESS, opsKernelBuilders["aicpu_tf_builder"]->GenSingleOpRunTask(nodePtr, task, task_info));
}

TEST(TfKernelBuilder, GenMemCopyTask_SUCCESS)
{
    STR_FWK_OP_KERNEL task;
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);
    std::string task_info;
    ASSERT_EQ(SUCCESS, opsKernelBuilders["aicpu_tf_builder"]->GenMemCopyTask(2, task, task_info));
}

TEST(TfKernelBuilder, GetTaskInfoCallback_SUCCESS)
{
    RunContext runContext;
    runContext.model = (void *)123321;
    rtTaskInfo_t taskInfo;
    taskInfo.type = RT_MODEL_TASK_KERNEL_EX;
    string str = "tf kernel";
    taskInfo.u.kernelTaskEx.args = (void *)(str.c_str());
    taskInfo.u.kernelTaskEx.argsSize = str.size();
    TfKernelBuilder::GetTaskInfoCallback(runContext.model, &taskInfo);
}

TEST(TfKernelBuilder, GenerateTask_UNKNOWSHAPE_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    GetOpsKernelBuilderObjs(opsKernelBuilders);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Test", "Test");
    vector<int64_t> tensorShape = {1, 1, 3, -1};
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
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->Initialize(options, nullptr), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_tf_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->CalcOpRunningParam(*node), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;

    ASSERT_EQ(opsKernelBuilders["aicpu_tf_builder"]->GenerateTask(*node, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("end===================\n");
}

TEST(TfKernelBuilder, CreateNodeDef_FAILED_001)
{
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    opDescPtr->SetType("FunctionOp");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    TfKernelBuilder *tfKernelBuilder = (TfKernelBuilder *)(TfKernelBuilder::Instance().get());
    ASSERT_EQ(CreateNodeDef(*(graphPtr->AddNode(opDescPtr))), NODE_DEF_NOT_EXIST);
}

TEST(TfKernelBuilder, CreateNodeDef_SUCCESS)
{
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "FrameworkOp");
    AttrUtils::SetStr(opDescPtr, "original_type", "Add");
    vector<int64_t> tensorShape = {1, 1, 3, 1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    TfKernelBuilder *tfKernelBuilder = (TfKernelBuilder *)(TfKernelBuilder::Instance().get());
    ASSERT_EQ(CreateNodeDef(*(graphPtr->AddNode(opDescPtr))), SUCCESS);
}

TEST(TfKernelBuilder, SetUnknownShape_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    Initialize(options);
    string kernelConfig;
    ConfigFile::GetInstance().GetValue("DNN_VM_AICPUOpsKernel", kernelConfig);
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    opsKernelInfoStores["aicpu_tf_kernel"]->Initialize(options);

    TfKernelBuilder tfKernelBuiler;
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add","Add");
    vector<int64_t> tensorShape = {1,1,3,1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    AttrUtils::SetInt(opDescPtr, "topic_type", 1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    AttrUtils::SetStr(opDescPtr, "original_type", "Add");
    Node *node = graphPtr->AddNode(opDescPtr).get();
    ASSERT_EQ(tfKernelBuiler.CalcOpRunningParam(*node), SUCCESS);
    RunContext context = CreateContext();
    vector<domi::TaskDef> tasks;
    ASSERT_EQ(tfKernelBuiler.GenerateTask(*node, context, tasks), SUCCESS);
    DestroyContext(context);
    printf("end===================\n");
}