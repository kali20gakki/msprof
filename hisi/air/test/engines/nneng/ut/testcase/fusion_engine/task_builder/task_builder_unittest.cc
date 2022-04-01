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
#include "graph/compute_graph.h"
#include "common/util/op_info_util.h"
#include "../fe_test_utils.h"

#define protected public
#define private public
#include "ops_kernel_builder/aicore_ops_kernel_builder.h"
#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "fusion_manager/fusion_manager.h"
#include "task_builder/task_builder.h"

#include "common/fe_log.h"
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "adapter/adapter_itf/op_store_adapter.h"
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
#include "param_calculate/tensorsize_calculator.h"
#include "graph/model_serialize.h"
#include "graph/ge_attr_value.h"
#include "graph/detail/model_serialize_imp.h"


#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace ge;
using namespace fe;
using namespace ffts;
using AICoreOpsKernelBuilderPtr =  shared_ptr<fe::AICoreOpsKernelBuilder>;

FEOpsStoreInfo taskBuilderUnitTbeOpinfoAdapter  {
        6,
        "tbe-builtin",
        fe::EN_IMPL_HW_TBE,
        "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_opinfo",
        "",
        false,
        false
};

std::vector<FEOpsStoreInfo> taskBuilderUnitAapter{
        taskBuilderUnitTbeOpinfoAdapter ,
};

#define SET_SIZE 128

class UTEST_TaskBuilder : public testing::Test
{
protected:
    static void SetOpDecSize(NodePtr& node){
        OpDesc::Vistor<GeTensorDesc> tensors = node->GetOpDesc()->GetAllInputsDesc();
        for (int i = 0; i < node->GetOpDesc()->GetAllInputsDesc().size(); i++){
            ge::GeTensorDesc tensor = node->GetOpDesc()->GetAllInputsDesc().at(i);
            ge::TensorUtils::SetSize(tensor, SET_SIZE);
            node->GetOpDesc()->UpdateInputDesc(i, tensor);
        }
        OpDesc::Vistor<GeTensorDesc> tensorsOutput = node->GetOpDesc()->GetAllOutputsDesc();
        for (int i = 0; i < tensorsOutput.size(); i++){
            ge::GeTensorDesc tensorOutput = tensorsOutput.at(i);
            ge::TensorUtils::SetSize(tensorOutput, SET_SIZE);
            node->GetOpDesc()->UpdateOutputDesc(i, tensorOutput);
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
        rtContext_t rtContext;
        assert(rtCtxCreate(&rtContext, RT_CTX_GEN_MODE, 0) == ACL_RT_SUCCESS);
        assert(rtCtxSetCurrent(rtContext) == ACL_RT_SUCCESS);

        node_ = CreateNode();
        context_ = CreateContext();

        FusionManager::Instance(AI_CORE_NAME).op_store_adapter_manager_ = make_shared<fe::OpStoreAdapterManager>();
        task_builder_ = shared_ptr<TaskBuilder> (new (nothrow) TaskBuilder());

        TbeOpStoreAdapter tbeAdapter;
        std:: map<string, string> options;
        FusionManager::Instance(AI_CORE_NAME).op_store_adapter_manager_ = make_shared<fe::OpStoreAdapterManager>();
        Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (taskBuilderUnitAapter);
        FusionManager::Instance(fe::AI_CORE_NAME).op_store_adapter_manager_->Initialize(options, fe::AI_CORE_NAME);

        aicore_ops_kernel_builder_ptr = make_shared<AICoreOpsKernelBuilder>();
        aicore_ops_kernel_builder_ptr->Initialize(options);
    }

    void TearDown()
    {
        task_builder_.reset();
        DestroyContext(context_);
        node_.reset();

        rtContext_t rtContext;
        assert(rtCtxGetCurrent(&rtContext) == ACL_RT_SUCCESS);
        assert(rtCtxDestroy(rtContext) == ACL_RT_SUCCESS);
        aicore_ops_kernel_builder_ptr->Finalize();
    }

    static NodePtr CreateNode()
    {
        FeTestOpDescBuilder builder;
        builder.SetName("test_tvm");
        builder.SetType("conv");
        builder.SetInputs({1});
        builder.SetOutputs({1});
        builder.AddInputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        builder.AddOutputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        auto node = builder.Finish();

        const char tbeBin[] = "tbe_bin";
        vector<char> buffer(tbeBin, tbeBin+strlen(tbeBin));
        OpKernelBinPtr tbeKernelPtr = std::make_shared<OpKernelBin>("test_tvm", std::move(buffer));
        node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbeKernelPtr);

        ge::AttrUtils::SetInt(node->GetOpDesc(), "_fe_imply_type", (int64_t)fe::EN_IMPL_CUSTOM_TBE);
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
        ge::AttrUtils::SetBool(node->GetOpDesc(), "is_first_node", true);
        ge::AttrUtils::SetBool(node->GetOpDesc(), "is_last_node", true);
        ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetOpDesc()->GetName()+"_kernelname", "kernelname");

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

    static NodePtr CreateNormalNode(const int &type)
    {
      FeTestOpDescBuilder builder;
      builder.SetName("test_tvm");
      builder.SetType("conv");
      builder.SetInputs({1});
      builder.SetOutputs({1});
      builder.AddInputDesc({1,2,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
      builder.AddOutputDesc({1,2,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
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
        assert(rtModelDestroy (context.model) == ACL_RT_SUCCESS);
        assert(rtStreamDestroy (context.stream) == ACL_RT_SUCCESS);
    }

protected:
    NodePtr node_ { nullptr };
    RunContext context_;
    std::shared_ptr<TaskBuilder> task_builder_;
    std::shared_ptr<fe::OpStoreAdapterManager> op_store_adapter_manager_;
    AICoreOpsKernelBuilderPtr aicore_ops_kernel_builder_ptr;
};

TEST_F(UTEST_TaskBuilder, destroyhandle_success)
{
  ccHandle_t handle = nullptr;
  Status ret = CreateHandle(&handle);
  EXPECT_EQ(ret, fe::SUCCESS);
  ret = DestroyHandle(&handle);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, dynamic_node_generate_task_1)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(0);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, dynamic_node_generate_task_2)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(1);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, dynamic_node_generate_task_3)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(2);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, dynamic_node_generate_task_4)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(3);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, dynamic_node_generate_task_5)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateDynamicNode(4);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, static_node_generate_task_1)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateNormalNode(0);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, static_node_generate_task_2)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateNormalNode(1);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, static_node_generate_task_3)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateNormalNode(2);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, static_node_generate_task_4)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateNormalNode(3);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, static_node_generate_task_5)
{
  std::vector<domi::TaskDef> task_defs;
  ge::NodePtr node = CreateNormalNode(4);
  Status status = task_builder_->GenerateKernelTask(*node, context_, task_defs);
  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, fill_taskdef_after_gentask_1)
{
  TaskBuilder task_builder;
  ge::NodePtr node = CreateNormalNode(1);
  domi::TaskDef task_def = {};
  task_def.set_type(static_cast<uint32_t>(RT_MODEL_TASK_ALL_KERNEL));
  Status status = task_builder.FillTaskDefAfterGenTask(node->GetOpDesc(), task_def);
  EXPECT_EQ(status, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_TaskBuilder, fill_taskdef_after_gentask_2)
{
  TaskBuilder task_builder;
  ge::NodePtr node = CreateNormalNode(4);
  domi::TaskDef task_def = {};
  task_def.set_type(static_cast<uint32_t>(RT_MODEL_TASK_ALL_KERNEL));
  Status status = task_builder.FillTaskDefAfterGenTask(node->GetOpDesc(), task_def);
  EXPECT_EQ(status, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_TaskBuilder, parse_impl_failed)
{
    string json_file_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_failed_2.json";
    TbeJsonFileParseImpl tbe_json_file_parse_impl;
    Status ret = tbe_json_file_parse_impl.Initialize(json_file_path);
    EXPECT_EQ(fe::SUCCESS, ret);
    int32_t block_dim;
    ret = tbe_json_file_parse_impl.ParseTvmBlockDim(block_dim);
    EXPECT_EQ(fe::FAILED, ret);
    string magic;
    ret = tbe_json_file_parse_impl.ParseTvmMagic(magic);
    EXPECT_EQ(fe::FAILED, ret);
    uint32_t task_ratio;
    ret = tbe_json_file_parse_impl.ParseTvmTaskRatio(task_ratio);
    EXPECT_EQ(fe::FAILED, ret);
    vector<int64_t> compress_param_vec;
    ret = tbe_json_file_parse_impl.ParseConvCompressParameters(compress_param_vec);
    EXPECT_EQ(fe::FAILED, ret);
    int64_t weight_repeat;
    ret = tbe_json_file_parse_impl.ParseWeightRepeat(weight_repeat);
    EXPECT_EQ(fe::FAILED, ret);
    string file_name;
    std::vector<char> buffer;
    ret = tbe_json_file_parse_impl.ReadBytesFromBinaryFile(file_name, buffer);
    EXPECT_EQ(fe::FAILED, ret);
    string kernel_list_first;
    ret = tbe_json_file_parse_impl.ParseTvmKernelList(kernel_list_first);
    EXPECT_EQ(fe::SUCCESS, ret);
    vector<int64_t> tvm_workspace_sizes;
    vector<int64_t> tvm_workspace_types;
    ret = tbe_json_file_parse_impl.ParseTvmWorkSpace(tvm_workspace_sizes, tvm_workspace_types);
    EXPECT_EQ(fe::FAILED, ret);
    string meta_data;
    ret = tbe_json_file_parse_impl.ParseTvmMetaData(meta_data);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(UTEST_TaskBuilder, parse_impl_suc)
{
   string json_file_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_suc.json";
    TbeJsonFileParseImpl tbe_json_file_parse_impl;
    Status ret = tbe_json_file_parse_impl.Initialize(json_file_path);
    EXPECT_EQ(fe::SUCCESS, ret);
    string kernel_list_first;
    ret = tbe_json_file_parse_impl.ParseTvmKernelList(kernel_list_first);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(UTEST_TaskBuilder, read_binary_file_success)
{
    const char* bin_file = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    vector<char> buffer;
    TbeJsonFileParseImpl tbe_json_file_parse_impl;
    EXPECT_EQ(tbe_json_file_parse_impl.ReadBytesFromBinaryFile(bin_file, buffer), fe::SUCCESS);
}

TEST_F(UTEST_TaskBuilder, read_binary_file_lock_fail)
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

TEST_F(UTEST_TaskBuilder, CalcSingleTensorSize)
{
  ComputeGraphPtr sub_graph = std::make_shared<ComputeGraph>("sub_graph");
  auto node = MakeNode(sub_graph, 1, 1, "node", "Test");
  GeTensorDesc tensor_desc(GeShape({1,3,224,224}), FORMAT_NCHW, DT_UNDEFINED);
  node->GetOpDesc()->UpdateInputDesc(0, tensor_desc);
  int64_t tensor_size = 0;
  GeTensorDescPtr input_desc = node->GetOpDesc()->MutableInputDesc(0);
  TensorSizeCalculator::CalcSingleTensorSize(*node->GetOpDesc(), input_desc, "TEST", 1, true, tensor_size);
  GeTensorDesc tensor_desc1(GeShape({1,-1,-1,224}), FORMAT_NCHW, DT_BOOL);
  node->GetOpDesc()->UpdateInputDesc(0, tensor_desc1);
  input_desc = node->GetOpDesc()->MutableInputDesc(0);
  TensorSizeCalculator::CalcSingleTensorSize(*node->GetOpDesc(), input_desc, "TEST", 1, true, tensor_size);
}

TEST_F(UTEST_TaskBuilder, generate_manual_aic_aiv_ctx_succ)
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

ffts::ThreadSliceMapPtr slice_info_ptr = make_shared<ffts::ThreadSliceMap>();
slice_info_ptr->thread_mode = static_cast<uint32_t>(ThreadMode::MANUAL_THREAD);
src_op->SetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
(void)ge::AttrUtils::SetStr(src_op, ge::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC");
(void)ge::AttrUtils::SetBool(src_op, fe::kTypeFFTSPlus, true);
auto src_node = graph->AddNode(src_op);
SetFFTSOpDecSize(src_node);
ge::RunContext context = CreateContext();
std::vector<domi::TaskDef> tasks;

Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node, context, tasks);
EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(UTEST_TaskBuilder, generate_kernel_task_succ)
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


TEST_F(UTEST_TaskBuilder, generate_auto_aic_aiv_ctx_succ)
{
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

TEST_F(UTEST_TaskBuilder, generate_manual_mix_aic_aiv_ctx_succ)
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

TEST_F(UTEST_TaskBuilder, generate_auto_mix_aic_aiv_ctx_succ)
{
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
(void)ge::AttrUtils::SetListStr(src_op, fe::ATTR_NAME_THREAD_CUBE_VECTOR_CORE_TYPE,{"MIX_AIC"});
(void)ge::AttrUtils::SetBool(src_op, fe::kTypeFFTSPlus, true);
auto src_node = graph->AddNode(src_op);
SetFFTSOpDecSize(src_node);
ge::RunContext context = CreateContext();
std::vector<domi::TaskDef> tasks;

Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node, context, tasks);
EXPECT_EQ(fe::SUCCESS, ret);
}