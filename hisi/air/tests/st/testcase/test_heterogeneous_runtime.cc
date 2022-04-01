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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <condition_variable>
#include <memory>
#include <gtest/gtest.h>
#include "external/ge/ge_api.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "exec_runtime/execution_runtime.h"
#include "depends/mmpa/src/mmpa_stub.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "executor/runtime/local_execution_runtime.h"

using namespace testing;
using namespace std;
using namespace ge;
rtError_t rtMemQueueDeQueueBuff(int32_t device, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout) {
  RuntimeTensorDesc mbuf_tensor_desc;
  mbuf_tensor_desc.shape[0] = 4;
  mbuf_tensor_desc.shape[1] = 1;
  mbuf_tensor_desc.shape[2] = 1;
  mbuf_tensor_desc.shape[3] = 224;
  mbuf_tensor_desc.shape[4] = 224;
  mbuf_tensor_desc.dtype = static_cast<int64_t>(DT_INT64);
  mbuf_tensor_desc.data_addr = static_cast<int64_t>(reinterpret_cast<intptr_t>(outBuf->buffInfo->addr));
  if (memcpy_s(outBuf->buffInfo->addr, sizeof(RuntimeTensorDesc), &mbuf_tensor_desc, sizeof(RuntimeTensorDesc)) !=
      EOK) {
    printf("Failed to copy mbuf data, dst size:%zu, src size:%zu\n", outBuf->buffInfo->len, sizeof(RuntimeTensorDesc));
    return -1;
  }
  return 0;
}

rtError_t rtMemQueuePeek(int32_t device, uint32_t qid, size_t *bufLen, int32_t timeout) {
  *bufLen = sizeof(RuntimeTensorDesc) + 224U * 224U;
  return 0;
}
namespace ge {
namespace {
void *mock_handle = nullptr;
void *mock_method = nullptr;

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

  Status DequeueTensor(const int32_t device_id, const uint32_t queue_id, GeTensor &tensor,
                       ControlInfo &control_info) override {
    return 0;
  }

  MOCK_METHOD5(Dequeue, Status(int32_t, uint32_t, void *, size_t, ControlInfo &));
};

Status DequeueNoTilingStub(int32_t device_id, uint32_t queue_id, void *data, size_t size, ControlInfo &control_info) {
  RuntimeTensorDesc mbuf_tensor_desc;
  mbuf_tensor_desc.shape[0] = 4;
  mbuf_tensor_desc.shape[1] = 1;
  mbuf_tensor_desc.shape[2] = 1;
  mbuf_tensor_desc.shape[3] = 224;
  mbuf_tensor_desc.shape[4] = 224;
  mbuf_tensor_desc.dtype = static_cast<int64_t>(DT_INT64);
  mbuf_tensor_desc.data_addr = static_cast<int64_t>(reinterpret_cast<intptr_t>(data));
  if (memcpy_s(data, sizeof(RuntimeTensorDesc), &mbuf_tensor_desc, sizeof(RuntimeTensorDesc)) != EOK) {
    printf("Failed to copy mbuf data, dst size:%zu, src size:%zu\n", size, sizeof(RuntimeTensorDesc));
    return FAILED;
  }
  return 0;
}

class ModelDeployerMock : public ModelDeployer {
 public:
  Status DeployModel(const FlowModelPtr &flow_model,
                     const vector<uint32_t> &input_queue_ids,
                     const vector<uint32_t> &output_queue_ids,
                     DeployResult &deploy_result) override {
    deploy_result.input_queue_ids = {1, 2, 3};
    deploy_result.output_queue_ids = {4};
    return SUCCESS;
  }

  Status Undeploy(uint32_t model_id) override {
    return SUCCESS;
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
 public:
  ExchangeServiceMock exchange_service_;
  ModelDeployerMock model_deployer_;
};

Status InitializeHelperRuntime(const std::map<std::string, std::string> &options) {
  ExecutionRuntime::SetExecutionRuntime(std::make_shared<ExecutionRuntimeMock>());
  return SUCCESS;
}

class ExecutionRuntimeLocalMock : public ExecutionRuntime {
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

 public:
  LocalExchangeService exchange_service_;
  ModelDeployerMock model_deployer_;
};

Status InitializeHelperRuntimeLocal(const std::map<std::string, std::string> &options) {
  ExecutionRuntime::SetExecutionRuntime(std::make_shared<ExecutionRuntimeLocalMock>());
  return SUCCESS;
}

class MockMmpa : public MmpaStubApi {
 public:
  void *DlOpen(const char *file_name, int32_t mode) override {
    return mock_handle;
  }
  void *DlSym(void *handle, const char *func_name) override {
    return mock_method;
  }

  int32_t DlClose(void *handle) override {
    return 0;
  }
};

}  // namespace
class HeterogeneousRuntimeTest : public testing::Test {
  void SetUp() {
    MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  }
  void TearDown() {
    ExecutionRuntime::FinalizeExecutionRuntime();
    MmpaStub::GetInstance().Reset();
    mock_handle = nullptr;
    mock_method = nullptr;
  }
};

TEST_F(HeterogeneousRuntimeTest, TestInitHeterogeneousRuntime) {
  std::map<std::string, std::string> options_runtime;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options_runtime), FAILED); // error load so
  mock_handle = (void *) 0xffffffff;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options_runtime), FAILED); // error find init func
}

TEST_F(HeterogeneousRuntimeTest, TestDeployModel) {
  mock_handle = (void *) 0xffffffff;
  mock_method = (void *) &InitializeHelperRuntime;
  std::map<std::string, std::string> options_runtime;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options_runtime), SUCCESS);

  std::vector<std::string> engine_list = {"AIcoreEngine"};
  auto add_1 = OP_CFG(ADD);
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
  auto runtime = (ExecutionRuntimeMock *)ExecutionRuntime::GetInstance();
  EXPECT_CALL(runtime->exchange_service_, Dequeue).WillRepeatedly(Return(SUCCESS));

  std::vector<Tensor> output_tensors;
  ret = session.RunGraph(1, input_tensors, output_tensors);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(output_tensors.size(), 1);

  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  auto callback = [&](Status status, std::vector<ge::Tensor> &outputs) {
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

TEST_F(HeterogeneousRuntimeTest, TestDeployModelNoTiling) {
  mock_handle = (void *) 0xffffffff;
  mock_method = (void *) &InitializeHelperRuntime;
  std::map<std::string, std::string> options_runtime;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options_runtime), SUCCESS);

  std::vector<std::string> engine_list = {"AIcoreEngine"};
  auto add_1 = OP_CFG(ADD).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto add_2 = OP_CFG(ADD).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto data1 = OP_CFG(DATA).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto data2 = OP_CFG(DATA);
  auto data3 = OP_CFG(DATA);
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add_1)->EDGE(0, 0)->NODE("add_2", add_2));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add_1));
    CHAIN(NODE("data_3", data3)->EDGE(0, 1)->NODE("add_2", add_2));
    CHAIN(NODE("add_2", add_2)->EDGE(0, 0)->NODE("Node_Output", NETOUTPUT));
  };

  const auto SetUnknownOpKernelForNoTiling = [](const ComputeGraph::Vistor<NodePtr> &all_nodes) {
    GeTensorDesc tensor0(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    std::vector<std::pair<int64_t, int64_t>> tensor0_range;
    tensor0_range.push_back(std::make_pair(1, 1));
    tensor0_range.push_back(std::make_pair(1, 1));
    tensor0_range.push_back(std::make_pair(224, 224));
    tensor0_range.push_back(std::make_pair(224, 224));
    tensor0.SetShapeRange(tensor0_range);
    TensorUtils::SetSize(tensor0, 501760);
    AttrUtils::SetBool(tensor0, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor0, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 0);
    std::vector<int64_t> max_shape_list = {1, 10, 224, 224};
    AttrUtils::SetListInt(tensor0, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    GeTensorDesc tensor1(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    std::vector<std::pair<int64_t, int64_t>> tensor1_range;
    tensor1_range.push_back(std::make_pair(1, 1));
    tensor1_range.push_back(std::make_pair(1, 10));
    tensor1_range.push_back(std::make_pair(224, 224));
    tensor1_range.push_back(std::make_pair(224, 224));
    tensor1.SetShapeRange(tensor1_range);
    TensorUtils::SetSize(tensor1, 501760);
    AttrUtils::SetBool(tensor1, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor1, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 1024);
    AttrUtils::SetListInt(tensor1, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      if (op_desc->GetType() == DATA) {
        op_desc->SetOpKernelLibName("AiCoreLib");
        op_desc->SetOpEngineName("AIcoreEngine");
        op_desc->UpdateOutputDesc(0, tensor0);
        op_desc->SetOutputOffset({2048});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
      } else if (op_desc->GetType() == ADD) {
        op_desc->SetOpKernelLibName("AiCoreLib");
        op_desc->SetOpEngineName("AIcoreEngine");
        op_desc->UpdateInputDesc(0, tensor0);
        op_desc->UpdateOutputDesc(0, tensor1);
        op_desc->SetInputOffset({2048});
        op_desc->SetOutputOffset({2112});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
        vector<std::string> tiling_inline;
        vector<std::string> export_shape;
        AttrUtils::GetListStr(op_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
        tiling_inline.push_back("AIcoreEngine");
        AttrUtils::SetListStr(op_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
        AttrUtils::GetListStr(op_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
        export_shape.push_back("AIcoreEngine");
        AttrUtils::SetListStr(op_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
      } else {
        op_desc->SetOpKernelLibName("AiCoreLib");
        op_desc->SetOpEngineName("AIcoreEngine");
        op_desc->UpdateInputDesc(0, tensor1);
        op_desc->UpdateOutputDesc(0, tensor1);
        op_desc->SetInputOffset({2112});
        op_desc->SetSrcName( { "add" } );
        op_desc->SetSrcIndex({ 0 });
      }
    }
  };
  auto compute_graph = ToComputeGraph(g1);
  EXPECT_NE(compute_graph, nullptr);
  SetUnknownOpKernelForNoTiling(compute_graph->GetDirectNode());

  auto graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);
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

  auto runtime = (ExecutionRuntimeMock *)ExecutionRuntime::GetInstance();
  EXPECT_CALL(runtime->exchange_service_, Dequeue).WillRepeatedly(testing::Invoke(DequeueNoTilingStub));

  std::vector<Tensor> output_tensors;
  ret = session.RunGraph(1, input_tensors, output_tensors);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(output_tensors.size(), 1);

  std::mutex mu;
  std::condition_variable cv;
  bool done = false;
  auto callback = [&](Status status, std::vector<ge::Tensor> &outputs) {
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

TEST_F(HeterogeneousRuntimeTest, TestDeployModel_local) {
  mock_handle = (void *)0xffffffff;
  mock_method = (void *)&InitializeHelperRuntimeLocal;
  std::map<std::string, std::string> options_runtime;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options_runtime), SUCCESS);

  std::vector<std::string> engine_list = {"AIcoreEngine"};
  auto add_1 = OP_CFG(ADD);
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
  auto callback = [&](Status status, std::vector<ge::Tensor> &outputs) {
    std::unique_lock<std::mutex> lk(mu);
    done = true;
    ret = status;
    cv.notify_all();
  };
  session.RunGraphAsync(1, input_tensors, callback);
  std::unique_lock<std::mutex> lk(mu);
  cv.wait_for(lk, std::chrono::seconds(5), [&]() { return done; });
  ASSERT_EQ(ret, SUCCESS);
}

TEST_F(HeterogeneousRuntimeTest, TestDeployModelNoTiling_local) {
  mock_handle = (void *)0xffffffff;
  mock_method = (void *)&InitializeHelperRuntimeLocal;
  std::map<std::string, std::string> options_runtime;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options_runtime), SUCCESS);

  std::vector<std::string> engine_list = {"AIcoreEngine"};
  auto add_1 = OP_CFG(ADD).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto add_2 = OP_CFG(ADD).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto data1 = OP_CFG(DATA).Attr(ATTR_NAME_OP_NO_TILING, true);
  auto data2 = OP_CFG(DATA);
  auto data3 = OP_CFG(DATA);
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add_1)->EDGE(0, 0)->NODE("add_2", add_2));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add_1));
    CHAIN(NODE("data_3", data3)->EDGE(0, 1)->NODE("add_2", add_2));
    CHAIN(NODE("add_2", add_2)->EDGE(0, 0)->NODE("Node_Output", NETOUTPUT));
  };

  const auto SetUnknownOpKernelForNoTiling = [](const ComputeGraph::Vistor<NodePtr> &all_nodes) {
    GeTensorDesc tensor0(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    std::vector<std::pair<int64_t, int64_t>> tensor0_range;
    tensor0_range.push_back(std::make_pair(1, 1));
    tensor0_range.push_back(std::make_pair(1, 1));
    tensor0_range.push_back(std::make_pair(224, 224));
    tensor0_range.push_back(std::make_pair(224, 224));
    tensor0.SetShapeRange(tensor0_range);
    TensorUtils::SetSize(tensor0, 501760);
    AttrUtils::SetBool(tensor0, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor0, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 0);
    std::vector<int64_t> max_shape_list = {1, 10, 224, 224};
    AttrUtils::SetListInt(tensor0, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    GeTensorDesc tensor1(GeShape({1, -1, 224, 224}), FORMAT_NCHW, DT_INT64);
    std::vector<std::pair<int64_t, int64_t>> tensor1_range;
    tensor1_range.push_back(std::make_pair(1, 1));
    tensor1_range.push_back(std::make_pair(1, 10));
    tensor1_range.push_back(std::make_pair(224, 224));
    tensor1_range.push_back(std::make_pair(224, 224));
    tensor1.SetShapeRange(tensor1_range);
    TensorUtils::SetSize(tensor1, 501760);
    AttrUtils::SetBool(tensor1, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
    AttrUtils::SetInt(tensor1, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 1024);
    AttrUtils::SetListInt(tensor1, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list);

    for (const auto node : all_nodes) {
      const auto op_desc = node->GetOpDesc();
      if (op_desc->GetType() == DATA) {
        op_desc->SetOpKernelLibName("AiCoreLib");
        op_desc->SetOpEngineName("AIcoreEngine");
        op_desc->UpdateOutputDesc(0, tensor0);
        op_desc->SetOutputOffset({2048});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
      } else if (op_desc->GetType() == ADD) {
        op_desc->SetOpKernelLibName("AiCoreLib");
        op_desc->SetOpEngineName("AIcoreEngine");
        op_desc->UpdateInputDesc(0, tensor0);
        op_desc->UpdateOutputDesc(0, tensor1);
        op_desc->SetInputOffset({2048});
        op_desc->SetOutputOffset({2112});
        op_desc->SetWorkspace({});
        op_desc->SetWorkspaceBytes({});
        vector<std::string> tiling_inline;
        vector<std::string> export_shape;
        AttrUtils::GetListStr(op_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
        tiling_inline.push_back("AIcoreEngine");
        AttrUtils::SetListStr(op_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline);
        AttrUtils::GetListStr(op_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
        export_shape.push_back("AIcoreEngine");
        AttrUtils::SetListStr(op_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, export_shape);
      } else {
        op_desc->SetOpKernelLibName("AiCoreLib");
        op_desc->SetOpEngineName("AIcoreEngine");
        op_desc->UpdateInputDesc(0, tensor1);
        op_desc->UpdateOutputDesc(0, tensor1);
        op_desc->SetInputOffset({2112});
        op_desc->SetSrcName({"add"});
        op_desc->SetSrcIndex({0});
      }
    }
  };
  auto compute_graph = ToComputeGraph(g1);
  EXPECT_NE(compute_graph, nullptr);
  SetUnknownOpKernelForNoTiling(compute_graph->GetDirectNode());

  auto graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  map<AscendString, AscendString> options;
  Session session(options);
  session.AddGraph(1, graph, options);
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
  auto callback = [&](Status status, std::vector<ge::Tensor> &outputs) {
    std::unique_lock<std::mutex> lk(mu);
    done = true;
    ret = status;
    cv.notify_all();
  };
  session.RunGraphAsync(1, input_tensors, callback);
  std::unique_lock<std::mutex> lk(mu);
  cv.wait_for(lk, std::chrono::seconds(5), [&]() { return done; });
  ASSERT_EQ(ret, SUCCESS);
}
}  // namespace ge