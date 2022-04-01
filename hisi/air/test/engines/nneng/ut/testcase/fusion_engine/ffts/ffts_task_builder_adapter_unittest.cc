/**
 *
 * @file ffts_task_builder_adapter_unittest.cc
 *
 * @brief
 *
 * @version 1.0
 *
 */
#include <gtest/gtest.h>
#include <iostream>

#include <list>

#define private public
#define protected public
#include "ffts/ffts_task_builder_adapter.h"
#include "graph/debug/ge_attr_define.h"
#include "common/util/op_info_util.h"
#include "../fe_test_utils.h"
#include "external/runtime/rt_error_codes.h"
#include "../../../../graph_constructor/graph_constructor.h"

using namespace std;
using namespace fe;
using namespace ge;
using FftsTaskBuilderAdapterPtr = shared_ptr<FftsTaskBuilderAdapter>;

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

class FFTSTaskBuilderAdapterUTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    cout << "FFTSTaskBuilderUTest SetUpTestCase" << endl;
  }
  static void TearDownTestCase() {
    cout << "FFTSTaskBuilderUTest TearDownTestCase" << endl;
  }

  virtual void SetUp() {
    context_ = CreateContext();
    NodePtr node_ptr = CreateNode();
    ffts_task_builder_adapter_ptr = std::make_shared<FftsTaskBuilderAdapter>(*node_ptr, context_);
  }

  virtual void TearDown() {
    cout << "a test Tear Down" << endl;
    DestroyContext(context_);
  }

  void GenerateTensorSlice(vector<vector<vector<ffts::DimRange>>> &tensor_slice, const vector<vector<int64_t>> &shape) {
    for (size_t i = 0; i < 2; i++) {
      for (size_t j = 0; j < shape.size(); j++) {
        vector<int64_t> temp = shape[j];
        vector<ffts::DimRange> vdr;
        for (size_t z = 0; z < temp.size() - 1;) {
            ffts::DimRange dr;
            dr.lower = temp[z];
            dr.higher = temp[z + 1];
            vdr.push_back(dr);
            z = z + 2;
        }
        vector<vector<ffts::DimRange>> threadSlice;
        threadSlice.push_back(vdr);
        tensor_slice.push_back(threadSlice);
      }
    }
    return ;
  }

  NodePtr CreateNode()
  {
    FeTestOpDescBuilder builder;
    builder.SetName("test_tvm");
    builder.SetType("test_tvm");
    // builder.SetInputs({1,1});
    // builder.SetOutputs({1,1});
    builder.AddInputDesc({288,32,16,16}, ge::FORMAT_NCHW, ge::DT_FLOAT);
    builder.AddOutputDesc({288,32,16,16}, ge::FORMAT_NCHW, ge::DT_FLOAT);
    NodePtr node =  builder.Finish();
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    OpKernelBinPtr tbe_kernel_ptr = std::make_shared<OpKernelBin>(node->GetName(), std::move(buffer));
    node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    ge::AttrUtils::SetStr(node->GetOpDesc(), node->GetName() + "_kernelname", "my_kernel");
    ge::AttrUtils::SetInt(node->GetOpDesc(), "tvm_blockdim", 10);
    ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_magic", "RT_DEV_BINARY_MAGIC_ELF");
    ge::AttrUtils::SetStr(node->GetOpDesc(), "tvm_metadata", "my_metadata");

    vector<vector<vector<ffts::DimRange>>> tensor_slice_;
    vector<int64_t> shape_input = {0, 288, 0, 32, 0, 16, 0, 16};
    vector<vector<int64_t>> shape_;
    shape_.push_back(shape_input);
    GenerateTensorSlice(tensor_slice_, shape_);
    ffts::ThreadSliceMap thread_slice_map;
    thread_slice_map.thread_mode = 0;
    thread_slice_map.input_tensor_slice = tensor_slice_;
    thread_slice_map.output_tensor_slice = tensor_slice_;
    thread_slice_map.slice_instance_num = 2;
    ffts::ThreadSliceMapPtr thread_slice_map_ptr = std::make_shared<ffts::ThreadSliceMap>(thread_slice_map);
    node->GetOpDesc()->SetExtAttr(ffts::kAttrSgtStructInfo, thread_slice_map_ptr);
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

 public:
  FftsTaskBuilderAdapterPtr ffts_task_builder_adapter_ptr;
  TaskBuilderContext context_;
};

TEST_F(FFTSTaskBuilderAdapterUTest, FFTS_Task_Builder_Adapter_Init_SUCCESS) {
  Status ret = ffts_task_builder_adapter_ptr->Init();
  EXPECT_EQ(fe::SUCCESS, ret); 
}

TEST_F(FFTSTaskBuilderAdapterUTest, Get_Thread_Param_Offset_SUCCESS) {
  Status ret = ffts_task_builder_adapter_ptr->Init();
  ThreadParamOffset param_offset;
  ffts_task_builder_adapter_ptr->GetThreadParamOffset(param_offset);
  EXPECT_EQ(1, param_offset.first_thread_input_addrs.size()); 
  EXPECT_EQ(fe::SUCCESS, ret); 
}

TEST_F(FFTSTaskBuilderAdapterUTest, Get_handle_anchor_data_SUCCESS) {
  Status ret = ffts_task_builder_adapter_ptr->Init();
  ge::InDataAnchorPtr anchor;
  size_t input_index = 0;
  size_t anchor_index = 0;
  size_t weight_index = 0;
  ret = ffts_task_builder_adapter_ptr->HandleAnchorData(input_index, anchor_index, weight_index);
  EXPECT_EQ(fe::FAILED, ret); 
}

TEST_F(FFTSTaskBuilderAdapterUTest, Init_Workspace_SUCCESS) {
  vector<int64_t> work_space_bytes = {1, 2};
  vector<int64_t> work_space = {1, 2};
  int64_t non_tail_workspace_size = 1;
  (void)ge::AttrUtils::SetInt(ffts_task_builder_adapter_ptr->op_desc_, fe::NON_TAIL_WORKSPACE_SIZE,
  non_tail_workspace_size);
  ffts_task_builder_adapter_ptr->op_desc_->SetWorkspaceBytes(work_space_bytes);
  ffts_task_builder_adapter_ptr->op_desc_->SetWorkspace(work_space);
  Status ret = ffts_task_builder_adapter_ptr->InitWorkspace();
  EXPECT_EQ(fe::SUCCESS, ret);
  domi::TaskDef task_def = {};
  ret = ffts_task_builder_adapter_ptr->Run(task_def);
  EXPECT_EQ(fe::SUCCESS, ret);
}