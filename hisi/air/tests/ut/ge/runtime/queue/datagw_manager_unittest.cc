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
#include <vector>
#include <string>
#define protected public
#define private public
#include "deploy/flowrm/datagw_manager.h"
#include "deploy/flowrm/network_manager.h"
#undef protected public
#undef private public

#include "common/config/configurations.h"

namespace ge {
class UtDataGwManager : public testing::Test {
 public:
  UtDataGwManager() {}
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UtDataGwManager, run_InitHostDataGwServer) {
  std::string group_name = "DM_QS_GROUP";
  pid_t dgw_pid = 0;
  auto ret = ge::DataGwManager::GetInstance().InitHostDataGwServer(0, 0, group_name, dgw_pid);
}

TEST_F(UtDataGwManager, run_InitDeviceDataGwServer) {
  std::string group_name = "DM_QS_GROUP";
  pid_t dgw_pid = 0;
  auto ret = ge::DataGwManager::GetInstance().InitDeviceDataGwServer(0, group_name, dgw_pid);
  group_name = "TEST_ERROR";
  ret = ge::DataGwManager::GetInstance().InitDeviceDataGwServer(0, group_name, dgw_pid);
  group_name = "DM_QS_GROUP";
  ret = ge::DataGwManager::GetInstance().InitDeviceDataGwServer(0xff, group_name, dgw_pid);
}

TEST_F(UtDataGwManager, run_InitDataGwClient) {
  auto ret = ge::DataGwManager::GetInstance().InitDataGwClient(100, 0);
}

TEST_F(UtDataGwManager, run_CreateDataGwTag) {
  const uint64_t hcom_handle = 5;
  const std::string tag_name = "TAG";
  uint32_t device_id = 0;
  int32_t hcom_tag = 0;
  auto ret = ge::DataGwManager::GetInstance().CreateDataGwTag(hcom_handle, tag_name, device_id, hcom_tag);
}

TEST_F(UtDataGwManager, run_GrantQueueForDataGw) {
  uint32_t device_id = 0;
  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::QUEUE;
  queue_route.dst.type = bqs::EndpointType::QUEUE;
  queue_route.src.attr.queueAttr.queueId = 1;
  queue_route.dst.attr.queueAttr.queueId = 2;
  auto ret = ge::DataGwManager::GetInstance().GrantQueueForDataGw(device_id, queue_route);
}

TEST_F(UtDataGwManager, run_BindQueues) {
  uint32_t device_id = 0;
  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::QUEUE;
  queue_route.dst.type = bqs::EndpointType::QUEUE;
  queue_route.src.attr.queueAttr.queueId = 1;
  queue_route.dst.attr.queueAttr.queueId = 2;
  std::vector<bqs::Route> queue_routes{queue_route};
  auto ret = ge::DataGwManager::GetInstance().BindQueues(device_id, queue_routes);
}

TEST_F(UtDataGwManager, run_GrantQueue) {
  uint32_t device_id = 0;
  uint32_t qid = 1;
  pid_t pid = 100;
  ge::GrantType grant_type = READ_ONLY;
  auto ret = ge::DataGwManager::GetInstance().GrantQueue(device_id, qid, pid, grant_type);
}

TEST_F(UtDataGwManager, run_InitQueue) {
  auto ret = ge::DataGwManager::GetInstance().InitQueue(0);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtDataGwManager, run_UnbindQueues) {
  uint32_t device_id = 0;
  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::QUEUE;
  queue_route.dst.type = bqs::EndpointType::QUEUE;
  queue_route.src.attr.queueAttr.queueId = 1;
  queue_route.dst.attr.queueAttr.queueId = 2;
  auto ret = ge::DataGwManager::GetInstance().UnbindQueues(device_id, queue_route);
}

TEST_F(UtDataGwManager, run_DestroyHandle) {
  uint32_t device_id = 0;
  uint64_t handle = 1;
  auto ret = ge::DataGwManager::GetInstance().DestroyDataGwHandle(device_id, handle);
}

TEST_F(UtDataGwManager, run_GetHostPort) {
  EXPECT_EQ(Configurations::GetInstance().InitHostInformation("../tests/ut/ge/runtime/data/valid/host"), SUCCESS);
  auto ret = ge::NetworkManager::GetInstance().Initialize();
  ret = ge::NetworkManager::GetInstance().GetDataPanelPort();
}

TEST_F(UtDataGwManager, run_GetHostIp) {
  EXPECT_EQ(Configurations::GetInstance().InitHostInformation("../tests/ut/ge/runtime/data/valid/host"), SUCCESS);
  std::string ip;
  EXPECT_EQ(ge::NetworkManager::GetInstance().GetDataPanelIp(ip), SUCCESS);
}

TEST_F(UtDataGwManager, run_Shutdown) {
  auto ret = ge::DataGwManager::GetInstance().Shutdown();
}

TEST_F(UtDataGwManager, run_CreateDataGwHandle) {
  const uint64_t hcom_handle = 5;
  const std::string tag_name = "TAG";
  uint32_t device_id = 0;
  bqs::CreateHcomInfo hcomInfo;
  hcomInfo.masterPort = 1;
  auto ret = ge::DataGwManager::GetInstance().CreateDataGwHandle(device_id, hcomInfo);
}

TEST_F(UtDataGwManager, run_CreateDataGwGroup) {
  uint32_t device_id = 0;
  bqs::Endpoint endpoint;
  endpoint.type = bqs::EndpointType::QUEUE;
  std::vector<bqs::Endpoint> endpoint_list;
  endpoint_list.emplace_back(endpoint);
  int32_t group_id = 0;
  auto ret = ge::DataGwManager::GetInstance().CreateDataGwGroup(device_id, endpoint_list, group_id);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtDataGwManager, run_CreateDataGwGroup_Empty) {
  uint32_t device_id = 0;
  std::vector<bqs::Endpoint> endpoint_list;
  int32_t group_id = 0;
  auto ret = ge::DataGwManager::GetInstance().CreateDataGwGroup(device_id, endpoint_list, group_id);
  ASSERT_NE(ret, ge::SUCCESS);
}

TEST_F(UtDataGwManager, run_GrantQueue_WriteOnly) {
  uint32_t device_id = 0;
  uint32_t qid = 1;
  pid_t pid = 100;
  ge::GrantType grant_type = WRITE_ONLY;
  auto ret = ge::DataGwManager::GetInstance().GrantQueue(device_id, qid, pid, grant_type);
}

TEST_F(UtDataGwManager, run_GrantQueue_ReadAndWrite) {
  uint32_t device_id = 0;
  uint32_t qid = 1;
  pid_t pid = 100;
  ge::GrantType grant_type = READ_AND_WRITE;
  auto ret = ge::DataGwManager::GetInstance().GrantQueue(device_id, qid, pid, grant_type);
}

TEST_F(UtDataGwManager, run_GrantQueue_GRANT_INVALID) {
  uint32_t device_id = 0;
  uint32_t qid = 1;
  pid_t pid = 100;
  ge::GrantType grant_type = GRANT_INVALID;
  auto ret = ge::DataGwManager::GetInstance().GrantQueue(device_id, qid, pid, grant_type);
}

TEST_F(UtDataGwManager, run_GrantQueue_ErrorPid) {
  uint32_t device_id = 0;
  uint32_t qid = 1;
  pid_t pid = 10001;
  ge::GrantType grant_type = READ_ONLY;
  auto ret = ge::DataGwManager::GetInstance().GrantQueue(device_id, qid, pid, grant_type);
}

TEST_F(UtDataGwManager, run_DestroyDataGwTag) {
  const uint32_t device_id = 0;
  const uint64_t hcom_handle = 0;
  const int32_t hcom_tag = 0;
  auto ret = ge::DataGwManager::GetInstance().DestroyDataGwTag(device_id, hcom_handle, hcom_tag);
}

TEST_F(UtDataGwManager, run_GrantQueueForDataGwErrorType) {
  uint32_t device_id = 0;
  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::QUEUE;
  queue_route.dst.type = bqs::EndpointType::QUEUE;
  queue_route.src.attr.queueAttr.queueId = 1;
  queue_route.dst.attr.queueAttr.queueId = 2;
  auto ret = ge::DataGwManager::GetInstance().GrantQueueForDataGw(device_id, queue_route);
}

TEST_F(UtDataGwManager, run_BindQueuesError) {
  uint32_t device_id = 0;
  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::QUEUE;
  queue_route.dst.type = bqs::EndpointType::QUEUE;
  queue_route.src.attr.queueAttr.queueId = 1;
  queue_route.dst.attr.queueAttr.queueId = 2;
  std::vector<bqs::Route> queue_routes{queue_route};
  auto ret = ge::DataGwManager::GetInstance().BindQueues(device_id, queue_routes);
}

TEST_F(UtDataGwManager, run_UnbindQueuesError) {
  uint32_t device_id = 100001;
  bqs::Route queue_route;
  auto ret = ge::DataGwManager::GetInstance().UnbindQueues(device_id, queue_route);
}

TEST_F(UtDataGwManager, run_GrantQueueError) {
  uint32_t device_id = 0;
  uint32_t qid = 1;
  pid_t pid = -1;
  ge::GrantType grant_type = READ_ONLY;
  auto ret = ge::DataGwManager::GetInstance().GrantQueue(device_id, qid, pid, grant_type);
}

TEST_F(UtDataGwManager, run_TryToBindPortError) {
  auto ret = ge::NetworkManager::GetInstance().TryToBindPort(18000);
  ret = ge::NetworkManager::GetInstance().TryToBindPort(18000);
}

TEST_F(UtDataGwManager, run_DestroyHandleError) {
  uint32_t device_id = 0;
  uint64_t handle = 100001;
  auto ret = ge::DataGwManager::GetInstance().DestroyDataGwHandle(device_id, handle);
}

TEST_F(UtDataGwManager, run_InitQueueError) {
  auto ret = ge::DataGwManager::GetInstance().InitQueue(1000001);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtDataGwManager, run_BindQueuesErrorDeviceId) {
  uint32_t device_id = 900000;
  bqs::Route queue_route;
  queue_route.src.type = bqs::EndpointType::QUEUE;
  queue_route.dst.type = bqs::EndpointType::QUEUE;
  queue_route.src.attr.queueAttr.queueId = 1;
  queue_route.dst.attr.queueAttr.queueId = 2;
  std::vector<bqs::Route> queue_routes{queue_route};
  auto ret = ge::DataGwManager::GetInstance().BindQueues(device_id, queue_routes);
}

TEST_F(UtDataGwManager, run_CreateDataGwTagError) {
  const uint64_t hcom_handle = 5;
  const std::string tag_name = "TAG";
  uint32_t device_id = 0;
  int32_t hcom_tag = 90000;
  auto ret = ge::DataGwManager::GetInstance().CreateDataGwTag(hcom_handle, tag_name, device_id, hcom_tag);
}

TEST_F(UtDataGwManager, run_DestroyDataGwTagError) {
  const uint32_t device_id = 900;
  const uint64_t hcom_handle = 0;
  const int32_t hcom_tag = -1;
  auto ret = ge::DataGwManager::GetInstance().DestroyDataGwTag(device_id, hcom_handle, hcom_tag);
}

TEST_F(UtDataGwManager, run_CreateDataGwHandleError) {
  const uint64_t hcom_handle = 5;
  const std::string tag_name = "TAG";
  uint32_t device_id = 9000;
  bqs::CreateHcomInfo hcomInfo;
  hcomInfo.masterPort = 65537;
  auto ret = ge::DataGwManager::GetInstance().CreateDataGwHandle(device_id, hcomInfo);
}

TEST_F(UtDataGwManager, run_BindMainPortError) {
  char path[200];
  realpath("../tests/ut/ge/runtime/data/error_port/host/", path);
  setenv("HELPER_RES_FILE_PATH", path, 1);
  EXPECT_EQ(Configurations::GetInstance().InitHostInformation(path), SUCCESS);
  auto ret = ge::NetworkManager::GetInstance().BindMainPort();
}

TEST_F(UtDataGwManager, run_BindMainPortError2) {
  char path[200];
  realpath("../tests/ut/ge/runtime/data/zero_port/host/", path);
  EXPECT_EQ(Configurations::GetInstance().InitHostInformation(path), SUCCESS);
  auto ret = ge::NetworkManager::GetInstance().BindMainPort();
}

TEST_F(UtDataGwManager, run_init_and_finalize) {
  auto ret = ge::NetworkManager::GetInstance().Initialize();
  ret = ge::NetworkManager::GetInstance().Finalize();
}

TEST_F(UtDataGwManager, run_grant_queue_for_cpu_schedule) {
  uint32_t device_id = 0;
  uint32_t pid = 123;
  std::vector<uint32_t> input_queues{1};
  std::vector<uint32_t> output_queues{2};
  auto ret = ge::DataGwManager::GetInstance().GrantQueueForCpuSchedule(device_id, pid, input_queues, output_queues);
}
}