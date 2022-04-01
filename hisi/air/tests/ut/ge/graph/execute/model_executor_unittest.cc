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
#include <atomic>
#include <mutex>
#include <future>

#define protected public
#define private public
#include "graph/load/graph_loader.h"
#include "graph/execute/model_executor.h"
#include "graph/manager/graph_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/load/model_manager/model_manager.h"

using namespace std;

namespace ge {
class UtestModelExecutorTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {
    EXPECT_TRUE(ModelManager::GetInstance().model_map_.empty());
    EXPECT_TRUE(ModelManager::GetInstance().hybrid_model_map_.empty());
  }
};

TEST_F(UtestModelExecutorTest, test_get_total_memory_size) {
  ModelExecutor model_executor;
  size_t total_mem_size = 0;
  EXPECT_EQ(model_executor.GetTotalMemorySize(total_mem_size), SUCCESS);
  EXPECT_EQ(total_mem_size, 128UL * 1024UL * 1024UL);
}

TEST_F(UtestModelExecutorTest, test_load_graph_sync) {
  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(compute_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(false);

  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);

  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(UtestModelExecutorTest, test_load_graph_async) {
  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  Graph graph("test_graph");
  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(compute_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 1001;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);

  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(UtestModelExecutorTest, test_load_graph_failed) {
  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  Graph graph("test_graph");
  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");

  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);
  graph_node->SetAsync(true);

  // GeModel is null, DavinciModel::Assign will return FAILED
  setenv(kEnvGeuseStaticMemory, "1", true);
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), PARAM_INVALID);  // GeModel is null
  EXPECT_EQ(model_executor.UnloadGraph(flow_root_model, graph_id), SUCCESS);

  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  unsetenv(kEnvGeuseStaticMemory);
}

TEST_F(UtestModelExecutorTest, test_check_and_release_memory) {
  {
    auto listener = MakeShared<RunAsyncListener>();
    shared_ptr<DavinciModel> davinci_model1 = MakeShared<DavinciModel>(1, listener);
    davinci_model1->SetId(1);
    ModelManager::GetInstance().InsertModel(1, davinci_model1);
    shared_ptr<DavinciModel> davinci_model2 = MakeShared<DavinciModel>(2, listener);
    davinci_model1->SetId(2);
    ModelManager::GetInstance().InsertModel(2, davinci_model2);
  }

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  int64_t memory_size = 25 * 1024UL * 1024UL * 1024UL;
  int64_t weight_size = 25 * 1024UL * 1024UL * 1024UL;
  uint64_t session_id = 0;
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, memory_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_SESSION_ID, session_id));

  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  model_executor.AddGraphNode(graph_id, graph_node);

  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  ge_root_model->SetModelId(1);
  ge_root_model->SetModelId(2);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);

  EXPECT_EQ(model_executor.CheckAndReleaseMemory(ge_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);

  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(2U), SUCCESS);
}

TEST_F(UtestModelExecutorTest, test_check_and_release_memory_extend_size_static_memory) {
  {
    auto listener = MakeShared<RunAsyncListener>();
    shared_ptr<DavinciModel> davinci_model1 = MakeShared<DavinciModel>(1, listener);
    davinci_model1->SetId(1);
    ModelManager::GetInstance().InsertModel(1, davinci_model1);
    shared_ptr<DavinciModel> davinci_model2 = MakeShared<DavinciModel>(2, listener);
    davinci_model1->SetId(2);
    ModelManager::GetInstance().InsertModel(2, davinci_model2);
  }

  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  int64_t memory_size = 2 * 1024UL;
  int64_t weight_size = 2 * 1024UL;
  int64_t zero_copy_memory_size = 1 * 1024UL;
  uint64_t session_id = 0;
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, memory_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_WEIGHT_SIZE, weight_size));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, MODEL_ATTR_SESSION_ID, session_id));
  EXPECT_TRUE(AttrUtils::SetInt(ge_model, ATTR_MODEL_ZERO_COPY_MEMORY_SIZE, zero_copy_memory_size));

  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<GraphNode>(graph_id);
  model_executor.AddGraphNode(graph_id, graph_node);

  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("test_graph");
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  ge_root_model->SetModelId(1);
  ge_root_model->SetModelId(2);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(true);

  // set extend size static memory option
  std::map<std::string, std::string> graph_options;
  graph_options[kOptionExecGeUseStaticMemory] = "2";
  graph_options[OPTION_EXEC_REUSE_ZERO_COPY_MEMORY] = "1";
  GetThreadLocalContext().SetGraphOption(graph_options);

  int64_t sum_size = 0;
  bool is_resuse = false;
  // static memory is not malloced, can not reuse static memory
  EXPECT_EQ(model_executor.GetMemorySizeAfterReuse(ge_model, graph_node, sum_size, is_resuse), SUCCESS);
  EXPECT_EQ(sum_size, memory_size - zero_copy_memory_size + weight_size);
  EXPECT_FALSE(is_resuse);

  // malloc static memory, reuse static memroy
  auto &mem_instance = SessionMemAllocator::Instance().GetMemAllocator(0);
  (void)mem_instance.MallocMemory("", kFeatureMemoryKey, memory_size, 0);
  EXPECT_EQ(model_executor.GetMemorySizeAfterReuse(ge_model, graph_node, sum_size, is_resuse), SUCCESS);
  EXPECT_EQ(sum_size, weight_size);
  EXPECT_TRUE(is_resuse);

  EXPECT_EQ(model_executor.CheckAndReleaseMemory(ge_model, graph_node), SUCCESS);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);

  (void)mem_instance.FreeMemory(kFeatureMemoryKey, 0);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(2U), SUCCESS);
  graph_options[kOptionExecGeUseStaticMemory] = "";
  graph_options[OPTION_EXEC_REUSE_ZERO_COPY_MEMORY] = "";
  GetThreadLocalContext().SetGraphOption(graph_options);
}

TEST_F(UtestModelExecutorTest, test_run_thread) {
  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  GraphId graph_id = 1;
  uint64_t session_id = 0;
  error_message::Context error_context;
  GEThreadLocalContext context;
  const auto callback = [](Status status, std::vector<ge::Tensor> &outputs) { };

  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(compute_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(false);
  graph_node->SetAsync(true);
  graph_node->IncreaseLoadCount();
  graph_node->Lock();

  Tensor tensor;
  std::vector<Tensor> input_tensors;
  input_tensors.emplace_back(tensor);

  RunArgs run_args{graph_node, graph_id, session_id, error_context, input_tensors, flow_root_model, context, callback};
  EXPECT_EQ(model_executor.PushRunArgs(run_args), SUCCESS);

  while (model_executor.run_args_q_.Size() > 0) {
    usleep(10);  // 0.01ms, Wait for RunThread.
  }
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(ge_root_model->GetModelId()), SUCCESS);
}

static void test_run_graph(ModelExecutor &model_executor) {
  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");
  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(compute_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphId graph_id = 1;
  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(false);
  graph_node->SetAsync(false);  // RunGraph is Synchronization.
  EXPECT_EQ(model_executor.LoadGraph(flow_root_model, graph_node), SUCCESS);

  std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  EXPECT_EQ(model_executor.RunGraph(graph_node, graph_id, inputs, outputs), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(ge_root_model->GetModelId()), SUCCESS);
}

TEST_F(UtestModelExecutorTest, test_run_graph_train) {
  GetThreadLocalContext().SetGlobalOption({{OPTION_GRAPH_RUN_MODE, "1"}});
  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  test_run_graph(model_executor);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(UtestModelExecutorTest, test_run_graph_infer) {
  GetThreadLocalContext().SetGlobalOption({});
  GetThreadLocalContext().SetSessionOption({});
  GetThreadLocalContext().SetGraphOption({});
  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);
  test_run_graph(model_executor);
  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
}

TEST_F(UtestModelExecutorTest, test_run_graph_with_stream) {
  ModelExecutor model_executor;
  EXPECT_EQ(model_executor.Initialize({}, 0), SUCCESS);

  GraphId graph_id = 1;
  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  auto flow_root_compute_graph = MakeShared<ComputeGraph>("test_graph");
  flow_root_compute_graph->SetAllSubgraphs({compute_graph});
  auto flow_root_model = MakeShared<FlowModel>(flow_root_compute_graph);
  flow_root_model->AddSubModel(ge_root_model, "pne_npu");

  GeModelPtr ge_model = MakeShared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(compute_graph));
  ge_root_model->SetSubgraphInstanceNameToModel(compute_graph->GetName(), ge_model);

  GraphNodePtr graph_node = MakeShared<ge::GraphNode>(graph_id);
  graph_node->SetFlowModel(flow_root_model);
  graph_node->SetLoadFlag(false);
  graph_node->SetAsync(true);

  GeTensor tensor;
  std::vector<GeTensor> inputs{tensor};
  std::vector<GeTensor> outputs;

  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  EXPECT_EQ(model_executor.RunGraphWithStream(graph_node, graph_id, stream, inputs, outputs), 145003);

  EXPECT_EQ(model_executor.Finalize(), SUCCESS);
  rtStreamDestroy(stream);
}

TEST_F(UtestModelExecutorTest, success_multi_load_model) {

  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  vector<NamedAttrs> deploy_info;
  NamedAttrs thread_instance;
  (void) thread_instance.SetAttr("_need_return_result", GeAttrValue::CreateFrom<bool>(true));
  (void) thread_instance.SetAttr("_device_id", GeAttrValue::CreateFrom<int64_t>(0));
  deploy_info.push_back(thread_instance);
  NamedAttrs thread_instance2;
  (void) thread_instance2.SetAttr("_need_return_result", GeAttrValue::CreateFrom<bool>(false));
  (void) thread_instance2.SetAttr("_device_id", GeAttrValue::CreateFrom<int64_t>(0));
  deploy_info.push_back(thread_instance2);
  (void) ge::AttrUtils::SetListNamedAttrs(*compute_graph, ATTR_NAME_DEPLOY_INFO, deploy_info);
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  GraphNodePtr graph_node = MakeShared<GraphNode>(0);

  ModelExecutor model_executor;
  auto ret = SUCCESS;
  std::future<Status> f_ = std::async(std::launch::async, [&ret, ge_root_model, graph_node, deploy_info]() -> Status {
    ret = GraphLoader::MultiLoadModelOnline(ge_root_model, graph_node, deploy_info);
    return SUCCESS;
  });
  std::chrono::milliseconds span(10);
  while (f_.wait_for(span) != std::future_status::ready) {
    std::cout << ".";
  }
  EXPECT_EQ(ret, PARAM_INVALID);  // GeModel is null
}

TEST_F(UtestModelExecutorTest, success_multi_load_model_multi_mode) {

  auto compute_graph = MakeShared<ComputeGraph>("test_graph");
  vector<NamedAttrs> deploy_info;
  NamedAttrs thread_instance;
  (void) thread_instance.SetAttr("_need_return_result", GeAttrValue::CreateFrom<bool>(true));
  (void) thread_instance.SetAttr("_device_id", GeAttrValue::CreateFrom<int64_t>(0));
  (void) thread_instance.SetAttr("_device_type", GeAttrValue::CreateFrom<std::string>("MultiMode"));
  deploy_info.push_back(thread_instance);
  (void) ge::AttrUtils::SetListNamedAttrs(*compute_graph, ATTR_NAME_DEPLOY_INFO, deploy_info);
  GeRootModelPtr ge_root_model = MakeShared<GeRootModel>(compute_graph);
  GraphNodePtr graph_node = MakeShared<GraphNode>(0);

  ModelExecutor model_executor;
  auto ret = SUCCESS;
  std::future<Status> f_ = std::async(std::launch::async, [&ret, ge_root_model, graph_node, deploy_info]() -> Status {
    ret = GraphLoader::MultiLoadModelOnline(ge_root_model, graph_node, deploy_info);
    return SUCCESS;
  });
  std::chrono::milliseconds span(10);
  while (f_.wait_for(span) != std::future_status::ready) {
    std::cout << ".";
  }
  EXPECT_EQ(ret, PARAM_INVALID);  // GeModel is null
}

static bool is_err_cb_called = false;
static void err_cb_stub(Status sta, std::vector<ge::Tensor> tens) {
  is_err_cb_called = true;
}
TEST_F(UtestModelExecutorTest, ReturnError) {
  RunAsyncCallback callback = err_cb_stub;
  Status ret = 0;
  string log_info = string("err log info");

  ModelExecutor model_executor;
  model_executor.ReturnError(callback, ret, log_info);
  EXPECT_TRUE(is_err_cb_called);
}

} // namespace ge
