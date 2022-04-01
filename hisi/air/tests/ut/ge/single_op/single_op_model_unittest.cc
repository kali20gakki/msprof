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

#include <gtest/gtest.h>
#include <vector>

#define protected public
#define private public
#include "graph/load/model_manager/model_utils.h"
#include "graph/utils/graph_utils.h"
#include "runtime/rt.h"
#include "single_op/single_op_model.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "single_op/task/tbe_task_builder.h"
#include "single_op/task/rts_kernel_task_builder.h"
#include "single_op/task/op_task.h"
#include "framework/common/helper/model_helper.h"
#include "single_op/single_op.h"
#include "single_op/stream_resource.h"
#include "graph/passes/graph_builder_utils.h"
#include "graph/op_desc_impl.h"
#include "framework/generator/ge_generator.h"
#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace ge;

namespace {
struct AicpuTaskStruct {
  aicpu::AicpuParamHead head;
  uint64_t io_addrp[6];
}__attribute__((packed));
}  // namespace

class UtestSingleOpModel : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

//rt api stub
rtError_t rtGetTaskIdAndStreamID(uint32_t *taskId, uint32_t *streamId) {
  return RT_ERROR_NONE;
}

TEST_F(UtestSingleOpModel, test_init_model) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  ASSERT_NE(model.InitModel(), SUCCESS);

  ge::OpDescPtr op_desc = std::make_shared<OpDesc>("Mul", MATMUL);
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->SetOutputOffset(std::vector<int64_t>{4});
  ge::ComputeGraphPtr compute_graph = std::make_shared<ge::ComputeGraph>("default");
  ge::NodePtr node = compute_graph->AddNode(op_desc);

  GeTensorDesc tensor(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32);

  ge::OpDescPtr data_op_desc = std::make_shared<OpDesc>("data", DATA_TYPE);
  data_op_desc->AddOutputDesc(tensor);
  AttrUtils::SetInt(tensor, ATTR_NAME_PLACEMENT, 1);
  data_op_desc->AddInputDesc(tensor);
  TensorUtils::SetSize(*data_op_desc->MutableOutputDesc(0), 16);
  data_op_desc->SetOutputOffset(std::vector<int64_t>{4});
  ge::NodePtr data_node = compute_graph->AddNode(data_op_desc);

  ge::OpDescPtr const_op_desc = std::make_shared<OpDesc>("const", CONSTANT);
  ge::NodePtr const_node = compute_graph->AddNode(const_op_desc);
  const_op_desc->AddOutputDesc(tensor);
  const_op_desc->AddInputDesc(tensor);
  const_op_desc->SetOutputOffset(std::vector<int64_t>{4});
  TensorUtils::SetSize(*const_op_desc->MutableOutputDesc(0), 16);

  ge::OpDescPtr netoutput_op_desc = std::make_shared<OpDesc>("netoutput", NETOUTPUT);
  netoutput_op_desc->AddOutputDesc(tensor);
  netoutput_op_desc->AddInputDesc(tensor);
  netoutput_op_desc->SetOutputOffset(std::vector<int64_t>{4});
  TensorUtils::SetSize(*netoutput_op_desc->MutableOutputDesc(0), 16);
  ge::NodePtr netoutput_node = compute_graph->AddNode(netoutput_op_desc);

  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(graph);
  std::shared_ptr<domi::ModelTaskDef> tasks = std::make_shared<domi::ModelTaskDef>();
  domi::TaskDef *task = tasks->add_task();
  task->set_type(RT_MODEL_TASK_KERNEL);
  auto context = task->mutable_kernel()->mutable_context();
  ge_model->task_ = tasks;

  ModelHelper model_helper;
  model_helper.model_ = ge_model;
  model.model_helper_ = model_helper;
  ASSERT_NE(model.InitModel(), SUCCESS);
  ASSERT_EQ(model.LoadAllNodes(), SUCCESS);
  ASSERT_EQ(model.ParseInputsAndOutputs(), SUCCESS);

  StreamResource *stream_resource = new StreamResource(1);
  std::mutex *stream_mutex = nullptr;
  rtStream_t stream;
  SingleOp single_op(stream_resource, stream_mutex, stream);
  model.input_offset_list_ = std::vector<ptrdiff_t>({0, 0});
  model.output_offset_list_ = std::vector<ptrdiff_t>({0});

  model.input_sizes_ = std::vector<size_t>({16, 16});
  model.output_sizes_ = std::vector<size_t>({16});

  ASSERT_EQ(model.SetInputsAndOutputs(single_op), SUCCESS);

  context->set_kernel_type(static_cast<uint32_t>(ccKernelType::TE));
  ASSERT_NE(model.BuildTaskList(stream_resource, single_op), SUCCESS);

  context->set_kernel_type(static_cast<uint32_t>(ccKernelType::AI_CPU));
  ASSERT_NE(model.BuildTaskList(stream_resource, single_op), SUCCESS);

  context->set_kernel_type(static_cast<uint32_t>(ccKernelType::CUST_AI_CPU));
  ASSERT_NE(model.BuildTaskList(stream_resource, single_op), SUCCESS);

  context->set_kernel_type(static_cast<uint32_t>(ccKernelType::INVALID));
  ASSERT_NE(model.BuildTaskList(stream_resource, single_op), SUCCESS);

  task->set_type(RT_MODEL_TASK_KERNEL_EX);
  ASSERT_EQ(model.BuildTaskList(stream_resource, single_op), SUCCESS);

  task->set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
  ASSERT_NE(model.BuildTaskList(stream_resource, single_op), SUCCESS);

  task->set_type(RT_MODEL_TASK_EVENT_WAIT);
  ASSERT_EQ(model.BuildTaskList(stream_resource, single_op), SUCCESS);
  delete stream_resource;
}

TEST_F(UtestSingleOpModel, test_InitModelMem) {
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge::ComputeGraphPtr compute_graph = std::make_shared<ge::ComputeGraph>("default");
  ge::OpDescPtr op_desc = std::make_shared<OpDesc>("Mul", MATMUL);
  ge::NodePtr node = compute_graph->AddNode(op_desc);
  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  ge_model->SetGraph(graph);
  uint8_t *data = new uint8_t[5];
  size_t length = 5;
  Buffer buffer = BufferUtils::CreateCopyFrom(data, length);

  ge_model->SetWeight(buffer);

  std::string model_data_str = "model";
  SingleOpModel model(model_data_str, model_data_str.c_str(), model_data_str.size());

  ModelHelper model_helper;
  StreamResource resource(1);
  ASSERT_NE(model.InitModelMem(resource), SUCCESS);
  model_helper.model_ = ge_model;

  model.model_helper_ = model_helper;
  model.has_weight_ = true;

  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_ZERO_COPY_MEMORY_SIZE, 5);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, 5);

  AttrUtils::SetInt(compute_graph, "globleworkspace_status", 1);
  AttrUtils::SetInt(compute_graph, "globleworkspace_status_bytes", 0);
  ASSERT_EQ(model.InitModelMem(resource), SUCCESS);

  AttrUtils::SetInt(compute_graph, "globleworkspace_status_bytes", 1);
  EXPECT_EQ(model.InitModelMem(resource), SUCCESS);
}

TEST_F(UtestSingleOpModel, test_BuildOp) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());

  ge::OpDescPtr op_desc = std::make_shared<OpDesc>("Mul", MATMUL);
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddInputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->AddOutputDesc(GeTensorDesc(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32));
  op_desc->SetOutputOffset(std::vector<int64_t>{4});
  ge::ComputeGraphPtr compute_graph = std::make_shared<ge::ComputeGraph>("default");
  ge::NodePtr node = compute_graph->AddNode(op_desc);

  std::vector<std::string> depend_names = {"__input0"};
  (void)AttrUtils::SetListStr(op_desc, "_op_infer_depends", depend_names);
  (void)AttrUtils::SetBool(op_desc, kAttrSupportDynamicShape, true);

  GeTensorDesc tensor(GeShape(std::vector<int64_t>{4}), FORMAT_NCHW, DT_INT32);

  AttrUtils::SetInt(tensor, ATTR_NAME_PLACEMENT, 1);

  ge::OpDescPtr data_op_desc = std::make_shared<OpDesc>("data", DATA_TYPE);
  data_op_desc->AddOutputDesc(tensor);
  AttrUtils::SetInt(tensor, ATTR_NAME_PLACEMENT, 1);
  data_op_desc->AddInputDesc(tensor);
  TensorUtils::SetSize(*data_op_desc->MutableOutputDesc(0), 16);
  data_op_desc->SetOutputOffset(std::vector<int64_t>{4});
  ge::NodePtr data_node = compute_graph->AddNode(data_op_desc);


  GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), node->GetInDataAnchor(0));

  ge::OpDescPtr const_op_desc = std::make_shared<OpDesc>("const", CONSTANT);
  ge::NodePtr const_node = compute_graph->AddNode(const_op_desc);
  const_op_desc->AddOutputDesc(tensor);
  const_op_desc->AddInputDesc(tensor);
  const_op_desc->SetOutputOffset(std::vector<int64_t>{4});
  TensorUtils::SetSize(*const_op_desc->MutableOutputDesc(0), 16);

  ge::OpDescPtr netoutput_op_desc = std::make_shared<OpDesc>("netoutput", NETOUTPUT);
  netoutput_op_desc->AddOutputDesc(tensor);
  netoutput_op_desc->AddInputDesc(tensor);
  netoutput_op_desc->SetOutputOffset(std::vector<int64_t>{4});
  TensorUtils::SetSize(*netoutput_op_desc->MutableOutputDesc(0), 16);
  ge::NodePtr netoutput_node = compute_graph->AddNode(netoutput_op_desc);

  Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(graph);
  std::shared_ptr<domi::ModelTaskDef> tasks = std::make_shared<domi::ModelTaskDef>();
  domi::TaskDef *task = tasks->add_task();
  task->set_type(RT_MODEL_TASK_KERNEL);
  auto context = task->mutable_kernel()->mutable_context();
  ge_model->task_ = tasks;

  ModelHelper model_helper;
  model_helper.model_ = ge_model;

  GeRootModelPtr root_model = std::make_shared<GeRootModel>(compute_graph);
  model_helper.root_model_ = root_model;
  model.model_helper_ = model_helper;
  ASSERT_NE(model.InitModel(), SUCCESS);

  StreamResource *stream_resource = new StreamResource(1);
  std::mutex *stream_mutex = nullptr;
  rtStream_t stream;
  SingleOp single_op(stream_resource, stream_mutex, stream);
  ASSERT_NE(model.BuildOp(*stream_resource, single_op), SUCCESS);

  delete stream_resource;
}

void ParseOpModelParamsMock(ModelHelper &model_helper, SingleOpModelParam &param) {}

TEST_F(UtestSingleOpModel, test_parse_input_node) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  auto op_desc = make_shared<OpDesc>("Data", "Data");

  ASSERT_NE(model.ParseInputNode(op_desc), SUCCESS);

  vector<int64_t> shape{1, 2, 3, 4};
  vector<int64_t> offsets{16};
  GeShape ge_shape(shape);
  GeTensorDesc desc(ge_shape);
  op_desc->AddOutputDesc(desc);
  op_desc->SetOutputOffset(offsets);
  ASSERT_EQ(model.ParseInputNode(op_desc), SUCCESS);

  op_desc->AddOutputDesc(desc);
  offsets.push_back(32);
  op_desc->SetOutputOffset(offsets);
  ASSERT_NE(model.ParseInputNode(op_desc), SUCCESS);
}

TEST_F(UtestSingleOpModel, test_parse_output_node) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  auto op_desc = make_shared<OpDesc>("NetOutput", "NetOutput");

  vector<int64_t> shape{1, 2, 3, 4};
  vector<int64_t> offsets{16};

  GeShape ge_shape(shape);
  GeTensorDesc desc(ge_shape);
  op_desc->AddInputDesc(desc);
  op_desc->SetInputOffset(offsets);
  op_desc->AddOutputDesc(desc);
  op_desc->SetOutputOffset(offsets);

  ASSERT_NO_THROW(model.ParseOutputNode(op_desc));
  ASSERT_NO_THROW(model.ParseOutputNode(op_desc));
}

TEST_F(UtestSingleOpModel, test_set_inputs_and_outputs) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  model.input_offset_list_.push_back(0);
  model.input_sizes_.push_back(16);

  model.output_offset_list_.push_back(0);
  model.output_sizes_.push_back(16);

  std::mutex stream_mu_;
  rtStream_t stream_ = nullptr;
//  SingleOp single_op(&stream_mu_, stream_);
//
//  ASSERT_EQ(model.SetInputsAndOutputs(single_op), SUCCESS);
}
/*
TEST_F(UtestSingleOpModel, test_build_kernel_task) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  model.input_offset_list_.push_back(0);
  model.input_sizes_.push_back(16);

  model.output_offset_list_.push_back(0);
  model.output_sizes_.push_back(16);

  auto graph = make_shared<ComputeGraph>("graph");
  auto op_desc = make_shared<OpDesc>("AddN", "AddN");
  vector<int64_t> shape{16, 16};
  GeShape ge_shape(shape);
  GeTensorDesc desc(ge_shape);
  op_desc->AddInputDesc(desc);
  op_desc->AddOutputDesc(desc);
  auto node = graph->AddNode(op_desc);
  std::mutex stream_mu_;
  rtStream_t stream_ = nullptr;
  SingleOp single_op(&stream_mu_, stream_);

  domi::KernelDef kernel_def;
  kernel_def.mutable_context()->set_kernel_type(cce::ccKernelType::TE);
  TbeOpTask *task = nullptr;
  ASSERT_EQ(model.BuildKernelTask(kernel_def, &task), UNSUPPORTED);

  kernel_def.mutable_context()->set_kernel_type(cce::ccKernelType::TE);
  ASSERT_EQ(model.BuildKernelTask(kernel_def, &task), INTERNAL_ERROR);

  model.op_list_[0] = node;

  ASSERT_EQ(model.BuildKernelTask(kernel_def, &task), PARAM_INVALID);
  ASSERT_EQ(task, nullptr);
  delete task;
}

*/
/*
TEST_F(UtestSingleOpModel, test_parse_arg_table) {
  string model_data_str = "123456789";
  SingleOpModel op_model("model", model_data_str.c_str(), model_data_str.size());

  TbeOpTask task;
  OpDescPtr op_desc;
  std::mutex stream_mu_;
  rtStream_t stream_ = nullptr;
  SingleOp op(&stream_mu_, stream_);
  op.arg_table_.resize(2);

  auto args = std::unique_ptr<uint8_t[]>(new uint8_t[sizeof(uintptr_t) * 2]);
  auto *arg_base = (uintptr_t*)args.get();
  arg_base[0] = 0x100000;
  arg_base[1] = 0x200000;
  task.SetKernelArgs(std::move(args), 16, 1, op_desc);

  op_model.model_params_.addr_mapping_[0x100000] = 1;
  op_model.ParseArgTable(&task, op);

  ASSERT_EQ(op.arg_table_[0].size(), 0);
  ASSERT_EQ(op.arg_table_[1].size(), 1);
  ASSERT_EQ(op.arg_table_[1].front(), &arg_base[0]);
}
*/
TEST_F(UtestSingleOpModel, test_op_task_get_profiler_args) {
  string name = "relu";
  string type = "relu";
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  op_desc->SetId(0);
  TbeOpTask task;
  task.op_desc_ = op_desc;
  task.model_name_ = "resnet_50";
  TaskDescInfo task_desc_info;
  task.GetProfilingArgs(task_desc_info);

  ASSERT_EQ(task_desc_info.model_name, "resnet_50");
}

TEST_F(UtestSingleOpModel, test_build_dynamic_op) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  model.netoutput_op_ = make_shared<OpDesc>("NetOutput", "NetOutput");
  model.model_helper_.model_ = ge::MakeShared<ge::GeModel>();

  // make graph
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data = builder.AddNode("Data", "Data", 1, 1);
  auto transdata = builder.AddNode("Transdata", "Transdata", 1, 1);
  auto netoutput = builder.AddNode("Netoutput", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, transdata, 0);
  builder.AddDataEdge(transdata, 0, netoutput, 0);
  auto compute_graph = builder.GetGraph();

  auto graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  model.model_helper_.model_->SetGraph(graph);
  model.op_list_[0] = transdata;

  auto op_desc = transdata->GetOpDesc();
  const vector<string> depend_names = { "Data" };
  op_desc->SetOpInferDepends(depend_names);
  (void)AttrUtils::SetBool(op_desc, kAttrSupportDynamicShape, true);

  // set task_def
  auto model_task_def = make_shared<domi::ModelTaskDef>();
  domi::TaskDef *task_def = model_task_def->add_task();
  task_def->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def = task_def->mutable_kernel();
  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_kernel_type(2);    // ccKernelType::TE
  model.model_helper_.model_->SetModelTaskDef(model_task_def);

  std::mutex stream_mu_;
  DynamicSingleOp dynamic_single_op(nullptr, 0, &stream_mu_, nullptr);

  GeRootModelPtr root_model = std::make_shared<GeRootModel>(compute_graph);
  dynamic_single_op.hybrid_model_ = std::unique_ptr<hybrid::HybridModel>(new hybrid::HybridModel(root_model));

  StreamResource res((uintptr_t)1);
  model.BuildDynamicOp(res, dynamic_single_op);

  op_desc->impl_->input_name_idx_["Data"] = 0;
  model.BuildDynamicOp(res, dynamic_single_op);

  auto tensor = std::make_shared<GeTensor>();
  auto data_desc = data->GetOpDesc();
  auto tensor_desc = data_desc->MutableInputDesc(0);
  AttrUtils::SetTensor(tensor_desc, "_value", tensor);
  model.BuildDynamicOp(res, dynamic_single_op);
}

TEST_F(UtestSingleOpModel, test_host_mem) {
  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());

  // make graph
  ut::GraphBuilder builder = ut::GraphBuilder("graph");
  auto data = builder.AddNode("Data", "Data", 0, 1);
  auto netoutput = builder.AddNode("Netoutput", "NetOutput", 1, 0);
  builder.AddDataEdge(data, 0, netoutput, 0);
  auto graph = builder.GetGraph();
  model.op_with_hostmem_[0] = data;

  std::mutex stream_mu_;
  DynamicSingleOp single_op(nullptr, 0, &stream_mu_, nullptr);
  model.SetHostMemTensorAndNode(single_op);
  ASSERT_EQ(single_op.hostmem_node_id_map_[0U], data->GetOpDesc()->GetId());
}

TEST_F(UtestSingleOpModel, BuildTaskList) {
  ComputeGraphPtr graph = make_shared<ComputeGraph>("single_op");
  GeModelPtr ge_model = make_shared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  shared_ptr<domi::ModelTaskDef> model_task_def = make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);
  NodePtr node = nullptr;
  {
    auto op_desc = std::make_shared<ge::OpDesc>("memcpy", MEMCPYASYNC);
    GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({0});
    op_desc->SetOutputOffset({0});
    node = graph->AddNode(op_desc);

    domi::TaskDef *task_def = model_task_def->add_task();
    task_def->set_stream_id(0);
    task_def->set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
    domi::MemcpyAsyncDef *memcpy_async = task_def->mutable_memcpy_async();
    memcpy_async->set_src(0);
    memcpy_async->set_dst(0);
    memcpy_async->set_dst_max(512);
    memcpy_async->set_count(1);
    memcpy_async->set_kind(RT_MEMCPY_DEVICE_TO_DEVICE);
    memcpy_async->set_op_index(0);
  }

  string model_data_str = "123456789";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  StreamResource *res = new (std::nothrow) StreamResource(1);
  std::mutex stream_mu;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  SingleOp single_op(res, &stream_mu, stream);
  model.model_helper_.model_ = ge_model;
  model.op_list_.emplace(0, node);
  ASSERT_EQ(model.BuildTaskList(res, single_op), SUCCESS);
  MemcpyAsyncTask mem_task;
  for (auto &task: single_op.tasks_) {
    ASSERT_EQ(task->LaunchKernel(single_op.stream_), SUCCESS);
  }
}

TEST_F(UtestSingleOpModel, build_dynamic_task01) {
  ComputeGraphPtr graph = make_shared<ComputeGraph>("single_op");
  GeModelPtr ge_model = make_shared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  shared_ptr<domi::ModelTaskDef> model_task_def = make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);

  domi::TaskDef *task_def = model_task_def->add_task();
  task_def->set_type(RT_MODEL_TASK_KERNEL_EX);

  domi::TaskDef *task_def2 = model_task_def->add_task();
  task_def2->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def = task_def2->mutable_kernel();
  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_kernel_type(6);    // ccKernelType::AI_CPU

  domi::TaskDef *task_def3 = model_task_def->add_task();
  task_def3->set_type(RT_MODEL_TASK_ALL_KERNEL);
  auto kernel_def3 = task_def3->mutable_kernel_with_handle();
  auto context3 = kernel_def3->mutable_context();
  context3->set_kernel_type(2);    // ccKernelType::TE

  domi::TaskDef *task_def4 = model_task_def->add_task();
  task_def4->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def4 = task_def4->mutable_kernel();
  domi::KernelContext *context4 = kernel_def4->mutable_context();
  context4->set_kernel_type(2);    // ccKernelType::TE

  domi::TaskDef *task_def5 = model_task_def->add_task();
  task_def5->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def5 = task_def5->mutable_kernel();
  domi::KernelContext *context5 = kernel_def5->mutable_context();
  context5->set_op_index(0);
  context5->set_kernel_type(2);    // ccKernelType::TE

  string model_data_str = "dynamic_model";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  std::mutex stream_mu;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  DynamicSingleOp single_op(nullptr, 0, &stream_mu, stream);
  model.model_helper_.model_ = ge_model;
  auto op_desc = std::make_shared<ge::OpDesc>("add", "Add");
  AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
  std::vector<char> kernelBin;
  TBEKernelPtr tbe_kernel = std::make_shared<ge::OpKernelBin>("name/Add", std::move(kernelBin));
  op_desc->SetExtAttr(ge::OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel);
  NodePtr node = graph->AddNode(op_desc);
  model.op_list_[0] = node;
  StreamResource *res = new (std::nothrow) StreamResource(1);

  ASSERT_EQ(model.ParseTasks(), SUCCESS);
  model.node_tasks_[node] = { *task_def3, *task_def4 };
  op_desc->SetOpKernelLibName(kEngineNameAiCore);

  AttrUtils::SetInt(op_desc, "op_para_size", 64);
  AttrUtils::SetBool(op_desc, kAttrSupportDynamicShape, true);
  model.BuildTaskListForDynamicOp(res, single_op);

  model.node_tasks_[node] = { *task_def };
  op_desc->SetOpKernelLibName(kEngineNameAiCpuTf);
  ASSERT_EQ(model.BuildTaskListForDynamicOp(res, single_op), SUCCESS);

  model.node_tasks_[node] = { *task_def2 };
  op_desc->SetOpKernelLibName(kEngineNameAiCpu);
  ASSERT_NE(model.BuildTaskListForDynamicOp(res, single_op), SUCCESS);

  AtomicAddrCleanOpTask *atomic_task = nullptr;
  ASSERT_NE(model.BuildAtomicTask(*task_def5, &atomic_task), SUCCESS);
}

TEST_F(UtestSingleOpModel, build_dynamic_task02) {
  ComputeGraphPtr graph = make_shared<ComputeGraph>("single_op");
  GeModelPtr ge_model = make_shared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  shared_ptr<domi::ModelTaskDef> model_task_def = make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);

  AicpuTaskStruct args;
  args.head.length = sizeof(args);
  args.head.ioAddrNum = 6;
  domi::TaskDef *task_def = model_task_def->add_task();
  task_def->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def = task_def->mutable_kernel();
  kernel_def->set_args(reinterpret_cast<const char *>(&args), args.head.length);
  kernel_def->set_args_size(args.head.length);

  char ext_mem[sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t)]{};
  ge::hybrid::AicpuExtInfo &aicpu_ext_info = *(ge::hybrid::AicpuExtInfo *)(ext_mem);
  aicpu_ext_info.infoType = aicpu::FWKAdapter::FWK_ADPT_EXT_SHAPE_TYPE;
  aicpu_ext_info.infoLen = sizeof(int32_t);
  int32_t type = ge::DEPEND_COMPUTE;
  memcpy_s(aicpu_ext_info.infoMsg, sizeof(int32_t), &type, sizeof(int32_t));

  kernel_def->set_kernel_ext_info(ext_mem, sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t));
  kernel_def->set_kernel_ext_info_size(sizeof(ge::hybrid::AicpuExtInfo) + sizeof(int32_t));
  domi::KernelContext *context = kernel_def->mutable_context();
  context->set_kernel_type(6);    // ccKernelType::AI_CPU

  string model_data_str = "dynamic_model";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());
  std::mutex stream_mu;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  DynamicSingleOp single_op(nullptr, 0, &stream_mu, stream);
  model.model_helper_.model_ = ge_model;
  auto op_desc = std::make_shared<ge::OpDesc>("add", "Add");
  AttrUtils::SetInt(op_desc, ::ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, ge::DEPEND_COMPUTE);
  NodePtr node = graph->AddNode(op_desc);
  model.op_list_[0] = node;
  StreamResource *res = new (std::nothrow) StreamResource(1);

  ASSERT_EQ(model.ParseTasks(), SUCCESS);
  model.node_tasks_[node] = { *task_def, *task_def };
  op_desc->SetOpKernelLibName(kEngineNameAiCpu);
  model.BuildTaskListForDynamicOp(res, single_op);
  model.node_tasks_[node] = { *task_def};
  model.BuildTaskListForDynamicOp(res, single_op);
}

TEST_F(UtestSingleOpModel, build_memcpoy_task) {
  AicpuTaskStruct args;
  args.head.length = sizeof(args);
  args.head.ioAddrNum = 6;
  domi::KernelDef kernel_def;
  kernel_def.set_args(reinterpret_cast<const char *>(&args), args.head.length);
  kernel_def.set_args_size(args.head.length);
  AiCpuCCTask aicpu_task;
  ASSERT_EQ(aicpu_task.SetMemCopyTask(kernel_def), INTERNAL_ERROR);
  kernel_def.set_args_size(0);
  ASSERT_EQ(aicpu_task.SetMemCopyTask(kernel_def), FAILED);
  char* args2 = "123";
  kernel_def.set_args(reinterpret_cast<const char *>(&args2), 3);
  kernel_def.set_args_size(3);
  ASSERT_EQ(aicpu_task.SetMemCopyTask(kernel_def), FAILED);
}


TEST_F(UtestSingleOpModel, load_atomic_workspace) {

  string model_data_str = "dynamic_model";
  SingleOpModel model("model", model_data_str.c_str(), model_data_str.size());

  OpDescPtr op_desc = std::make_shared<OpDesc>("workspace", DATA);
  GeAttrValue::NAMED_ATTRS workspaces;

  GeAttrValue::NamedAttrs workspaces_attrs;
  vector<int> dimTypeList;
  dimTypeList.push_back(1);
  dimTypeList.push_back(2);
  dimTypeList.push_back(3);
  AttrUtils::SetListInt(workspaces_attrs,op_desc->GetName(), dimTypeList);
  AttrUtils::SetNamedAttrs(op_desc, EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspaces_attrs);

  map<string, map<int64_t, int64_t>> workspace_info;
  map<int64_t, int64_t> workspace_info_pair;
  workspace_info_pair.insert(std::make_pair(1, 1));
  workspace_info.insert(std::make_pair("1", workspace_info_pair));
  op_desc->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO,workspace_info);
  EXPECT_EQ(model.LoadAtomicWorkspace(op_desc),ge::SUCCESS);
}