/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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
#include <gmock/gmock.h>

#define protected public
#define private public
#include "deploy/deployer/helper_deploy_planner.h"
#include "deploy/deployer/master_model_deployer.h"
#include "executor/event_handler.h"
#undef private
#undef protected

#include "graph/build/graph_builder.h"
#include "runtime/deploy/stub_models.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "generator/ge_generator.h"
#include "ge/ge_api.h"

using namespace std;

namespace ge {
namespace {
class ModelHandleMock : public ExecutorContext::ModelHandle {
 public:
  explicit ModelHandleMock(uint64_t model_size) : ModelHandle(model_size) {}

  MOCK_METHOD3(DoLoadModel, Status(const shared_ptr<GeRootModel> &root_model,
      const vector<uint32_t> &input_queues,
      const vector<uint32_t> &output_queues));
  MOCK_CONST_METHOD1(DoUnloadModel, Status(const uint32_t));
};

class ExecutionContextMock : public ExecutorContext {
 public:
 protected:
  std::unique_ptr<ModelHandle> CreateModelHandle(uint64_t model_size) const override {
   return MakeUnique<ModelHandleMock>(model_size);
 }
};
}
class EventHandlerTest : public testing::Test {
 protected:
  void SetUp() override {
  }
  void TearDown() override {
  }

  void BuildModel(ModelBufferData &model_buffer_data) {
    //  dlog_setlevel(GE_MODULE_NAME, DLOG_DEBUG, 1);
    vector<std::string> engine_list = {"AIcoreEngine"};
    auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
    auto add2 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
    auto data1 = OP_CFG(DATA);
    auto data2 = OP_CFG(DATA);
    DEF_GRAPH(g1) {
      CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
      CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
      CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("add_2", add2));
      CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_2", add2));
    };

    auto graph = ToGeGraph(g1);
    auto compute_graph = GraphUtils::GetComputeGraph(graph);
    auto root_model = SubModels::BuildRootModel(compute_graph, false);
    EXPECT_EQ(MasterModelDeployer::SerializeModel(root_model, model_buffer_data), SUCCESS);
  }
};

TEST_F(EventHandlerTest, TestSharedVariables) {
  EventHandler handler;
  EXPECT_EQ(handler.Initialize(), SUCCESS);
  handler.context_ = MakeUnique<ExecutionContextMock>();
  deployer::ExecutorRequest request;
  deployer::ExecutorResponse response;

  auto multi_var_manager_req = request.mutable_multi_var_manager_info();
  auto var_manager_info = multi_var_manager_req->add_var_manager_info();
  var_manager_info->set_device_id(0);
  var_manager_info->set_graph_mem_max_size(1024);
  var_manager_info->set_var_mem_max_size(1024);

  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);

  deployer::ExecutorRequest request2;
  auto shared_content_desc = request2.mutable_shared_content_desc();
  shared_content_desc->set_om_content("hello world");
  shared_content_desc->set_mem_type(2);
  shared_content_desc->set_total_length(5);

  handler.HandleEvent(request2, response);
  EXPECT_EQ(response.status_code(), SUCCESS);
}

TEST_F(EventHandlerTest, TestEventHandler) {
  ModelBufferData model_buffer_data{};
  BuildModel(model_buffer_data);

  EventHandler handler;
  EXPECT_EQ(handler.Initialize(), SUCCESS);
  handler.context_ = MakeUnique<ExecutionContextMock>();
  deployer::ExecutorRequest request;

  uint32_t root_model_id = 0;
  uint32_t submodel_id = 1;
  uint32_t submodel_id_2 = 2;
  // 1. pre-download
  auto pre_download_req = request.mutable_pre_download_message();
  pre_download_req->set_root_model_id(root_model_id);
  pre_download_req->set_model_id(submodel_id);
  pre_download_req->set_model_size(model_buffer_data.length);
  deployer::ExecutorResponse response;
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);
  pre_download_req->set_model_id(submodel_id_2);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);
  request.clear_pre_download_message();

  // 1. download
  auto download_request = request.mutable_download_model_message();
  download_request->set_root_model_id(root_model_id);
  download_request->set_model_id(submodel_id);
  //  download_request->set_model_data(model_buffer_data.data.get(), model_buffer_data.length);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  download_request->set_model_data(model_buffer_data.data.get(), model_buffer_data.length);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);
  request.clear_download_model_message();

  auto &model_handler = dynamic_cast<ModelHandleMock &>(*handler.context_->models_[0][1]);
  auto load_model_request = request.mutable_load_model_message();
  load_model_request->set_root_model_id(root_model_id);
  load_model_request->set_model_id(submodel_id);
  load_model_request->add_input_queues(0);
  load_model_request->add_output_queues(1);

  EXPECT_CALL(model_handler, DoLoadModel).WillOnce(testing::Return(FAILED)).WillOnce((testing::Return(SUCCESS)));
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);
  request.clear_load_model_message();

  auto unload_model_request = request.mutable_unload_model_message();
  unload_model_request->set_model_id(root_model_id);
  EXPECT_CALL(model_handler,
              DoUnloadModel).WillOnce(testing::Return(FAILED)).WillRepeatedly((testing::Return(SUCCESS)));
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);
  request.clear_unload_model_message();
}

TEST_F(EventHandlerTest, TestInvalidModelId) {
  EventHandler handler;
  EXPECT_EQ(handler.Initialize(), SUCCESS);

  uint32_t root_model_id = 0;
  uint32_t invalid_root_model_id = 1;
  uint32_t submodel_id = 1;
  uint32_t invalid_submodel_id = 2;
  deployer::ExecutorRequest request;
  deployer::ExecutorResponse response;
  // pre-downloaded
  // 1. pre-download
  auto pre_download_req = request.mutable_pre_download_message();
  pre_download_req->set_root_model_id(root_model_id);
  pre_download_req->set_model_id(submodel_id);
  pre_download_req->set_model_size(2333);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);

  // 1. download
  auto download_request = request.mutable_download_model_message();
  download_request->set_root_model_id(invalid_root_model_id);
  download_request->set_model_id(invalid_submodel_id);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  download_request->set_root_model_id(root_model_id);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  request.clear_download_model_message();

  auto load_model_request = request.mutable_load_model_message();
  load_model_request->set_root_model_id(root_model_id);
  load_model_request->set_model_id(invalid_submodel_id);
  load_model_request->add_input_queues(0);
  load_model_request->add_output_queues(1);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  request.clear_load_model_message();

  auto unload_model_request = request.mutable_unload_model_message();
  unload_model_request->set_model_id(invalid_root_model_id);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  request.clear_unload_model_message();
}

TEST_F(EventHandlerTest, TestInvalidRequest) {
  EventHandler handler;
  EXPECT_EQ(handler.Initialize(), SUCCESS);
  deployer::ExecutorRequest request;
  deployer::ExecutorResponse response;
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), UNSUPPORTED);
}

TEST_F(EventHandlerTest, TestRepeatAdd) {
  EventHandler handler;
  EXPECT_EQ(handler.Initialize(), SUCCESS);
  handler.context_ = MakeUnique<ExecutionContextMock>();
  deployer::ExecutorRequest request;
  deployer::ExecutorResponse response;

  uint32_t root_model_id = 0;
  uint32_t submodel_id = 1;
  // 1. pre-download
  auto pre_download_req = request.mutable_pre_download_message();
  pre_download_req->set_root_model_id(root_model_id);
  pre_download_req->set_model_id(submodel_id);
  pre_download_req->set_model_size(2333);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), SUCCESS);
  handler.HandleEvent(request, response);
  EXPECT_EQ(response.status_code(), FAILED);
  request.clear_pre_download_message();
}
}  // namespace ge