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

#include <condition_variable>
#include <gtest/gtest.h>
#include "external/ge/ge_api.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "exec_runtime/execution_runtime.h"
#include "runtime/deploy/local_model_deployer.h"

using namespace std;
using namespace ge;

class LocalQueueTest : public testing::Test {
  void SetUp() {}
  void TearDown() {}
};

class ExchangeServiceMock : public ExchangeService {
 public:
  Status CreateQueue(int32_t device_id,
                     const string &name,
                     uint32_t depth,
                     uint32_t work_mode,
                     uint32_t &queue_id) override {
    return 0;
  }
  Status Enqueue(int32_t device_id, uint32_t queue_id, size_t size, const FillFunc &fill_func) override {
    return 0;
  }
  Status DestroyQueue(int32_t device_id, uint32_t queue_id) override {
    return 0;
  }
  Status Enqueue(int32_t device_id, uint32_t queue_id, const void *data, size_t size) override {
    return 0;
  }
  Status Peek(int32_t device_id, uint32_t queue_id, size_t &size) override {
    return 0;
  }
  Status Dequeue(int32_t device_id, uint32_t queue_id, void *data, size_t size, ControlInfo &control_info) override {
    return 0;
  }
  Status DequeueTensor(const int32_t device_id, const uint32_t queue_id, GeTensor &tensor,
                       ControlInfo &control_info) override {
    return 0;
  }
};

class ExecutionRuntimeMock : public ExecutionRuntime {
 public:
  Status Initialize(const map<std::string, std::string> &options) override {
    return 0;
  }
  Status Finalize() override {
    return 0;
  }
  ModelDeployer &GetModelDeployer() override {
    return model_deployer_;
  }
  ExchangeService &GetExchangeService() override {
    return exchange_service_;
  }
 private:
  ExchangeServiceMock exchange_service_;
  LocalModelDeployer model_deployer_;
};

TEST_F(LocalQueueTest, TestLocalQueueWithLocalEnv) {
  std::vector<std::string> engine_list = {"AIcoreEngine"};
  auto add_1 = OP_CFG(ADD).Attr(ATTR_NAME_OUTPUT_PIPELINE, "pipeline-1");
  auto add_2 = OP_CFG(ADD);
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto data3 = OP_CFG(DATA);
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add_1)->EDGE(0, 0)->NODE("add_2", add_2));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add_1));
    CHAIN(NODE("data_3", data3)->EDGE(0, 1)->NODE("add_2", add_2));
    CHAIN(NODE("add_2", add_2)->EDGE(0, 0)->NODE("Node_Output", NETOUTPUT));
  };

  auto graph = ToGeGraph(g1);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);

  ExecutionRuntime::SetExecutionRuntime(std::make_shared<ExecutionRuntimeMock>());
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  ASSERT_EQ(ret, SUCCESS);

  Shape shape({1, 1, 224, 224});
  TensorDesc tensor_desc(shape, FORMAT_NCHW, DT_FLOAT);
  std::vector<Tensor> input_tensors;
  for (int i = 0; i < 3; ++i) {
    std::vector<uint8_t> data(224 * 224 * sizeof(float));
    Tensor tensor(tensor_desc, data);
    input_tensors.emplace_back(tensor);
  }

  std::vector<Tensor> output_tensors;
  ret = session.RunGraph(1, input_tensors, output_tensors);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(output_tensors.size(), 1);

  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  auto callback = [&] (Status status, std::vector<ge::Tensor> &outputs) {
    std::unique_lock<std::mutex> lk(mu);
    done = true;
    ret = status;
    cv.notify_all();
  };
  session.RunGraphAsync(1, input_tensors, callback);
  std::unique_lock<std::mutex> lk(mu);
  cv.wait_for(lk, std::chrono::seconds(5), [&]() {
    return done;
  });
  ASSERT_EQ(ret, SUCCESS);
}

