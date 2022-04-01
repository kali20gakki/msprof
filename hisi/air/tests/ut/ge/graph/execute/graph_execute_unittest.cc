/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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
#include <memory>

#define protected public
#define private public
#include "common/profiling/profiling_manager.h"
#include "graph/execute/graph_execute.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/compute_graph_impl.h"

using namespace std;
using namespace testing;
using namespace domi;

namespace ge {

class UtestGraphExecuteTest : public testing::Test {
 protected:
  void SetUp() {
    const auto davinci_model = MakeShared<DavinciModel>(2001U, MakeShared<RunAsyncListener>());
    ModelManager::GetInstance().InsertModel(2001U, davinci_model);
  }

  void TearDown() {
    EXPECT_EQ(ModelManager::GetInstance().DeleteModel(2001U), SUCCESS);
    EXPECT_TRUE(ModelManager::GetInstance().model_map_.empty());
    EXPECT_TRUE(ModelManager::GetInstance().hybrid_model_map_.empty());
  }
};

TEST_F(UtestGraphExecuteTest, get_execute_model_id_invalid) {
  GraphExecutor executor;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  auto model_id = executor.GetExecuteModelId(ge_root_model);
  EXPECT_EQ(model_id, kInvalidModelId);
}

TEST_F(UtestGraphExecuteTest, get_execute_model_id_1) {
  GraphExecutor executor;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  shared_ptr<DavinciModel> davinci_model1 = MakeShared<DavinciModel>(1, nullptr);
  davinci_model1->SetId(1);
  ModelManager::GetInstance().InsertModel(1, davinci_model1);
  ge_root_model->SetModelId(1);
  auto model_id = executor.GetExecuteModelId(ge_root_model);
  EXPECT_EQ(model_id, 1);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
}

TEST_F(UtestGraphExecuteTest, get_execute_model_id_2) {
  GraphExecutor executor;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  // model1 with 2 load
  shared_ptr<DavinciModel> davinci_model1 = MakeShared<DavinciModel>(1, nullptr);
  davinci_model1->SetId(1);

  InputData input_data;
  OutputData output_data;
  auto data = MakeShared<InputDataWrapper>(input_data, output_data);
  davinci_model1->data_inputer_.Push(data);
  davinci_model1->data_inputer_.Push(data);
  ModelManager::GetInstance().InsertModel(1, davinci_model1);
  // model 2 with 3 load
  shared_ptr<DavinciModel> davinci_model2 = MakeShared<DavinciModel>(1, nullptr);
  davinci_model2->SetId(2);
  davinci_model2->data_inputer_.Push(data);
  davinci_model2->data_inputer_.Push(data);
  davinci_model2->data_inputer_.Push(data);
  ModelManager::GetInstance().InsertModel(2, davinci_model2);
  // model 3 witH 1 load
  shared_ptr<DavinciModel> davinci_model3 = MakeShared<DavinciModel>(1, nullptr);
  davinci_model3->SetId(3);
  davinci_model3->data_inputer_.Push(data);
  ModelManager::GetInstance().InsertModel(3, davinci_model3);

  ge_root_model->SetModelId(1);
  ge_root_model->SetModelId(2);
  ge_root_model->SetModelId(3);

  auto model_id = executor.GetExecuteModelId(ge_root_model);
  // model 3 is picked for having least loads
  EXPECT_EQ(model_id, 3);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(2U), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(3U), SUCCESS);
}

TEST_F(UtestGraphExecuteTest, test_set_callback) {
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  // is_unknown_shape_graph_ = false
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  RunAsyncCallback callback = [](Status, std::vector<ge::Tensor> &) {};

  auto listener = MakeShared<RunAsyncListener>();
  shared_ptr<DavinciModel> davinci_model1 = MakeShared<DavinciModel>(1, listener);
  davinci_model1->SetId(1);
  ModelManager::GetInstance().InsertModel(1, davinci_model1);
  auto status = ModelManager::GetInstance().SetCallback(1, ge_root_model, callback);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
}

TEST_F(UtestGraphExecuteTest, test_without_subscribe) {
  auto ret = ModelManager::GetInstance().ModelSubscribe(1);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, test_with_subscribe_failed1) {
  uint32_t graph_id = 1;
  ProfilingManager::Instance().SetSubscribeInfo(0, 1, true);
  auto ret = ModelManager::GetInstance().ModelSubscribe(graph_id);
  ProfilingManager::Instance().CleanSubscribeInfo();
  EXPECT_NE(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, test_with_subscribe_failed2) {
  uint32_t graph_id = 1;
  uint32_t model_id = 1;
  ProfilingManager::Instance().SetSubscribeInfo(0, 1, true);
  ProfilingManager::Instance().SetGraphIdToModelMap(2, model_id);
  auto ret = ModelManager::GetInstance().ModelSubscribe(graph_id);
  ProfilingManager::Instance().CleanSubscribeInfo();
  EXPECT_NE(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, test_with_subscribe_success) {
  uint32_t graph_id = 1;
  uint32_t model_id = 2001U;
  GraphNodePtr graph_node = std::make_shared<GraphNode>(graph_id);
  DavinciModel model(model_id, nullptr);
  auto &profiling_manager = ProfilingManager::Instance();
  profiling_manager.reporter_max_len_ = 1024;
  ProfilingManager::Instance().SetSubscribeInfo(0, 1, true);
  ProfilingManager::Instance().SetGraphIdToModelMap(graph_id, model_id);
  auto ret = ModelManager::GetInstance().ModelSubscribe(graph_id);
  ProfilingManager::Instance().CleanSubscribeInfo();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, SetDynamicSize) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  std::vector<uint64_t> batch_num_empty;
  std::vector<uint64_t> batch_num{1};
  auto ret = executor.SetDynamicSize(model_id, batch_num_empty, static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH));
  ret = executor.SetDynamicSize(model_id, batch_num, static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH));
  EXPECT_EQ(ret, SUCCESS);
  ret = executor.SetDynamicSize(UINT32_MAX, batch_num, static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH));
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphExecuteTest, GetInputOutputDescInfo) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  std::vector<InputOutputDescInfo> input_desc;
  std::vector<InputOutputDescInfo> output_desc;
  std::vector<uint32_t> input_formats;
  std::vector<uint32_t> out_formats;
  auto ret = executor.GetInputOutputDescInfo(model_id, input_desc, output_desc, input_formats, out_formats, true);
  ret = executor.GetInputOutputDescInfo(model_id, input_desc, output_desc, input_formats, out_formats, false);
  EXPECT_EQ(ret, GRAPH_SUCCESS);
}

TEST_F(UtestGraphExecuteTest, GetDynamicBatchInfo) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH);
  std::vector<std::vector<int64_t>> batch_info;
  auto ret = executor.GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, GetCombinedDynamicDims) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  std::vector<std::vector<int64_t>> batch_info;
  auto ret = executor.GetCombinedDynamicDims(model_id, batch_info);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, GetUserDesignateShapeOrder) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  std::vector<std::string> user_input_shape_order;
  auto ret = executor.GetUserDesignateShapeOrder(model_id, user_input_shape_order);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, GetCurrentShape) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  std::vector<int64_t> batch_info;
  int32_t dynamic_type = static_cast<int32_t>(DynamicInputType::DYNAMIC_BATCH);
  auto ret = executor.GetCurrentShape(model_id, batch_info, dynamic_type);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, GetOutputShapeInfo) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  std::vector<std::string> dynamic_output_shape_info;
  auto ret = executor.GetOutputShapeInfo(model_id, dynamic_output_shape_info);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, GetAippInfo) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  uint32_t index = 0;
  AippConfigInfo aipp_info;
  auto ret = executor.GetAippInfo(model_id, index, aipp_info);
  EXPECT_EQ(ret, ACL_ERROR_GE_AIPP_NOT_EXIST);
}

TEST_F(UtestGraphExecuteTest, GetAippType) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  uint32_t index = 0;
  InputAippType type = DATA_WITHOUT_AIPP;
  size_t aipp_index = 0;
  auto ret = executor.GetAippType(model_id, index, type, aipp_index);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestGraphExecuteTest, GetOrigInputInfo) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  uint32_t index = 0;
  OriginInputInfo orig_input_info;
  auto ret = executor.GetOrigInputInfo(model_id, index, orig_input_info);
  EXPECT_EQ(ret, ACL_ERROR_GE_AIPP_NOT_EXIST);
}

TEST_F(UtestGraphExecuteTest, GetAllAippInputOutputDims) {
  GraphExecutor executor;
  uint32_t model_id = 2001U;
  uint32_t index = 0;
  std::vector<InputOutputDims> input_dims;
  std::vector<InputOutputDims> output_dims;
  auto ret = executor.GetAllAippInputOutputDims(model_id, index, input_dims, output_dims);
  EXPECT_EQ(ret, ACL_ERROR_GE_AIPP_NOT_EXIST);
}

TEST_F(UtestGraphExecuteTest, GetOpDescInfo) {
  GraphExecutor executor;
  uint32_t device_id = 0;
  uint32_t stream_id = 0;
  uint32_t task_id = 0;
  OpDescInfo op_desc_info;
  auto ret = executor.GetOpDescInfo(device_id, stream_id, task_id, op_desc_info);
  EXPECT_EQ(ret, GRAPH_FAILED);
}

TEST_F(UtestGraphExecuteTest, MallocInOutBuffer_Invalid) {
  GraphExecutor executor;
  BufferInfo bi1;
  BufferInfo bi2;

  bi1.addr = new char_t[10];
  bi1.size = 10;
  bi2.addr = new char_t[10];
  bi2.size = 10;

  uint32_t model_id1 = 1;
  uint32_t model_id2 = 2;
  std::vector<BufferInfo> buffer_info1;
  buffer_info1.emplace_back(bi1);

  std::vector<BufferInfo> buffer_info2;
  buffer_info2.emplace_back(bi2);

  executor.buffer_cache_.insert({model_id1, buffer_info1});
  executor.buffer_cache_.insert({model_id2, buffer_info2});

  auto ret = executor.MallocInOutBuffer(model_id1, buffer_info1);
  EXPECT_EQ(ret, SUCCESS);

  ret = executor.MallocInOutBuffer(model_id1, buffer_info2);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, PrepareInputData_Invalid) {
  GraphExecutor executor;
  GeTensorDesc td(GeShape(), FORMAT_NCHW, DT_FLOAT);
  GeTensor ge_tensor(td);
  std::vector<GeTensor> input_tensor;
  input_tensor.emplace_back(ge_tensor);
  InputData graph_input_data;
  OutputData graph_output_data;
  std::vector<InputOutputDescInfo> output_desc;

  auto ret = executor.PrepareInputData(input_tensor, graph_input_data, graph_output_data, output_desc);

  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, SyncMultiExecuteModel_Invalid) {
  GraphExecutor executor;
  GeTensorDesc td(GeShape(), FORMAT_NCHW, DT_FLOAT);
  GeTensor ge_tensor(td);
  std::vector<GeTensor> input_tensor;
  input_tensor.emplace_back(ge_tensor);
  std::vector<GeTensor> output_tensor;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);

  auto ret = executor.SyncMultiExecuteModel(ge_root_model, input_tensor, output_tensor);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestGraphExecuteTest, SyncExecuteModel_Invalid) {
  GraphExecutor executor;
  GeTensorDesc td(GeShape(), FORMAT_NCHW, DT_FLOAT);
  GeTensor ge_tensor(td);
  std::vector<GeTensor> input_tensor;
  input_tensor.emplace_back(ge_tensor);
  std::vector<GeTensor> output_tensor;
  uint32_t model_id = 2001U;
  error_message::Context error_context;

  auto ret = executor.SyncExecuteModel(model_id, input_tensor, output_tensor, error_context);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, FreeExecuteMemory_Invalid) {
  GraphExecutor executor;
  auto ret = executor.FreeExecuteMemory();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphExecuteTest, ExecuteGraph_Invalid) {
  GraphExecutor executor;
  GeTensorDesc td(GeShape(), FORMAT_NCHW, DT_FLOAT);
  GeTensor ge_tensor(td);
  std::vector<GeTensor> input_tensor;
  input_tensor.emplace_back(ge_tensor);
  std::vector<GeTensor> output_tensor;

  GraphId graph_id = 0;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);

  auto ret = executor.ExecuteGraph(graph_id, ge_root_model, input_tensor, output_tensor);
  EXPECT_EQ(ret, GE_GRAPH_SYNC_MODEL_FAILED);
}

TEST_F(UtestGraphExecuteTest, ExecuteGraphAsync_Invalid) {
  GraphExecutor executor;
  TensorDesc td(Shape(), FORMAT_NCHW, DT_FLOAT);
  Tensor tensor(td);
  std::vector<Tensor> input_tensor;
  input_tensor.emplace_back(tensor);
  RunAsyncCallback callback;

  GraphId graph_id = 0;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);

  auto ret = executor.ExecuteGraphAsync(graph_id, ge_root_model, input_tensor, callback);
  EXPECT_EQ(ret, GE_GRAPH_SYNC_MODEL_FAILED);
}

TEST_F(UtestGraphExecuteTest, SetCallback_Invalid) {
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  graph->impl_->is_unknown_shape_graph_ = true;
  // is_unknown_shape_graph_ = true
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  RunAsyncCallback callback = [](Status, std::vector<ge::Tensor> &) {};

  auto listener = MakeShared<RunAsyncListener>();
  shared_ptr<DavinciModel> davinci_model1 = MakeShared<DavinciModel>(1, listener);
  davinci_model1->SetId(1);
  ModelManager::GetInstance().InsertModel(1, davinci_model1);
  auto ret = ModelManager::GetInstance().SetCallback(1, ge_root_model, callback);
  EXPECT_EQ(ret, PARAM_INVALID);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
}

TEST_F(UtestGraphExecuteTest, AsyncMultiExecuteModel_Invalid) {
  GraphExecutor executor;
  ComputeGraphPtr graph = MakeShared<ComputeGraph>("test");
  graph->impl_->is_unknown_shape_graph_ = true;
  // is_unknown_shape_graph_ = true
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(graph);
  RunAsyncCallback callback = [](Status, std::vector<ge::Tensor> &) {};

  auto listener = MakeShared<RunAsyncListener>();
  shared_ptr<DavinciModel> davinci_model1 = MakeShared<DavinciModel>(1, listener);
  davinci_model1->SetId(1);
  ModelManager::GetInstance().InsertModel(1, davinci_model1);

  TensorDesc td(Shape(), FORMAT_NCHW, DT_FLOAT);
  Tensor tensor(td);
  std::vector<Tensor> inputs;
  inputs.emplace_back(tensor);

  auto ret = executor.AsyncMultiExecuteModel(ge_root_model, inputs, callback);
  EXPECT_EQ(ret, FAILED);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
}

TEST_F(UtestGraphExecuteTest, GetModelByID_Invalid) {
  EXPECT_EQ(ModelManager::GetInstance().GetModel(UINT32_MAX), nullptr);
}

}  // namespace ge