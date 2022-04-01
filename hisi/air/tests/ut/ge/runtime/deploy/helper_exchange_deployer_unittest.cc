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
#include "deploy/flowrm/helper_exchange_deployer.h"
#undef private
#undef protected

#include "framework/common/ge_inner_error_codes.h"
#include "proto/deployer.pb.h"

using namespace std;
using namespace ::testing;

namespace ge {
class HelperHelperExchangeDeployerTest : public testing::Test {
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

  class MockHelperExchangeDeployer : public HelperExchangeDeployer {
   public:
    MockHelperExchangeDeployer(ExchangeService &exchange_service,
                               const deployer::ExchangePlan &deploy_plan,
                               int32_t device_id)
        : HelperExchangeDeployer(exchange_service, deploy_plan, device_id) {}
    //    MOCK_METHOD2(DoBindQueue, Status(const QueueDeploymentInfo &, const QueueDeploymentInfo &));

    MOCK_METHOD1(BindRoute, Status(const bqs::QueueRoute &));
    MOCK_CONST_METHOD1(CreateTag, Status(ExchangeRoute::ExchangeEndpoint & ));
    MOCK_METHOD3(CreateHcomHandle, Status(const deployer::QueueDesc &,
        const deployer::QueueDesc &,
        uint32_t &));
  };

  static void AddExchangeEndpoint(deployer::ExchangePlan &exchange_plan, int32_t type, const std::string &name) {
    auto endpoint = exchange_plan.add_queues();
    endpoint->set_type(type);
    endpoint->set_name(name);
    endpoint->set_depth(2);
  }

  static void AddExchangeGroup(deployer::ExchangePlan &exchange_plan, int32_t type,
                               const std::string &name, const std::vector<int32_t> &group) {
    auto endpoint = exchange_plan.add_queues();
    endpoint->set_type(type);
    endpoint->set_name(name);
    endpoint->set_depth(2);
    endpoint->mutable_queue_indices()->Add(group.begin(), group.end());
  }

  ///    NetOutput
  ///        |
  ///      PC_2
  ///        |
  ///      PC_1
  ///        |
  ///      data1
  deployer::ExchangePlan BuildExchangePlanSimple() {
    deployer::ExchangePlan exchange_plan;
    // Model input
    AddExchangeEndpoint(exchange_plan, 1, "input-0");
    // PartitionedCall_1 input
    AddExchangeEndpoint(exchange_plan, 2, "input-0");
    // PartitionedCall_1 output
    AddExchangeEndpoint(exchange_plan, 1, "internal-queue-0");
    // PartitionedCall_2 input
    AddExchangeEndpoint(exchange_plan, 1, "internal-queue-0");
    // PartitionedCall_2 output
    AddExchangeEndpoint(exchange_plan, 2, "output-0");
    // Model output
    AddExchangeEndpoint(exchange_plan, 1, "output-0");

    auto data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(0);
    data_binding->set_dst_queue_index(1);

    auto internal_binding = exchange_plan.add_bindings();
    internal_binding->set_src_queue_index(2);
    internal_binding->set_dst_queue_index(3);

    auto output_binding = exchange_plan.add_bindings();
    output_binding->set_src_queue_index(4);
    output_binding->set_dst_queue_index(5);
    return exchange_plan;
  }

  ///    NetOutput
  ///        |
  ///      PC_2
  ///        |  \
  ///      PC_1 PC_1
  ///        |  /
  ///      data1
  deployer::ExchangePlan BuildExchangePlanSimple2PG() {
    deployer::ExchangePlan exchange_plan;
    // Model input, index:0
    AddExchangeEndpoint(exchange_plan, 1, "input");
    // PartitionedCall_1@0_0_0, index:1
    AddExchangeEndpoint(exchange_plan, 2, "data1_output_-1_0_to_pc1_input0_0_0");
    // PartitionedCall_1@0_0_0 output, index:2
    AddExchangeEndpoint(exchange_plan, 1, "pc1_output0_0_0_to_pc2_input0_0_0");
    // PartitionedCall_1@0_0_1 input, index:3
    AddExchangeEndpoint(exchange_plan, 2, "data1_output_-1_0_to_pc1_input0_0_1");
    // PartitionedCall_1@0_0_1 output, index:4
    AddExchangeEndpoint(exchange_plan, 1, "pc1_output0_0_0_to_pc2_input0_0_1");
    // PartitionedCall_2 input, index:5
    AddExchangeEndpoint(exchange_plan, 1, "pc2_input");
    // PartitionedCall_2 output, index:6
    AddExchangeEndpoint(exchange_plan, 2, "pc2_output");
    // Model output, index:7
    AddExchangeEndpoint(exchange_plan, 1, "output");

    // pc1 input group, index:8
    AddExchangeGroup(exchange_plan, 3, "pc1_input", {1, 3});
    // pc1 input@0_0_0 group num,  index:9
    AddExchangeGroup(exchange_plan, 3, "data1_output_-1_0_to_pc1_input0_0_0", {0});
    // pc1 input@0_0_1 group num,  index:10
    AddExchangeGroup(exchange_plan, 3, "data1_output_-1_0_to_pc1_input0_0_1", {0});

    // pc1 output group, index:11
    AddExchangeGroup(exchange_plan, 3, "pc1_output", {2, 4});
    // pc1 output@0_0_0 group num, index:12
    AddExchangeGroup(exchange_plan, 3, "pc1_output0_0_0_to_pc2_input0_0_0", {5});
    // pc1 output@0_0_1 group num, index:13
    AddExchangeGroup(exchange_plan, 3, "pc1_output0_0_0_to_pc2_input0_0_1", {5});

    auto data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(0);
    data_binding->set_dst_queue_index(8);
    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(0);
    data_binding->set_dst_queue_index(1);
    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(0);
    data_binding->set_dst_queue_index(3);

    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(9);
    data_binding->set_dst_queue_index(1);

    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(10);
    data_binding->set_dst_queue_index(3);

    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(2);
    data_binding->set_dst_queue_index(12);
    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(2);
    data_binding->set_dst_queue_index(5);

    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(4);
    data_binding->set_dst_queue_index(13);
    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(4);
    data_binding->set_dst_queue_index(5);

    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(11);
    data_binding->set_dst_queue_index(5);

    data_binding = exchange_plan.add_bindings();
    data_binding->set_src_queue_index(6);
    data_binding->set_dst_queue_index(7);
    return exchange_plan;
  }
};

TEST_F(HelperHelperExchangeDeployerTest, TestDeployFailedDueToGroupToGroup) {
  auto exchange_plan = BuildExchangePlanSimple();
  exchange_plan.mutable_queues(0)->set_type(3);
  exchange_plan.mutable_queues(1)->set_type(3);
  MockExchangeService exchange_service;
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  MockHelperExchangeDeployer exchange_deployer(exchange_service, exchange_plan, 0);
  EXPECT_CALL(exchange_deployer, CreateHcomHandle).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_deployer, CreateTag).WillRepeatedly(Return(SUCCESS));
  ExchangeRoute exchange_route;
  ASSERT_EQ(exchange_deployer.Deploy(exchange_route), INTERNAL_ERROR);
}

TEST_F(HelperHelperExchangeDeployerTest, TestDeployFailedDueToIndexOutOfRange) {
  auto exchange_plan = BuildExchangePlanSimple();
  MockExchangeService exchange_service;
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  MockHelperExchangeDeployer exchange_deployer(exchange_service, exchange_plan, 0);
  EXPECT_CALL(exchange_deployer, CreateHcomHandle).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_deployer, CreateTag).WillRepeatedly(Return(SUCCESS));
  ExchangeRoute exchange_route;
  exchange_plan.mutable_bindings(0)->set_dst_queue_index(99);
  ASSERT_EQ(exchange_deployer.Deploy(exchange_route), PARAM_INVALID);
  exchange_plan.mutable_bindings(0)->set_src_queue_index(99);
  ASSERT_EQ(exchange_deployer.Deploy(exchange_route), PARAM_INVALID);
}

TEST_F(HelperHelperExchangeDeployerTest, TestDeployModel) {
  auto exchange_plan = BuildExchangePlanSimple();
  MockExchangeService exchange_service;
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  MockHelperExchangeDeployer exchange_deployer(exchange_service, exchange_plan, 0);
  EXPECT_CALL(exchange_deployer, CreateHcomHandle).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_deployer, CreateTag).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_deployer, BindRoute).WillRepeatedly(Return(SUCCESS));
  ExchangeRoute exchange_route;
  ASSERT_EQ(exchange_deployer.Deploy(exchange_route), SUCCESS);

  std::vector<uint32_t> queue_ids;
  // out of range
  ASSERT_EQ(exchange_route.GetQueueIds({99, 100}, queue_ids), PARAM_INVALID);
  // endpoint is not a queue
  ASSERT_EQ(exchange_route.GetQueueIds({1}, queue_ids), PARAM_INVALID);
  ASSERT_EQ(exchange_route.GetQueueIds({4}, queue_ids), PARAM_INVALID);
  ASSERT_EQ(exchange_route.GetQueueIds({0, 2, 3, 5}, queue_ids), SUCCESS);
}

TEST_F(HelperHelperExchangeDeployerTest, TestDeployModel2PG) {
  auto exchange_plan = BuildExchangePlanSimple2PG();
  MockExchangeService exchange_service;
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  MockHelperExchangeDeployer exchange_deployer(exchange_service, exchange_plan, 0);
  EXPECT_CALL(exchange_deployer, CreateHcomHandle).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_deployer, CreateTag).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_deployer, BindRoute).WillRepeatedly(Return(SUCCESS));
  ExchangeRoute exchange_route;
  ASSERT_EQ(exchange_deployer.Deploy(exchange_route), SUCCESS);
}
}  // namespace ge