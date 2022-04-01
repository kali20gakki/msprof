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
#include "deploy/rpc/deployer_client.h"
#include "framework/common/debug/ge_log.h"

#define protected public
#define private public

using namespace std;
namespace ge {
class UtDeployerClient : public testing::Test {
 public:
  UtDeployerClient() {}
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UtDeployerClient, run_init) {
  std::string ip("127.0.0.1:1000");
  ge::DeployerClient client;
  auto ret = client.Initialize(ip);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtDeployerClient, run_sendrequest) {
  ge::DeployerClient client;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  std::string ip("127.0.0.1:1000");
  auto ret = client.Initialize(ip);
  client.SendRequest(request, response);
}
}

