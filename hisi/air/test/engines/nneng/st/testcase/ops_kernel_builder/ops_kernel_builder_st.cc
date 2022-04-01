/**
 *
 * @file ops_kernel_builder_ut.cc
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
#include "ops_kernel_builder/aicore_ops_kernel_builder.h"
#include "task_builder/task_builder.h"
#include "graph/node.h"
#include "graph/utils/tensor_utils.h"
#include "graph/compute_graph.h"
#include "common/constants_define.h"
#include "common/aicore_util_attr_define.h"

using namespace std;
using namespace fe;
using namespace ge;
using AICoreOpsKernelBuilderPtr =  shared_ptr<AICoreOpsKernelBuilder>;

class AICoreOpsKernelBuilderSTest : public testing::Test{
 protected:
  static void SetUpTestCase() {
    cout << "AICoreOpsKernelBuilderTest SetUP" << endl;
  }
  static void TearDownTestCase() {
    cout << "AICoreOpsKernelBuilderTest SetUP" << endl;
  }
  // Some expensive resource shared by all tests.
  virtual void SetUp(){
    aicore_ops_kernel_builder_ptr = make_shared<AICoreOpsKernelBuilder>();
    std::map<std::string, std::string> options;
    aicore_ops_kernel_builder_ptr->Initialize(options);
  }
  virtual void TearDown(){
    cout << "a test Tear Down" << endl;
    aicore_ops_kernel_builder_ptr->Finalize();

  }

  OpDescPtr GreateOpDesc() {
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);
    shared_ptr<ge::GeTensorDesc> tensor_desc_ptr = make_shared<ge::GeTensorDesc>();
    tensor_desc_ptr->SetShape(shape);
    tensor_desc_ptr->SetDataType(ge::DT_FLOAT);
    tensor_desc_ptr->SetFormat(ge::FORMAT_NCHW);

    OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("test_op_desc", "conv");
    op_desc_ptr->SetId(123456);
    op_desc_ptr->AddInputDesc(tensor_desc_ptr->Clone());
    op_desc_ptr->AddOutputDesc(tensor_desc_ptr->Clone());

    return op_desc_ptr;
  }
  static RunContext CreateContext()
  {
    rtStream_t stream = nullptr;
    rtModel_t model = nullptr;

    assert(rtStreamCreate(&stream, 0) == RT_ERROR_NONE);
    assert(rtModelCreate(&model, 0) == RT_ERROR_NONE);
    assert(rtModelBindStream(model, stream, 0) == RT_ERROR_NONE);

    RunContext context;
    context.model = model;
    context.stream = stream;
    context.dataMemSize = 101;
    context.dataMemBase = (uint8_t *) (intptr_t) 1000;
    context.weightMemSize = 200;
    context.weightMemBase = (uint8_t *) (intptr_t) 1101;
    context.weightsBuffer = Buffer(20);

    return context;
  }
 public:
  AICoreOpsKernelBuilderPtr aicore_ops_kernel_builder_ptr;
};

TEST_F(AICoreOpsKernelBuilderSTest, calcoprunningparam_success_1){
  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("test_graph");
  OpDescPtr op_desc_ptr = GreateOpDesc();
  NodePtr node = graph->AddNode(op_desc_ptr);

  Status status = aicore_ops_kernel_builder_ptr->CalcOpRunningParam(*node);
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(AICoreOpsKernelBuilderSTest, calcoprunningparam_success_2){
  ge::ComputeGraphPtr graph = make_shared<ge::ComputeGraph>("test_graph");
  OpDescPtr op_desc_ptr = GreateOpDesc();
  op_desc_ptr->SetType("ROIPooling");
  ge::AttrUtils::SetStr(op_desc_ptr, "_unregst_oppath", "/usr/local");
  NodePtr node = graph->AddNode(op_desc_ptr);

  Status status = aicore_ops_kernel_builder_ptr->CalcOpRunningParam(*node);
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(AICoreOpsKernelBuilderSTest, calc_op_running_param_succ)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({100, 2, 3, 512, 4}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  src_tensor_desc.SetOriginShape(GeShape({10, 11, 12, 13}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);

  Status ret = aicore_ops_kernel_builder_ptr->CalcOpRunningParam(*src_node.get());
  int64_t data_size = 0;
  ge::GeTensorDesc tensor_desc = src_op->GetInputDesc(0);
  ge::TensorUtils::GetSize(tensor_desc, data_size);

  EXPECT_EQ(2457632, data_size);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(AICoreOpsKernelBuilderSTest, GenerateTask_mix_l2_success)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({10}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  ge::TensorUtils::SetSize(src_tensor_desc, 64);
  src_tensor_desc.SetOriginShape(GeShape({10}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::RunContext context = CreateContext();
  std::vector<domi::TaskDef> tasks;
  (void)ge::AttrUtils::SetStr(src_op, fe::ATTR_NAME_ALIAS_ENGINE_NAME, "ffts_plus");
  string bin_magic = "FFTS_BINARY_MAGIC_ELF_MIX_AIC";
  (void)ge::AttrUtils::SetStr(src_op, "tvm_magic", bin_magic);
  (void)ge::AttrUtils::SetStr(src_op, fe::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIC");
  Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node.get(), context, tasks);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(AICoreOpsKernelBuilderSTest, GenDynamicAICAIVCtxDef_success)
{
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test_graph_input");
  OpDescPtr src_op = std::make_shared<OpDesc>("A", "A");
  GeTensorDesc src_tensor_desc(GeShape({10}), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  ge::TensorUtils::SetSize(src_tensor_desc, 64);
  src_tensor_desc.SetOriginShape(GeShape({10}));
  src_tensor_desc.SetOriginFormat(ge::FORMAT_NHWC);
  src_op->AddOutputDesc(src_tensor_desc);
  src_op->AddInputDesc(src_tensor_desc);
  auto src_node = graph->AddNode(src_op);
  ge::RunContext context = CreateContext();
  std::vector<domi::TaskDef> tasks;
  (void)ge::AttrUtils::SetStr(src_op, fe::kTypeFFTSPlus, "ffts_plus");
  ffts::ThreadSliceMapPtr slice_info_ptr = make_shared<ffts::ThreadSliceMap>();
  slice_info_ptr->thread_mode = static_cast<uint32_t>(ffts::ThreadMode::AUTO_THREAD);
  src_op->SetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  (void)ge::AttrUtils::SetBool(src_op, "OwnerGraphIsUnknown", true);
  (void)ge::AttrUtils::SetStr(src_op, fe::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "AIC");
  Status ret = aicore_ops_kernel_builder_ptr->GenerateTask(*src_node.get(), context, tasks);
  EXPECT_EQ(fe::SUCCESS, ret);
}