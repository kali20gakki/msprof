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
#include "framework/common/debug/ge_log.h"

#define protected public
#define private public
#include "device_manager.h"

using namespace std;
namespace ge {
class UtDeviceManager : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UtDeviceManager, run_init) {
  ge::DeviceManger deviceManger;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/host", path);
  Configurations::GetInstance().InitHostInformation(path);
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);

  auto ret = deviceManger.Initialize();
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtDeviceManager, run_get_device_info) {
  ge::DeviceManger deviceManger;
  uint32_t device_id = 0;
  const ge::DeviceInfo *device_info = nullptr;
  auto ret = deviceManger.GetDeviceInfo(device_id, &device_info);
}

TEST_F(UtDeviceManager, run_host_info) {
  ge::DeviceManger deviceManger;
  deviceManger.GetHostInfo();
}

TEST_F(UtDeviceManager, run_get_device_info_list) {
  ge::DeviceManger deviceManger;
  deviceManger.GetDeviceInfoList();
}

// RemoteDevice
class UtRemoteDevice : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UtRemoteDevice, TestInitiailzeFailed_wrong_token) {
  ge::DeviceInfo device_info;
  device_info.ipaddr = "127.0.0.1";
  device_info.port = 8080;
  device_info.token = "yyyyy";
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  ge::RemoteDevice remoteDevice(device_info);
  EXPECT_EQ(remoteDevice.Initialize(), FAILED);
}

TEST_F(UtRemoteDevice, TestRemoteDevice) {
  ge::DeviceInfo device_info;
  device_info.ipaddr = "127.0.0.1";
  device_info.port = 8080;
  device_info.token = "xxxxxxx";
  ge::RemoteDevice remoteDevice(device_info);
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  EXPECT_EQ(remoteDevice.Finalize(), SUCCESS);  // finalize while not initialized
  EXPECT_EQ(remoteDevice.Initialize(), SUCCESS);
  EXPECT_EQ(remoteDevice.Initialize(), SUCCESS);  // repeat initialize
  EXPECT_EQ(remoteDevice.SendRequest(request, response), SUCCESS);
  this_thread::sleep_for(std::chrono::milliseconds (500));  // wait for keepalive run
  EXPECT_EQ(remoteDevice.Finalize(), SUCCESS);
  EXPECT_EQ(remoteDevice.Finalize(), SUCCESS);  // repeat finalize
}

TEST_F(UtRemoteDevice, TestAutoFinalize) {
  ge::DeviceInfo device_info;
  device_info.ipaddr = "127.0.0.1";
  device_info.port = 8080;
  device_info.token = "xxxxxxx";
  ge::RemoteDevice remoteDevice(device_info);
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  EXPECT_EQ(remoteDevice.Initialize(), SUCCESS);  // finalize while not initialized
}
}
