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
#include <stdlib.h>

#include <unistd.h>
#include "framework/common/debug/ge_log.h"

#define protected public
#define private public
#include "common/config/json_parser.h"
#undef protected public
#undef private public

using namespace std;
namespace ge {
class UtJsonParser : public testing::Test {
 public:
  UtJsonParser() {}
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// host
TEST_F(UtJsonParser, run_parse_host_info_from_config_file) {
  ge::JsonParser jsonParser;
  ge::DeployerConfig hostInformation_;

  char path[200];
  realpath("./", path);
  std::cout << path << std::endl;
  realpath("../tests/ut/ge/runtime/data/valid/host", path);
  std::string real_path = path;
  auto ret = jsonParser.ParseHostInfoFromConfigFile(real_path, hostInformation_);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtJsonParser, run_parse_host_info_from_config_file_2) {
  ge::JsonParser jsonParser;
  ge::DeployerConfig hostInformation_;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/wrong_key/host", path);
  std::string real_path = path;
  auto ret = jsonParser.ParseHostInfoFromConfigFile(real_path, hostInformation_);
  ASSERT_NE(ret, ge::SUCCESS);
}

TEST_F(UtJsonParser, run_parse_host_info_from_config_file_3) {
  ge::JsonParser jsonParser;
  ge::DeployerConfig hostInformation_;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/wrong_value/host", path);
  std::string real_path = path;
  auto ret = jsonParser.ParseHostInfoFromConfigFile(real_path, hostInformation_);
  ASSERT_NE(ret, ge::SUCCESS);
}

TEST_F(UtJsonParser, run_read_config_file) {
  ge::JsonParser jsonParser;
  nlohmann::json js;
  auto ret = jsonParser.ReadConfigFile("", js);
  ASSERT_EQ(ret, ge::FAILED);
}

// device
TEST_F(UtJsonParser, run_device_config_from_config_file) {
  ge::JsonParser jsonParser;
  ge::NodeConfig device_config;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  std::string real_path = path;
  auto ret = jsonParser.ParseDeviceConfigFromConfigFile(real_path, device_config);
  ASSERT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtJsonParser, run_device_config_from_config_file_2) {
  ge::JsonParser jsonParser;
  ge::NodeConfig device_config;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/wrong_key/device", path);
  std::string real_path = path;
  auto ret = jsonParser.ParseDeviceConfigFromConfigFile(real_path, device_config);
  ASSERT_NE(ret, ge::SUCCESS);
}

TEST_F(UtJsonParser, run_device_config_from_config_file_3) {
  ge::JsonParser jsonParser;
  ge::NodeConfig device_config;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/wrong_value/device", path);
  std::string real_path = path;
  auto ret = jsonParser.ParseDeviceConfigFromConfigFile(real_path, device_config);
  ASSERT_NE(ret, ge::SUCCESS);
}
}

