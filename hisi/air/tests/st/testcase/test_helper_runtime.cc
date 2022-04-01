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
#include <stdlib.h>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "generator/ge_generator.h"
#include "daemon/grpc_server.h"
#include "common/config/json_parser.h"
#include "common/profiling/profiling_properties.h"
#include "framework/common/debug/ge_log.h"
#include "ge/ge_api_error_codes.h"
#include "graph/ge_context.h"
#include "third_party/inc/runtime/rt_mem_queue.h"
#define protected public
#define private public
#include "executor/hybrid/node_executor/node_executor.h"
#include "common/mem_grp/memory_group_manager.h"
#include "deploy/flowrm/datagw_manager.h"
#include "deploy/flowrm/network_manager.h"
#include "deploy/hcom/communication_domain.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/utils/tensor_utils.h"
#include "deploy/deployer/helper_deploy_planner.h"
#include "deploy/deployer/master_model_deployer.h"
#include "daemon/deployer_daemon_client.h"
#undef private
#undef protected

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "graph/ge_local_context.h"
#include "proto/deployer.pb.h"
#include "common/data_flow/queue/helper_exchange_service.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "graph/debug/ge_attr_define.h"
#include "exec_runtime/execution_runtime.h"
#include "depends/mmpa/src/mmpa_stub.h"
#include "external/ge/ge_api.h"
#include "graph/utils/graph_utils.h"

#include "deploy/helper_execution_runtime.h"
#include "executor/cpu_sched_event_dispatcher.h"
#include "depends/mmpa/src/mmpa_stub.h"
#include "depends/runtime/src/runtime_stub.h"
#include "init_ge.h"


using namespace std;
using namespace ::testing;
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

vector<int8_t> placeholder(224U * 224U * sizeof(int64_t) * 10);

rtError_t rtMbufAlloc(rtMbufPtr_t *mbuf, uint64_t size) {
  *mbuf = placeholder.data();
  return 0;
}

rtError_t rtMbufFree(rtMbufPtr_t mbuf) {
  return 0;
}

rtError_t rtMbufGetBuffAddr(rtMbufPtr_t mbuf, void **databuf) {
  *databuf = mbuf;
  return 0;
}

rtError_t rtMbufGetPrivInfo(rtMbufPtr_t mbuf, void **priv, uint64_t *size) {
  *priv = mbuf;
  return 0;
}

rtError_t rtMemQueueEnQueue(int32_t devId, uint32_t qid, void *mbuf) {
  return 0;
}

rtError_t rtMemQueueDeQueue(int32_t devId, uint32_t qid, void **mbuf) {
  *mbuf = placeholder.data();
  return 0;
}

rtError_t rtMbufGetBuffSize(rtMbufPtr_t mbuf, uint64_t *size) {
  *size = placeholder.size();
  return 0;
}

#define RT_QUEUE_MODE_PUSH 1

namespace ge {
void *mock_handle = nullptr;
void *mock_method = nullptr;

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

class ModelDeployerMock : public ModelDeployer {
 public:
  Status DeployModel(const FlowModelPtr &flow_model,
                     const vector<uint32_t> &input_queue_ids, const vector<uint32_t> &output_queue_ids,
                     DeployResult &deploy_result) override {
    deploy_result.input_queue_ids = {1, 2, 3};
    deploy_result.output_queue_ids = {4};
    uint32_t input0_qid = 1;
    ExecutionRuntime::GetInstance()->GetExchangeService().CreateQueue(0, "input0", 2, 0, input0_qid);
    uint32_t input1_qid = 2;
    ExecutionRuntime::GetInstance()->GetExchangeService().CreateQueue(0, "input1", 2, 0, input1_qid);
    uint32_t input2_qid = 3;
    ExecutionRuntime::GetInstance()->GetExchangeService().CreateQueue(0, "input2", 2, 0, input2_qid);
    return SUCCESS;
  }

  Status Undeploy(uint32_t model_id) override {
    return SUCCESS;
  }
};

class MockRemoteDeployer : public RemoteDeployer {
 public:
  explicit MockRemoteDeployer(const NodeConfig &node_config) : RemoteDeployer(node_config) {}
  MOCK_METHOD2(Process, Status(deployer::DeployerRequest & , deployer::DeployerResponse & ));
};

class ModelDeployerMock2 : public ModelDeployer {
 public:
  Status DeployModel(const FlowModelPtr &flow_model,
                     const vector<uint32_t> &input_queue_ids, const vector<uint32_t> &output_queue_ids,
                     DeployResult &deploy_result) override {
    deploy_result.input_queue_ids = {};
    deploy_result.output_queue_ids = {1};

    NodeConfig npu_node_1;
    npu_node_1.device_id = 0;

    auto &deployer_proxy = DeployerProxy::GetInstance();
    deployer_proxy.deployers_.emplace_back(MakeUnique<LocalDeployer>());
    deployer_proxy.deployers_.emplace_back(MakeUnique<MockRemoteDeployer>(npu_node_1));

    auto &remote_device =
        reinterpret_cast<MockRemoteDeployer &>(*deployer_proxy.deployers_[1]);
    EXPECT_CALL(remote_device, Process).WillRepeatedly(Return(SUCCESS));

    EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 1, 0), SUCCESS);
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
    master_deployer_.GetVarManagerAndSendToRemote(sub_device_ids, sessions, node_need_transfer_memory);
    return SUCCESS;
  }

  Status Undeploy(uint32_t model_id) override {
    return SUCCESS;
  }
 public:
  MasterModelDeployer master_deployer_;
};

class ExecutionRuntimeHelperMock : public ExecutionRuntime {
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
  HelperExchangeService exchange_service_;
  ModelDeployerMock model_deployer_;
};

class ExecutionRuntimeHelperMock2 : public ExecutionRuntime {
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
  HelperExchangeService exchange_service_;
  ModelDeployerMock2 model_deployer_;
};

Status MockInitializeHelperRuntime(const std::map<std::string, std::string> &options) {
  ExecutionRuntime::SetExecutionRuntime(std::make_shared<ExecutionRuntimeHelperMock>());
  return SUCCESS;
}

Status MockInitializeHelperRuntime2(const std::map<std::string, std::string> &options) {
  ExecutionRuntime::SetExecutionRuntime(std::make_shared<ExecutionRuntimeHelperMock2>());
  return SUCCESS;
}

class STEST_helper_runtime : public testing::Test {
 protected:
  void SetUp() {
    hybrid::NodeExecutorManager::GetInstance().
        engine_mapping_.emplace("AiCoreLib", hybrid::NodeExecutorManager::ExecutorType::AICORE);
    hybrid::NodeExecutorManager::GetInstance().
        engine_mapping_.emplace("AicpuLib", hybrid::NodeExecutorManager::ExecutorType::AICPU_CUSTOM);
    MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  }
  void TearDown() {
    ExecutionRuntime::FinalizeExecutionRuntime();
    MmpaStub::GetInstance().Reset();
    mock_handle = nullptr;
    mock_method = nullptr;
  }
};

class MockDataGwManager : public DataGwManager {
 public:
  MockDataGwManager() : DataGwManager() {}
  MOCK_METHOD2(execv, int64_t(const char *pathname, char *const argv[]));
  MOCK_METHOD4(InitHostDataGwServer, ge::Status(const uint32_t, uint32_t, const std::string &, pid_t &));
  MOCK_METHOD3(InitDeviceDataGwServer, ge::Status(const uint32_t, const std::string &, pid_t &));
};

const char *config_path = "HELPER_RES_FILE_PATH";
using RemoteDeployerPtr = std::shared_ptr<RemoteDeployer>;

static void StartServer(ge::GrpcServer &grpc_server) {
  auto res = grpc_server.Run();
  if (res != ge::SUCCESS) {
    std::cout<<"run failed"<<std::endl;
    return;
  }
}

static void SetDeviceEnv() {
  const char *path = "../tests/st/config_file/right_json/device";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitDeviceInformation(real_path);
}

static void BuildModel(ModelBufferData &model_buffer_data) {
//  dlog_setlevel(GE_MODULE_NAME, DLOG_DEBUG, 1);
  vector<std::string> engine_list = {"AIcoreEngine"};
  auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto add2 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
  auto data1 = OP_CFG(DATA);
  auto data2 = OP_CFG(DATA);
  auto print = OP_CFG("Print");
  DEF_GRAPH(g1) {
    CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
    CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("add_2", add2));
    CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_2", add2));
    CHAIN(NODE("add_2", add2)->EDGE(0, 0)->NODE("Print", print));
  };

  auto graph = ToGeGraph(g1);

  GeGenerator ge_generator;
  std::map<std::string, std::string> options;
  options.emplace(ge::SOC_VERSION, "Ascend910");
  ge_generator.Initialize(options);

  auto ret = ge_generator.GenerateOnlineModel(graph, {}, model_buffer_data);
  EXPECT_EQ(ret, SUCCESS);
}

static ge::Status CreateQueue(uint32_t device_id, uint32_t &qid) {
  rtMemQueueAttr_t queue_attr = {0};
  queue_attr.depth = 100;
  queue_attr.workMode = RT_QUEUE_MODE_PUSH;
  queue_attr.flowCtrlFlag = 1;
  queue_attr.overWriteFlag = 0;
  queue_attr.flowCtrlDropTime = 1;

  uint32_t queue_id = 0;
  rtError_t rt_ret = rtMemQueueCreate(device_id, &queue_attr, &queue_id);
  if (rt_ret != RT_ERROR_NONE) {
    printf("create queue failed. \n");
    return ge::FAILED;
  }
  qid = queue_id;
  return ge::SUCCESS;
}

static void SendPreDeployRequest(std::vector<RemoteDeployerPtr> &clients) {
  deployer::DeployerResponse response;
  deployer::DeployerRequest pre_deploy_request;
  pre_deploy_request.set_type(deployer::kPreDeployModel);
  auto pre_deploy_req_body = pre_deploy_request.mutable_pre_deploy_model_request();
  auto exchange_plan = pre_deploy_req_body->mutable_exchange_plan();
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

  auto submodel_desc = pre_deploy_req_body->add_submodels();
  submodel_desc->set_model_size(128);
  submodel_desc->set_model_name("any-model");
  submodel_desc->add_input_queue_indices(1);
  submodel_desc->add_output_queue_indices(2);

  for (auto &client : clients) {
    ASSERT_EQ(client->Process(pre_deploy_request, response), ge::SUCCESS);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }
}

TEST_F(STEST_helper_runtime, send_quest_success) {
  const char *path = "../tests/st/config_file/right_json/device";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitDeviceInformation(real_path);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });

  sleep(1);
  path = "../tests/st/config_file/right_json/host";
  realpath(path, real_path);
  Configurations::GetInstance().InitHostInformation(real_path);
  std::vector<RemoteDeployerPtr> clients;
  for (const auto &node_config : Configurations::GetInstance().GetHostInformation().remote_node_config_list) {
    RemoteDeployerPtr ptr(new RemoteDeployer(node_config));
    ASSERT_EQ(ptr->Initialize(), ge::SUCCESS);
    clients.emplace_back(ptr);
  }

  SendPreDeployRequest(clients);

  ModelBufferData model_buffer_data{};
  BuildModel(model_buffer_data);
  deployer::DeployerResponse response;
  deployer::DeployerRequest pre_download_request;
  pre_download_request.set_type(deployer::kPreDownloadModel);
  auto request1 = pre_download_request.mutable_pre_download_request();
  request1->set_model_id(0);
  request1->set_total_size(model_buffer_data.length);
  for (auto &client : clients) {
    ASSERT_EQ(client->Process(pre_download_request, response), ge::SUCCESS);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }

  const char *content = "helper";
  deployer::DeployerRequest download_request;
  download_request.set_type(deployer::kDownloadModel);
  auto request2 = download_request.mutable_download_model_request();
  request2->set_model_id(0);
  request2->set_offset(0);
  request2->set_om_content(model_buffer_data.data.get(), model_buffer_data.length);
  for (auto &client : clients) {
    ASSERT_EQ(client->Process(download_request, response), ge::SUCCESS);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }

  deployer::DeployerRequest load_model_request;
  load_model_request.set_type(deployer::kLoadModel);
  auto request3 = load_model_request.mutable_load_model_request();
  request3->set_model_id(0);
  for (auto &client : clients) {
    ASSERT_EQ(client->Process(load_model_request, response), ge::SUCCESS);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }

  deployer::DeployerRequest unload_model_request;
  unload_model_request.set_type(deployer::kUnloadModel);
  auto request4 = unload_model_request.mutable_unload_model_request();
  request4->set_model_id(0);
  for (auto &client : clients) {
    ASSERT_EQ(client->Process(unload_model_request, response), ge::SUCCESS);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }
  sleep(10);
  for (auto &client : clients) {
    ASSERT_EQ(client->Finalize(), ge::SUCCESS);
  }

  grpc_server.Finalize();
  server_thread.join();
}

void StartClient() {
  std::vector<RemoteDeployerPtr> clients;
  for (const auto &node_config : Configurations::GetInstance().GetHostInformation().remote_node_config_list) {
    RemoteDeployerPtr ptr(new RemoteDeployer(node_config));
    ASSERT_EQ(ptr->Initialize(), ge::SUCCESS);
    clients.emplace_back(ptr);
  }

  SendPreDeployRequest(clients);

  ModelBufferData model_buffer_data{};
  BuildModel(model_buffer_data);
  deployer::DeployerResponse response;
  deployer::DeployerRequest pre_download_request;
  pre_download_request.set_type(deployer::kPreDownloadModel);
  auto request1 = pre_download_request.mutable_pre_download_request();
  request1->set_model_id(0);
  request1->set_total_size(model_buffer_data.length);
  for (auto &client : clients) {
    // ASSERT_EQ(client->SendRequest(pre_download_request, response), ge::SUCCESS);
    client->Process(pre_download_request, response);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }

  deployer::DeployerRequest download_request;
  download_request.set_type(deployer::kDownloadModel);
  auto request2 = download_request.mutable_download_model_request();
  request2->set_model_id(0);
  request2->set_offset(0);
  request2->set_om_content(model_buffer_data.data.get(), model_buffer_data.length);
  for (auto &client : clients) {
    // ASSERT_EQ(client->SendRequest(download_request, response), ge::SUCCESS);
    client->Process(download_request, response);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }

  deployer::DeployerRequest load_model_request;
  load_model_request.set_type(deployer::kLoadModel);
  auto request3 = load_model_request.mutable_load_model_request();
  request3->set_model_id(0);
  for (auto &client : clients) {
    // ASSERT_EQ(client->SendRequest(load_model_request, response), ge::SUCCESS);
    client->Process(load_model_request, response);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }

  deployer::DeployerRequest unload_model_request;
  unload_model_request.set_type(deployer::kUnloadModel);
  auto request4 = unload_model_request.mutable_unload_model_request();
  request4->set_model_id(0);
  for (auto &client : clients) {
    // ASSERT_EQ(client->SendRequest(unload_model_request, response), ge::SUCCESS);
    client->Process(unload_model_request, response);
    // ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }
  for (auto &client : clients) {
    ASSERT_EQ(client->Finalize(), ge::SUCCESS);
  }
}


TEST_F(STEST_helper_runtime, multi_thread_send_quest_success) {
  Configurations::GetInstance().InitHostInformation("../tests/st/config_file/right_json/host");
  Configurations::GetInstance().InitDeviceInformation("../tests/st/config_file/right_json/device");
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });
  sleep(1);
  std::thread client_thread[10];
  const auto &all_options = GetThreadLocalContext().GetAllOptions();
  for (int i = 0; i < 10; i++) {
    client_thread[i] = std::thread([&]() {
      GetThreadLocalContext().SetGlobalOption(all_options);
      StartClient();
    });
  }
  for (int i = 0 ;i < 10; i++) {
    client_thread[i].join();
  }
  grpc_server.Finalize();
  server_thread.join();
}

TEST_F(STEST_helper_runtime, json_parser_fail) {
  ASSERT_NE(Configurations::GetInstance().InitHostInformation(""), ge::SUCCESS);
  const char *path = "../tests/st/config_file/wrong_json/value_fail/host";
  ASSERT_NE(Configurations::GetInstance().InitHostInformation(path), ge::SUCCESS);
  ASSERT_NE(Configurations::GetInstance().InitDeviceInformation(path), ge::SUCCESS);
}

TEST_F(STEST_helper_runtime, check_token_fail) {
  const char *path = "../tests/st/config_file/wrong_json/token_fail/device";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitDeviceInformation(real_path);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });

  sleep(1);
  path = "../tests/st/config_file/wrong_json/token_fail/host";
  realpath(path, real_path);
  setenv(config_path, real_path, 1);
  Configurations::GetInstance().InitHostInformation(real_path);
  std::vector<RemoteDeployerPtr> clients;
  for (const auto &node_config : Configurations::GetInstance().GetHostInformation().remote_node_config_list) {
    RemoteDeployerPtr ptr(new RemoteDeployer(node_config));
    ASSERT_NE(ptr->Initialize(), ge::SUCCESS);
  }
  grpc_server.Finalize();
  server_thread.join();
}

TEST_F(STEST_helper_runtime, server_same_port_fail) {
  const char *path = "../tests/st/config_file/right_json/device";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitDeviceInformation(real_path);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });

  ge::GrpcServer grpc_server1;
  std::thread server_thread1 = std::thread([&]() {
    StartServer(grpc_server1);
  });

  sleep(1);
  grpc_server.Finalize();
  grpc_server1.Finalize();
  server_thread.join();
  server_thread1.join();
}

TEST_F(STEST_helper_runtime, parser_wrong_ip_fail) {
  const char *path = "../tests/st/config_file/wrong_json/ip_fail/device";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitDeviceInformation(real_path);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });

  sleep(1);
  grpc_server.Finalize();
  server_thread.join();
}

TEST_F(STEST_helper_runtime, send_model_fail) {
  const char *path = "../tests/st/config_file/right_json/device";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitDeviceInformation(real_path);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });

  sleep(1);

  path = "../tests/st/config_file/right_json/host";
  realpath(path, real_path);
  Configurations::GetInstance().InitHostInformation(real_path);
  std::vector<RemoteDeployerPtr> clients;
  for (const auto &node_config : Configurations::GetInstance().GetHostInformation().remote_node_config_list) {
    RemoteDeployerPtr ptr(new RemoteDeployer(node_config));
    ASSERT_EQ(ptr->Initialize(), ge::SUCCESS);
    clients.emplace_back(ptr);
  }

  deployer::DeployerResponse response;
  deployer::DeployerRequest pre_download_request;
  pre_download_request.set_type(deployer::kPreDownloadModel);
  auto request1 = pre_download_request.mutable_pre_download_request();
  request1->set_model_id(0);
  request1->set_total_size(14);
  for (auto &client : clients) {
    ASSERT_EQ(client->Process(pre_download_request, response), ge::SUCCESS);
    ASSERT_EQ(response.error_code(), ge::SUCCESS);
  }

  std::string content = "helper_runtime";
  deployer::DeployerRequest download_request;
  download_request.set_type(deployer::kDownloadModel);
  auto request2 = download_request.mutable_download_model_request();
  request2->set_model_id(0);
  request2->set_offset(0);
  request2->set_om_content(content.substr(0, 5));
  for (auto &client : clients) {
    ASSERT_EQ(client->Process(download_request, response), ge::SUCCESS);
    ASSERT_NE(response.error_code(), ge::SUCCESS);  // less than[sizeof(ModelFileHeader)]
  }
  request2->set_offset(5);
  request2->set_om_content(content);
  for (auto &client : clients) {
    ASSERT_EQ(client->Process(download_request, response), ge::SUCCESS);
    ASSERT_NE(response.error_code(), ge::SUCCESS);
  }

  for (auto &client : clients) {
    ASSERT_EQ(client->Finalize(), ge::SUCCESS);
  }
  grpc_server.Finalize();
  server_thread.join();
}

TEST_F(STEST_helper_runtime, InitHostDatagw) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGroupInit(group_name);
  uint32_t device_id = 0;
  uint32_t vf_id = 0;
  pid_t dgw_pid = 0;
  ge::DataGwManager::GetInstance().InitHostDataGwServer(device_id, vf_id, group_name, dgw_pid);
}

TEST_F(STEST_helper_runtime, Init_GetIp_And_Port_Success) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGroupInit(group_name);
  uint32_t device_id = 0;
  uint32_t vf_id = 0;
  pid_t dgw_pid = 0;
  ge::DataGwManager::GetInstance().InitQueue(device_id);
  MockDataGwManager mock;
  EXPECT_CALL(mock, InitHostDataGwServer).WillRepeatedly(Return(ge::SUCCESS));

  const char *path = "../tests/st/config_file/right_json/host";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitHostInformation(real_path);
  ge::Status res = ge::NetworkManager::GetInstance().Initialize();
  ASSERT_EQ(res, ge::SUCCESS);
  std::string ip = "";
  ge::NetworkManager::GetInstance().GetDataPanelIp(ip);
  printf("get ip = %s \n", ip.c_str());

  int32_t port = ge::NetworkManager::GetInstance().GetDataPanelPort();
  printf("get port %d \n", port);
}

TEST_F(STEST_helper_runtime, Init_Host_Bind_Queue_And_Tag_Success) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGroupInit(group_name);
  uint32_t device_id = 0;
  uint32_t vf_id = 0;
  pid_t dgw_pid = 0;
  ge::Status res = ge::SUCCESS;

  res = ge::DataGwManager::GetInstance().InitQueue(device_id);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("InitQueue failed \n");
  }
  MockDataGwManager mock;
  EXPECT_CALL(mock, InitHostDataGwServer).WillRepeatedly(Return(ge::SUCCESS));
  printf("dgw pid=%d \n", dgw_pid);
  dgw_pid = 9900;
  res = ge::DataGwManager::GetInstance().InitDataGwClient(dgw_pid, device_id);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("InitDataGwClient failed \n");
  }
  const char *path = "../tests/st/config_file/right_json/host";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitHostInformation(real_path);
  res = ge::NetworkManager::GetInstance().Initialize();
  if (res != ge::SUCCESS) {
    printf("Initialize failed \n");
  }
  ASSERT_EQ(res, ge::SUCCESS);
  std::string master_ip = "";
  ge::NetworkManager::GetInstance().GetDataPanelIp(master_ip);
  printf("get ip = %s \n", master_ip.c_str());

  int32_t port = ge::NetworkManager::GetInstance().GetDataPanelPort();
  printf("get port %d \n", port);
  uint32_t qid = 0;
  CreateQueue(device_id, qid);
  uint64_t hcom_handle = 0;
  int32_t hcom_tag = 0;
  bqs::CreateHcomInfo hcom_info;
  strcpy(hcom_info.masterIp, master_ip.c_str());
  hcom_info.masterPort = port;

  std::string local_ip = "127.0.0.1";
  strcpy(hcom_info.localIp, local_ip.c_str());
  hcom_info.localPort = 1;

  std::string remote_ip = "127.0.0.1";
  strcpy(hcom_info.remoteIp, remote_ip.c_str());
  hcom_info.remotePort = 2;
  res = ge::DataGwManager::GetInstance().CreateDataGwHandle(device_id, hcom_info);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("CreateDataGwHandle failed \n");
  }

  res = ge::DataGwManager::GetInstance().CreateDataGwTag(hcom_info.handle, "tag_name", device_id, hcom_tag);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("CreateDataGwTag failed \n");
  }

  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::QUEUE;
  queue_route.src.attr.queueAttr.queueId = qid;
  queue_route.dst.type = bqs::EndpointType::NAMED_COMM_CHANNEL;
  strcpy(queue_route.dst.attr.namedChannelAttr.localIp, local_ip.c_str());
  queue_route.dst.attr.namedChannelAttr.localPort = 1;
  strcpy(queue_route.dst.attr.namedChannelAttr.peerIp, remote_ip.c_str());
  queue_route.dst.attr.namedChannelAttr.peerPort = 2;
  strcpy(queue_route.dst.attr.namedChannelAttr.name, "tag_name");
  std::vector<bqs::Route> routes{queue_route};
  res = ge::DataGwManager::GetInstance().BindQueues(device_id, routes);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("BindQueues failed \n");
  }

  res = ge::DataGwManager::GetInstance().UnbindQueues(device_id, queue_route);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("UnbindQueues failed \n");
  }

  res = ge::DataGwManager::GetInstance().DestroyDataGwHandle(device_id, hcom_info.handle);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("DestroyDataGwHandle failed \n");
  }

  res = ge::DataGwManager::GetInstance().Shutdown();
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("Shutdown failed \n");
  }

  res = ge::DataGwManager::GetInstance().DestroyDataGwTag(device_id, hcom_info.handle, hcom_tag);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("DestroyDataGwTag failed \n");
  }

  res = ge::NetworkManager::GetInstance().Finalize();
}

TEST_F(STEST_helper_runtime, Init_Host_Bind_Tag_And_Queue_Success) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGroupInit(group_name);
  uint32_t device_id = 0;
  uint32_t vf_id = 0;
  pid_t dgw_pid = 0;
  ge::Status res = ge::SUCCESS;

  res = ge::DataGwManager::GetInstance().InitQueue(device_id);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("InitQueue failed \n");
  }
  MockDataGwManager mock;
  EXPECT_CALL(mock, InitHostDataGwServer).WillRepeatedly(Return(ge::SUCCESS));
  printf("dgw pid=%d \n", dgw_pid);
  dgw_pid = 9900;
  res = ge::DataGwManager::GetInstance().InitDataGwClient(dgw_pid, device_id);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("InitDataGwClient failed \n");
  }
  const char *path = "../tests/st/config_file/right_json/host";
  char real_path[200];
  realpath(path, real_path);
  Configurations::GetInstance().InitHostInformation(real_path);
  res = ge::NetworkManager::GetInstance().Initialize();
  if (res != ge::SUCCESS) {
    printf("Initialize failed \n");
  }
  ASSERT_EQ(res, ge::SUCCESS);

  std::string master_ip = "";
  ge::NetworkManager::GetInstance().GetDataPanelIp(master_ip);
  printf("get ip = %s \n", master_ip.c_str());

  int32_t port = ge::NetworkManager::GetInstance().GetDataPanelPort();
  printf("get port %d \n", port);
  uint32_t qid = 0;
  CreateQueue(device_id, qid);
  uint64_t hcom_handle = 0;
  int32_t hcom_tag = 0;
  bqs::CreateHcomInfo hcom_info;
  strcpy(hcom_info.masterIp, master_ip.c_str());
  hcom_info.masterPort = port;

  std::string local_ip = "127.0.0.1";
  strcpy(hcom_info.localIp, local_ip.c_str());
  hcom_info.localPort = 1;

  std::string remote_ip = "127.0.0.1";
  strcpy(hcom_info.remoteIp, remote_ip.c_str());
  hcom_info.remotePort = 2;
  res = ge::DataGwManager::GetInstance().CreateDataGwHandle(device_id, hcom_info);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("CreateDataGwHandle failed \n");
  }

  res = ge::DataGwManager::GetInstance().CreateDataGwTag(hcom_info.handle, "tag_name", device_id, hcom_tag);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("CreateDataGwTag failed \n");
  }

  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::NAMED_COMM_CHANNEL;
  strcpy(queue_route.src.attr.namedChannelAttr.localIp, local_ip.c_str());
  queue_route.src.attr.namedChannelAttr.localPort = 1;
  strcpy(queue_route.src.attr.namedChannelAttr.peerIp, remote_ip.c_str());
  queue_route.src.attr.namedChannelAttr.peerPort = 2;
  strcpy(queue_route.src.attr.namedChannelAttr.name, "tag_name");
  queue_route.dst.type = bqs::EndpointType::QUEUE;
  queue_route.dst.attr.queueAttr.queueId = qid;
  std::vector<bqs::Route> routes{queue_route};
  res = ge::DataGwManager::GetInstance().BindQueues(device_id, routes);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("BindQueues failed \n");
  }

  res = ge::DataGwManager::GetInstance().UnbindQueues(device_id, queue_route);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("UnbindQueues failed \n");
  }

  res = ge::DataGwManager::GetInstance().DestroyDataGwHandle(device_id, hcom_info.handle);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("DestroyDataGwHandle failed \n");
  }

  res = ge::DataGwManager::GetInstance().Shutdown();
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("Shutdown failed \n");
  }

  res = ge::DataGwManager::GetInstance().DestroyDataGwTag(device_id, hcom_info.handle, hcom_tag);
  ASSERT_EQ(res, ge::SUCCESS);
  if (res != ge::SUCCESS) {
    printf("DestroyDataGwTag failed \n");
  }

  res = ge::NetworkManager::GetInstance().Finalize();
}


TEST_F(STEST_helper_runtime, test_var_manager_serial_deserial) {
  const map<string, string> options{{"ge.graphMemoryMaxSize", "536870912"}};
  Status ret = VarManager::Instance(1)->Init(0x5a, 1, 0, 0x5a5a);
  ret = VarManager::Instance(1)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);
  size_t graph_mem_max_size = VarManager::Instance(1)->graph_mem_max_size_;
  size_t var_mem_max_size = VarManager::Instance(1)->var_mem_max_size_;
  size_t var_mem_logic_base = VarManager::Instance(1)->var_mem_logic_base_;
  size_t use_max_mem_size = VarManager::Instance(1)->use_max_mem_size_;
  std::vector<int64_t> s = {1,2,3,4};
  GeShape shape(s);
  GeTensorDesc tensor_desc(shape);
  TensorUtils::SetSize(tensor_desc, shape.GetShapeSize());
  std::string str = "global_step";
  ret = VarManager::Instance(1)->AssignVarMem(str, tensor_desc, RT_MEMORY_HBM);
  EXPECT_EQ(ret, SUCCESS);
  TransNodeInfo trans_node_info;
  VarTransRoad fusion_road;
  fusion_road.emplace_back(trans_node_info);
  VarManager::Instance(1)->SetTransRoad(str, fusion_road);

  VarBroadCastInfo broadcast_info;
  broadcast_info.var_name = "test";
  VarManager::Instance(1)->SaveBroadCastInfo(0, broadcast_info);

  deployer::VarManagerInfo info;
  ret = VarManager::Instance(1)->VarManagerToSerial(1, info);
  EXPECT_EQ(ret, SUCCESS);
  auto session_id = info.session_id();
  EXPECT_EQ(session_id, 1);

  ret = VarManager::Instance(1)->VarManagerToDeserial(1, info);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(VarManager::Instance(1)->graph_mem_max_size_, floor(1024UL * 1024UL * 1024UL / 2));
  EXPECT_EQ(VarManager::Instance(1)->var_mem_max_size_, floor(1024UL * 1024UL * 1024UL * (5.0f / 32.0f)));
  EXPECT_EQ(VarManager::Instance(1)->version_, 0x5a);
  EXPECT_EQ(VarManager::Instance(1)->device_id_, 0);
  EXPECT_EQ(VarManager::Instance(1)->job_id_, 0x5a5a);
  EXPECT_EQ(VarManager::Instance(1)->graph_mem_max_size_, graph_mem_max_size);
  EXPECT_EQ(VarManager::Instance(1)->var_mem_max_size_, var_mem_max_size);
  EXPECT_EQ(VarManager::Instance(1)->var_mem_logic_base_, var_mem_logic_base);
  EXPECT_EQ(VarManager::Instance(1)->use_max_mem_size_, use_max_mem_size);
  EXPECT_EQ(VarManager::Instance(1)->var_resource_->session_id_, 1);

  EXPECT_EQ(VarManager::Instance(1)->var_resource_->var_offset_map_.size(), 1);
  EXPECT_EQ(VarManager::Instance(1)->var_resource_->var_addr_mgr_map_.size(), 1);
  EXPECT_EQ(VarManager::Instance(1)->var_resource_->cur_var_tensor_desc_map_.size(), 1);

  EXPECT_EQ(VarManager::Instance(1)->var_resource_->IsVarExist(str, tensor_desc), true);
  uintptr_t ptr = reinterpret_cast<uintptr_t>(VarManager::Instance(1)->GetVarMemoryBase(RT_MEMORY_HBM, 0));
  EXPECT_EQ(ptr, 0);
  EXPECT_EQ(VarManager::Instance(1)->mem_resource_map_.size(), 1);
  auto resource_src = VarManager::Instance(1)->mem_resource_map_[RT_MEMORY_HBM];
  auto resource = VarManager::Instance(1)->mem_resource_map_[RT_MEMORY_HBM];
  EXPECT_EQ(resource->var_mem_size_, 1536);
  EXPECT_EQ(resource->var_mem_size_, resource_src->var_mem_size_);

  ret = VarManager::Instance(1)->AssignVarMem("Hello_variable", tensor_desc, RT_MEMORY_HBM);
  EXPECT_EQ(ret, SUCCESS);

  EXPECT_EQ(resource->total_size_, resource_src->total_size_);
  EXPECT_EQ(resource->total_size_, var_mem_max_size);
}

TEST_F(STEST_helper_runtime, Init_Device_Dgw_Server_Success) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGroupInit(group_name);
  uint32_t device_id = 0;
  uint32_t vf_id = 0;
  pid_t dgw_pid = 0;
  ge::Status res = ge::DataGwManager::GetInstance().InitDeviceDataGwServer(device_id, group_name, dgw_pid);
  ASSERT_EQ(res, ge::SUCCESS);

  group_name = "TEST";
  res = ge::DataGwManager::GetInstance().InitDeviceDataGwServer(device_id, group_name, dgw_pid);
  ASSERT_EQ(res, ge::FAILED);

  group_name = "DM_QS_GROUP";
  res = ge::DataGwManager::GetInstance().InitDeviceDataGwServer(0xff, group_name, dgw_pid);
  ASSERT_EQ(res, ge::FAILED);
}


TEST_F(STEST_helper_runtime, Dgw_GrantQueue_Error) {
  uint32_t device_id = 0xff;
  uint32_t qid = 0;
  pid_t dgw_pid = 0;
  GrantType grant_type = READ_ONLY;
  ge::Status res = ge::DataGwManager::GetInstance().GrantQueue(device_id, qid, dgw_pid, grant_type);
  ASSERT_EQ(res, ge::FAILED);
}

TEST_F(STEST_helper_runtime, ProcessMultiVarManager) {
  const map<string, string> options{{"ge.graphMemoryMaxSize", "536870912"}};
  Status ret = VarManager::Instance(1)->Init(0x5a, 1, 0, 0x5a5a);
  ret = VarManager::Instance(1)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);
  EXPECT_EQ(ret, SUCCESS);

  std::vector<int64_t> s = {1,2,3,4};
  GeShape shape(s);
  GeTensorDesc tensor_desc(shape);
  TensorUtils::SetSize(tensor_desc, shape.GetShapeSize());
  ret = VarManager::Instance(1)->AssignVarMem("var_node", tensor_desc, RT_MEMORY_HBM);
  EXPECT_EQ(ret, SUCCESS);

  DeployContext deploy_context;
  deployer::DeployerResponse response;
  deployer::MultiVarManagerRequest info;
  deploy_context.executor_manager_.SetMaintenanceCfg(&deploy_context.dev_maintenance_cfg_);
  ret = deploy_context.ProcessMultiVarManager(info, response);
  ASSERT_EQ(ret, ge::SUCCESS);
  deployer::SharedContentDescRequest shared_request;
  auto shared_info = shared_request.mutable_shared_content_desc();
  shared_info->set_session_id(1);
  shared_info->set_mem_type(2);
  shared_info->set_total_length(5);
  shared_info->set_node_name("var_node");
  ret = deploy_context.ProcessSharedContent(shared_request, response);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(STEST_helper_runtime, TestDeployModel) {
  mock_handle = (void *)0xffffffff;
  mock_method = (void *)&MockInitializeHelperRuntime;
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

TEST_F(STEST_helper_runtime, TestDeployModelWithFileConstant) {
  mock_handle = (void *)0xffffffff;
  mock_method = (void *)&MockInitializeHelperRuntime2;
  std::map<std::string, std::string> options_runtime;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options_runtime), SUCCESS);

  std::vector<std::string> engine_list = {"AIcoreEngine"};
  std::vector<int64_t> shape = {2, 2, 2, 2};
  auto file_const_op = OP_CFG(FILECONSTANT).Attr("shape", shape).Attr("dtype", DT_FLOAT).Attr("file_id", "vector_search_bucker_value_bin");

  int64_t dims_size = 1;
  vector<int64_t> data_vec = {2, 2, 2, 2};
  for_each(data_vec.begin(), data_vec.end(), [&](int64_t &data) { dims_size *= data; });
  vector<float> data_value_vec(dims_size, 1);
  GeTensorDesc data_tensor_desc(GeShape(data_vec), FORMAT_NCHW, DT_FLOAT);
  GeTensorPtr data_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec.data(),
                                                  data_value_vec.size() * sizeof(float));
  std::cout << "davinci_model_execute_with_file_constant" << data_value_vec.size() << std::endl;
  auto const_op = OP_CFG(CONSTANT).Weight(data_tensor);
  auto add = OP_CFG(ADD);
  auto output = OP_CFG(NETOUTPUT);
  DEF_GRAPH(g1) {
    CHAIN(NODE("file_constant_1", file_const_op)->EDGE(0, 0)->NODE("add", add));
    CHAIN(NODE("const_op", const_op)->EDGE(0, 1)->NODE("add", add));
    CHAIN(NODE("add", add)->EDGE(0, 0)->NODE(NODE_NAME_NET_OUTPUT, output));
  };

  {
    size_t file_const_size = 64;
    float *float_buf = (float *)malloc(file_const_size);
    if (float_buf == nullptr) {
      return;
    }
    std::ofstream out1("test_copy_one_weight.bin", std::ios::binary);
    if (!out1.is_open()) {
      free(float_buf);
      return;
    }
    out1.write((char *)float_buf, file_const_size);
    out1.close();
    free(float_buf);
  }

  auto graph = ToGeGraph(g1);
  AscendString a = "ge.exec.value_bins";
  AscendString b =
      "{\"value_bins\":[{\"value_bin_id\":\"vector_search_bucker_value_bin\", "
      "\"value_bin_file\":\"./test_copy_one_weight.bin\"}]}";
  map<AscendString, AscendString> options{{a, b}};
  Session session(options);
  session.AddGraph(1, graph, options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  ASSERT_EQ(ret, SUCCESS);

  std::vector<Tensor> input_tensors, output_tensors;
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
  (void) remove("test_copy_one_weight.bin");
}

TEST_F(STEST_helper_runtime, TestDeployModelNoTiling) {
  mock_handle = (void *)0xffffffff;
  mock_method = (void *)&MockInitializeHelperRuntime;
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

namespace {
class MockRuntime : public RuntimeStub {
 public:
  rtError_t rtGetIsHeterogenous(int32_t *heterogeneous) override {
    *heterogeneous = 1;
    return RT_ERROR_NONE;
  }
};

class MockRuntime2PG : public RuntimeStub {
 public:
  rtError_t rtGetIsHeterogenous(int32_t *heterogeneous) override {
    *heterogeneous = 1;
    return RT_ERROR_NONE;
  }

  rtError_t rtGetDeviceCount(int32_t *count) override {
    *count = 2;
    return RT_ERROR_NONE;
  }
};

HcclResult HcomSetRankTable(const string &rankTable) {
  return HCCL_SUCCESS;
}

class MockMmpaForHelperRuntime : public MmpaStubApi {
 public:
  void *DlOpen(const char *file_name, int32_t mode) {
    if (std::string(file_name) == "libgrpc_client.so") {
      return (void *) 0x12345678;
    } else if (std::string(file_name).find("libhcom_graph_adaptor.so") != std::string::npos) {
      return mock_handle;
    }
    return dlopen(file_name, mode);
  }

  void *DlSym(void *handle, const char *func_name) override {
    if (std::string(func_name) == "InitializeHelperRuntime") {
      return (void *) &InitializeHelperRuntime;
    } else if (std::string(func_name) == "HcomSetRankTable") {
      return (void *) &HcomSetRankTable;
    }
    return dlsym(handle, func_name);
  }

  int32_t DlClose(void *handle) override {
    if (handle == (void *) 0x12345678) {
      return 0;
    }
    return dlclose(handle);
  }

  int32_t RealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen) override {
    strncpy(realPath, path, realPathLen);
    return EN_OK;
  }
};

Graph BuildGraph() {
  DEF_GRAPH(graph_def) {
    auto arg_0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto var = OP_CFG(VARIABLE)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto neg_1 = OP_CFG(NEG)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(2)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, {16});

    CHAIN(NODE("arg_0", arg_0)
              ->NODE("neg_1", neg_1)
              ->NODE("Node_Output", net_output));

    CHAIN(NODE("var", var)
              ->NODE("Node_Output", net_output));
  };

  auto graph = ToGeGraph(graph_def);
  return graph;
}

Graph BuildDynamicGraph() {
  auto shape = std::vector<int64_t>{-1};
  DEF_GRAPH(graph_def) {
    auto arg_0 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .Attr(ATTR_NAME_INDEX, 0)
        .TensorDesc(FORMAT_ND, DT_INT32, shape);

    auto neg_1 = OP_CFG(NEG)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, shape);

    auto net_output = OP_CFG(NETOUTPUT)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, shape);

    CHAIN(NODE("arg_0", arg_0)
              ->NODE("neg_1", neg_1)
              ->NODE("Node_Output", net_output));
  };

  auto graph = ToGeGraph(graph_def);
  return graph;
}
}  // namespace

TEST_F(STEST_helper_runtime, TestDeployHeterogeneousModel) {
  std::string help_clu = "{\"chief\":\"127.0.0.1:2222\"}";
  setenv("HELP_CLUSTER", help_clu.c_str(), 1);
  setenv("RANK_ID", "1", 1);
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpaForHelperRuntime>());
  RuntimeStub::SetInstance(std::make_shared<MockRuntime>());

  // 1. start server
  char real_path[200];
  realpath("st_run_data/json/helper_runtime/device", real_path);
  EXPECT_EQ(Configurations::GetInstance().InitDeviceInformation(real_path), SUCCESS);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });
  sleep(1);

  // 2. Init master
  realpath("st_run_data/json/helper_runtime/host", real_path);
  setenv("HELPER_RES_FILE_PATH", real_path, 1);
  GEFinalize();
  std::map<AscendString, AscendString> options;
  EXPECT_EQ(ge::GEInitialize(options), SUCCESS);
  GeRunningEnvFaker ge_env;
  ge_env.InstallDefault();

  // 3. BuildAndExecuteGraph
  auto graph = BuildGraph();
  Session session(options);
  uint32_t graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape(std::vector<int64_t>({16}));
  TensorDesc tensor_desc(shape, FORMAT_ND, DT_INT32);
  Tensor tensor(tensor_desc);
  uint8_t buffer[16 * 4];
  tensor.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> input{tensor};
  mock_handle = nullptr;
  EXPECT_EQ(session.BuildGraph(graph_id, input), FAILED);
  CommDomainManager::GetInstance().DestroyRankTable();
  std::vector<Tensor> inputs{tensor};
  mock_handle = (void *)0x12345678;
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
  mock_handle = nullptr;
  GEFinalize();

  // 4. Cleanup
  grpc_server.Finalize();
  if (server_thread.joinable()) {
    server_thread.join();
  }
  ExecutionRuntime::FinalizeExecutionRuntime();
  MmpaStub::GetInstance().Reset();
  RuntimeStub::Reset();
  unsetenv("HELP_CLUSTER");
  unsetenv("RANK_ID");
}

TEST_F(STEST_helper_runtime, TestDeployHeterogeneousModel2PG) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpaForHelperRuntime>());
  RuntimeStub::SetInstance(std::make_shared<MockRuntime2PG>());

  // 1. start server
  char real_path[200];
  realpath("st_run_data/json/helper_runtime/device", real_path);
  EXPECT_EQ(Configurations::GetInstance().InitDeviceInformation(real_path), SUCCESS);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });
  sleep(1);

  // 2. Init master
  realpath("st_run_data/json/helper_runtime/host", real_path);
  setenv("HELPER_RES_FILE_PATH", real_path, 1);
  GEFinalize();
  std::map<AscendString, AscendString> options;
  EXPECT_EQ(ge::GEInitialize(options), SUCCESS);
  GeRunningEnvFaker ge_env;
  ge_env.InstallDefault();

  // 3. BuildAndExecuteGraph
  auto graph = BuildGraph();
  Session session(options);
  uint32_t graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape(std::vector<int64_t>({16}));
  TensorDesc tensor_desc(shape, FORMAT_ND, DT_INT32);
  Tensor tensor(tensor_desc);
  uint8_t buffer[16 * 4];
  tensor.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{tensor};
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
  GEFinalize();

  // 4. Cleanup
  grpc_server.Finalize();
  if (server_thread.joinable()) {
    server_thread.join();
  }
  ExecutionRuntime::FinalizeExecutionRuntime();
  MmpaStub::GetInstance().Reset();
  RuntimeStub::Reset();
}
TEST_F(STEST_helper_runtime, TestDeployHeterogeneousDynamicModel) {
//  const char *const kHostCpuEngineName = "DNN_VM_HOST_CPU";
//  const char *const kHostCpuOpKernelLibName = "DNN_VM_HOST_CPU_OP_STORE";
//  dlog_setlevel(0, 0, 0);
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpaForHelperRuntime>());
  RuntimeStub::SetInstance(std::make_shared<MockRuntime>());

  // 1. start server
  char real_path[200];
  realpath("st_run_data/json/helper_runtime/device", real_path);
  EXPECT_EQ(Configurations::GetInstance().InitDeviceInformation(real_path), SUCCESS);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });
  sleep(1);

  // 2. Init master
  realpath("st_run_data/json/helper_runtime/host", real_path);
  setenv("HELPER_RES_FILE_PATH", real_path, 1);
  GEFinalize();
  std::map<AscendString, AscendString> options;
  EXPECT_EQ(ge::GEInitialize(options), SUCCESS);
  GeRunningEnvFaker ge_env;
  ge_env.InstallDefault();

  CpuSchedEventDispatcher::GetInstance().Initialize(0);

  EXPECT_EQ(VarManager::Instance(0)->Init(0x5a, 1, 0, 0x5a5a), SUCCESS);
  // 3. BuildAndExecuteGrap
  auto graph = BuildDynamicGraph();
  const map<AscendString, AscendString> session_options{{"ge.graphMemoryMaxSize", "536870912"},
                                                        {"ge.variableMemoryMaxSize", "536870912"},
                                                        {"ge.exec.placement", "DEVICE"}};
  Session session(session_options);

  std::map<AscendString, AscendString> graph_options;
  graph_options[OPTION_EXEC_DYNAMIC_EXECUTE_MODE] = "dynamic_execute";
  graph_options[OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE] = "[1~20]";

  uint32_t graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph, graph_options), SUCCESS);

  Shape shape(std::vector<int64_t>({16}));
  TensorDesc tensor_desc(shape, FORMAT_ND, DT_INT32);
  Tensor tensor(tensor_desc);
  uint8_t buffer[16 * 4];
  tensor.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{tensor};
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
  GEFinalize();

  CpuSchedEventDispatcher::GetInstance().Finalize();
  // 4. Cleanup
  grpc_server.Finalize();
  if (server_thread.joinable()) {
    server_thread.join();
  }
  MmpaStub::GetInstance().Reset();
  RuntimeStub::Reset();
}

TEST_F(STEST_helper_runtime, TestDeployHeterogeneousModelMaintenanceCfg) {
  std::string help_clu = "{\"chief\":\"127.0.0.1:2222\"}";
  const std::string kEnableFlag = "1";
  setenv("HELP_CLUSTER", help_clu.c_str(), 1);
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpaForHelperRuntime>());
  RuntimeStub::SetInstance(std::make_shared<MockRuntime>());
  // init log/dump/profiling cfg
  const std::map<std::string, std::string> kLogEnvs =
      {{"ASCEND_GLOBAL_LOG_LEVEL", "1"}, {"ASCEND_GLOBAL_EVENT_ENABLE", "1"},
       {"ASCEND_HOST_LOG_FILE_NUM", "1"}, {"ASCEND_PROCESS_LOG_PATH", "/"}};
  DevMaintenanceCfgUtils::SetEnvByOption(kLogEnvs);
  DumpProperties dump_properties;
  dump_properties.enable_dump_ = kEnableFlag;
  DumpManager::GetInstance().AddDumpProperties(0, dump_properties);
  ProfilingProperties::Instance().UpdateDeviceIdCommandParams("mode=all");

  // 1. start server
  char real_path[200];
  realpath("st_run_data/json/helper_runtime/device", real_path);
  EXPECT_EQ(Configurations::GetInstance().InitDeviceInformation(real_path), SUCCESS);
  ge::GrpcServer grpc_server;
  std::thread server_thread = std::thread([&]() {
    StartServer(grpc_server);
  });
  sleep(1);

  // 2. Init master
  realpath("st_run_data/json/helper_runtime/host", real_path);
  setenv("HELPER_RES_FILE_PATH", real_path, 1);
  GEFinalize();
  std::map<AscendString, AscendString> options;
  EXPECT_EQ(ge::GEInitialize(options), SUCCESS);
  GeRunningEnvFaker ge_env;
  ge_env.InstallDefault();

  // 3. BuildAndExecuteGraph
  auto graph = BuildGraph();
  Session session(options);
  uint32_t graph_id = 1;
  EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

  Shape shape(std::vector<int64_t>({16}));
  TensorDesc tensor_desc(shape, FORMAT_ND, DT_INT32);
  Tensor tensor(tensor_desc);
  uint8_t buffer[16 * 4];
  tensor.SetData(buffer, sizeof(buffer));

  std::vector<Tensor> inputs{tensor};
  EXPECT_EQ(session.BuildGraph(graph_id, inputs), SUCCESS);
  GEFinalize();

  // 4. Cleanup
  grpc_server.Finalize();
  if (server_thread.joinable()) {
    server_thread.join();
  }
  MmpaStub::GetInstance().Reset();
  RuntimeStub::Reset();
  unsetenv("HELP_CLUSTER");
}

}  // namespace ge
