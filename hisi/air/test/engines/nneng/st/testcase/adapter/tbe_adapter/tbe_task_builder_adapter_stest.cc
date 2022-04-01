/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include <vector>
#include "runtime/rt_model.h"
#include "external/runtime/rt_error_codes.h"
#include "graph/ge_tensor.h"
#include "graph/compute_graph.h"
#include "graph/op_desc.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "../../fe_test_utils.h"
#include "securec.h"
#include "graph/debug/ge_attr_define.h"
#define protected public
#define private public
#include "common/fe_log.h"
#include "common/resource_def.h"
#include "common/l2_stream_info.h"
#include "common/constants_define.h"
#include "common/comm_error_codes.h"
#include "common/configuration.h"
#include "common/graph_comm.h"
#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "adapter/tbe_adapter/kernel_launch/tbe_kernel_launch.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_optimizer.h"
#include "common/util/constants.h"
#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace ge;
using namespace fe;

using NodePtr = std::shared_ptr<Node>;
#define SET_SIZE 128
#define SET_SIZE_2 100352

class STEST_TbeTaskBuilderAdapter : public testing::Test
{
    friend class TbeTaskBuilderAdapter;

protected:

    static void CheckSizeFuction(ge::GeTensorDesc& tensor)
    {
      vector<int64_t> dims = {1,4,5,2,7};
      ge::GeShape input1_shape(dims);
      tensor.SetShape(input1_shape);
      tensor.SetDataType(ge::DT_DOUBLE);
      tensor.SetFormat(ge::FORMAT_NC1HWC0_C04);
    }

    static void SetOpDecSize(NodePtr& node, const int set_size){
        OpDesc::Vistor<GeTensorDesc> tensors = node->GetOpDesc()->GetAllInputsDesc();
        for (int i = 0; i < tensors.size(); i++){
            ge::GeTensorDesc tensor = tensors.at(i);
            ge::TensorUtils::SetSize(tensor, set_size);
            node->GetOpDesc()->UpdateInputDesc(i, tensor);
        }
        OpDesc::Vistor<GeTensorDesc> tensors_output = node->GetOpDesc()->GetAllOutputsDesc();
        for (int i = 0; i < tensors_output.size(); i++){
            ge::GeTensorDesc tensor_output = tensors_output.at(i);
            ge::TensorUtils::SetSize(tensor_output, set_size);
            node->GetOpDesc()->UpdateOutputDesc(i, tensor_output);
        }
    }
    void SetUp()
    {
        rtContext_t rtContext;
        assert(rtCtxCreate(&rtContext, RT_CTX_GEN_MODE, 0) == ACL_RT_SUCCESS);
        assert(rtCtxSetCurrent(rtContext) == ACL_RT_SUCCESS);

        Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_L2_FUSION;
        node_ = CreateNode();
        context_ = CreateContext();
        SetOpDecSize(node_, SET_SIZE);
        adapter_ = shared_ptr<TbeTaskBuilderAdapter> (new (nothrow) TbeTaskBuilderAdapter(*node_, context_));
    }
    void TearDown()
    {
        adapter_.reset();
        DestroyContext(context_);
        node_.reset();

        rtContext_t rtContext;
        assert(rtCtxGetCurrent(&rtContext) == ACL_RT_SUCCESS);
        assert(rtCtxDestroy(rtContext) == ACL_RT_SUCCESS);
        Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
    }
  static void CheckGraphReadMode(GeTensorDescPtr tensor_desc, L2CacheReadMode expect) {
    int32_t read_mode;
    bool get_status = ge::AttrUtils::GetInt(tensor_desc, ATTR_NAME_L2CACHE_GRAPH_READ_MODE, read_mode);
    EXPECT_EQ(get_status, true);
    EXPECT_EQ(read_mode, static_cast<int32_t>(expect));
  }

    static NodePtr CreateNodeWithoutAttrs(bool has_weight = false)
    {
        FeTestOpDescBuilder builder;
        builder.SetName("test_tvm");
        builder.SetType("test_tvm");
        builder.SetInputs({1});
        builder.SetOutputs({1});
        builder.AddInputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        builder.AddOutputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        if (has_weight) {
            size_t len = 10;
            unique_ptr<float[]> buf(new float[len]);
            auto weight = builder.AddWeight((uint8_t*) buf.get(), len * sizeof(float), { 1, 1, 2, 5 },
                    ge::FORMAT_NCHW, ge::DT_FLOAT);
            ge::TensorUtils::SetWeightSize(weight->MutableTensorDesc(), len * sizeof(float));
        }

        return builder.Finish();
    }

    static NodePtr CreateNode(bool has_weight = false)
    {
        NodePtr node = CreateNodeWithoutAttrs(has_weight);

        const char tbe_bin[] = "tbe_bin";
        vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
        OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
        node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

        ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
        ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

        SetOpDecSize(node, SET_SIZE);
        return node;
    }

    static NodePtr CreateNodeWithoutAttrs3(bool has_weight = false)
    {
      FeTestOpDescBuilder builder;
      builder.SetName("test_tvm");
      builder.SetType("test_tvm");
      builder.SetInputs({1});
      builder.SetOutputs({1});
      builder.AddInputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
      builder.AddOutputDesc({1, 8, 7, 56, 32}, ge::FORMAT_NC1HWC0, ge::DT_INT8);
      if (has_weight) {
        size_t len = 10;
        unique_ptr<float[]> buf(new float[len]);
        auto weight = builder.AddWeight((uint8_t*) buf.get(), len * sizeof(float), { 1, 1, 2, 5 },
                                        ge::FORMAT_NCHW, ge::DT_FLOAT);
        ge::TensorUtils::SetWeightSize(weight->MutableTensorDesc(), len * sizeof(float));
      }

      return builder.Finish();
    }

    static NodePtr CreateNode3(bool has_weight = false)
    {
      NodePtr node = CreateNodeWithoutAttrs3(has_weight);

      const char tbe_bin[] = "tbe_bin";
      vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
      OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
      node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

      ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
      ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
      ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
      ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

      SetOpDecSize(node, SET_SIZE_2);
      return node;
    }

      static NodePtr CreateUnknownShapeNodeWithoutAttrs(bool has_weight = false)
      {
          FeTestOpDescBuilder builder;
          builder.SetName("test_tvm");
          builder.SetType("test_tvm");
          builder.SetInputs({1});
          builder.SetOutputs({1});
          builder.AddInputDesc({1,-1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
          builder.AddOutputDesc({1,-1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
          if (has_weight) {
              size_t len = 10;
              unique_ptr<float[]> buf(new float[len]);
              auto weight = builder.AddWeight((uint8_t*) buf.get(), len * sizeof(float), { 1, 1, 2, 5 },
                                              ge::FORMAT_NCHW, ge::DT_FLOAT);
              ge::TensorUtils::SetWeightSize(weight->MutableTensorDesc(), len * sizeof(float));
          }

          return builder.Finish();
      }

      static NodePtr CreateUnknownShapeNode(bool has_weight = false)
      {
          NodePtr node = CreateUnknownShapeNodeWithoutAttrs(has_weight);

          const char tbe_bin[] = "tbe_bin";
          vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
          OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
          node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

          ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
          ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
          ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
          ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

          return node;
      }

    static NodePtr CreateNodeWithInputNode(string input_op)
    {
        FeTestOpDescBuilder builder;
        builder.SetName("test_tvm");
        builder.SetType("test_tvm");
        builder.SetInputs({1});
        builder.SetOutputs({1});
        builder.AddInputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        builder.AddOutputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        NodePtr node =  builder.Finish();
        const char tbe_bin[] = "tbe_bin";
        vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
        OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
        node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
        ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
        ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

        FeTestOpDescBuilder builder1;
        builder1.SetName(input_op);
        builder1.SetType(input_op);
        builder1.AddOutputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        NodePtr input_node =  builder1.Finish();
        node->AddLinkFrom(input_node);
        SetOpDecSize(node, SET_SIZE);
        return node;
   }

  static NodePtr CreateNodeWithoutAttrsWithLargeWeightOffset(bool has_weight = false)
  {
      FeTestOpDescBuilder builder;
      builder.SetName("test_tvm");
      builder.SetInputs({1});
      builder.SetOutputs({1});
      builder.AddInputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
      builder.AddOutputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
      if (has_weight) {
          size_t len = 10;
          unique_ptr<float[]> buf(new float[len]);
          auto weight = builder.AddWeight((uint8_t*) buf.get(), len * sizeof(float), { 1, 1, 2, 5 },
                                          ge::FORMAT_NCHW, ge::DT_FLOAT);
          ge::TensorUtils::SetWeightSize(weight->MutableTensorDesc(), len * sizeof(float));
          ge::TensorUtils::SetDataOffset(weight->MutableTensorDesc(), 30*1024*1024*1024L + 1L);
      }

      return builder.Finish();
  }

  static NodePtr CreateNodeWithVarInputAttr(bool has_weight = false)
  {
      NodePtr node = CreateNodeWithoutAttrsWithLargeWeightOffset(has_weight);

      const char tbe_bin[] = "tbe_bin";
      vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
      OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
      node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

      ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
      ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
      ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
      ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

      vector<bool> input_is_addr_var = {true, true, true};

      ge::AttrUtils::SetListBool(node->GetOpDesc(), "INPUT_IS_VAR", input_is_addr_var);
      SetOpDecSize(node, SET_SIZE);
      return node;
  }

    static NodePtr CreateNodeWithoutAttrs2(bool has_weight = false)
    {
        FeTestOpDescBuilder builder;
        builder.SetName("test_tvm");
        builder.SetInputs({0,1});
        builder.SetOutputs({1});
        builder.AddInputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        builder.AddInputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        builder.AddOutputDesc({1,1,1,1}, ge::FORMAT_NCHW, ge::DT_FLOAT);
        if (has_weight) {
            size_t len = 10;
            unique_ptr<float[]> buf(new float[len]);
            auto weight = builder.AddWeight((uint8_t*) buf.get(), len * sizeof(float), { 1, 1, 2, 5 },
                    ge::FORMAT_NCHW, ge::DT_FLOAT);
            ge::TensorUtils::SetWeightSize(weight->MutableTensorDesc(), len * sizeof(float));
        }

        return builder.Finish2();
    }

    static NodePtr CreateNode2(bool has_weight = false)
    {
        NodePtr node = CreateNodeWithoutAttrs2(has_weight);

        const char tbe_bin[] = "tbe_bin";
        vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
        OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
        node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

        ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
        ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
        ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

        SetOpDecSize(node, SET_SIZE);
        return node;
    }

    static TaskBuilderContext CreateContext()
    {
        rtStream_t stream = nullptr;
        rtModel_t model = nullptr;

        assert(rtStreamCreate(&stream, 0) == ACL_RT_SUCCESS);
        assert(rtModelCreate(&model, 0) == ACL_RT_SUCCESS);
        assert(rtModelBindStream(model, stream, 0) == ACL_RT_SUCCESS);


        void *ccContext = nullptr;
        rtMallocHost(&ccContext, sizeof(ccContext_t));
        (void)memset_s(ccContext, sizeof(ccContext_t), 0, sizeof(ccContext_t));
        ccHandle_t handle = static_cast<ccHandle_t>(ccContext);
        handle->streamId = stream;

        TaskBuilderContext context;
        context.dataMemSize = 100;
        context.dataMemBase = (uint8_t *) (intptr_t) 1000;
        context.weightMemSize = 200;
        context.weightMemBase = (uint8_t *) (intptr_t) 1100;
        context.weightBufferHost = Buffer(20);
        context.model = model;
        context.stream = stream;
        context.handle = handle;

        return context;
    }

    static Status DestroyHandle(ccHandle_t *handle)
    {
        if (handle == nullptr || *handle == nullptr)
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
        *handle = nullptr;
        return fe::SUCCESS;
    };

    static void DestroyContext(TaskBuilderContext& context)
    {
        assert(rtModelUnbindStream(context.model, context.stream) == ACL_RT_SUCCESS);
        assert(rtModelDestroy (context.model) == ACL_RT_SUCCESS);
        assert(rtStreamDestroy (context.stream) == ACL_RT_SUCCESS);
        assert(DestroyHandle (&context.handle) == fe::SUCCESS);
    }

protected:
    NodePtr node_ { nullptr };
    TaskBuilderContext context_;
    std::shared_ptr<TbeTaskBuilderAdapter> adapter_;
};

TEST_F(STEST_TbeTaskBuilderAdapter, case_no_bin_attr_error)
{
    NodePtr node = CreateNodeWithoutAttrs();
    ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
    ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
    ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
    ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

    TbeTaskBuilderAdapter adapter(*node, context_);
    EXPECT_NE(adapter.Init(), fe::SUCCESS);
}

TEST_F(STEST_TbeTaskBuilderAdapter, case_no_magic_attr_error)
{
    NodePtr node = CreateNodeWithoutAttrs();

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
    node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
    ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
    ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

    TbeTaskBuilderAdapter adapter(*node, context_);
    EXPECT_NE(adapter.Init(), fe::SUCCESS);
}

TEST_F(STEST_TbeTaskBuilderAdapter, check_size_failed_change_input)
{
  NodePtr node = CreateNode(true);
  ge::OpDesc::Vistor<ge::GeTensorDesc> tensors_input = node->GetOpDesc()->GetAllInputsDesc();
  for (size_t i = 0; i < tensors_input.size(); i++) {
    ge::GeTensorDesc tensor_input = tensors_input.at(i);
    CheckSizeFuction(tensor_input);
    node->GetOpDesc()->UpdateInputDesc(i, tensor_input);
  }
  TbeTaskBuilderAdapter adapter(*node, context_);
  EXPECT_EQ(adapter.Init(), fe::FAILED);
}

TEST_F(STEST_TbeTaskBuilderAdapter, check_value_skip_failed_change_input)
{
  NodePtr node = CreateNode(true);
  ge::OpDesc::Vistor<ge::GeTensorDesc> tensors_input = node->GetOpDesc()->GetAllInputsDesc();
  bool value_skip = true;
  for (size_t i = 0; i < tensors_input.size(); i++) {
    ge::GeTensorDesc tensor_input = tensors_input.at(i);
    CheckSizeFuction(tensor_input);
    node->GetOpDesc()->UpdateInputDesc(i, tensor_input);

    ge::GeTensorDescPtr tensor_input_ptr = node->GetOpDesc()->MutableInputDesc(i);
    ge::AttrUtils::SetBool(tensor_input_ptr, "valueSkip", value_skip);
    value_skip = (value_skip == true) ? false : true;
  }

  TbeTaskBuilderAdapter adapter(*node, context_);
  EXPECT_EQ(adapter.Init(), fe::FAILED);
}


TEST_F(STEST_TbeTaskBuilderAdapter, check_size_failed_change_output)
{
  NodePtr node = CreateNode(true);
  ge::OpDesc::Vistor<ge::GeTensorDesc> tensors_output = node->GetOpDesc()->GetAllOutputsDesc();
  for (size_t i = 0; i < tensors_output.size(); i++) {
    ge::GeTensorDesc tensor_output = tensors_output.at(i);
    CheckSizeFuction(tensor_output);
    node->GetOpDesc()->UpdateOutputDesc(i, tensor_output);
  }
  TbeTaskBuilderAdapter adapter(*node, context_);
  EXPECT_EQ(adapter.Init(), fe::FAILED);
}

TEST_F(STEST_TbeTaskBuilderAdapter, check_size_success_change_output_real_calc_flag)
{
  NodePtr node = CreateNode3(true);
  ge::OpDesc op_desc = *(node->GetOpDesc().get());
  ge::AttrUtils::SetInt(node->GetOpDesc(), ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, 1);
  for (size_t i = 0; i < op_desc.GetOutputsSize(); i++) {
    ge::GeTensorDesc tensor_output = op_desc.GetOutputDesc(i);
    node->GetOpDesc()->UpdateOutputDesc(i, tensor_output);
  }
  TbeTaskBuilderAdapter adapter(*node, context_);
  EXPECT_EQ(adapter.Init(), fe::SUCCESS);
}

TEST_F(STEST_TbeTaskBuilderAdapter, check_size_failed_change_output_real_calc_flag)
{
  NodePtr node = CreateNode3(true);
  ge::OpDesc op_desc = *(node->GetOpDesc().get());
  ge::AttrUtils::SetInt(op_desc, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, 0);
  for (size_t i = 0; i < op_desc.GetOutputsSize(); i++) {
    ge::GeTensorDesc tensor_output = op_desc.GetOutputDesc(i);
    node->GetOpDesc()->UpdateOutputDesc(i, tensor_output);
  }
  TbeTaskBuilderAdapter adapter(*node, context_);
  EXPECT_EQ(adapter.Init(), fe::FAILED);
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_01_no_args) {
  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::NO_ARGS;
  NodePtr node_ptr = CreateNode();

  OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  auto input_desc = op_desc_ptr->MutableInputDesc(0);
  ge::AttrUtils::SetBool(input_desc, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE, true);
  vector<int32_t> read_dist_vec = {0, 0};
  ge::AttrUtils::SetListInt(input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);
  std::shared_ptr<TbeTaskBuilderAdapter> tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node_ptr, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  EXPECT_EQ(ge::AttrUtils::HasAttr(input_desc, ATTR_NAME_L2CACHE_GRAPH_READ_MODE), false);
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_01_no_args_02) {
  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::NO_ARGS;
  NodePtr node_ptr = CreateNode();

  OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  ge::AttrUtils::SetStr(op_desc_ptr, ATTR_NAME_KERNEL_LIST_FIRST_NAME, op_desc_ptr->GetName());
  auto input_desc = op_desc_ptr->MutableInputDesc(0);
  ge::AttrUtils::SetBool(input_desc, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE, true);
  vector<int32_t> read_dist_vec = {0, 0};
  ge::AttrUtils::SetListInt(input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);
  std::shared_ptr<TbeTaskBuilderAdapter> tbe_task_builder_adapter =
          shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node_ptr, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  EXPECT_EQ(ge::AttrUtils::HasAttr(input_desc, ATTR_NAME_L2CACHE_GRAPH_READ_MODE), false);
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_02_withlifecycle_nnw) {

  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node = CreateNode();
  OpDescPtr op = node->GetOpDesc();
  auto op_input_desc = op->MutableInputDesc(0);
  ge::AttrUtils::SetBool(op_input_desc, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE, true);
  vector<int32_t> read_dist_vec = {1, 2};
  ge::AttrUtils::SetListInt(op_input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);
  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(op_input_desc, L2CacheReadMode::READ_INVALID);

  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_03_withlifecycle_ri) {

  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node = CreateNode();
  OpDescPtr op = node->GetOpDesc();
  auto op_input_desc = op->MutableInputDesc(0);
  ge::AttrUtils::SetBool(op_input_desc, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE, true);
  vector<int32_t> read_dist_vec = {6};
  ge::AttrUtils::SetListInt(op_input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);

  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(op_input_desc, L2CacheReadMode::READ_INVALID);

  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_04_withlifecycle_distance_notexist) {
  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node = CreateNode();
  OpDescPtr op = node->GetOpDesc();
  auto op_input_desc = op->MutableInputDesc(0);
  ge::AttrUtils::SetBool(op_input_desc, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE, true);
  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(op_input_desc, L2CacheReadMode::NOT_NEED_WRITEBACK);

  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_05_withoutlifecycle_rl) {
  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node = CreateNode();
  OpDescPtr op = node->GetOpDesc();
  auto op_input_desc = op->MutableInputDesc(0);
  vector<int32_t> read_dist_vec = {0, 2};
  ge::AttrUtils::SetListInt(op_input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);
  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(op_input_desc, L2CacheReadMode::READ_LAST);
  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_06_withoutlifecycle_ri) {

  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node = CreateNode();
  OpDescPtr op = node->GetOpDesc();
  auto op_input_desc = op->MutableInputDesc(0);
  vector<int32_t> read_dist_vec = {0, 6};
  ge::AttrUtils::SetListInt(op_input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);
  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(op_input_desc, L2CacheReadMode::READ_INVALID);
  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_07_withoutlifecycle_distance_notexist) {
  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node = CreateNode();
  OpDescPtr op = node->GetOpDesc();
  auto op_input_desc = op->MutableInputDesc(0);
  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(op_input_desc, L2CacheReadMode::READ_LAST);
  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_08_datainput) {

  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node_ptr = CreateNodeWithInputNode(OP_TYPE_DATA);
  OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  auto input_desc = op_desc_ptr->MutableInputDesc(1);
  vector<int32_t> read_dist_vec = {-1, 2};
  ge::AttrUtils::SetListInt(input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);

  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node_ptr, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(input_desc, L2CacheReadMode::NOT_NEED_WRITEBACK);

  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_09_constinput) {
  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node_ptr = CreateNodeWithInputNode(OP_TYPE_CONSTANT);
  OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  auto input_desc = op_desc_ptr->MutableInputDesc(1);
  vector<int32_t> read_dist_vec = {-1, 2};
  ge::AttrUtils::SetListInt(input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);

  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node_ptr, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(input_desc, L2CacheReadMode::NOT_NEED_WRITEBACK);

  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, l2cache_rc_cache_10_variableinput) {
  fe::setFuncState(FuncParamType::FUSION_L2, false);
  Configuration &config_inst = Configuration::Instance(fe::AI_CORE_NAME);
  config_inst.append_args_mode_ = AppendArgsMode::L2_CACHE_ARGS;
  config_inst.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;

  NodePtr node_ptr = CreateNodeWithInputNode(OP_TYPE_VARIABLE);
  OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  auto input_desc = op_desc_ptr->MutableInputDesc(1);
  vector<int32_t> read_dist_vec = {-1, 2};
  ge::AttrUtils::SetListInt(input_desc, ge::ATTR_NAME_DATA_VISIT_DISTANCE, read_dist_vec);

  auto tbe_task_builder_adapter =
      shared_ptr<TbeTaskBuilderAdapter>(new (nothrow) TbeTaskBuilderAdapter(*node_ptr, context_));
  tbe_task_builder_adapter->Init();
  domi::TaskDef task_def = {};
  Status ret = tbe_task_builder_adapter->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
  CheckGraphReadMode(input_desc, L2CacheReadMode::READ_LAST);

  config_inst.buffer_fusion_mode_ = EN_L2_FUSION;
}

TEST_F(STEST_TbeTaskBuilderAdapter, kernel_lanuch_1) {
  std::string str = "hello...";
  domi::TaskDef task_def = {};
  char* str_p = "hello...";
  rtSmDesc_t sm_desc;
  bool ret = TbeKernelLaunch::KernelLaunch(str, 4, str_p, 8, &sm_desc, task_def);
  EXPECT_EQ(ret, true);
}

TEST_F(STEST_TbeTaskBuilderAdapter, kernel_launch_with_handle_1) {
  std::string str = "hello...";
  const char* const_str_p = str.c_str();
  domi::TaskDef task_def = {};
  char* str_p = "hello...";
  rtSmDesc_t sm_desc;
  bool ret = TbeKernelLaunch::KernelLaunchWithHandle(4, str_p, 8, &sm_desc, task_def);
  EXPECT_EQ(ret, true);
}

TEST_F(STEST_TbeTaskBuilderAdapter, set_input_addr_from_database) {
  int64_t input_offset = 1;
  std::map<int64_t, uint8_t *> mem_type_to_data_mem_base1;
  uint8_t zero = 0;
  uint8_t one = 1;
  mem_type_to_data_mem_base1[0] = &zero;
  mem_type_to_data_mem_base1[1] = &one;
  adapter_->context_.mem_type_to_data_mem_base = mem_type_to_data_mem_base1;
  ge::OpDescPtr op_desc_ = std::make_shared<OpDesc>("test", "test");
  (void)ge::AttrUtils::SetInt(op_desc_, "_tensor_memory_type", -1);
  adapter_->SetInputAddrFromDataBase(input_offset);
}