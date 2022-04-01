/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "deploy/deployer/deployer_service_impl.h"

#include <string>
#include <vector>
#include "ge/ge_api_error_codes.h"
#include "framework/common/debug/ge_log.h"
#include "proto/deployer.pb.h"
#include "securec.h"

namespace ge {
namespace {
constexpr int32_t kIpIndex = 1;
constexpr int32_t kPortIndex = 2;
}
class ServiceRegister {
 public:
  ServiceRegister(const deployer::DeployerRequestType type, const DeployerServiceImpl::ProcessFunc &fn) {
    DeployerServiceImpl::GetInstance().RegisterReqProcessor(type, fn);
  }

  ~ServiceRegister() = default;
};

#define REGISTER_REQUEST_PROCESSOR(type, fn) \
REGISTER_REQUEST_PROCESSOR_UNIQ_DEPLOYER(__COUNTER__, type, fn)

#define REGISTER_REQUEST_PROCESSOR_UNIQ_DEPLOYER(ctr, type, fn) \
REGISTER_REQUEST_PROCESSOR_UNIQ(ctr, type, fn)

#define REGISTER_REQUEST_PROCESSOR_UNIQ(ctr, type, fn) \
static ::ge::ServiceRegister register_request_processor##ctr \
__attribute__((unused)) = \
::ge::ServiceRegister(type, fn)

REGISTER_REQUEST_PROCESSOR(deployer::kPreDeployModel, DeployerServiceImpl::PreDeployModelProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kDownloadConf, DeployerServiceImpl::DownloadDevMaintenanceCfgProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kPreDownloadModel, DeployerServiceImpl::PreDownloadModelProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kDownloadModel, DeployerServiceImpl::DownloadModelProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kLoadModel, DeployerServiceImpl::LoadModelProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kUnloadModel, DeployerServiceImpl::UnloadModelProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kDownloadVarManager, DeployerServiceImpl::MultiVarManagerInfoProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kDownloadSharedContent, DeployerServiceImpl::SharedContentProcess);
REGISTER_REQUEST_PROCESSOR(deployer::kDeployRankTable, DeployerServiceImpl::DeployRankTableProcess);

void DeployerServiceImpl::RegisterReqProcessor(deployer::DeployerRequestType type,
                                               const DeployerServiceImpl::ProcessFunc &fn) {
  process_fns_.emplace(type, fn);
}

void DeployerServiceImpl::PreDeployModelProcess(DeployContext &context,
                                                const deployer::DeployerRequest &request,
                                                deployer::DeployerResponse &response) {
  context.PreDeployModel(request.pre_deploy_model_request(), response);
}

void DeployerServiceImpl::DownloadDevMaintenanceCfgProcess(DeployContext &context,
                                                           const deployer::DeployerRequest &request,
                                                           deployer::DeployerResponse &response) {
  context.DownloadDevMaintenanceCfg(request, response);
}

void DeployerServiceImpl::PreDownloadModelProcess(DeployContext &context,
                                                  const deployer::DeployerRequest &request,
                                                  deployer::DeployerResponse &response) {
  auto ret = context.PreDownloadModel(request, response);
  if (ret != SUCCESS) {
    response.set_error_code(ret);
    response.set_error_message("PreDownloadModel failed");
  }
}

void DeployerServiceImpl::DownloadModelProcess(DeployContext &context,
                                               const deployer::DeployerRequest &request,
                                               deployer::DeployerResponse &response) {
  auto ret = context.DownloadModel(request, response);
  if (ret != SUCCESS) {
    response.set_error_code(ret);
    response.set_error_message("DownloadModel failed");
  }
}

void DeployerServiceImpl::LoadModelProcess(DeployContext &context,
                                           const deployer::DeployerRequest &request,
                                           deployer::DeployerResponse &response) {
  auto ret = context.LoadModel(request, response);
  if (ret != SUCCESS) {
    response.set_error_code(ret);
    response.set_error_message("LoadModel failed");
  }
}

void DeployerServiceImpl::UnloadModelProcess(DeployContext &context,
                                             const deployer::DeployerRequest &request,
                                             deployer::DeployerResponse &response) {
  (void) context.UnloadModel(request, response);
}

void DeployerServiceImpl::MultiVarManagerInfoProcess(DeployContext &context,
                                                     const deployer::DeployerRequest &request,
                                                     deployer::DeployerResponse &response) {
  context.ProcessMultiVarManager(request.multi_var_manager_request(), response);
}

void DeployerServiceImpl::SharedContentProcess(DeployContext &context,
                                               const deployer::DeployerRequest &request,
                                               deployer::DeployerResponse &response) {
  context.ProcessSharedContent(request.shared_content_desc_request(), response);
}

void DeployerServiceImpl::DeployRankTableProcess(DeployContext &context,
                                                 const deployer::DeployerRequest &request,
                                                 deployer::DeployerResponse &response) {
  context.DeployRankTable(request.deploy_rank_table_request(), response);
}

Status DeployerServiceImpl::Process(DeployContext &context,
                                    const deployer::DeployerRequest &request,
                                    deployer::DeployerResponse &response) {
  auto type = request.type();
  GELOGD("[Process][Request] start, type = %d", static_cast<int32_t>(type));
  auto it = process_fns_.find(type);
  if (it == process_fns_.end()) {
    REPORT_INNER_ERROR("E19999", "Find api type[%d]  failed.", type);
    GELOGE(FAILED, "[Find][Api] Find api type[%d] failed.", type);
    response.set_error_code(FAILED);
    response.set_error_message("Api dose not exist");
    return SUCCESS;
  }

  auto process_fn = it->second;
  if (process_fn == nullptr) {
    REPORT_INNER_ERROR("E19999", "Function pointer dose not exist.");
    GELOGE(FAILED, "[Find][Function] Find pointer dose not exist.");
    response.set_error_code(FAILED);
    response.set_error_message("Find pointer dose not exist.");
    return SUCCESS;
  }

  process_fn(context, request, response);
  GELOGD("[Process][Request] succeeded, type = %d", static_cast<int32_t>(type));
  return SUCCESS;
}
}  // namespace ge
