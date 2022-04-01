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
#include <vector>
#include <string>
#include <map>
#include "framework/common/debug/ge_log.h"
#include "hccl/hccl_types.h"
#include "depends/mmpa/src/mmpa_stub.h"
#define protected public
#define private public
#include "deploy/flowrm/flow_route_manager.h"
#include "deploy/deployer/deploy_context.h"
#undef protected public
#undef private public

using namespace std;
namespace ge {
void *mock_handle = nullptr;

HcclResult HcomInitByString(const string &rankTable) {
  return HCCL_SUCCESS;
}

class MockMmpa : public MmpaStubApi {
 public:
  void *DlSym(void *handle, const char *func_name) override {
    if (std::string(func_name) == "HcomInitByString") {
      return (void *) &HcomInitByString;
    }
    return dlsym(handle, func_name);
  }

  int32_t RealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen) override {
    strncpy(realPath, path, realPathLen);
    return EN_OK;
  }

  void *DlOpen(const char *fileName, int32_t mode) override {
    return (void *) mock_handle;
  }
  int32_t DlClose(void *handle) override {
    return 0L;
  }
};

class DeployContextTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DeployContextTest, TestLoadModel) {
  DeployContext context;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  auto res = context.LoadModel(request, response);
  ASSERT_EQ(res, ge::SUCCESS);
  ASSERT_EQ(response.error_code(), ge::FAILED);
}

TEST_F(DeployContextTest, TestGrantQueueForCpuSchedule_host) {
  DeployContext context(true);
  ExecutorProcess *executor = nullptr;
  context.executor_manager_.GetOrForkExecutorProcess({0, 0}, &executor);
  context.executor_manager_.SetMaintenanceCfg(&context.dev_maintenance_cfg_);
  std::vector<uint32_t> input_queues{1};
  std::vector<uint32_t> output_queues{2};
  auto res = context.GrantQueuesForCpuSchedule(0, true, input_queues, output_queues);
  ASSERT_EQ(res, ge::SUCCESS);
}

TEST_F(DeployContextTest, TestGrantQueueForCpuSchedule_device) {
  DeployContext context(false);
  ExecutorProcess *executor = nullptr;
  context.executor_manager_.GetOrForkExecutorProcess({0, 0}, &executor);
  context.executor_manager_.SetMaintenanceCfg(&context.dev_maintenance_cfg_);
  std::vector<uint32_t> input_queues{1};
  std::vector<uint32_t> output_queues{2};
  auto res = context.GrantQueuesForCpuSchedule(0, false, input_queues, output_queues);
  ASSERT_EQ(res, ge::SUCCESS);
}

TEST_F(DeployContextTest, TestDownloadModel) {
  DeployContext context;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  auto res = context.DownloadModel(request, response);
  ASSERT_NE(res, ge::SUCCESS);
}

TEST_F(DeployContextTest, TestPreDeployModel) {
  deployer::DeployerRequest request;
  request.set_type(deployer::kPreDeployModel);
  auto pre_deploy_req = request.mutable_pre_deploy_model_request();
  pre_deploy_req->set_root_model_id(1);
  auto exchange_plan = pre_deploy_req->mutable_exchange_plan();
  deployer::QueueDesc queue_desc;
  queue_desc.set_name("data-1");
  queue_desc.set_depth(2);
  queue_desc.set_type(2);  // tag
  *exchange_plan->add_queues() = queue_desc;
  queue_desc.set_type(1);  // queue
  *exchange_plan->add_queues() = queue_desc;

  queue_desc.set_name("output-1");
  queue_desc.set_type(1);  // queue
  *exchange_plan->add_queues() = queue_desc;
  queue_desc.set_type(2);  // tag
  *exchange_plan->add_queues() = queue_desc;
  auto input_binding = exchange_plan->add_bindings();
  input_binding->set_src_queue_index(0);
  input_binding->set_dst_queue_index(1);
  auto output_binding = exchange_plan->add_bindings();
  output_binding->set_src_queue_index(2);
  output_binding->set_dst_queue_index(3);

  auto submodel_desc = pre_deploy_req->add_submodels();
  submodel_desc->set_model_size(128);
  submodel_desc->set_model_name("any-model");
  submodel_desc->add_input_queue_indices(1);
  submodel_desc->add_output_queue_indices(2);

  DeployContext context;
  deployer::DeployerResponse response;
  context.PreDeployModel(*pre_deploy_req, response);
  ASSERT_EQ(response.error_code(), ge::SUCCESS);
}

TEST_F(DeployContextTest, TestUnloadModel) {
  DeployContext context;
  deployer::DeployerRequest request;
  request.set_type(deployer::kUnloadModel);
  deployer::DeployerResponse response;
  context.UnloadModel(request, response);
}

TEST_F(DeployContextTest, TestProcessVarManagerAndSharedInfo) {
  DeployContext context;
  deployer::DeployerResponse response;
  deployer::MultiVarManagerRequest info;
  context.ProcessMultiVarManager(info, response);
  deployer::SharedContentDescRequest shared_info;
  context.ProcessSharedContent(shared_info, response);
}

TEST_F(DeployContextTest, TestGetQueuesFail) {
  DeployContext context;
  deployer::SubmodelDesc subDesc;
  context.submodel_descs_[{0, 1, 1}] = subDesc;
  std::vector<uint32_t> input_queues;
  std::vector<uint32_t> output_queues;
  auto ret = context.GetQueues({0, 1, 2}, input_queues, output_queues);
  ASSERT_EQ(ret, ge::FAILED);
}

TEST_F(DeployContextTest, TestGetQueuesSuccess) {
  DeployContext context;
  deployer::SubmodelDesc sub_desc;
  context.submodel_descs_[{0, 1, 1}] = sub_desc;
  FlowRouteManager::GetInstance().AddRoute(ExchangeRoute{}, context.exchange_routes_[{0, 1, 0}]);
  std::vector<uint32_t> input_queues;
  std::vector<uint32_t> output_queues;
  auto ret = context.GetQueues({0, 1, 1}, input_queues, output_queues);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(DeployContextTest, TestDeployRankTable) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *) 0xFFFFFFFF;
  std::string rank_table = "{\"collective_id\":\"192.168.1.30-world_group\",\"master_ip\":\"192.168.1.30\","
                           "\"master_port\":\"18001\",\"node_list\":[{\"node_addr\":\"192.168.1.30\","
                           "\"ranks\":[{\"device_id\":\"0\",\"rank_id\":\"0\"}]}],\"status\":\"completed\","
                           "\"version\":\"1.1\"}";
  DeployContext context;
  deployer::DeployerRequest request;
  request.set_type(deployer::kDeployRankTable);
  auto deploy_req = request.mutable_deploy_rank_table_request();
  deploy_req->set_device_id(0);
  deployer::DeployerResponse response;
  context.DeployRankTable(*deploy_req, response);

  deploy_req->set_rank_table(rank_table);
  context.DeployRankTable(*deploy_req, response);
  MmpaStub::GetInstance().Reset();
  mock_handle = nullptr;
}
}  // namespace ge

