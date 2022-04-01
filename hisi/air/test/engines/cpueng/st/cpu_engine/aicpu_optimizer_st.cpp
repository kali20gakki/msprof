#include <gtest/gtest.h>
#include <unistd.h>
#include <dirent.h>
#include "aicpu_engine/engine/aicpu_engine.h"
#define private public
#include "aicpu_engine/optimizer/aicpu_optimizer.h"
#undef private
#include "util/util.h"
#include "util/util_stub.h"
#include "config/config_file.h"
#include "cpu_engine_util.h"
#include "common/sgt_slice_type.h"
#include "ge/ge_api_types.h"

using namespace aicpu;
using namespace ge;
using namespace std;

TEST(AicpuGraphOptimizer, Initialize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    string kernelConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPU_ASCENDOpsKernel", kernelConfig), true);
    ASSERT_EQ(kernelConfig, "CUSTAICPUKernel,AICPUKernel");
    string optimizerConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPU_ASCENDGraphOptimizer", optimizerConfig), true);
    ASSERT_EQ(optimizerConfig, "AICPUOptimizer");
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->Initialize(options, nullptr), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_001)
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

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_002)
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
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("RandomChoiceWithMask", "RandomChoiceWithMask");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("RandomChoiceWithMask", "RandomChoiceWithMask");

    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_003)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 1;
    thread_slice_map.thread_mode = 1;
    thread_slice_map.input_tensor_slice = {{{{0, 1}, {1, 2}}}};
    thread_slice_map.output_tensor_slice = {{{{0, 1}, {1, 2}}}};
    ffts::ThreadSliceMapPtr slice_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);

    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    opDescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(opDescPtr, kAttrNameThreadScopeId, 1);
    vector<int64_t> tensorShape = {1,2,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("Add", "Add");
    Op1DescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(Op1DescPtr, kAttrNameThreadScopeId, 1);
    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("Cast", "Cast");

    Op2DescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(Op1DescPtr, kAttrNameThreadScopeId, 1);
    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);
    
    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_FAIL_001)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 2;
    thread_slice_map.thread_mode = 1;
    thread_slice_map.input_tensor_slice = {{{{0, 1}, {1, 2}}}};
    thread_slice_map.output_tensor_slice = {{{{0, 1}, {1, 2}}}};
    ffts::ThreadSliceMapPtr slice_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);

    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    opDescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(opDescPtr, kAttrNameThreadScopeId, 1);
    vector<int64_t> tensorShape = {1,2,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("Add", "Add");
    Op1DescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(Op1DescPtr, kAttrNameThreadScopeId, 1);
    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("Cast", "Cast");

    Op2DescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(Op1DescPtr, kAttrNameThreadScopeId, 1);
    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);
    
    node3->AddLinkFrom(node2);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraphFftsCast_FAILED)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 2;
    thread_slice_map.thread_mode = 0;
    ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    vector<int64_t> tensorShape = {1, 2, 3, 4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));
    opDescPtr->AddOutputDesc("z", tensor1);
    opDescPtr->SetExtAttr(kAttrNameSgtStruct, tsmp_ptr);
    ge::AttrUtils::SetInt(opDescPtr, "_thread_scope_id", 1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("RandomChoiceWithMask2", "RandomChoiceWithMask2");
    Op1DescPtr->AddOutputDesc("z", tensor1);
    Op1DescPtr->SetExtAttr(kAttrNameSgtStruct, tsmp_ptr);
    ge::AttrUtils::SetInt(Op1DescPtr, "_thread_scope_id", 1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);
    node2->AddLinkFrom(node1);
    OpDescPtr Op2DescPtr = make_shared<OpDesc>("RandomChoiceWithMask2", "RandomChoiceWithMask2");
    Op2DescPtr->AddOutputDesc("z", tensor1);
    Op2DescPtr->SetExtAttr(kAttrNameSgtStruct, tsmp_ptr);
    ge::AttrUtils::SetInt(Op2DescPtr, "_thread_scope_id", 1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);
    node3->AddLinkFrom(node2);

    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), PACKAGE_BIN_FILE);
}

TEST(AicpuGraphOptimizer, GetAttrValueFromGe_STRING_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    opDescPtr->SetAttr("test", GeAttrValue::CreateFrom<std::string>("test"));

    aicpuops::AttrValue attrValue;
    AicpuOptimizer *aicpuOptimizer = (AicpuOptimizer*)(graphOptimizers["aicpu_ascend_optimizer"].get());
    string attrName = "test";
    ASSERT_EQ(GetAttrValueFromGe(opDescPtr, attrName, GeAttrValue::ValueType::VT_STRING, attrValue), SUCCESS);
}

TEST(AicpuGraphOptimizer, GetAttrValueFromGe_FLOAT_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    opDescPtr->SetAttr("test", GeAttrValue::CreateFrom<float>(1));

    aicpuops::AttrValue attrValue;
    AicpuOptimizer *aicpuOptimizer = (AicpuOptimizer*)(graphOptimizers["aicpu_ascend_optimizer"].get());
    string attrName = "test";
    ASSERT_EQ(GetAttrValueFromGe(opDescPtr, attrName, GeAttrValue::ValueType::VT_FLOAT, attrValue), SUCCESS);
}

TEST(AicpuGraphOptimizer, GetAttrValueFromGe_BOOL_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    opDescPtr->SetAttr("test", GeAttrValue::CreateFrom<bool>(true));

    aicpuops::AttrValue attrValue;
    AicpuOptimizer *aicpuOptimizer = (AicpuOptimizer*)(graphOptimizers["aicpu_ascend_optimizer"].get());
    string attrName = "test";
    ASSERT_EQ(GetAttrValueFromGe(opDescPtr, attrName, GeAttrValue::ValueType::VT_BOOL, attrValue), SUCCESS);
}

TEST(AicpuGraphOptimizer, GetAttrValueFromGe_INT_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    opDescPtr->SetAttr("test", GeAttrValue::CreateFrom<int64_t>(1));

    aicpuops::AttrValue attrValue;
    AicpuOptimizer *aicpuOptimizer = (AicpuOptimizer*)(graphOptimizers["aicpu_ascend_optimizer"].get());
    string attrName = "test";
    ASSERT_EQ(GetAttrValueFromGe(opDescPtr, attrName, GeAttrValue::ValueType::VT_INT, attrValue), SUCCESS);
}

TEST(AicpuGraphOptimizer, GetAttrValueFromGe_TENSOR_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    auto tensor = GeTensor();
    opDescPtr->SetAttr("test", GeAttrValue::CreateFrom<GeTensor>(tensor));

    aicpuops::AttrValue attrValue;
    AicpuOptimizer *aicpuOptimizer = (AicpuOptimizer*)(graphOptimizers["aicpu_ascend_optimizer"].get());
    string attrName = "test";

    ASSERT_EQ(GetAttrValueFromGe(opDescPtr, attrName, GeAttrValue::ValueType::VT_TENSOR, attrValue), SUCCESS);
}
TEST(AicpuGraphOptimizer, OptimizeFusedGraph_OPTIMIZE_CPU_KERNEL_FAILED)
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
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("RandomChoiceWithMask2", "RandomChoiceWithMask2");
    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);
    node2->AddLinkFrom(node1);
    OpDescPtr Op2DescPtr = make_shared<OpDesc>("RandomChoiceWithMask2", "RandomChoiceWithMask2");
    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);
    node3->AddLinkFrom(node2);

    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), PACKAGE_BIN_FILE);
}

TEST(AicpuGraphOptimizer, GetCpuKernelSoPath_FAILED)
{
    char buf[80];
    getcwd(buf, sizeof(buf));
    std::cout << "==============" << buf << std::endl;
    AicpuOptimizer optimizer;
    std::string dir = optimizer.GetCpuKernelSoPath();
    std::string dest_dir(buf);
    dest_dir +=  "/../../ops/op_impl/built-in/aicpu/aicpu_kernel/lib/Ascend310/";
    ASSERT_EQ(dir, dest_dir);
}

TEST(AicpuGraphOptimizer, ValidateStr_FAILED)
{
    std::string str = "libcpu_kernels.so";
    std::string mode = "libcpu_kernels_v[0-9]+(\\.[0-9]+){0,2}.so";
    ASSERT_EQ(ValidateStr(str, mode), false);

    str = "libcpu_kernels_v2.1.0.so";
    ASSERT_EQ(ValidateStr(str, mode), true);

    mode = "libcpu_kernels_v[0-9]]]]]]]]]]]]]]+";
    ASSERT_EQ(ValidateStr(str, mode), false);
}

TEST(AicpuGraphOptimizer, CheckAndSetSocVersion_SUCCESS)
{
    AicpuOptimizer optimizer;
    optimizer.CheckAndSetSocVersion("Ascend310");
    optimizer.CheckAndSetSocVersion("Ascend910A");
    optimizer.CheckAndSetSocVersion("Ascend710A");
    optimizer.CheckAndSetSocVersion("test");
}