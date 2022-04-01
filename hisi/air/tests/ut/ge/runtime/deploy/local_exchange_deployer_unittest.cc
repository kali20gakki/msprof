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
#include "runtime/deploy/local_exchange_deployer.h"
#undef private
#undef protected

using namespace std;
using namespace ::testing;

namespace ge {
class ExchangeDeployerTest : public testing::Test {
 protected:
  void SetUp() override {
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
      return DoCreateQueue();
    }

    MOCK_METHOD0(DoCreateQueue, Status());
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

  class MockLocalExchangeDeployer : public LocalExchangeDeployer {
   public:
    MockLocalExchangeDeployer(ExchangeService *exchange_service,
                              const DeployPlan *deploy_plan,
                              int32_t device_id)
        : LocalExchangeDeployer(exchange_service, deploy_plan, device_id) {}
    MOCK_METHOD2(DoBindQueue, Status(const QueueDeploymentInfo &, const QueueDeploymentInfo &));
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
    submodel_1.device_info.node_id_ = -1;
    submodel_1.model = make_shared<GeRootModel>();

    auto &submodel_2 = deploy_plan.submodels_["subgraph-2"];
    submodel_2.input_queue_indices.emplace_back(1);
    submodel_2.output_queue_indices.emplace_back(2);
    submodel_2.device_info.node_id_ = -1;
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
    submodel_1.device_info.node_id_ = -1;
    submodel_1.model = make_shared<GeRootModel>();

    auto &submodel_2 = deploy_plan.submodels_["subgraph-2"];
    submodel_2.input_queue_indices.emplace_back(3);
    submodel_2.input_queue_indices.emplace_back(4);
    submodel_2.output_queue_indices.emplace_back(5);
    submodel_2.device_info.node_id_ = -1;
    submodel_2.model = make_shared<GeRootModel>();

    deploy_plan.queue_bindings_.emplace_back(1, 2);
    deploy_plan.queue_bindings_.emplace_back(1, 4);
    return deploy_plan;
  }
};

///    NetOutput
///        |
///      PC_2
///        |
///      PC_1
///        |
///      data1
TEST_F(ExchangeDeployerTest, TestDeployModel) {
  DeployPlan deploy_plan = BuildDeployPlanWithoutQueueBindings();
  MockExchangeService exchange_service;
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  LocalExchangeDeployer exchange_deployer(&exchange_service, &deploy_plan, 0);
  ModelExchangeInfo model_exchange_info;
  ASSERT_EQ(exchange_deployer.DeployModelExchange(model_exchange_info), SUCCESS);

  ASSERT_EQ(model_exchange_info.submodel_queues_.size(), 2);
  ASSERT_EQ(model_exchange_info.deployed_queues_.size(), deploy_plan.queues_.size());
  ASSERT_EQ(model_exchange_info.GetInputQueueIds().size(), 1);
  ASSERT_EQ(model_exchange_info.GetInputQueueIds()[0], 100);
  ASSERT_EQ(model_exchange_info.GetOutputQueueIds().size(), 1);
  ASSERT_EQ(model_exchange_info.GetOutputQueueIds()[0], 102);
  ASSERT_TRUE(model_exchange_info.GetSubmodelQueues("subgraph-1") != nullptr);
  ASSERT_TRUE(model_exchange_info.GetSubmodelQueues("subgraph-2") != nullptr);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids.size(), 1);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids[0], 100);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->output_queue_ids.size(), 1);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->output_queue_ids[0], 101);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids.size(), 1);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids[0], 101);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->output_queue_ids.size(), 1);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->output_queue_ids[0], 102);
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
TEST_F(ExchangeDeployerTest, TestMultipleInputs) {
  DeployPlan deploy_plan = BuildDeployPlanWithQueueBindings();
  MockExchangeService exchange_service;
  ModelExchangeInfo model_exchange_info;
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  MockLocalExchangeDeployer exchange_deployer(&exchange_service, &deploy_plan, 0);
  EXPECT_CALL(exchange_deployer, DoBindQueue).WillRepeatedly(Return(SUCCESS));
  ASSERT_EQ(exchange_deployer.DeployModelExchange(model_exchange_info), SUCCESS);

  ASSERT_TRUE(model_exchange_info.GetSubmodelQueues("subgraph-1") != nullptr);
  ASSERT_TRUE(model_exchange_info.GetSubmodelQueues("subgraph-2") != nullptr);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids.size(), 2);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids[0], 100);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids[1], 102);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->output_queue_ids.size(), 1);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->output_queue_ids[0], 103);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids.size(), 2);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids[0], 103);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids[1], 104);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->output_queue_ids.size(), 1);
  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->output_queue_ids[0], 105);
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
TEST_F(ExchangeDeployerTest, TestBindQueues) {
  DeployPlan deploy_plan = BuildDeployPlanWithQueueBindings();
  MockExchangeService exchange_service;
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  MockLocalExchangeDeployer exchange_deployer(&exchange_service, &deploy_plan, 0);
  EXPECT_CALL(exchange_deployer, DoBindQueue).WillRepeatedly(Return(SUCCESS));
  ASSERT_EQ(exchange_deployer.Initialize(), SUCCESS);
  ModelExchangeInfo model_exchange_info;
  ASSERT_EQ(exchange_deployer.DeployModelExchange(model_exchange_info), SUCCESS);
  ASSERT_EQ(model_exchange_info.queue_bindings_.size(), 2);
}

//TEST_F(ExchangeDeployerTest, TestCreateQueuesWithProvidedQueuesFaield) {
//  DeployPlan deploy_plan = BuildDeployPlanWithQueueBindings();
//  MockExchangeService exchange_service;
//  ModelExchangeInfo model_exchange_info;
//  LocalExchangeDeployer deployer1(&exchange_service, &deploy_plan, {100}, {200}, 0);
//  ASSERT_EQ(deployer1.Initialize(), SUCCESS);
//  ASSERT_EQ(deployer1.DeployModelExchange(model_exchange_info), PARAM_INVALID);
//
//  LocalExchangeDeployer deployer2(&exchange_service, &deploy_plan, {100, 101}, {200, 201}, 0);
//  ASSERT_EQ(deployer2.Initialize(), SUCCESS);
//  ASSERT_EQ(deployer2.DeployModelExchange(model_exchange_info), PARAM_INVALID);
//}

/////         NetOutput
/////             |
/////             |(5)
/////           PC_2
/////        /       \(4)
/////        |(3)    |
/////        PC_1    |
/////       /  \(2)  |
/////     /     \    |
/////    |       \   |
/////  data1(0)  data2(1)
//TEST_F(ExchangeDeployerTest, TestCreateQueuesWithProvidedQueues) {
//  DeployPlan deploy_plan = BuildDeployPlanWithQueueBindings();
//  MockExchangeService exchange_service;
//  EXPECT_CALL(exchange_service, DoCreateQueue).Times(3).WillRepeatedly(Return(SUCCESS));
//  LocalExchangeDeployer exchange_deployer(&exchange_service, &deploy_plan, {1000, 1001}, {2000}, 0);
//  ModelExchangeInfo model_exchange_info;
//  ASSERT_EQ(exchange_deployer.Initialize(), SUCCESS);
//  ASSERT_EQ(exchange_deployer.DeployModelExchange(model_exchange_info), SUCCESS);
//  // do not create queues if already provided by client
//  ASSERT_EQ(exchange_service.handle_id_gen_, 106);
//  ASSERT_TRUE(model_exchange_info.GetSubmodelQueues("subgraph-1") != nullptr);
//  ASSERT_TRUE(model_exchange_info.GetSubmodelQueues("subgraph-2") != nullptr);
//  auto &idx_to_qid = exchange_deployer.queue_handles_;
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids.size(), 2);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids[0], 1000);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->input_queue_ids[1], idx_to_qid[2]);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->output_queue_ids.size(), 1);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-1")->output_queue_ids[0], idx_to_qid[3]);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids.size(), 2);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids[0], idx_to_qid[3]);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->input_queue_ids[1], idx_to_qid[4]);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->output_queue_ids.size(), 1);
//  ASSERT_EQ(model_exchange_info.GetSubmodelQueues("subgraph-2")->output_queue_ids[0], 2000);
//}

TEST_F(ExchangeDeployerTest, TestValidateParamFailed) {
  DeployPlan deploy_plan = BuildDeployPlanWithQueueBindings();
  MockExchangeService exchange_service;
  // input queue size mismatches
  ASSERT_EQ(LocalExchangeDeployer(&exchange_service, &deploy_plan, 0).Initialize(), SUCCESS);
  deploy_plan.root_model_info_.input_queue_indices[0] = static_cast<int32_t>(deploy_plan.queues_.size());
  ASSERT_EQ(LocalExchangeDeployer(&exchange_service, &deploy_plan, 0).Initialize(), PARAM_INVALID);

  deploy_plan = BuildDeployPlanWithQueueBindings();
  deploy_plan.root_model_info_.output_queue_indices[0] = static_cast<int32_t>(deploy_plan.queues_.size());
  ASSERT_EQ(LocalExchangeDeployer(&exchange_service, &deploy_plan, 0).Initialize(), PARAM_INVALID);

  deploy_plan = BuildDeployPlanWithQueueBindings();
  deploy_plan.submodels_["subgraph-1"].input_queue_indices[0] = static_cast<int32_t>(deploy_plan.queues_.size());
  ASSERT_EQ(LocalExchangeDeployer(&exchange_service, &deploy_plan, 0).Initialize(), PARAM_INVALID);

  deploy_plan = BuildDeployPlanWithQueueBindings();
  deploy_plan.submodels_["subgraph-2"].output_queue_indices[0] = static_cast<int32_t>(deploy_plan.queues_.size());
  ASSERT_EQ(LocalExchangeDeployer(&exchange_service, &deploy_plan, 0).Initialize(), PARAM_INVALID);
}

TEST_F(ExchangeDeployerTest, TestRollbackOnFailure) {
  DeployPlan deploy_plan = BuildDeployPlanWithQueueBindings();
  MockExchangeService exchange_service;
  // Create 2 queues successfully, failed on the 3rd
  // then rollback, destroy 2 queues
  EXPECT_CALL(exchange_service, DestroyQueue)
      .Times(2)
      .WillRepeatedly(Return(FAILED));
  EXPECT_CALL(exchange_service, DoCreateQueue)
      .Times(3)
      .WillOnce(Return(SUCCESS))
      .WillOnce(Return(SUCCESS))
      .WillRepeatedly(Return(FAILED));

  LocalExchangeDeployer exchange_deployer(&exchange_service, &deploy_plan, 0);
  ModelExchangeInfo model_exchange_info;
  ASSERT_EQ(exchange_deployer.DeployModelExchange(model_exchange_info), FAILED);
  ASSERT_TRUE(model_exchange_info.GetSubmodelQueues("subgraph-1") == nullptr);
}
}  // namespace ge