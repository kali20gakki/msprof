/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include <tuple>
#include <gtest/gtest.h>
#include "runtime/rt.h"
#include "framework/executor/ge_executor.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "single_op/single_op.h"
#include "single_op/single_op_manager.h"
#include "utils/model_data_builder.h"
#include "single_op/task/build_task_utils.h"
#include "single_op/task/tbe_task_builder.h"
#include "utils/tensor_descs.h"
#include "utils/data_buffers.h"
#include "register/op_tiling_registry.h"
#include "graph/debug/ge_attr_define.h"
#include "hybrid/node_executor/aicore/aicore_node_executor.h"
#include "hybrid/node_executor/ge_local/ge_local_node_executor.h"
#include "graph/manager/graph_mem_manager.h"
#include "utils/bench_env.h"
#include "utils/graph_factory.h"
#include "utils/mock_runtime.h"
#include "common/profiling/profiling_manager.h"
#include "common/profiling/profiling_properties.h"
#include "common/profiling/profiling_init.h"
#include "common/tbe_handle_store/tbe_handle_store.h"
#include "hybrid/executor/hybrid_model_pipeline_executor.h"
#include "hybrid/model/hybrid_model_builder.h"
#include "graph/utils/graph_utils.h"
#include "graph/operator_factory_impl.h"
#include "common/tbe_handle_store/tbe_handle_store.h"

using namespace ge;
using namespace std;
using namespace optiling::utils;
using namespace ge::hybrid;

namespace {
constexpr int64_t kMemtypeHostCompileIndependent = 2;
uint8_t kBufferAddr[1024] = {};
using SingleOpArgsTuple = std::tuple<std::vector<GeTensorDesc>,
    std::vector<DataBuffer>,
    std::vector<GeTensorDesc>,
    std::vector<DataBuffer>>;
SingleOpArgsTuple CreateSingleOpArgsForHostMemInput() {
  std::vector<GeTensorDesc> inputs;
  std::vector<GeTensorDesc> outputs;
  GeShape shape0({4});
  GeTensorDesc tensor_desc0(shape0, FORMAT_ND, DT_UINT64);
  AttrUtils::SetInt(tensor_desc0, ge::ATTR_NAME_PLACEMENT, kMemtypeHostCompileIndependent);
  inputs.emplace_back(tensor_desc0);

  GeShape shape1({1});
  GeTensorDesc tensor_desc1(shape1, FORMAT_ND, DT_UINT64);
  AttrUtils::SetInt(tensor_desc1, ge::ATTR_NAME_PLACEMENT, kMemtypeHostCompileIndependent);
  inputs.emplace_back(tensor_desc1);

  GeShape shape2({4});
  GeTensorDesc tensor_desc2(shape2, FORMAT_ND, DT_UINT64);
  outputs.emplace_back(tensor_desc2);

  std::vector<DataBuffer> input_buffers;
  std::vector<DataBuffer> output_buffers;
  const size_t input0_size = 4 * sizeof(uint64_t);
  const size_t input1_size = 1 * sizeof(uint64_t);
  const size_t output_size = 4 * sizeof(uint64_t);
  uint64_t *addr = PtrToPtr<uint8_t, uint64_t>(kBufferAddr);
  for (size_t i = 0; i < (input0_size + input1_size); i++) {
    addr[i] = kHostMemInputValue;
  }
  input_buffers.emplace_back(DataBuffer(kBufferAddr, input0_size, false, 1));
  input_buffers.emplace_back(DataBuffer(kBufferAddr + input0_size, input1_size, false, 1));
  output_buffers.emplace_back(DataBuffer(kBufferAddr + input0_size + input1_size, output_size));
  return {inputs, input_buffers, outputs, output_buffers};
}
}
namespace ge {
class HybridModelAsyncTest : public testing::Test {
protected:
    void SetUp() {}

    void TearDown() {}
};

TEST_F(HybridModelAsyncTest, test_hybrid_model_dynamic_shape_success) {
    ProfilingProperties::Instance().SetLoadProfiling(true);
    BenchEnv::Init();
    uint8_t model_data[8192];
    ge::ModelData modelData{.model_data = model_data};
    ModelDataBuilder(modelData).AddGraph(GraphFactory::HybridSingeOpGraph()).AddTask(2, 2)
    .AddTask(2, 4)
    .AddTask(2, 4)
    .AddTask(2, 5)
    .AddTask(2, 5)
    .Build();

    auto input_desc = TensorDescs(2).Value();
    auto input_buffers = DataBuffers(2).Value();
    auto output_desc = TensorDescs(1).Value();
    auto output_buffers = DataBuffers(1).Value();

    ge::DynamicSingleOp *singleOp = nullptr;
    EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op", modelData, nullptr, &singleOp, 4), SUCCESS);
    EXPECT_EQ(singleOp->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_singleop_dynamic_shape_success) {
    ProfilingProperties::Instance().SetLoadProfiling(true);
    BenchEnv::Init();
    uint8_t model_data[8192];
    ge::ModelData modelData{.model_data = model_data};
    ModelDataBuilder(modelData).AddGraph(GraphFactory::SingeOpGraph()).AddTask(2, 2).Build();

    auto input_desc = TensorDescs(1).Value();
    auto input_buffers = DataBuffers(1).Value();
    auto output_desc = TensorDescs(1).Value();
    auto output_buffers = DataBuffers(1).Value();

    ge::DynamicSingleOp *single_op = nullptr;
    std::vector<char> kernelBin;
    TBEKernelPtr tbe_kernel = std::make_shared<ge::OpKernelBin>("name/MyTransdata", std::move(kernelBin));
    auto holder = std::unique_ptr<KernelHolder>(new (std::nothrow) KernelHolder("0/_tvmbin", tbe_kernel));
    KernelBinRegistry::GetInstance().AddKernel("0/_tvmbin", std::move(holder));
    EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op2", modelData, nullptr, &single_op, 0), SUCCESS);
    EXPECT_EQ(single_op->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
}

// dynamic hybrid , aicore with host mem input
TEST_F(HybridModelAsyncTest, test_singleop_with_hostmem_success) {
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(GraphFactory::HybridSingeOpGraphForHostMemInput()).AddTask(2, 2)
      .AddTask(2, 4)
      .AddTask(2, 4)
      .AddTask(2, 5)
      .AddTask(2, 5)
      .Build();
  SingleOpArgsTuple arg = CreateSingleOpArgsForHostMemInput();

  ge::DynamicSingleOp *single_op = nullptr;
  std::vector<char> kernelBin;
  TBEKernelPtr tbe_kernel = std::make_shared<ge::OpKernelBin>("name/transdata", std::move(kernelBin));
  auto holder = std::unique_ptr<KernelHolder>(new (std::nothrow) KernelHolder("0/_tvmbin", tbe_kernel));
  KernelBinRegistry::GetInstance().AddKernel("0/_tvmbin", std::move(holder));
  EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op2", modelData, nullptr, &single_op, 110), SUCCESS);
  EXPECT_EQ(single_op->ExecuteAsync(get<0>(arg), get<1>(arg), get<2>(arg), get<3>(arg)), SUCCESS);
}

// dynamic hybrid , aicpu with host mem input
TEST_F(HybridModelAsyncTest, test_singleop_aicpu_with_hostmem_success) {
  auto runtime_stub = MockForKernelLaunchWithHostMemInput();
  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(GraphFactory::HybridSingeOpGraphAicpuForHostMemInput()).AddTask(2, 2)
      .AddAicpuTask(4)
      .AddAicpuTask(5)
      .Build();
  SingleOpArgsTuple arg = CreateSingleOpArgsForHostMemInput();

  ge::DynamicSingleOp *single_op = nullptr;
  EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op2", modelData, nullptr, &single_op, 111), SUCCESS);
  EXPECT_EQ(single_op->ExecuteAsync(get<0>(arg), get<1>(arg), get<2>(arg), get<3>(arg)), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_singleop_aicpu_load_success) {
    BenchEnv::Init();
    uint8_t model_data[8192];
    ge::ModelData modelData{.model_data = model_data};
    auto graph = GraphFactory::SingeOpGraph2();
    ComputeGraphPtr compute_graph_ = ge::GraphUtils::GetComputeGraph(graph);
    AttrUtils::SetInt(compute_graph_, "globleworkspace_status", 1);
    AttrUtils::SetInt(compute_graph_, "globleworkspace_status_bytes", 512);
    graph = ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph_);
    ModelDataBuilder(modelData).AddGraph(graph).AddAicpuTask(2).Build();

    auto input_desc = TensorDescs(1).Value();
    auto input_buffers = DataBuffers(1).Value();
    auto output_desc = TensorDescs(1).Value();
    auto output_buffers = DataBuffers(1).Value();

    ge::DynamicSingleOp *single_op = nullptr;
    EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op2", modelData, nullptr, &single_op, 112), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_hybird_with_hostmem_success) {
  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(GraphFactory::SingeOpGraph3()).AddTask(2, 3).Build();

  auto input_desc = TensorDescs(1).Value();
  auto input_buffers = DataBuffers(1, 1).Value();
  input_buffers[0].placement = 1;
  auto output_desc = TensorDescs(1).Value();
  auto output_buffers = DataBuffers(1).Value();

  ge::DynamicSingleOp *single_op = nullptr;
  EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op2", modelData, nullptr, &single_op, 3), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_pipeline_execute_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  GeModelPtr ge_sub_model = make_shared<GeModel>();
  ge_root_model->SetSubgraphInstanceNameToModel("sub", ge_sub_model);
  HybridModel hybrid_model(ge_root_model);
  HybridModelBuilder builder(hybrid_model);
  EXPECT_EQ(builder.Build(), SUCCESS);

  HybridModelPipelineExecutor pipeline_executor(&hybrid_model, 1);
  EXPECT_EQ(pipeline_executor.Init(), SUCCESS);

  StageSubject stage_subject;
  stage_subject.Release(1);
  EXPECT_EQ(stage_subject.Await(1), SUCCESS);
  HybridModelExecutor::ExecuteArgs args;
  EXPECT_EQ(pipeline_executor.Execute(args), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_ascend_aicpu_load_success) {
    ProfilingProperties::Instance().SetLoadProfiling(true);
    BenchEnv::Init();
    uint8_t model_data[8192];
    ge::ModelData modelData{.model_data = model_data};
    ModelDataBuilder(modelData).AddGraph(GraphFactory::BuildAicpuSingeOpGraph()).AddTask(2, 2)
    .AddAicpuTask(4)
    .AddAicpuTask(5)
    .Build();

    auto input_desc = TensorDescs(2).Value();
    auto input_buffers = DataBuffers(2).Value();
    auto output_desc = TensorDescs(1).Value();
    auto output_buffers = DataBuffers(1).Value();

    ge::DynamicSingleOp *singleOp = nullptr;
    EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op3", modelData, nullptr, &singleOp, 5), SUCCESS);
    EXPECT_EQ(singleOp->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_dynamic_shape_with_squeezev3_success) {
  ProfilingProperties::Instance().SetLoadProfiling(false);
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
                      .InCnt(1)
                      .OutCnt(1)
                      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 3, 4})
                      .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                      .Attr("compile_info_key", "ddd")
                      .Attr("compile_info_json", "cccc")
                      .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
                       .InCnt(1)
                       .OutCnt(1)
                       .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1})
                       .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                       .Attr("compile_info_key", "ddd")
                       .Attr("compile_info_json", "cccc")
                       .Attr("_force_unknown_shape", true)
                       .Build("axes");

    auto squeezev3 = OP_CFG(SQUEEZEV3)
                         .InCnt(2)
                         .OutCnt(1)
                         .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                         .Attr("_force_infershape_when_running", true)
                         .Attr("_force_unknown_shape", true)
                         .Build("SqueezeV3");

    auto net_output = OP_CFG(NETOUTPUT)
                          .InCnt(1)
                          .OutCnt(1)
                          .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                          .Build("net_output");

    CHAIN(NODE(op_ptr)->EDGE(0, 0)->NODE(squeezev3)->EDGE(0, 0)->NODE(net_output));
    CHAIN(NODE(op_ptr2)->EDGE(0, 1)->NODE(squeezev3));
  };

  const auto ShapeInfer = [](Operator &op) {
    auto op_desc = OpDescUtils::GetOpDescFromOperator(op);
    const auto input_desc_x = op_desc->MutableInputDesc(0);
    const auto &input_shape = input_desc_x->GetShape();
    const auto x_shape_dim = input_shape.GetDims();
    auto output_desc = op_desc->MutableOutputDesc(0);

    output_desc->SetDataType(input_desc_x->GetDataType());
    output_desc->SetOriginDataType(input_desc_x->GetDataType());

    const std::vector<string> dep_inputs = {"axes"};
    op_desc->SetOpInferDepends(dep_inputs);
    std::vector<int64_t> axes_val;
    const auto axes_idx = static_cast<uint32_t>(op_desc->GetInputIndexByName("axes"));
    const GeTensor *tensor = OpDescUtils::GetInputConstData(op, axes_idx);
    if (tensor != nullptr) {
      auto pbuff = tensor->GetData().GetData();
      if (pbuff == nullptr) {
        GELOGE(FAILED, "[InferShape] Get data from axis input failed, as data buff is null");
        return GRAPH_FAILED;
      }
      const auto axes_len = tensor->GetData().GetSize();
      auto axes_pbuff = const_cast<int64_t *>(reinterpret_cast<const int64_t *>(pbuff));
      for (size_t i = 0UL; i < (axes_len / sizeof(int64_t)); ++i) {
        axes_val.emplace_back(axes_pbuff[i]);
      }
    } else {
      GELOGW("Op get input const data of axes failed");
    }
    GeShape &output_shape = output_desc->MutableShape();
    const auto dim_size = x_shape_dim.size();
    output_shape.SetDimNum(dim_size);

    std::vector<uint64_t> dim_idx(dim_size);
    std::vector<std::pair<int64_t, int64_t>> output_range;
    std::for_each(axes_val.begin(), axes_val.end(), [&dim_idx](const int64_t axis) { dim_idx[axis] = -1; });

    uint64_t idx = 0UL;
    for (size_t i = 0UL; i < dim_size; ++i) {
      if (dim_idx[i] != -1) {
        output_shape.SetDim(idx, x_shape_dim[i]);
        ++idx;
      }
    }
    output_shape.SetDimNum(idx);
    output_desc->SetShapeRange(output_range);
    output_desc->SetOriginShape(output_shape);
    return GRAPH_SUCCESS;
  };
  auto graph = ToGeGraph(dynamic_op);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto squeezev3 = compute_graph->FindNode("SqueezeV3");
  std::map<string, uint32_t> names;
  names["input0"] = 0;
  names["axes"] = 1;
  squeezev3->GetOpDesc()->UpdateInputName(names);
  auto axes_desc = squeezev3->GetOpDesc()->MutableInputDesc(1);
  GeTensor weight;
  int64_t data = 0;
  weight.SetData(reinterpret_cast<uint8_t *>(&data), sizeof(int64_t));
  GeTensorDesc weight_desc;
  weight.SetTensorDesc(weight_desc);
  GeTensorPtr tensor = MakeShared<GeTensor>(weight);
  AttrUtils::SetTensor(axes_desc, ATTR_NAME_VALUE, tensor);
  auto graph_2 = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(graph_2).AddTask(2, 3).Build();

  auto input_desc = TensorDescs(2).Value();
  auto buffers = DataBuffers(3, true).Value();
  std::vector<DataBuffer> input_buffers;
  input_buffers.emplace_back(buffers[0]);
  input_buffers.emplace_back(buffers[1]);
  auto output_desc = TensorDescs(1).Value();
  std::vector<DataBuffer> output_buffers;
  output_buffers.emplace_back(buffers[2]);

  ge::DynamicSingleOp *singleOp = nullptr;
  EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op", modelData, nullptr, &singleOp, 1), SUCCESS);

  OperatorFactoryImpl::operator_infershape_funcs_->emplace("SqueezeV3", ShapeInfer);
  EXPECT_EQ(singleOp->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_dynamic_shape_with_unsqueezev3_success) {
  DEF_GRAPH(dynamic_op) {
    auto op_ptr = OP_CFG(DATA)
                      .InCnt(1)
                      .OutCnt(1)
                      .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 3, 4})
                      .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                      .Attr("compile_info_key", "ddd")
                      .Attr("compile_info_json", "cccc")
                      .Build("data1");

    auto op_ptr2 = OP_CFG(DATA)
                       .InCnt(1)
                       .OutCnt(1)
                       .TensorDesc(FORMAT_NCHW, DT_FLOAT, {1})
                       .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                       .Attr("compile_info_key", "ddd")
                       .Attr("compile_info_json", "cccc")
                       .Attr("_force_unknown_shape", true)
                       .Build("axes");

    auto unsqueezev3 = OP_CFG(UNSQUEEZEV3)
                         .InCnt(2)
                         .OutCnt(1)
                         .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                         .Attr("_force_infershape_when_running", true)
                         .Attr("_force_unknown_shape", true)
                         .Build("UnsqueezeV3");

    auto net_output = OP_CFG(NETOUTPUT)
                          .InCnt(1)
                          .OutCnt(1)
                          .Attr("_ge_attr_op_kernel_lib_name", "DNN_VM_GE_LOCAL_OP_STORE")
                          .Build("net_output");

    CHAIN(NODE(op_ptr)->EDGE(0, 0)->NODE(unsqueezev3)->EDGE(0, 0)->NODE(net_output));
    CHAIN(NODE(op_ptr2)->EDGE(0, 1)->NODE(unsqueezev3));
  };

  const auto ShapeInfer = [](Operator &op) {
    return GRAPH_SUCCESS;
  };

  auto graph = ToGeGraph(dynamic_op);
  auto compute_graph = GraphUtils::GetComputeGraph(graph);
  auto unsqueezev3 = compute_graph->FindNode("UnsqueezeV3");
  std::map<string, uint32_t> names;
  names["input0"] = 0;
  names["axes"] = 1;
  unsqueezev3->GetOpDesc()->UpdateInputName(names);
  auto axes_desc = unsqueezev3->GetOpDesc()->MutableInputDesc(1);
  GeTensor weight;
  int64_t data = 0;
  weight.SetData(reinterpret_cast<uint8_t *>(&data), sizeof(int64_t));
  GeTensorDesc weight_desc;
  weight.SetTensorDesc(weight_desc);
  GeTensorPtr tensor = MakeShared<GeTensor>(weight);
  AttrUtils::SetTensor(axes_desc, ATTR_NAME_VALUE, tensor);
  auto graph_2 = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(graph_2).AddTask(2, 3).Build();

  auto input_desc = TensorDescs(2).Value();
  auto buffers = DataBuffers(3, true).Value();
  std::vector<DataBuffer> input_buffers;
  input_buffers.emplace_back(buffers[0]);
  input_buffers.emplace_back(buffers[1]);
  auto output_desc = TensorDescs(1).Value();
  std::vector<DataBuffer> output_buffers;
  output_buffers.emplace_back(buffers[2]);

  ge::DynamicSingleOp *singleOp = nullptr;
  EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op", modelData, nullptr, &singleOp, 1), SUCCESS);

  OperatorFactoryImpl::operator_infershape_funcs_->emplace("UnsqueezeV3", ShapeInfer);
  EXPECT_EQ(singleOp->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
}

TEST_F(HybridModelAsyncTest, test_hybrid_model_dynamic_shape_failed) {
    ProfilingProperties::Instance().SetLoadProfiling(false);
    BenchEnv::Init();
    uint8_t model_data[8192];
    ge::ModelData modelData{.model_data = model_data};
    ModelDataBuilder(modelData).AddGraph(GraphFactory::HybridSingeOpGraph2()).AddTask(2, 2)
    .AddTask(2, 5)
    .AddTask(2, 6)
    .Build();

    auto input_desc = TensorDescs(2).Value();
    auto input_buffers = DataBuffers(2).Value();
    auto output_desc = TensorDescs(1).Value();
    auto &output_buffers = DataBuffers(1).Value();
    output_buffers[0] = DataBuffer(0, 1, false);

    ge::DynamicSingleOp *singleOp = nullptr;
    EXPECT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op", modelData, nullptr, &singleOp, 1), SUCCESS);
    EXPECT_EQ(singleOp->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), GRAPH_PARAM_INVALID);
}

}  // namespace ge