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
#include <map>
#include <common/plugin/ge_util.h>
#include "framework/common/debug/ge_log.h"
#include "hccl/hccl_types.h"
#include "depends/mmpa/src/mmpa_stub.h"
#define protected public
#define private public
#include "daemon/daemon_client_manager.h"
#undef protected public
#undef private public

using namespace std;
namespace ge {
// client manager
class DaemonClientManagerTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DaemonClientManagerTest, run_initialize) {
  DaemonClientManager client_manager;
  client_manager.Initialize();
}

TEST_F(DaemonClientManagerTest, run_finalize) {
  DaemonClientManager client_manager;
  client_manager.Finalize();
}

TEST_F(DaemonClientManagerTest, run_create_client) {
  DaemonClientManager client_manager;
  int64_t client_id = 0;
  auto ret = client_manager.CreateClient(client_id);
  ASSERT_NE(ret, ge::FAILED);
}

TEST_F(DaemonClientManagerTest, run_close_client) {
  DaemonClientManager client_manager;
  int64_t client_id = 0;
  client_manager.CloseClient(client_id);
}

TEST_F(DaemonClientManagerTest, run_get_client) {
  DaemonClientManager client_manager;
  int64_t client_id = 0;
  client_manager.GetClient(client_id);
}

TEST_F(DaemonClientManagerTest, TestEvict) {
  DaemonClientManager client_manager;
  int64_t client_id = 0;
  client_manager.clients_[0] = MakeUnique<DeployerDaemonClient>(client_id);
  client_manager.clients_[0]->last_heartbeat_ts_ = {};
  client_manager.clients_[0]->is_executing_ = true;
  client_manager.EvictExpiredClients();
  ASSERT_EQ(client_manager.clients_.size(), 1);
  client_manager.clients_[0]->is_executing_ = false;
  client_manager.EvictExpiredClients();
  ASSERT_EQ(client_manager.clients_.size(), 0);
}

// client
class DeployerDaemonClientTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DeployerDaemonClientTest, run_initialize) {
  DeployerDaemonClient client(1);
  client.Initialize();
}

TEST_F(DeployerDaemonClientTest, TestIsExpired) {
  DeployerDaemonClient client(1);
  client.last_heartbeat_ts_ = {};
  ASSERT_TRUE(client.IsExpired());
  client.OnHeartbeat();
  ASSERT_FALSE(client.IsExpired());
}

TEST_F(DeployerDaemonClientTest, TestIsExecuting) {
  DeployerDaemonClient client(1);
  ASSERT_FALSE(client.IsExecuting());
  client.SetIsExecuting(true);
  ASSERT_TRUE(client.IsExecuting());
}

TEST_F(DeployerDaemonClientTest, run_finalize) {
  DeployerDaemonClient client(1);
  client.Finalize();
}
}
