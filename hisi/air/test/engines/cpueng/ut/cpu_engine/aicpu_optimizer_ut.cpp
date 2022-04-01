#include <gtest/gtest.h>
#include <dlfcn.h>
#include <dirent.h>
#include "aicpu_engine/engine/aicpu_engine.h"
#define private public
#include "aicpu_engine/optimizer/aicpu_optimizer.h"
#undef private
#include "util/util.h"
#include "util/util_stub.h"
#include "config/config_file.h"
#include "graph/ge_local_context.h"
#include "graph/ge_context.h"
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
    ge::GetThreadLocalContext().SetGraphOption(options);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->Initialize(options, nullptr), SUCCESS);
    map<string, string> options2;
    ge::GetThreadLocalContext().SetGraphOption(options2);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->Initialize(options, nullptr), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_001)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    vector<int64_t> tensorShape = {1, 2, 3, 4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z", tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "Add");

    Op1DescPtr->AddOutputDesc("z", tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");
    Op2DescPtr->SetAttr("test_str", GeAttrValue::CreateFrom<std::string>("test_str"));
    Op2DescPtr->SetAttr("test_float", GeAttrValue::CreateFrom<float>(3.14));
    Op2DescPtr->SetAttr("test_bool", GeAttrValue::CreateFrom<bool>(1));
    Op2DescPtr->SetAttr("test_int", GeAttrValue::CreateFrom<int64_t>(10));
    GeTensorPtr attrTensorPtr = make_shared<GeTensor>(tensor1);
    Op2DescPtr->SetAttr("test_tensor",
                        GeAttrValue::CreateFrom<GeTensorPtr>(attrTensorPtr));
    Op2DescPtr->SetAttr("test_data_type",
                        GeAttrValue::CreateFrom<DataType>(DT_FLOAT));
    Op2DescPtr->SetAttr("test_list_float",
                        GeAttrValue::CreateFrom<std::vector<float>>({1.2, 2.9}));
    Op2DescPtr->SetAttr("test_list_int",
                        GeAttrValue::CreateFrom<std::vector<int64_t>>({0, 1, 2}));
    vector<vector<int64_t>> attrListListInt = {{1},{2}};
    Op2DescPtr->SetAttr("test_list_list_int",
                        GeAttrValue::CreateFrom<std::vector<std::vector<int64_t>>>(attrListListInt));
    Op2DescPtr->SetAttr("test_list_datatype",
                        GeAttrValue::CreateFrom<std::vector<DataType>>({DT_FLOAT, DT_INT8}));
    Op2DescPtr->SetAttr("test_list_str",
                        GeAttrValue::CreateFrom<std::vector<std::string>>({"str1", "str2"}));
    Op2DescPtr->SetAttr("test_warn", GeAttrValue::CreateFrom<GeTensorDesc>(tensor1));

    Op2DescPtr->AddOutputDesc("z", tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeOriginalGraph_SUCCESS_001) {
  map<string, GraphOptimizerPtr> graphOptimizers;
  GetGraphOptimizerObjs(graphOptimizers);
  ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

  shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
  OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

  vector<int64_t> tensorShape = {1, 2, 3, 4};
  GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
  tensor1.SetOriginFormat(ge::FORMAT_NCHW);
  tensor1.SetOriginShape(GeShape(tensorShape));

  opDescPtr->AddOutputDesc("z", tensor1);
  auto node1 = graphPtr->AddNode(opDescPtr);
  OpDescPtr Op1DescPtr = make_shared<OpDesc>("add1", "Add");

  Op1DescPtr->AddOutputDesc("z", tensor1);
  auto node2 = graphPtr->AddNode(Op1DescPtr);

  node2->AddLinkFrom(node1);

  OpDescPtr Op2DescPtr = make_shared<OpDesc>("add2", "Add");
  Op2DescPtr->SetAttr("test_str", GeAttrValue::CreateFrom<std::string>("test_str"));
  Op2DescPtr->SetAttr("test_float", GeAttrValue::CreateFrom<float>(3.14));
  Op2DescPtr->SetAttr("test_bool", GeAttrValue::CreateFrom<bool>(1));
  Op2DescPtr->SetAttr("test_int", GeAttrValue::CreateFrom<int64_t>(10));
  GeTensorPtr attrTensorPtr = make_shared<GeTensor>(tensor1);
  Op2DescPtr->SetAttr("test_tensor", GeAttrValue::CreateFrom<GeTensorPtr>(attrTensorPtr));
  Op2DescPtr->SetAttr("test_data_type", GeAttrValue::CreateFrom<DataType>(DT_FLOAT));
  Op2DescPtr->SetAttr("test_list_float", GeAttrValue::CreateFrom<std::vector<float>>({1.2, 2.9}));
  Op2DescPtr->SetAttr("test_list_int", GeAttrValue::CreateFrom<std::vector<int64_t>>({0, 1, 2}));
  vector<vector<int64_t>> attrListListInt = {{1}, {2}};
  Op2DescPtr->SetAttr("test_list_list_int",
                      GeAttrValue::CreateFrom<std::vector<std::vector<int64_t>>>(attrListListInt));
  Op2DescPtr->SetAttr("test_list_datatype", GeAttrValue::CreateFrom<std::vector<DataType>>({DT_FLOAT, DT_INT8}));
  Op2DescPtr->SetAttr("test_list_str", GeAttrValue::CreateFrom<std::vector<std::string>>({"str1", "str2"}));
  Op2DescPtr->SetAttr("test_warn", GeAttrValue::CreateFrom<GeTensorDesc>(tensor1));

  Op2DescPtr->AddOutputDesc("z", tensor1);
  auto node3 = graphPtr->AddNode(Op2DescPtr);

  node3->AddLinkFrom(node2);
  ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeOriginalGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_002)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    vector<int64_t> tensorShape = {1, 2, 3, 4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z", tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("RandomChoiceWithMask", "RandomChoiceWithMask");

    Op1DescPtr->AddOutputDesc("z", tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("FrameworkOpCase", kFrameworkOp);
    ge::AttrUtils::SetStr(Op2DescPtr, kOriginalType, kFrameworkOp);

    Op2DescPtr->AddOutputDesc("z", tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);

    node3->AddLinkFrom(node2);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
    ge::AttrUtils::SetInt(graphPtr, kAttrNameRootGraphId, 99);
    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_004)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph_04");
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

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_006)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph_04");
    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 1;
    thread_slice_map.thread_mode = 0;
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

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_SUCCESS_007)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph_04");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");

    ge::AttrUtils::SetInt(opDescPtr, kAttrNameThreadScopeId, 1);
    vector<int64_t> tensorShape = {-1,-1,3,4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));

    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("Add", "Add");

    ge::AttrUtils::SetInt(Op1DescPtr, kAttrNameThreadScopeId, 1);
    Op1DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);

    node2->AddLinkFrom(node1);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("Cast", "Cast");

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


TEST(AicpuGraphOptimizer, OptimizeFusedGraph_FAIL_002)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_scope_id = 1;
    thread_slice_map.slice_instance_num = 2;
    thread_slice_map.thread_mode = 1;
    thread_slice_map.input_tensor_slice = {{{{0, 1}, {1, 2}}, {{0, 1}, {1, 2}}}};
    thread_slice_map.output_tensor_slice = {{{{0, 1}, {1, 2}}}};
    ffts::ThreadSliceMapPtr slice_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    opDescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(opDescPtr, kAttrNameThreadScopeId, 1);
    vector<int64_t> tensorShape = {2,2,2,4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));
    opDescPtr->AddInputDesc("x", tensor1);
    opDescPtr->AddInputDesc("y", tensor1);
    opDescPtr->AddOutputDesc("z",tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);

    OpDescPtr Op2DescPtr = make_shared<OpDesc>("Cast", "Cast");
    Op2DescPtr->SetExtAttr(kAttrNameSgtStruct, slice_ptr);
    ge::AttrUtils::SetInt(Op2DescPtr, kAttrNameThreadScopeId, 1);
    Op2DescPtr->AddOutputDesc("z",tensor1);
    auto node2 = graphPtr->AddNode(Op2DescPtr);
     
    node2->AddLinkFrom(node1);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), SUCCESS);
}

TEST(AicpuGraphOptimizer, OptimizeFusedGraph_OPTIMIZE_CPU_KERNEL_FAILED)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);

    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    OpDescPtr opDescPtr = make_shared<OpDesc>("placeholder", "PlaceHolder");
    vector<int64_t> tensorShape = {1, 2, 3, 4};
    GeTensorDesc tensor1(GeShape(tensorShape), ge::FORMAT_NCHW, ge::DT_INT32);
    tensor1.SetOriginFormat(ge::FORMAT_NCHW);
    tensor1.SetOriginShape(GeShape(tensorShape));
    opDescPtr->AddOutputDesc("z", tensor1);
    auto node1 = graphPtr->AddNode(opDescPtr);
    OpDescPtr Op1DescPtr = make_shared<OpDesc>("RandomChoiceWithMask2", "RandomChoiceWithMask2");
    Op1DescPtr->AddOutputDesc("z", tensor1);
    auto node2 = graphPtr->AddNode(Op1DescPtr);
    node2->AddLinkFrom(node1);
    OpDescPtr Op2DescPtr = make_shared<OpDesc>("RandomChoiceWithMask2", "RandomChoiceWithMask2");
    Op2DescPtr->AddOutputDesc("z", tensor1);
    auto node3 = graphPtr->AddNode(Op2DescPtr);
    node3->AddLinkFrom(node2);

    ASSERT_EQ(graphOptimizers["aicpu_ascend_optimizer"]->OptimizeFusedGraph(*(graphPtr.get())), PACKAGE_BIN_FILE);
}


TEST(AicpuGraphOptimizer, GetBinFileName_FIALED)
{
    Dl_info dl_info;
    dladdr((void*) GetGraphOptimizerObjs, &dl_info);
    string so_path = dl_info.dli_fname;
    string so_file_path = RealPath(so_path);
    string real_dir_file_path = so_file_path.substr(0, so_file_path.rfind('/') + 1) + "op_impl/";
    string command = "rm -rf " + real_dir_file_path;
    system((char*) command.c_str());
    real_dir_file_path = so_file_path.substr(0, so_file_path.rfind('/') + 1) +
                         "op_impl/custom/cpu/aicpu_kernel/custom_impl";
    command = "mkdir -p " + real_dir_file_path;
    system((char*) command.c_str());
    AicpuOptimizer optimizer;
    
    OpFullInfo opInfo;
    opInfo.kernelSo = "libcpu_kernels.so";
    opInfo.userDefined = false;
    string bin_file_name;
    string fileSo0 = real_dir_file_path + "/1.so";
    command = "touch " + fileSo0;
    system((char*) command.c_str());
    ASSERT_EQ(optimizer.GetBinFileName(opInfo, real_dir_file_path,
                                       bin_file_name), NO_CPU_KERNELS_SO_EXIST);
    string fileSo1 = real_dir_file_path + "/libcpu_kernels_v1.so";
    command = "touch " + fileSo1;
    system((char*) command.c_str());
    string fileSo2 = real_dir_file_path + "/libcpu_kernels_v2.so";
    command = "touch " + fileSo2;
    system((char*) command.c_str());
    ASSERT_EQ(optimizer.GetBinFileName(opInfo, real_dir_file_path,
                                       bin_file_name), MULTI_CPU_KERNELS_SO_EXIST);

    real_dir_file_path = so_file_path.substr(0, so_file_path.rfind('/') + 1) + "op_impl/";
    command = "rm -rf " + real_dir_file_path;
    system((char*) command.c_str());
}

TEST(AicpuGraphOptimizer, GetCpuKernelSoPath_SUCCESS)
{
    char *path = "/home";
    setenv("ASCEND_AICPU_PATH", path, 1);
    AicpuOptimizer optimizer;
    std::string dir = optimizer.GetCpuKernelSoPath();
    cout<<"!!!"<<dir<<endl;
    ASSERT_EQ(dir, "/home/opp/op_impl/built-in/aicpu/aicpu_kernel/lib/Ascend910");
}

TEST(AicpuGraphOptimizer, GetCustKernelSoPath_SUCCESS)
{
    char *path = "/home";
    setenv("ASCEND_OPP_PATH", path, 1);
    AicpuOptimizer optimizer;
    std::string dir = optimizer.GetCustKernelSoPath();
    ASSERT_EQ(dir, "/home/op_impl/custom/cpu/aicpu_kernel/custom_impl/");
}

TEST(AicpuGraphOptimizer, ValidateStr_FAILED)
{
    std::string str = "libcpu_kernels.so";
    std::string mode = "libcpu_kernels_v[0-9]+(\\.[0-9]+){0,2}.so";
    ASSERT_EQ(ValidateStr(str, mode), false);

    str = "libcpu_kernels_v2.1.0.so";
    ASSERT_EQ(ValidateStr(str, mode), true);

    mode = "libcpu_kernels_v[0-9]]]]]]]]]]]]+";
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

TEST(AicpuGraphOptimizer, Initialize_FAILED)
{
    map<string, string> options;
    Initialize(options);
    options[SOC_VERSION] = "Ascend910";
    Initialize(options);
}
