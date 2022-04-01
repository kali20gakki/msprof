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
#include <vector>

#define protected public
#define private public
#include "graph/load/model_manager/heterogeneous_model_executor.h"
#include "exec_runtime/execution_runtime.h"
#include "exec_runtime/runtime_tensor_desc.h"
#undef private
#undef protected
#include "graph/utils/tensor_adapter.h"

#include "runtime/deploy/stub_models.h"

using namespace std;
using namespace testing;

namespace ge {
class HeterogeneousModelExecutorTest : public testing::Test {
 protected:
  class MockExchangeService : public ExchangeService {
   public:
    Status CreateQueue(int32_t device_id,
                       const string &name,
                       uint32_t depth,
                       uint32_t work_mode,
                       uint32_t &queue_id) override {
      queue_id = queue_id_gen_++;
      return SUCCESS;
    }

    MOCK_METHOD4(Enqueue, Status(int32_t, uint32_t, const void *, size_t));
    MOCK_METHOD5(Dequeue, Status(int32_t, uint32_t, void * , size_t, ControlInfo &));
    MOCK_METHOD4(DequeueTensor, Status(int32_t, uint32_t, GeTensor & , ControlInfo &));

    Status Enqueue(int32_t device_id, uint32_t queue_id, size_t size, const FillFunc &fill_func) override {
      std::vector<char> buffer;
      buffer.resize(224 * 224 * 10);
      fill_func(&buffer[0], size);
      return SUCCESS;
    }
    Status Peek(int32_t device_id, uint32_t queue_id, size_t &size) override {
      return SUCCESS;
    }
    Status DestroyQueue(int32_t device_id, uint32_t queue_id) override {
      return SUCCESS;
    }


    int queue_id_gen_ = 100;
  };

  class MockModelDeployer : public ModelDeployer {
   public:
    Status DeployModel(const FlowModelPtr &flow_model,
                       const vector<uint32_t> &input_queue_ids,
                       const vector<uint32_t> &output_queue_ids,
                       DeployResult &deploy_result) override {
      return SUCCESS;
    }
    Status Undeploy(uint32_t model_id) override {
      return SUCCESS;
    }
  };

  class MockExecutionRuntime : public ExecutionRuntime {
   public:
    Status Initialize(const map<std::string, std::string> &options) override {
      return SUCCESS;
    }
    Status Finalize() override {
      return SUCCESS;
    }
    ModelDeployer &GetModelDeployer() override {
      return model_deployer_;
    }
    ExchangeService &GetExchangeService() override {
      return exchange_service_;
    }

   private:
    MockModelDeployer model_deployer_;
    MockExchangeService exchange_service_;
  };

  void SetUp() override {
    default_root_model_ = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
    default_executor_ = DefaultExecutor();
    default_executor_->SetListener(nullptr);
    default_executor_->SetModelId(666);
    default_executor_->SetDeviceId(0);
    ExecutionRuntime::SetExecutionRuntime(make_shared<MockExecutionRuntime>());
  }

  void TearDown() override {
  }

  unique_ptr<HeterogeneousModelExecutor> DefaultExecutor() {
    DeployResult deploy_result;
    deploy_result.model_id = 777;
    deploy_result.input_queue_ids = {1, 2};
    deploy_result.output_queue_ids = {5};
    auto flow_model = std::make_shared<FlowModel>();
    flow_model->AddSubModel(default_root_model_);
    return unique_ptr<HeterogeneousModelExecutor>(new HeterogeneousModelExecutor(flow_model, deploy_result));
  }

  GeRootModelPtr default_root_model_;
  unique_ptr<HeterogeneousModelExecutor> default_executor_;
};

namespace {
Status DequeueNoTilingStub(int32_t device_id, uint32_t queue_id, void *data, size_t size, ControlInfo &control_Info) {
  RuntimeTensorDesc mbuf_tensor_desc;
  mbuf_tensor_desc.shape[0] = 4;
  mbuf_tensor_desc.shape[1] = 1;
  mbuf_tensor_desc.shape[2] = 1;
  mbuf_tensor_desc.shape[3] = 224;
  mbuf_tensor_desc.shape[4] = 224;
  mbuf_tensor_desc.dtype = static_cast<int64_t>(DT_INT64);
  mbuf_tensor_desc.data_addr = static_cast<int64_t>(reinterpret_cast<intptr_t>(data));
  if (memcpy_s(data, sizeof(RuntimeTensorDesc), &mbuf_tensor_desc, sizeof(RuntimeTensorDesc)) != EOK) {
    printf("Failed to copy mbuf data, dst addr:%p dst size:%zu, src size:%zu\n", data, size, sizeof(RuntimeTensorDesc));
    return FAILED;
  }
  return 0;
}

Status DequeueNoTilingStubWithEOS(int32_t device_id, uint32_t queue_id, GeTensor &tensor,
                                  ControlInfo &control_Info) {
  control_Info.end_of_sequence_flag = true;
  return 0;
}
}

///     NetOutput
///         |
///       PC_3
///      /   \
///    PC_1  PC_2
///    |      |
///  data1  data2
TEST_F(HeterogeneousModelExecutorTest, TestInitializeSuccess) {
  ASSERT_EQ(default_executor_->input_queue_ids_, std::vector<uint32_t>({1, 2}));
  ASSERT_EQ(default_executor_->output_queue_ids_, std::vector<uint32_t>({5}));
  ASSERT_EQ(default_executor_->GetDeployedModelId(), 777);
  ASSERT_EQ(default_executor_->model_id_, 666);

  ASSERT_EQ(default_executor_->Initialize(), SUCCESS);
  ASSERT_EQ(default_executor_->input_tensor_desc_.size(), 2);
  ASSERT_EQ(default_executor_->output_tensor_desc_.size(), 1);
  ASSERT_EQ(default_executor_->input_tensor_sizes_.size(), 2);
  ASSERT_EQ(default_executor_->output_tensor_sizes_.size(), 1);
}

TEST_F(HeterogeneousModelExecutorTest, TestInitializeWithSingleModel) {
  auto model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  auto &submodels = model->GetSubmodels();
  auto it = submodels.find("subgraph-1");
  auto single_model = it->second;
  DeployResult deploy_result;
  deploy_result.model_id = 777;
  deploy_result.input_queue_ids = {1};
  deploy_result.output_queue_ids = {2};
  auto flow_model = std::make_shared<FlowModel>();
  flow_model->AddSubModel(model);
  HeterogeneousModelExecutor executor(flow_model, deploy_result);
  ASSERT_EQ(executor.Initialize(), SUCCESS);
  ASSERT_EQ(executor.root_model_->GetSubmodels().size(), 1);
}

TEST_F(HeterogeneousModelExecutorTest, TestBuildTensorDescMappingFailed) {
  auto &model_relation = *default_root_model_->model_relation_;
  std::map<std::string, GeTensorDescPtr> mapping;

  default_executor_->root_model_->model_relation_ =  default_root_model_->model_relation_;

  // test input queue size mismatches
  model_relation.submodel_queue_infos["subgraph-1"].input_queue_names.emplace_back("queue_0");
  ASSERT_EQ(default_executor_->BuildInputTensorDescMapping(mapping), PARAM_INVALID);

  // test output queue size mismatches
  model_relation.submodel_queue_infos["subgraph-1"].output_queue_names.clear();
  ASSERT_EQ(default_executor_->BuildOutputTensorDescMapping(mapping), PARAM_INVALID);
}

TEST_F(HeterogeneousModelExecutorTest, TestExecuteModelSuccess) {
  ASSERT_EQ(default_executor_->Initialize(), SUCCESS);
  std::vector<GeTensor> input_tensors;
  for (size_t i = 0; i < default_executor_->input_queue_ids_.size(); ++i) {
    auto tensor_desc = default_executor_->input_tensor_desc_[i];
    auto tensor_size = default_executor_->input_tensor_sizes_[i];
    GeTensor tensor(*tensor_desc, std::vector<uint8_t>(tensor_size));
    input_tensors.emplace_back(tensor);
  }

  auto &exchange_service = (MockExchangeService &) ExecutionRuntime::GetInstance()->GetExchangeService();
  EXPECT_CALL(exchange_service, Enqueue).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_service, Dequeue).WillRepeatedly(Return(SUCCESS));
  std::vector<GeTensor> output_tensors;
  ASSERT_EQ(default_executor_->Execute(input_tensors, output_tensors), SUCCESS);
}

TEST_F(HeterogeneousModelExecutorTest, TestExecuteModelNoTilingSuccess) {
  ASSERT_EQ(default_executor_->Initialize(), SUCCESS);
  std::vector<GeTensor> input_tensors;
  for (size_t i = 0; i < default_executor_->input_queue_ids_.size(); ++i) {
    auto tensor_desc = default_executor_->input_tensor_desc_[i];
    auto tensor_size = default_executor_->input_tensor_sizes_[i];
    default_executor_->input_is_no_tiling_[i] = true;
    GeTensor tensor(*tensor_desc, std::vector<uint8_t>(tensor_size));
    input_tensors.emplace_back(tensor);
  }

  for (size_t i = 0; i < default_executor_->output_queue_ids_.size(); ++i) {
    default_executor_->output_is_no_tiling_[i] = true;
    default_executor_->output_tensor_sizes_[i] = 1 * 1 * 224 * 224 * 10;
  }

  auto &exchange_service = (MockExchangeService &) ExecutionRuntime::GetInstance()->GetExchangeService();
  EXPECT_CALL(exchange_service, Enqueue).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_service, Dequeue).WillRepeatedly(testing::Invoke(DequeueNoTilingStub));
  std::vector<GeTensor> output_tensors;
  ASSERT_EQ(default_executor_->Execute(input_tensors, output_tensors), SUCCESS);
}

TEST_F(HeterogeneousModelExecutorTest, TestInputSizeMismatches) {
  ASSERT_EQ(default_executor_->Initialize(), SUCCESS);
  std::vector<GeTensor> input_tensors;
  ASSERT_EQ(default_executor_->EnqueueInputTensors(input_tensors), PARAM_INVALID);
}

TEST_F(HeterogeneousModelExecutorTest, TestEnqueueFailed) {
  ASSERT_EQ(default_executor_->Initialize(), SUCCESS);
  std::vector<GeTensor> input_tensors;
  for (size_t i = 0; i < default_executor_->input_queue_ids_.size(); ++i) {
    auto tensor_desc = default_executor_->input_tensor_desc_[i];
    auto tensor_size = default_executor_->input_tensor_sizes_[i];
    GeTensor tensor(*tensor_desc, std::vector<uint8_t>(tensor_size));
    input_tensors.emplace_back(tensor);
  }

  auto &exchange_service = (MockExchangeService &) ExecutionRuntime::GetInstance()->GetExchangeService();
  EXPECT_CALL(exchange_service, Enqueue).WillRepeatedly(Return(RT_FAILED));
  ASSERT_EQ(default_executor_->EnqueueInputTensors(input_tensors), RT_FAILED);
}

TEST_F(HeterogeneousModelExecutorTest, TestRunAsync) {
  ASSERT_EQ(default_executor_->Initialize(), SUCCESS);

  std::vector<Tensor> input_tensors;
  for (size_t i = 0; i < default_executor_->input_queue_ids_.size(); ++i) {
    auto tensor_desc = default_executor_->input_tensor_desc_[i];
    auto tensor_size = default_executor_->input_tensor_sizes_[i];
    GeTensor tensor(*tensor_desc, std::vector<uint8_t>(tensor_size));
    input_tensors.emplace_back(TensorAdapter::AsTensor(tensor));
  }

  auto ret = default_executor_->ExecuteAsync(input_tensors, [&](Status status, std::vector<ge::Tensor> &outputs) {
  });
  ASSERT_EQ(ret, FAILED);  // not started

  ASSERT_EQ(default_executor_->ModelRunStart(), SUCCESS);

  auto &exchange_service = (MockExchangeService &) ExecutionRuntime::GetInstance()->GetExchangeService();
  EXPECT_CALL(exchange_service, Enqueue).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_service, Dequeue).WillRepeatedly(Return(SUCCESS));

  std::mutex mu;
  std::unique_lock<std::mutex> lk(mu);
  condition_variable cb;
  ret = FAILED;
  default_executor_->ExecuteAsync(input_tensors, [&](Status status, std::vector<ge::Tensor> &outputs) {
    cb.notify_all();
    ret = status;
  });

  cb.wait_for(lk, std::chrono::seconds(5));
  ASSERT_EQ(default_executor_->ModelRunStop(), SUCCESS);
  ASSERT_EQ(default_executor_->ModelRunStop(), SUCCESS);  // not started
}

TEST_F(HeterogeneousModelExecutorTest, TestExecuteModelWithEOS) {
  ASSERT_EQ(default_executor_->Initialize(), SUCCESS);
  std::vector<GeTensor> input_tensors;
  for (size_t i = 0; i < default_executor_->input_queue_ids_.size(); ++i) {
  auto tensor_desc = default_executor_->input_tensor_desc_[i];
  auto tensor_size = default_executor_->input_tensor_sizes_[i];
  default_executor_->input_is_no_tiling_[i] = true;
  GeTensor tensor(*tensor_desc, std::vector<uint8_t>(tensor_size));
  input_tensors.emplace_back(tensor);
  }

  for (size_t i = 0; i < default_executor_->output_queue_ids_.size(); ++i) {
  default_executor_->output_is_no_tiling_[i] = true;
  default_executor_->output_tensor_sizes_[i] = 1 * 1 * 224 * 224 * 10;
  }

  auto &exchange_service = (MockExchangeService &) ExecutionRuntime::GetInstance()->GetExchangeService();
  EXPECT_CALL(exchange_service, Enqueue).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_service, DequeueTensor).WillRepeatedly(testing::Invoke(DequeueNoTilingStubWithEOS));
  std::vector<GeTensor> output_tensors;
  ASSERT_EQ(default_executor_->Execute(input_tensors, output_tensors), END_OF_SEQUENCE);
}

}  // namespace ge
