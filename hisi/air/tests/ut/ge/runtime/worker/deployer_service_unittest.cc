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
#include "deploy/deployer/deployer_service_impl.h"
#define protected public
#define private public

using namespace std;
namespace ge {
class UtDeployerService : public testing::Test {
 protected:
  void SetUp() override {
    DeviceDebugConfig::global_configs_ = nlohmann::json{};
  }
  void TearDown() override {
    DeviceDebugConfig::global_configs_ = nlohmann::json{};
  }

};

TEST_F(UtDeployerService, run_register_req_processor) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  deployer_service.Process(context, request, response);
}

TEST_F(UtDeployerService, run_down_model) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  DeployerServiceImpl::DownloadModelProcess(context, request, response);
}

TEST_F(UtDeployerService, load_model) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  request.set_type(deployer::kLoadModel);
  DeployContext context;
  DeployerServiceImpl::LoadModelProcess(context, request, response);
}

TEST_F(UtDeployerService, load_model_1) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  char path[200];
  realpath("../tests/ut/ge/runtime/data/valid/device", path);
  Configurations::GetInstance().InitDeviceInformation(path);
  deployer::InitRequest init_request_;
  std::string token = "xxxxxxx";
  init_request_.set_token(token);
  request.set_client_id(0);
  *request.mutable_init_request() = init_request_;
  DeployContext context;
  DeployerServiceImpl::LoadModelProcess(context, request, response);
}

TEST_F(UtDeployerService, unload_model) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  request.set_type(deployer::kUnloadModel);
  DeployerServiceImpl::UnloadModelProcess(context, request, response);
}

TEST_F(UtDeployerService, run_pre_down_model) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  DeployerServiceImpl::PreDownloadModelProcess(context, request, response);
}

TEST_F(UtDeployerService, run_download_debug_cfg) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  DeployerServiceImpl::DownloadDevMaintenanceCfgProcess(context, request, response);
}

TEST_F(UtDeployerService, run_download_dev_conf) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;

  request.set_type(deployer::kDownloadConf);
  auto download_config_request = request.mutable_download_config_request();
  download_config_request->set_sub_type(deployer::kLogConfig);
  download_config_request->set_device_id(0);
  std::string conf_data;
  DeviceMaintenanceMasterCfg device_debug_conf;
  // init log env
  int32_t env_val = 1;
  int32_t expect_env_val = 1;
  char_t env[4][128] = {0};
  const std::string kLogLevelEnvName = "ASCEND_GLOBAL_LOG_LEVEL";
  const std::string kLogEventEnableEnvName = "ASCEND_GLOBAL_EVENT_ENABLE";
  const std::string kLogHostFileNumEnvName = "ASCEND_HOST_LOG_FILE_NUM";
  const std::string kLogProcessLogPathEnvName = "ASCEND_PROCESS_LOG_PATH";
  const std::vector<std::string> kLogEnvNames = {kLogLevelEnvName, kLogEventEnableEnvName, kLogHostFileNumEnvName,
                                                 kLogProcessLogPathEnvName};
  for (size_t id = 0; id < kLogEnvNames.size(); id++) {
    std::string env_set = (kLogEnvNames[id] + "=" + std::to_string(env_val)).c_str();
    (void)strncpy(env[id], env_set.c_str(), env_set.size());
    putenv(env[id]);
    env_val++;
  }
  device_debug_conf.InitGlobalMaintenanceConfigs();
  auto ret = device_debug_conf.GetJsonDataByType(DeviceDebugConfig::ConfigType::kLogConfigType,
                                                 conf_data);
  EXPECT_EQ(ret, SUCCESS);
  download_config_request->set_config_data(&conf_data[0], conf_data.size());
  DeployerServiceImpl::DownloadDevMaintenanceCfgProcess(context, request, response);
}

TEST_F(UtDeployerService, run_pre_down_model_1) {
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  DeployerServiceImpl::PreDownloadModelProcess(context, request, response);
}

TEST_F(UtDeployerService, run_deployer_service_process) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  request.set_type(deployer::kPreDownloadModel);
  DeployContext context;
  deployer_service.Process(context, request, response);
}

TEST_F(UtDeployerService, run_deployer_service_process_1) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  request.set_type(deployer::kLoadModel);
  deployer_service.Process(context, request, response);
}

TEST_F(UtDeployerService, MultiVarManagerInfoProcess) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  request.set_type(deployer::kDownloadVarManager);
  DeployerServiceImpl::MultiVarManagerInfoProcess(context, request, response);
}

TEST_F(UtDeployerService, SharedContentProcess) {
  ge::DeployerServiceImpl deployer_service;
  deployer::DeployerRequest request;
  deployer::DeployerResponse response;
  DeployContext context;
  request.set_type(deployer::kDownloadSharedContent);
  DeployerServiceImpl::SharedContentProcess(context, request, response);
}
}