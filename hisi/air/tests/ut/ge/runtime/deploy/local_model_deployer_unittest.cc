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
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define protected public
#define private public
#include "exec_runtime/deploy/model_deployer.h"
#include "exec_runtime/execution_runtime.h"
#include "runtime/deploy/local_model_deployer.h"
#undef private
#undef protected

#include "stub_models.h"

using namespace std;

namespace ge {
class LocalModelDeployerTest : public testing::Test {
 protected:
  void SetUp() override {
    ExecutionRuntime::SetExecutionRuntime(make_shared<MockExecutionRuntime>());
    model_deployer_ = (MockLocalModelDeployer * ) & ExecutionRuntime::GetInstance()->GetModelDeployer();
    exchange_service_ = (MockExchangeService * ) & ExecutionRuntime::GetInstance()->GetExchangeService();
  }

  void TearDown() override {
  }

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

    MOCK_METHOD2(DestroyQueue, Status(int32_t, uint32_t));

    Status Enqueue(int32_t device_id, uint32_t queue_id, const void *data, size_t size) override {
      return SUCCESS;
    }

    Status Enqueue(int32_t device_id, uint32_t queue_id, size_t size, const FillFunc &fill_func) override {
      return SUCCESS;
    }

    Status Peek(int32_t device_id, uint32_t queue_id, size_t &size) override {
      return SUCCESS;
    }

    Status Dequeue(int32_t device_id, uint32_t queue_id, void *data, size_t size, ControlInfo &control_Info) override {
      return SUCCESS;
    }

    Status DequeueTensor(const int32_t device_id, const uint32_t queue_id, GeTensor &tensor,
                         ControlInfo &control_Info) override {
      return 0;
    }

    int queue_id_gen_ = 100;
  };

  class MockLocalModelDeployer : public LocalModelDeployer {
   public:
    std::map<std::string, std::pair<std::vector<uint32_t>, std::vector<uint32_t>>> submodel_ids_;
    MOCK_METHOD0(DoLoadModel, Status());
    MOCK_METHOD1(UnloadSubmodel, Status(uint32_t));

   protected:
    Status LoadSubmodelWithQueue(const GeRootModelPtr &root_model,
                                 const vector<uint32_t> &input_queue_ids,
                                 const vector<uint32_t> &output_queue_ids,
                                 uint32_t &model_id) override {
      model_id = submodel_id_gen_++;
      submodel_ids_[root_model->GetRootGraph()->GetName()] = std::make_pair(input_queue_ids, output_queue_ids);
      return DoLoadModel();
    }

   private:
    uint32_t submodel_id_gen_ = 10000;
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
    MockLocalModelDeployer model_deployer_;
    MockExchangeService exchange_service_;
  };

  ///    NetOutput
  ///        |
  ///      PC_2
  ///        |
  ///      PC_1
  ///        |
  ///      data1
  DeployPlan BuildDeployPlanWithoutQueueBindings() {
    DeployPlan deploy_plan;
    deploy_plan.queues_.resize(3);
    deploy_plan.root_model_info_.input_queue_indices.emplace_back(0);
    deploy_plan.root_model_info_.output_queue_indices.emplace_back(2);
    auto &submodel_1 = deploy_plan.submodels_["subgraph-1"];
    submodel_1.input_queue_indices.emplace_back(0);
    submodel_1.output_queue_indices.emplace_back(1);
    submodel_1.device_info.device_id_ = -1;
    submodel_1.model = make_shared<GeRootModel>();

    auto &submodel_2 = deploy_plan.submodels_["subgraph-2"];
    submodel_2.input_queue_indices.emplace_back(1);
    submodel_2.output_queue_indices.emplace_back(2);
    submodel_2.device_info.device_id_ = -1;
    submodel_2.model = make_shared<GeRootModel>();
    return deploy_plan;
  }
  ///         NetOutput
  ///             |
  ///             |(5)
  ///           PC_2
  ///        /       \(4)
  ///        |(3)    |
  ///        PC_1    |
  ///       /  \(2)  |
  ///     /     \    |
  ///    |       \   |
  ///  data1(0)  data2(1)
  DeployPlan BuildDeployPlanWithQueueBindings() {
    DeployPlan deploy_plan;
    deploy_plan.queues_.resize(6);
    deploy_plan.root_model_info_.input_queue_indices.emplace_back(0);
    deploy_plan.root_model_info_.input_queue_indices.emplace_back(1);
    deploy_plan.root_model_info_.output_queue_indices.emplace_back(5);
    auto &submodel_1 = deploy_plan.submodels_["subgraph-1"];
    submodel_1.input_queue_indices.emplace_back(0);
    submodel_1.input_queue_indices.emplace_back(2);
    submodel_1.output_queue_indices.emplace_back(3);
    submodel_1.device_info.device_id_ = -1;
    submodel_1.model = make_shared<GeRootModel>();

    auto &submodel_2 = deploy_plan.submodels_["subgraph-2"];
    submodel_2.input_queue_indices.emplace_back(3);
    submodel_2.input_queue_indices.emplace_back(4);
    submodel_2.output_queue_indices.emplace_back(5);
    submodel_2.device_info.device_id_ = -1;
    submodel_2.model = make_shared<GeRootModel>();

    deploy_plan.queue_bindings_.emplace_back(1, 2);
    deploy_plan.queue_bindings_.emplace_back(1, 4);
    return deploy_plan;
  }

  MockExchangeService *exchange_service_;
  MockLocalModelDeployer *model_deployer_;
};

///     NetOutput
///         |
///       PC_3
///      /   \.
///    PC_1  PC_2
///    |      |
///  data1  data2
TEST_F(LocalModelDeployerTest, TestDeployModelAndUndeployModel) {
  DeployPlan deploy_plan = BuildDeployPlanWithoutQueueBindings();
  auto &model_deployer = *model_deployer_;
  auto &exchange_service = *exchange_service_;
  EXPECT_CALL(model_deployer, DoLoadModel()).Times(3).WillRepeatedly(::testing::Return(SUCCESS));
  EXPECT_CALL(*exchange_service_, DestroyQueue).WillRepeatedly(::testing::Return(SUCCESS));
  EXPECT_CALL(model_deployer, UnloadSubmodel).Times(3).WillRepeatedly(::testing::Return(SUCCESS));
  model_deployer.model_id_gen_ = 100;
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(root_model != nullptr);
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  DeployResult deploy_result;
  ASSERT_EQ(model_deployer.DeployModel(flow_model, {}, {}, deploy_result), SUCCESS);
  ASSERT_EQ(deploy_result.model_id, 100);
  EXPECT_EQ(model_deployer.deployed_models_[deploy_result.model_id].submodel_ids.size(), 3);
  ASSERT_EQ(model_deployer.model_id_gen_, 101);
  ASSERT_EQ(deploy_result.input_queue_ids.size(), 2);
  ASSERT_EQ(deploy_result.input_queue_ids[0], 100);
  ASSERT_EQ(deploy_result.input_queue_ids[1], 101);
  ASSERT_EQ(deploy_result.output_queue_ids.size(), 1);
  ASSERT_EQ(deploy_result.output_queue_ids[0], 104);

  ASSERT_EQ(model_deployer.submodel_ids_.size(), 3);
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-1"].first, std::vector<uint32_t>{100});
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-1"].second, std::vector<uint32_t>{102});
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-2"].first, std::vector<uint32_t>{101});
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-2"].second, std::vector<uint32_t>{103});
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-3"].first, std::vector<uint32_t>({102, 103}));
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-3"].second, std::vector<uint32_t>{104});

  ASSERT_EQ(model_deployer.Undeploy(deploy_result.model_id), SUCCESS);
  ASSERT_EQ(model_deployer.Undeploy(deploy_result.model_id), GE_EXEC_MODEL_ID_INVALID);
  ASSERT_TRUE(model_deployer.deployed_models_.empty());
}

TEST_F(LocalModelDeployerTest, TestUnloadModelWhenFinalize) {
  DeployPlan deploy_plan = BuildDeployPlanWithoutQueueBindings();
  MockLocalModelDeployer model_deployer;
  EXPECT_CALL(model_deployer, DoLoadModel()).Times(3).WillRepeatedly(::testing::Return(SUCCESS));
  EXPECT_CALL(*exchange_service_, DestroyQueue).WillRepeatedly(::testing::Return(SUCCESS));
  EXPECT_CALL(model_deployer, UnloadSubmodel).Times(3).WillRepeatedly(::testing::Return(SUCCESS));
  model_deployer.model_id_gen_ = 100;
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(root_model != nullptr);
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  DeployResult deploy_result;
  ASSERT_EQ(model_deployer.DeployModel(flow_model, {}, {}, deploy_result), SUCCESS);

  ASSERT_EQ(model_deployer.Finalize(), SUCCESS);
  ASSERT_TRUE(model_deployer.deployed_models_.empty());
}

///     NetOutput
///         |
///       PC_3
///      /   \.
///    PC_1  PC_2
///    |      |
///  data1  data2
TEST_F(LocalModelDeployerTest, TestLoadSubmodelsQueueIdMatches) {
  MockLocalModelDeployer model_deployer;
  EXPECT_CALL(model_deployer, DoLoadModel()).Times(3).WillRepeatedly(::testing::Return(SUCCESS));
  model_deployer.model_id_gen_ = 100;
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(root_model != nullptr);
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  DeployPlan deploy_plan;
  ASSERT_EQ(DeployPlanner(flow_model).BuildPlan(deploy_plan), SUCCESS);
  LocalModelDeployer::DeployedModel deployed_model;
  auto &model_exchange_info = deployed_model.deployed_exchange;
  std::vector<uint32_t> subgraph_1_inputs({111});
  std::vector<uint32_t> subgraph_1_outputs({222});
  std::vector<uint32_t> subgraph_2_inputs({333});
  std::vector<uint32_t> subgraph_2_outputs({444});
  std::vector<uint32_t> subgraph_3_inputs({333, 444});
  std::vector<uint32_t> subgraph_3_outputs({555});

  model_exchange_info.submodel_queues_.emplace("subgraph-1", ModelQueueInfo{subgraph_1_inputs, subgraph_1_outputs});
  model_exchange_info.submodel_queues_.emplace("subgraph-2", ModelQueueInfo{subgraph_2_inputs, subgraph_2_outputs});
  model_exchange_info.submodel_queues_.emplace("subgraph-3", ModelQueueInfo{subgraph_3_inputs, subgraph_3_outputs});
  model_deployer.LoadSubmodels(deploy_plan, deployed_model);
  ASSERT_EQ(model_deployer.submodel_ids_.size(), 3);
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-1"].first, subgraph_1_inputs);
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-1"].second, subgraph_1_outputs);
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-2"].first, subgraph_2_inputs);
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-2"].second, subgraph_2_outputs);
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-3"].first, subgraph_3_inputs);
  ASSERT_EQ(model_deployer.submodel_ids_["subgraph-3"].second, subgraph_3_outputs);
}


///     NetOutput
///         |
///       PC_3
///      /   \.
///    PC_1  PC_2
///    |      |
///  data1  data2
TEST_F(LocalModelDeployerTest, TestFinalize) {
  MockLocalModelDeployer model_deployer;
  EXPECT_CALL(model_deployer, DoLoadModel()).WillOnce(::testing::Return(INTERNAL_ERROR));
  // test all queues are destroyed
  EXPECT_CALL(*exchange_service_, DestroyQueue).Times(5).WillRepeatedly(::testing::Return(SUCCESS));

  model_deployer.model_id_gen_ = 100;
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(root_model != nullptr);
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  DeployResult deploy_result;
  ASSERT_EQ(model_deployer.DeployModel(flow_model, {}, {}, deploy_result), INTERNAL_ERROR);
}

TEST_F(LocalModelDeployerTest, TestRollbackWhenLoadFailed) {
  MockLocalModelDeployer model_deployer;
  EXPECT_CALL(model_deployer, DoLoadModel()).WillOnce(::testing::Return(INTERNAL_ERROR));
  // test all queues are destroyed
  EXPECT_CALL(*exchange_service_, DestroyQueue).Times(5).WillRepeatedly(::testing::Return(SUCCESS));

  model_deployer.model_id_gen_ = 100;
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(root_model != nullptr);
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  DeployResult deploy_result;
  ASSERT_EQ(model_deployer.DeployModel(flow_model, {}, {}, deploy_result), INTERNAL_ERROR);
}
}  // namespace ge