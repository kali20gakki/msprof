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

#include "gtest/gtest.h"

#include <fcntl.h>
#include "sys/stat.h"

#include "runtime/rt_model.h"
#include "external/runtime/rt_error_codes.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "common/l2_stream_info.h"
#include "common/resource_def.h"
#include "common/util/op_info_util.h"
#include "common/comm_error_codes.h"
#include "../fe_test_utils.h"
#include "securec.h"


#define protected public
#define private public
#include "common/fe_log.h"
#include "ops_kernel_builder/aicore_ops_kernel_builder.h"
#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "adapter/adapter_itf/op_store_adapter.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "fusion_manager/fusion_manager.h"
#include "task_builder/task_builder.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_optimizer.h"
#include <memory>
#include <mutex>
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "ge/ge_api_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_context.h"
#include "graph/tuning_utils.h"
#include "graph/node.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "common/sgt_slice_type.h"
#include "common/ffts_plus_type.h"

#include "graph/model_serialize.h"
#include "graph/ge_attr_value.h"
#include "graph/detail/model_serialize_imp.h"

#undef private
#undef protected
#define SET_SIZE 128

using namespace std;
using namespace testing;
using namespace ge;
using namespace domi;
using namespace fe;
using namespace ffts;

using AICoreOpsKernelBuilderPtr =  shared_ptr<fe::AICoreOpsKernelBuilder>;
using OpStoreAdapterManagerPtr = std::shared_ptr<fe::OpStoreAdapterManager>;

FEOpsStoreInfo cce_custom_opinfo_adapter {
      0,
      "cce-custom",
      fe::EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_custom_opinfo",
      ""
};
FEOpsStoreInfo tik_custom_opinfo_adapter  {
      1,
      "tik-custom",
      fe::EN_IMPL_CUSTOM_TIK,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tik_custom_opinfo",
      ""
};
FEOpsStoreInfo tbe_custom_opinfo_adapter  {
      2,
      "tbe-custom",
      fe::EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
      ""
};
FEOpsStoreInfo cce_constant_opinfo_adapter  {
      3,
      "cce-constant",
      fe::EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_constant_opinfo",
      ""
};
FEOpsStoreInfo cce_general_opinfo_adapter  {
      4,
      "cce-general",
      fe::EN_IMPL_CUSTOM_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_general_opinfo",
      ""
};
FEOpsStoreInfo tik_opinfo_adapter  {
      5,
      "tik-builtin",
      fe::EN_IMPL_HW_TIK,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tik_opinfo",
      ""
};
FEOpsStoreInfo tbe_opinfo_adapter  {
      6,
      "tbe-builtin",
      fe::EN_IMPL_HW_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo",
      ""
};
FEOpsStoreInfo rl_opinfo_adapter  {
      7,
      "rl-builtin",
      fe::EN_IMPL_RL,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/rl_opinfo",
      ""
};

std::vector<FEOpsStoreInfo> all_fe_ops_store_info_adapter{
      cce_custom_opinfo_adapter ,
      tik_custom_opinfo_adapter ,
      tbe_custom_opinfo_adapter ,
      cce_constant_opinfo_adapter ,
      cce_general_opinfo_adapter ,
      tik_opinfo_adapter ,
      tbe_opinfo_adapter ,
      rl_opinfo_adapter
};

class STEST_TaskBuilder: public testing::Test {
protected:

    static void SetOpDecSize(NodePtr& node){
        OpDesc::Vistor<GeTensorDesc> tensors = node->GetOpDesc()->GetAllInputsDesc();
        for (int i = 0; i < node->GetOpDesc()->GetAllInputsDesc().size(); i++){
            ge::GeTensorDesc tensor = node->GetOpDesc()->GetAllInputsDesc().at(i);
            ge::TensorUtils::SetSize(tensor, SET_SIZE);
            node->GetOpDesc()->UpdateInputDesc(i, tensor);
        }
        OpDesc::Vistor<GeTensorDesc> tensors_output = node->GetOpDesc()->GetAllOutputsDesc();
        for (int i = 0; i < tensors_output.size(); i++){
            ge::GeTensorDesc tensor_output = tensors_output.at(i);
            ge::TensorUtils::SetSize(tensor_output, SET_SIZE);
            node->GetOpDesc()->UpdateOutputDesc(i, tensor_output);
        }
    }

    static void SetFFTSOpDecSize(NodePtr& node) {
      OpDesc::Vistor<GeTensorDesc> tensors = node->GetOpDesc()->GetAllInputsDesc();
      for (int i = 0; i < node->GetOpDesc()->GetAllInputsDesc().size(); i++) {
        ge::GeTensorDesc tensor = node->GetOpDesc()->GetAllInputsDesc().at(i);
        ge::TensorUtils::SetSize(tensor, 10000);
        node->GetOpDesc()->UpdateInputDesc(i, tensor);
      }
      OpDesc::Vistor<GeTensorDesc> tensorsOutput = node->GetOpDesc()->GetAllOutputsDesc();
      for (int i = 0; i < tensorsOutput.size(); i++) {
        ge::GeTensorDesc tensorOutput = tensorsOutput.at(i);
        ge::TensorUtils::SetSize(tensorOutput, 10000);
        node->GetOpDesc()->UpdateOutputDesc(i, tensorOutput);
      }
      for (auto const &anchor : node->GetAllInDataAnchors()) {
        (void)ge::AnchorUtils::SetStatus(anchor, ge::ANCHOR_DATA);
      }
    }

    void SetUp()
    {
        rtContext_t rt_context;
        assert(rtCtxCreate(&rt_context, RT_CTX_GEN_MODE, 0) == ACL_RT_SUCCESS);
        assert(rtCtxSetCurrent(rt_context) == ACL_RT_SUCCESS);
        //cce::cceSysInit();

        Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (all_fe_ops_store_info_adapter);
        context_ = CreateContext();
        FusionManager::Instance(fe::AI_CORE_NAME).op_store_adapter_manager_ = make_shared<fe::OpStoreAdapterManager>();
        task_builder_ = shared_ptr <TaskBuilder> (new (nothrow) TaskBuilder());
        FusionManager::Instance(fe::AI_CORE_NAME).ops_kernel_info_store_ = make_shared<fe::FEOpsKernelInfoStore>(FusionManager::Instance(fe::AI_CORE_NAME).op_store_adapter_manager_);
        FusionManager::Instance(fe::AI_CORE_NAME).graph_opt_ = make_shared<fe::FEGraphOptimizer>(FusionManager::Instance(fe::AI_CORE_NAME).ops_kernel_info_store_,
                                                                                FusionManager::Instance(fe::AI_CORE_NAME).op_store_adapter_manager_);
        TbeOpStoreAdapter tbe_adapter;
        std:: map<string, string> options;
        FusionManager::Instance(fe::AI_CORE_NAME).op_store_adapter_manager_->Initialize(options, fe::AI_CORE_NAME);
        ops_adapter_manage_ptr = make_shared<OpStoreAdapterManager>();

        aicore_ops_kernel_builder_ptr = make_shared<AICoreOpsKernelBuilder>();
        aicore_ops_kernel_builder_ptr->Initialize(options);
    }

    void TearDown()
    {
        task_builder_.reset();
        FusionManager::Instance(fe::AI_CORE_NAME).Finalize();
        DestroyContext(context_);

        rtContext_t rt_context;
        assert(rtCtxGetCurrent(&rt_context) == ACL_RT_SUCCESS);
        assert(rtCtxDestroy(rt_context) == ACL_RT_SUCCESS);
        aicore_ops_kernel_builder_ptr->Finalize();
    }

    static NodePtr CreateNodeWithoutAttrs(bool hasWeight = false)
    {
        FeTestOpDescBuilder builder;
        builder.SetName("test_tvm");
        builder.SetType("conv");
        builder.SetInputs( { 1 });
        builder.SetOutputs( { 1 });
        builder.AddInputDesc( { 1, 1, 1, 1 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
        builder.AddOutputDesc( { 1, 1, 1, 1 }, ge::FORMAT_NCHW, ge::DT_FLOAT);
        if (hasWeight) {
            size_t len = 10;
            unique_ptr<float[]> buf(new float[len]);
            auto weight = builder.AddWeight((uint8_t*) buf.get(), len * sizeof(float), { 1, 1, 2, 5 }, ge::FORMAT_NCHW,
                    ge::DT_FLOAT);
            ge::TensorUtils::SetWeightSize(weight->MutableTensorDesc(), len * sizeof(float));
        }

        return builder.Finish();
    }

    static NodePtr CreateNode(bool hasWeight = false)
    {
        NodePtr node = CreateNodeWithoutAttrs(hasWeight);

        const char* bin_file = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
        vector<char> buffer;
        assert(ReadBytesFromBinaryFile(bin_file, buffer));
        OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
        node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

        ge::AttrUtils::SetInt(node->GetOpDesc(), "_fe_imply_type", (int64_t)fe::EN_IMPL_CUSTOM_TBE);
        ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "cce_reductionLayer_1_10_float16__1_SUMSQ_1_0__kernel0");
        ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 1);
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
        ge::AttrUtils::SetBool(node->GetOpDesc(), "is_first_node", true);
        ge::AttrUtils::SetBool(node->GetOpDesc(), "is_last_node", true);
        std::string meta_data = "";
        meta_data.append("cce_reductionLayer_1_10_float16__1_SUMSQ_1_0"); // binFileName
        meta_data.append(".o");  // binFileSuffix
        meta_data.append(",version,");
        meta_data.append("c53fcf5403daaf993a95a4aeea228eae30196565d8b287bae9a4ca6a52e58c2b");    // sha256
        meta_data.append(",shared");
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", meta_data);
        SetOpDecSize(node);
        return node;
    }

  static NodePtr CreateDynamicNode(const int &type)
  {
    FeTestOpDescBuilder builder;
    builder.SetName("test_tvm");
    builder.SetType("conv");
    builder.SetInputs({1});
    builder.SetOutputs({1});
    builder.AddInputDesc({1,2,-1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
    builder.AddOutputDesc({1,2,-1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
    auto node = builder.Finish();

    const char tbeBin[] = "tbe_bin";
    vector<char> buffer(tbeBin, tbeBin+strlen(tbeBin));
    OpKernelBinPtr tbeKernelPtr = std::make_shared<OpKernelBin>("test_tvm", std::move(buffer));
    node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbeKernelPtr);

    ge::AttrUtils::SetInt(node->GetOpDesc(), "_fe_imply_type", (int64_t)fe::EN_IMPL_CUSTOM_TBE);
    ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
    if (type == 1 || type ==3) {
      ge::AttrUtils::SetBool(node->GetOpDesc(), "is_first_node", true);
    }
    if (type == 2 || type ==3) {
      ge::AttrUtils::SetBool(node->GetOpDesc(), "is_last_node", true);
    }
    if (type == 4) {
      ge::AttrUtils::SetStr(node->GetOpDesc(), fe::ATTR_NAME_KERNEL_LIST_FIRST_NAME, node->GetName());
    }
    ge::AttrUtils::SetBool(node->GetOpDesc(), "support_dynamicshape", false);
    ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetOpDesc()->GetName()+"_kernelname", "kernelname");

    SetOpDecSize(node);
    return node;
  }

    static RunContext CreateContext()
    {
        rtStream_t stream = nullptr;
        rtModel_t model = nullptr;

        assert(rtStreamCreate(&stream, 0) == ACL_RT_SUCCESS);
        assert(rtModelCreate(&model, 0) == ACL_RT_SUCCESS);
        assert(rtModelBindStream(model, stream, 0) == ACL_RT_SUCCESS);

        RunContext context;
        context.model = model;
        context.stream = stream;
        context.dataMemSize = 100;
        context.dataMemBase = (uint8_t *) (intptr_t) 1000;
        context.weightMemSize = 200;
        context.weightMemBase = (uint8_t *) (intptr_t) 1100;
        context.weightsBuffer = Buffer(20);

        return context;
    }

    static void DestroyContext(RunContext& context)
    {
        assert(rtModelUnbindStream(context.model, context.stream) == ACL_RT_SUCCESS);
        assert(rtModelDestroy(context.model) == ACL_RT_SUCCESS);
        assert(rtStreamDestroy(context.stream) == ACL_RT_SUCCESS);
    }

    static bool ReadBytesFromBinaryFile(const char* path, std::vector<char>& buffer)
    {
        if (path == nullptr)
            return false;

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if(!file.is_open())
            return false;

        std::streamsize size = file.tellg();

        if(size <= 0) {
            file.close();
            return false;
        }

        if (size > INT_MAX) {
            file.close();
            return false;
        }

        file.seekg(0, std::ios::beg);

        buffer.resize(size);
        file.read(buffer.data(), size);
        file.close();

        return true;
    }

    static Status DestroyHandle(ccHandle_t *handle)
    {
        if (NULL == handle || *handle == NULL)
        {
            FE_LOGE("handle is NULL!");
            return TASK_BUILDER_STATUS_BAD_PARAM;
        }
        rtError_t ret;
        ret = rtFreeHost(*handle);
        if (ret != ACL_RT_SUCCESS)
        {
            FE_LOGE("free handler failed!");
            return fe::FAILED;
        }
        *handle = NULL;
        return fe::SUCCESS;
    };

protected:
    RunContext context_;
    std::shared_ptr<TaskBuilder> task_builder_;
    OpStoreAdapterManagerPtr ops_adapter_manage_ptr;
    AICoreOpsKernelBuilderPtr aicore_ops_kernel_builder_ptr;
};

TEST_F(STEST_TaskBuilder, case_all_success_01)
{
    NodePtr node = CreateNode();
    std::vector<TaskDef> task_defs;
    EXPECT_EQ(task_builder_->GenerateKernelTask(*node, context_, task_defs), fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, case_all_success_02)
{
  NodePtr node = CreateNode();
  ge::AttrUtils::SetStr(node->GetOpDesc(), fe::ATTR_NAME_KERNEL_LIST_FIRST_NAME, node->GetName());
  std::vector<TaskDef> task_defs;
  EXPECT_EQ(task_builder_->GenerateKernelTask(*node, context_, task_defs), fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, case_all_success_03)
{
  setFuncState(FuncParamType::FUSION_L2, false);
  NodePtr node = CreateNode();
  ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_LX_FUSION_PASS, true);
  std::vector<TaskDef> task_defs;
  EXPECT_EQ(task_builder_->GenerateKernelTask(*node, context_, task_defs), fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, case_all_success_04)
{
  setFuncState(FuncParamType::FUSION_L2, false);
  NodePtr node = CreateNode();
  ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_LX_FUSION_PASS, true);
  ge::AttrUtils::SetStr(node->GetOpDesc(), fe::ATTR_NAME_KERNEL_LIST_FIRST_NAME, node->GetName());
  std::vector<TaskDef> task_defs;
  EXPECT_EQ(task_builder_->GenerateKernelTask(*node, context_, task_defs), fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, read_binary_file_success)
{
    const char* bin_file = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    vector<char> buffer;
    TbeJsonFileParseImpl tbe_json_file_parse_impl;
    EXPECT_EQ(tbe_json_file_parse_impl.ReadBytesFromBinaryFile(bin_file, buffer), fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, read_binary_file_lock_fail)
{
    const char* bin_file = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    vector<char> buffer;

    // manually lock first
    std::string file = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    FILE *fp = fopen(file.c_str(), "r");
    if (fp == nullptr) {
        EXPECT_EQ(true, false);
    }
    if (FcntlLockFile(file, fileno(fp), F_RDLCK, 0) != fe::SUCCESS) {
        EXPECT_EQ(true, false);
    }

    TbeJsonFileParseImpl tbe_json_file_parse_impl;
    EXPECT_EQ(tbe_json_file_parse_impl.ReadBytesFromBinaryFile(bin_file, buffer), fe::SUCCESS);
    (void)FcntlLockFile(file, fileno(fp), F_UNLCK, 0);
    fclose(fp);
}

TEST_F(STEST_TaskBuilder, l2with_confirm)
{
    TaskL2Info_t ll;
    ll.nodeName = "1";
    //TaskL2Info_t *l2Data = &ll;
    TaskL2InfoFEMap_t hh;
    hh["0"] = ll;

    StreamL2Info::Instance().SetStreamL2Info(context_.stream, hh, "Batch_-1");
    setFuncState(FuncParamType::FUSION_L2, true);
    NodePtr node = CreateNode();
    std::vector<TaskDef> task_defs;
    EXPECT_EQ(task_builder_->GenerateKernelTask(*node, context_, task_defs), fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, dynamic_node_generate_task_1)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(0);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, dynamic_node_generate_task_2)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(1);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, dynamic_node_generate_task_3)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(2);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, dynamic_node_generate_task_4)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(3);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(STEST_TaskBuilder, dynamic_node_generate_task_5)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(4);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

ComputeGraphPtr BuildGraph_Readonly_ScopeWrite1() {

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_cast1 = std::make_shared<OpDesc>("cast", "Cast");
  OpDescPtr op_desc_relu = std::make_shared<OpDesc>("relu", "Relu");
  OpDescPtr op_desc_output = std::make_shared<OpDesc>("output", "NetOutput");

  vector<int64_t> dim_a = {8, 4, 16, 16};
  GeShape shape_a(dim_a);
  GeTensorDesc tensor_desc_a(shape_a);
  tensor_desc_a.SetFormat(FORMAT_NCHW);
  tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
  tensor_desc_a.SetDataType(DT_FLOAT16);
  tensor_desc_a.SetOriginDataType(DT_FLOAT);

  op_desc_cast1->AddOutputDesc(tensor_desc_a);

  op_desc_relu->AddInputDesc(tensor_desc_a);
  op_desc_relu->AddOutputDesc(tensor_desc_a);
  op_desc_output->AddInputDesc(tensor_desc_a);


  NodePtr node_cast1 = graph->AddNode(op_desc_cast1);
  NodePtr node_relu = graph->AddNode(op_desc_relu);
  NodePtr node_output = graph->AddNode(op_desc_output);
  GraphUtils::AddEdge(node_cast1->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_relu->GetOutDataAnchor(0), node_output->GetInDataAnchor(0));

  std::string subgraph_name_1 = "instance_branch_1";
  ComputeGraphPtr subgraph_1 = std::make_shared<ComputeGraph>(subgraph_name_1);
  subgraph_1->SetParentNode(node_cast1);
  subgraph_1->SetParentGraph(graph);
  node_relu->GetOpDesc()->AddSubgraphName("branch1");
  node_relu->GetOpDesc()->SetSubgraphInstanceName(0, subgraph_name_1);
  return graph;
}

NodePtr MakeNode(const ComputeGraphPtr &graph, uint32_t in_num, uint32_t out_num, string name, string type) {
  GeTensorDesc test_desc(GeShape(), FORMAT_NCHW, DT_FLOAT);
  auto op_desc = std::make_shared<OpDesc>(name, type);
  for (auto i = 0; i < in_num; ++i) {
    op_desc->AddInputDesc(test_desc);
  }
  for (auto i = 0; i < out_num; ++i) {
    op_desc->AddOutputDesc(test_desc);
  }
  return graph->AddNode(op_desc);
}

NodePtr CreateNode1(OpDescPtr op, ComputeGraphPtr owner_graph)
{ return owner_graph->AddNode(op); }

TEST_F(STEST_TaskBuilder, generate_auto_aic_aiv_ctx_succ) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({6, 2, 3, 3, 2}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(GeShape({6, 11, 3, 13}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  ge::AttrUtils::SetInt(src_op, fe::FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetStr(src_op, "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);

  ffts::ThreadSliceMapPtr slice_info_ptr = make_shared<ffts::ThreadSliceMap>();
  slice_info_ptr->thread_mode = static_cast<uint32_t>(ThreadMode::AUTO_THREAD);
  slice_info_ptr->input_tensor_slice = {{{{0, 3}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}},
                                        {{{3, 6}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}}};
  slice_info_ptr->output_tensor_slice = {{{{0, 3}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}},
                                        {{{3, 6}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}}};
  src_op->SetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  (void)ge::AttrUtils::SetListStr(src_op, fe::ATTR_NAME_THREAD_CUBE_VECTOR_CORE_TYPE, {"AIC"});
  (void)ge::AttrUtils::SetBool(src_op, fe::kTypeFFTSPlus, true);
  auto src_node = graph->AddNode(src_op);
  SetFFTSOpDecSize(src_node);
  ge::RunContext context = CreateContext();
  std::vector<domi::TaskDef> tasks;

  Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node, context, tasks);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_TaskBuilder, generate_kernel_task_succ)
{
ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
GeTensorDesc src_tensor_desc(GeShape({5, 2, 3, 3, 2}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
src_tensor_desc.SetOriginShape(GeShape({5, 11, 3, 13}));
src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
ge::AttrUtils::SetInt(src_op, fe::FE_IMPLY_TYPE, 6);
ge::AttrUtils::SetStr(src_op, "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
src_op->AddOutputDesc(src_tensor_desc);
src_op->AddInputDesc(src_tensor_desc);

(void)ge::AttrUtils::SetStr(src_op, ge::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC");
auto src_node = graph->AddNode(src_op);
ge::RunContext context = CreateContext();
std::vector<domi::TaskDef> tasks;

Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node, context, tasks);
//EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_TaskBuilder, generate_manual_mix_aic_aiv_ctx_succ) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({5, 2, 3, 3, 2}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(GeShape({5, 11, 3, 13}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  ge::AttrUtils::SetInt(src_op, fe::FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetStr(src_op, "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);

  ffts::ThreadSliceMapPtr slice_info_ptr = make_shared<ffts::ThreadSliceMap>();
  slice_info_ptr->thread_mode = static_cast<uint32_t>(ThreadMode::MANUAL_THREAD);
  src_op->SetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  (void)ge::AttrUtils::SetStr(src_op, ge::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIC");
  (void)ge::AttrUtils::SetBool(src_op, fe::kTypeFFTSPlus, true);
  auto src_node = graph->AddNode(src_op);
  SetFFTSOpDecSize(src_node);
  ge::RunContext context = CreateContext();
  std::vector<domi::TaskDef> tasks;

  Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node, context, tasks);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_TaskBuilder, generate_auto_mix_aic_aiv_ctx_succ) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({6, 2, 3, 3, 2}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(GeShape({6, 11, 3, 13}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  ge::AttrUtils::SetInt(src_op, fe::FE_IMPLY_TYPE, 6);
  ge::AttrUtils::SetStr(src_op, "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);

  ffts::ThreadSliceMapPtr slice_info_ptr = make_shared<ffts::ThreadSliceMap>();
  slice_info_ptr->thread_mode = static_cast<uint32_t>(ThreadMode::AUTO_THREAD);
  slice_info_ptr->input_tensor_slice = {{{{0, 3}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}},
                                        {{{3, 6}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}}};
  slice_info_ptr->output_tensor_slice = {{{{0, 3}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}},
                                        {{{3, 6}, {0, 2}, {0, 3}, {0, 3}, {0, 2}}}};
  src_op->SetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  (void)ge::AttrUtils::SetListStr(src_op, fe::ATTR_NAME_THREAD_CUBE_VECTOR_CORE_TYPE, {"MIX_AIC"});
  (void)ge::AttrUtils::SetBool(src_op, fe::kTypeFFTSPlus, true);
  auto src_node = graph->AddNode(src_op);
  SetFFTSOpDecSize(src_node);
  ge::RunContext context = CreateContext();
  std::vector<domi::TaskDef> tasks;

  Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node, context, tasks);
  EXPECT_EQ(fe::SUCCESS, ret);
}