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
#include <cstdlib>

#define protected public
#define private public
#include "deploy/deployer/master_model_deployer.h"
#include "deploy/resource/resource_manager.h"
#include "exec_runtime/execution_runtime.h"
#undef private
#undef protected

#include "proto/deployer.pb.h"
#include "stub_models.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_local_context.h"
#include "graph/utils/graph_utils.h"
#include "graph/manager/graph_var_manager.h"
#include "common/config/configurations.h"
#include "deploy/helper_execution_runtime.h"
#include "framework/common/types.h"
#include "deploy/flowrm/flow_route_manager.h"
#include "hccl/hccl_types.h"
#include "depends/mmpa/src/mmpa_stub.h"

using namespace std;
using namespace ::testing;

namespace ge {
namespace {
void *mock_handle = nullptr;

HcclResult HcomSetRankTable(const string &rankTable) {
  return HCCL_SUCCESS;
}

class MockMmpa : public MmpaStubApi {
 public:
  void *DlSym(void *handle, const char *func_name) override {
    if (std::string(func_name) == "HcomSetRankTable") {
      return (void *) &HcomSetRankTable;
    }
    return dlsym(handle, func_name);
  }

  int32_t RealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen) override {
    strncpy(realPath, path, realPathLen);
    return EN_OK;
  }

  void *DlOpen(const char *fileName, int32_t mode) override {
    return (void *)mock_handle;
  }
  int32_t DlClose(void *handle) override {
    return 0L;
  }
};

class MockLocalDeployer : public LocalDeployer {
 public:
  Status Process(deployer::DeployerRequest &request, deployer::DeployerResponse &response) override {
    return SUCCESS;
  }
};
class MockExchangeService : public ExchangeService {
 public:
  Status CreateQueue(const int32_t device_id,
                     const std::string &name,
                     const uint32_t depth,
                     const uint32_t work_mode,
                     uint32_t &queue_id) override {
    queue_id = queue_id_gen_++;
    return DoCreateQueue();
  }

  MOCK_METHOD0(DoCreateQueue, Status());
  MOCK_METHOD2(DestroyQueue, Status(int32_t, uint32_t));

  Status Enqueue(const int32_t device_id, const uint32_t queue_id, const void *const data, const size_t size) override {
    return SUCCESS;
  }
  Status Enqueue(const int32_t device_id,
                 const uint32_t queue_id,
                 const size_t size,
                 const FillFunc &fill_func) override {
    return SUCCESS;
  }
  Status Peek(const int32_t device_id, const uint32_t queue_id, size_t &size) override {
    return SUCCESS;
  }
  Status Dequeue(const int32_t device_id,
                 const uint32_t queue_id,
                 void *const data,
                 const size_t size,
                 ControlInfo &control_Info) override {
    return SUCCESS;
  }

  Status DequeueTensor(const int32_t device_id,
                       const uint32_t queue_id,
                       GeTensor &tensor,
                       ControlInfo &control_Info) override {
    return 0;
  }

  int queue_id_gen_ = 100;
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
  MasterModelDeployer model_deployer_;
  MockExchangeService exchange_service_;
};

class MockMasterModelDeployer : public MasterModelDeployer {
 public:
  MOCK_CONST_METHOD2(DeployLocalExchangePlan, Status(const deployer::ExchangePlan &, ExchangeRoute &));
  MOCK_METHOD3(DeployRemoteVarManager, Status(const DeployPlan &,
                                              const std::map<int32_t, std::vector<ConstSubmodelInfoPtr>> &,
                                              MasterModelDeployer::DeployedModel &));
};

class MockMasterModelDeployer2 : public MasterModelDeployer {
 public:
  explicit MockMasterModelDeployer2(int32_t mode = 0) : mode_(mode) {}
  Status DeploySubmodels(DeployPlan &deploy_plan, DeployedModel &deployed_model) override {
    // success
    if (mode_ == 0) {
      ExchangeRoute exchange_route;
      exchange_route.endpoints_[0].id = 0;
      exchange_route.endpoints_[0].type = 1;
      exchange_route.endpoints_[1].id = 1;
      exchange_route.endpoints_[1].type = 1;
      exchange_route.endpoints_[7].id = 2;
      exchange_route.endpoints_[7].type = 1;
      FlowRouteManager::GetInstance().AddRoute(exchange_route, deployed_model.route_id);
      return SUCCESS;
    }

    // partial success, then failed
    if (mode_ == 1) {
      deployed_model.deployed_remote_devices.emplace_back(DeployPlan::DeviceInfo{0, 0, 0});
      return FAILED;
    }

    return FAILED;
  }

 private:
  int32_t mode_;
};

Status SendRequestStub(deployer::DeployerRequest &request , deployer::DeployerResponse &response) {
  response.set_error_code(1);
  response.set_error_message("failed");
  return SUCCESS;
}

class MockRemoteDeployer : public RemoteDeployer {
 public:
  explicit MockRemoteDeployer(const NodeConfig &node_config) : RemoteDeployer(node_config) {}
  MOCK_METHOD2(Process, Status(deployer::DeployerRequest & , deployer::DeployerResponse & ));
};
}  // namespace

class MasterModelDeployerTest : public testing::Test {
 protected:
  void SetUp() override {
    SetConfigEnv("valid/host");
    auto &deployer_proxy = DeployerProxy::GetInstance();
    NodeConfig npu_node_1;
    npu_node_1.device_id = 0;
    deployer_proxy.deployers_.emplace_back(MakeUnique<MockLocalDeployer>());
    deployer_proxy.deployers_.emplace_back(MakeUnique<MockRemoteDeployer>(npu_node_1));

    ResourceManager::GetInstance().Initialize();
    ExecutionRuntime::SetExecutionRuntime(std::make_shared<MockExecutionRuntime>());
  }

  void TearDown() override {
    DeployerProxy::GetInstance().Finalize();
    ResourceManager::GetInstance().Finalize();
  }

  static void SetConfigEnv(const std::string &path) {
    std::string config_path = "../tests/ut/ge/runtime/data/"  + path;
    char real_path[1024]{};
    if (realpath(config_path.c_str(), real_path) == nullptr) {
      config_path = "runtime/data/" + path;
      realpath(config_path.c_str(), real_path);
    }

    setenv("HELPER_RES_FILE_PATH", real_path, 1);
    EXPECT_EQ(Configurations::GetInstance().InitHostInformation(config_path), SUCCESS);
  }

  ///    NetOutput (4)
  ///        |
  ///      PC_2 (2)/(3)
  ///        |
  ///      PC_1 (1)/(2)
  ///        |
  ///      data1(0)
  DeployPlan BuildSimpleDeployPlan(int32_t remote_device_id = 1) {
    DeployPlan deploy_plan;
    deploy_plan.queues_.resize(9);
    deploy_plan.queues_[0].device_info = DeployPlan::DeviceInfo(0, 0, -1);
    deploy_plan.queues_[1].device_info = DeployPlan::DeviceInfo(0, remote_device_id, 0);
    deploy_plan.queues_[2].device_info = DeployPlan::DeviceInfo(0, remote_device_id, 0);
    deploy_plan.queues_[3].device_info = DeployPlan::DeviceInfo(0, remote_device_id, 0);
    deploy_plan.queues_[4].device_info = DeployPlan::DeviceInfo(0, 0, -1);
    deploy_plan.queues_[5].device_info = DeployPlan::DeviceInfo(0, 0, -1);
    deploy_plan.groups_[5].emplace_back(1);
    deploy_plan.queues_[6].device_info = DeployPlan::DeviceInfo(0, remote_device_id, 0);
    deploy_plan.groups_[6].emplace_back(0);
    deploy_plan.queues_[7].device_info = DeployPlan::DeviceInfo(0, remote_device_id, 0);
    deploy_plan.groups_[7].emplace_back(4);
    deploy_plan.queues_[8].device_info = DeployPlan::DeviceInfo(0, 0, -1);
    deploy_plan.groups_[8].emplace_back(3);
    deploy_plan.root_model_info_.input_queue_indices.emplace_back(0);
    deploy_plan.root_model_info_.output_queue_indices.emplace_back(4);
    auto &submodel_1 = deploy_plan.submodels_["subgraph-1"];
    submodel_1.input_queue_indices.emplace_back(1);
    submodel_1.output_queue_indices.emplace_back(2);
    submodel_1.device_info = DeployPlan::DeviceInfo(0, remote_device_id, 0);
    submodel_1.model = make_shared<GeRootModel>();
    submodel_1.model->SetRootGraph(std::make_shared<ComputeGraph>("subgraph-1"));
    auto model = make_shared<GeModel>();
    auto model_task_def = make_shared<domi::ModelTaskDef>();
    model_task_def->add_task();
    model->SetModelTaskDef(model_task_def);
    model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(submodel_1.model->GetRootGraph()));
    model->SetWeight(Buffer(512));
    std::dynamic_pointer_cast<GeRootModel>(submodel_1.model)->SetSubgraphInstanceNameToModel("subgraph-1", model);

    auto &submodel_2 = deploy_plan.submodels_["subgraph-2"];
    submodel_2.input_queue_indices.emplace_back(2);
    submodel_2.output_queue_indices.emplace_back(3);
    submodel_2.device_info = DeployPlan::DeviceInfo(0, remote_device_id, 0);
    submodel_2.model = make_shared<GeRootModel>();
    submodel_2.model->SetRootGraph(std::make_shared<ComputeGraph>("subgraph-2"));
    model = make_shared<GeModel>();
    model->SetModelTaskDef(model_task_def);
    model->SetWeight(Buffer(512));
    model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(submodel_2.model->GetRootGraph()));
    std::dynamic_pointer_cast<GeRootModel>(submodel_2.model)->SetSubgraphInstanceNameToModel("subgraph-2",  model);

    deploy_plan.queue_bindings_.emplace_back(0, 1);
    deploy_plan.queue_bindings_.emplace_back(0, 5);
    deploy_plan.queue_bindings_.emplace_back(1, 6);
    deploy_plan.queue_bindings_.emplace_back(3, 4);
    deploy_plan.queue_bindings_.emplace_back(3, 7);
    deploy_plan.queue_bindings_.emplace_back(4, 8);
    return deploy_plan;
  }

  std::vector<DeviceInfo> device_info_list_;
};

TEST_F(MasterModelDeployerTest, TestResolveExchangePlan_IrrelevantDevice) {
  auto deploy_plan = BuildSimpleDeployPlan();
  deployer::ExchangePlan exchange_plan;
  ASSERT_EQ(MasterModelDeployer::ResolveExchangePlan(deploy_plan,
                                                     DeployPlan::DeviceInfo(0, 0, 0),
                                                     exchange_plan),
            SUCCESS);
  int count = 0;
  for (int i = 0; i < exchange_plan.queues_size(); ++i) {
    if (exchange_plan.queues(i).type() >= 0) {
      ++count;
    }
  }
  ASSERT_EQ(count, 0);
  ASSERT_EQ(exchange_plan.bindings_size(), 0);
}

TEST_F(MasterModelDeployerTest, TestResovleExchangePlan_LocalDevice) {
  auto deploy_plan = BuildSimpleDeployPlan();
  deployer::ExchangePlan exchange_plan;
  ASSERT_EQ(MasterModelDeployer::ResolveExchangePlan(deploy_plan,
                                                     DeployPlan::DeviceInfo(0, 0, -1),
                                                     exchange_plan),
            SUCCESS);
  int count = 0;
  for (int i = 0; i < exchange_plan.queues_size(); ++i) {
    if (exchange_plan.queues(i).type() >= 0) {
      ++count;
    }
  }
  ASSERT_EQ(count, 6);
  ASSERT_EQ(exchange_plan.bindings_size(), 2);
}

TEST_F(MasterModelDeployerTest, TestResovleExchangePlan_RemoteDevice) {
  auto deploy_plan = BuildSimpleDeployPlan();
  deployer::ExchangePlan exchange_plan;
  ASSERT_EQ(MasterModelDeployer::ResolveExchangePlan(deploy_plan,
                                                     DeployPlan::DeviceInfo(0, 1, 0),
                                                     exchange_plan),
            SUCCESS);
  ASSERT_EQ(exchange_plan.queues_size(), 9);
  ASSERT_EQ(exchange_plan.bindings_size(), 2);
}

TEST_F(MasterModelDeployerTest, TestDeploySubmodels_Success) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = (void *)0xFFFFFFFF;
  setenv("RANK_ID", "1", 1);
  std::string help_clu = R"({"chief":"192.168.1.38:1"})";
  setenv("HELP_CLUSTER", help_clu.c_str(), 1);
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 1, 0), SUCCESS);
  MasterModelDeployer::DeployedModel deployed_model;
  auto &exchange_service =
      reinterpret_cast<MockExchangeService &>(ExecutionRuntime::GetInstance()->GetExchangeService());
  EXPECT_CALL(exchange_service, DoCreateQueue).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(exchange_service, DestroyQueue).WillRepeatedly(Return(SUCCESS));

  MockMasterModelDeployer model_deployer;
  auto &remote_device =
      reinterpret_cast<MockRemoteDeployer &>(*DeployerProxy::GetInstance().deployers_[1]);
  EXPECT_CALL(remote_device, Process)
      .WillOnce(Return(FAILED))
      .WillOnce(Invoke(SendRequestStub))
      .WillRepeatedly(Return(SUCCESS));

  auto deploy_plan = BuildSimpleDeployPlan();
  EXPECT_CALL(model_deployer, DeployLocalExchangePlan).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(model_deployer, DeployRemoteVarManager).WillRepeatedly(Return(SUCCESS));
  // send request failed
  EXPECT_EQ(model_deployer.DeploySubmodels(deploy_plan, deployed_model), FAILED);

  // check response code failed
  EXPECT_EQ(model_deployer.DeploySubmodels(deploy_plan, deployed_model), FAILED);
  // success
  EXPECT_EQ(model_deployer.DeploySubmodels(deploy_plan, deployed_model), SUCCESS);
  unsetenv("HELP_CLUSTER");
  unsetenv("RANK_ID");
  MmpaStub::GetInstance().Reset();
  mock_handle = nullptr;
}

TEST_F(MasterModelDeployerTest, TestDeployModel_Failed) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  MasterModelDeployer::DeployedModel deployed_model;
  MockMasterModelDeployer2 model_deployer(1);
  DeployResult deploy_result;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  ASSERT_EQ(model_deployer.DeployModel(flow_model, {}, {}, deploy_result), FAILED);
  ASSERT_EQ(model_deployer.deployed_models_.size(), 0);
}

///     NetOutput
///         |
///       PC_3
///      /   \
///    PC_1  PC2
///    |      |
///  data1  data2
TEST_F(MasterModelDeployerTest, TestDeployModel_Success) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  MasterModelDeployer::DeployedModel deployed_model;
  MockMasterModelDeployer2 model_deployer(0);
  DeployResult deploy_result;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  ASSERT_EQ(model_deployer.DeployModel(flow_model, {}, {}, deploy_result), SUCCESS);
  ASSERT_EQ(model_deployer.deployed_models_.size(), 1);
  ASSERT_EQ(model_deployer.Undeploy(deploy_result.model_id), SUCCESS);
  ASSERT_EQ(model_deployer.deployed_models_.size(), 0);
}

TEST_F(MasterModelDeployerTest, TestInitinalize) {
  DeployerProxy::GetInstance().Finalize();
  ResourceManager::GetInstance().Finalize();
  MasterModelDeployer model_deployer;
  SetConfigEnv("empty_device/host");
  ASSERT_EQ(model_deployer.Initialize({}), SUCCESS);
}

TEST_F(MasterModelDeployerTest, TestFinalize) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  MasterModelDeployer::DeployedModel deployed_model;
  MockMasterModelDeployer2 model_deployer(0);
  DeployResult deploy_result;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  ASSERT_EQ(model_deployer.DeployModel(flow_model, {}, {}, deploy_result), SUCCESS);
  ASSERT_EQ(model_deployer.deployed_models_.size(), 1);
  ASSERT_EQ(model_deployer.Finalize(), SUCCESS);
  ASSERT_TRUE(model_deployer.deployed_models_.empty());
}

TEST_F(MasterModelDeployerTest, TestUndeployModel_InvalidModelId) {
  MasterModelDeployer model_deployer;
  ASSERT_EQ(model_deployer.Undeploy(0), PARAM_INVALID);
}

TEST_F(MasterModelDeployerTest, TestDeployRemoteVarManager) {
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 1, 0), SUCCESS);
  auto deploy_plan = BuildSimpleDeployPlan();
  MasterModelDeployer model_deployer;
  auto &remote_device =
      reinterpret_cast<MockRemoteDeployer &>(*DeployerProxy::GetInstance().deployers_[1]);
  EXPECT_CALL(remote_device, Process)
      .WillRepeatedly(Return(SUCCESS));
  MasterModelDeployer::DeployedModel deployed_model;
  std::vector<const DeployPlan::SubmodelInfo *> local_models;
  std::map<std::string, std::vector< const DeployPlan::SubmodelInfo *>> remote_models;
  MasterModelDeployer::GroupSubmodels(deploy_plan, local_models, remote_models);
  ASSERT_EQ(model_deployer.DeployRemoteVarManager(deploy_plan, remote_models, deployed_model), SUCCESS);
}

TEST_F(MasterModelDeployerTest, TestCopyOneWeightToTransfer) {
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 1, 0), SUCCESS);
  MasterModelDeployer::SendInfo send_info{};
  send_info.device_id = 1;
  MasterModelDeployer model_deployer;
  auto &remote_device =
      reinterpret_cast<MockRemoteDeployer &>(*DeployerProxy::GetInstance().deployers_[1]);
  EXPECT_CALL(remote_device, Process)
      .WillRepeatedly(Return(SUCCESS));
  auto op_desc = make_shared<OpDesc>("var_name", FILECONSTANT);
  GeShape shape({16, 16});
  GeTensorDesc tensor_desc(shape, FORMAT_ND, DT_INT16);
  op_desc->AddOutputDesc(tensor_desc);
  std::vector<int16_t> tensor(16 * 16);
  auto size = 16 * 16 * 2;
  EXPECT_EQ(VarManager::Instance(0)->AssignVarMem("var_name", tensor_desc, RT_MEMORY_HBM), SUCCESS);
  std::string buffer(reinterpret_cast<char *>(tensor.data()), size);
  std::stringstream ss(buffer);
  EXPECT_EQ(model_deployer.CopyOneWeightToTransfer(send_info, ss, size, op_desc), SUCCESS);
}

TEST_F(MasterModelDeployerTest, TestGetVarManagerAndSendToRemote) {
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 1, 0), SUCCESS);
  MasterModelDeployer::SendInfo send_info{};
  send_info.device_id = 1;
  MasterModelDeployer model_deployer;
  auto &remote_device =
      reinterpret_cast<MockRemoteDeployer &>(*DeployerProxy::GetInstance().deployers_[1]);
  EXPECT_CALL(remote_device, Process)
      .WillRepeatedly(Return(SUCCESS));
  auto op_desc = make_shared<OpDesc>("var_name", FILECONSTANT);
  GeShape shape({16, 16});
  GeTensorDesc tensor_desc(shape, FORMAT_ND, DT_INT16);
  op_desc->AddOutputDesc(tensor_desc);
  std::vector<int16_t> tensor(16 * 16);
  auto size = 16 * 16 * 2;
  EXPECT_EQ(VarManager::Instance(0)->AssignVarMem("var_name", tensor_desc, RT_MEMORY_HBM), SUCCESS);
  std::string buffer(reinterpret_cast<char *>(tensor.data()), size);
  std::stringstream ss(buffer);

  std::map<std::string, std::string> options;
  options["ge.exec.value_bins"] =
      R"({"value_bins":[{"value_bin_id":"vector_search_buchet_value_bin", "value_bin_file":"hello.bin"}]})";
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_TRUE(AttrUtils::SetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, "vector_search_buchet_value_bin"));

  std::map<int32_t, std::set<int32_t>> sub_device_ids{{0, {0, 1}}};
  std::map<int32_t, std::set<uint64_t>> sessions{{0, {0}}};
  std::map<int32_t, std::map<uint64_t, std::set<OpDescPtr>>> node_need_transfer_memory;
  std::map<uint64_t, std::set<OpDescPtr>> sess_map;
  std::set<OpDescPtr> op_desc_set;
  op_desc_set.insert(op_desc);
  sess_map.insert({0, op_desc_set});
  node_need_transfer_memory.insert({0, sess_map});
  ASSERT_NE(model_deployer.GetVarManagerAndSendToRemote(sub_device_ids, sessions, node_need_transfer_memory), SUCCESS);
}

TEST_F(MasterModelDeployerTest, TestHelperExecutionRuntimeFailed_config_env_not_set) {
  unsetenv("HELPER_RES_FILE_PATH");
  std::map<std::string, std::string> options;
  EXPECT_EQ(InitializeHelperRuntime(options), FAILED);
}

TEST_F(MasterModelDeployerTest, TestHelperExecutionRuntimeSuccess) {
  SetConfigEnv("empty_device/host");
  std::map<std::string, std::string> options;
  EXPECT_EQ(InitializeHelperRuntime(options), SUCCESS);
}

TEST_F(MasterModelDeployerTest, TestDeployLocalSubmodels) {
  DeployPlan deploy_plan = BuildSimpleDeployPlan(0);
  std::vector<const DeployPlan::SubmodelInfo *> local_models;
  std::map<std::string, std::vector<const DeployPlan::SubmodelInfo *>> remote_models;
  MasterModelDeployer::GroupSubmodels(deploy_plan, local_models, remote_models);
  MasterModelDeployer::DeployedModel deployed_model;
  MasterModelDeployer model_deployer;
  EXPECT_EQ(model_deployer.DeployLocalSubmodels(local_models, deployed_model), SUCCESS);
}

TEST_F(MasterModelDeployerTest, TestDeployRankSizeSuccess) {
  MasterModelDeployer model_deployer;
  EXPECT_EQ(model_deployer.MasterDeployRankSize(), FAILED);
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  mock_handle = nullptr;
  EXPECT_EQ(model_deployer.MasterDeployRankSize(), INTERNAL_ERROR);
  mock_handle = (void *)0xFFFFFFFF;
  EXPECT_EQ(model_deployer.MasterDeployRankSize(), SUCCESS);
  MmpaStub::GetInstance().Reset();
  mock_handle = nullptr;
}
}  // namespace ge
