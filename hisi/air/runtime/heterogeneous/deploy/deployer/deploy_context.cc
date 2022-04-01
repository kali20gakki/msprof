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


#include "deploy/deployer/deploy_context.h"
#include <string>
#include "nlohmann/json.hpp"
#include "debug/ge_util.h"
#include "framework/common/debug/ge_log.h"
#include "securec.h"
#include "deploy/flowrm/flow_route_manager.h"
#include "deploy/flowrm/datagw_manager.h"
#include "common/config/configurations.h"

namespace ge {
namespace {
struct CommDomainRankInfo {
  std::string rank_id;
  std::string device_id;
};

struct CommDomainNodeInfo {
  std::string node_addr;
  std::vector<CommDomainRankInfo> ranks;
};

void from_json(const nlohmann::json &j, CommDomainRankInfo &domain_info) {
  j.at("rank_id").get_to(domain_info.rank_id);
  j.at("device_id").get_to(domain_info.device_id);
}

void from_json(const nlohmann::json &j, CommDomainNodeInfo &domain_info) {
  j.at("node_addr").get_to(domain_info.node_addr);
  j.at("ranks").get_to(domain_info.ranks);
}
}  // namespace

DeployContext::DeployContext(bool is_local) : executor_manager_(is_local) {
}

void DeployContext::SetName(const string &name) {
  name_ = name;
}

void DeployContext::Initialize() {
  executor_manager_.SetMaintenanceCfg(&dev_maintenance_cfg_);
  GELOGI("[%s] DeployContext initialized", name_.c_str());
}

void DeployContext::Finalize() const {
  GELOGI("[%s] DeployContext finalized", name_.c_str());
}

void DeployContext::PreDeployModel(const deployer::PreDeployModelRequest &request,
                                   deployer::DeployerResponse &response) {
  uint32_t root_model_id = request.root_model_id();
  const int32_t device_id = request.device_id();
  GELOGI("PreDeploy model begin, device_id = %d, root_model_id = %u", device_id, root_model_id);
  DeployTaskId plan_context{device_id, root_model_id, 0};
  int64_t route_id = request.route_id();
  if (request.has_exchange_plan()) {
    const auto &exchange_plan = request.exchange_plan();
    auto ret = FlowRouteManager::GetInstance().DeployRoutePlan(exchange_plan, device_id, route_id);
    if (ret != SUCCESS) {
      GELOGE(ret, "Failed to deploy local exchange");
      response.set_error_code(FAILED);
      response.set_error_message("Failed to deploy local exchange");
      return;
    }
  }
  exchange_routes_[plan_context] = route_id;
  GELOGI("PreDeploy model successfully");
  std::map<uint32_t, deployer::SubmodelDesc> submodel_desc_map;
  for (int32_t i = 0; i < request.submodels_size(); ++i) {
    DeployTaskId submodel_context{device_id, root_model_id, static_cast<uint32_t>(i)};
    submodel_descs_[submodel_context] = request.submodels(i);
  }
  GELOGI("PreDeploy model successfully, device_id = %d, root_model_id = %u, sub model size = %d",
         device_id, root_model_id, request.submodels_size());
  response.set_error_code(SUCCESS);
}

Status DeployContext::DownloadDevMaintenanceCfg(const deployer::DeployerRequest &request,
                                                deployer::DeployerResponse &response) {
  auto &download_conf_req = request.download_config_request();
  const auto sub_type = download_conf_req.sub_type();
  auto &config_data = download_conf_req.config_data();
  const auto device_id = download_conf_req.device_id();
  // get device debug config handle
  GELOGI("DownloadDevMaintenanceCfg enter, device_id=%d, sub_type=%d", device_id, sub_type);
  const void *config_buffer = config_data.data();
  uint64_t config_buffer_size = config_data.size();
  std::string config_str(static_cast<const char *>(config_buffer), config_buffer_size);
  GELOGI("DownloadDevMaintenanceCfg config_buffer_size=%lu, config_str=%s.",
         config_buffer_size, config_str.c_str());
  // load json data to device debug config
  if (dev_maintenance_cfg_.LoadJsonData(config_str) != SUCCESS) {
    response.set_error_code(FAILED);
    response.set_error_message("Parse model failed");
    return FAILED;
  }
  response.set_error_code(SUCCESS);
  GELOGI("[Handle][DownloadDevMaintenanceCfg] success");
  return SUCCESS;
}

Status DeployContext::PreDownloadModel(const deployer::DeployerRequest &request,
                                       deployer::DeployerResponse &response) {
  const auto &req_body = request.pre_download_request();
  uint32_t model_id = req_body.model_id();
  uint32_t root_model_id = req_body.root_model_id();
  const int32_t device_id = req_body.device_id();
  GELOGD("[PreDownload][Model] start, request = %s", req_body.DebugString().c_str());
  ExecutorProcess *executor = nullptr;
  ExecutorManager::ExecutorId context = {0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetOrForkExecutorProcess(context, &executor),
                    "[PreDownload][Model] Failed to get executor");
  deployer::ExecutorRequest executor_request;
  auto exec_req_body = executor_request.mutable_pre_download_message();
  GE_CHECK_NOTNULL(exec_req_body);
  exec_req_body->set_model_id(model_id);
  exec_req_body->set_root_model_id(root_model_id);
  exec_req_body->set_model_size(req_body.total_size());
  deployer::ExecutorResponse executor_response;
  GE_CHK_STATUS_RET(executor->SendRequest(executor_request, executor_response),
                    "[PreDownload][Model] Failed to send request");
  GE_CHK_STATUS_RET(SetResponse(executor_response, response),
                    "[PreDownload][Model] failed, model_id = %u, model_size = %lu", model_id, req_body.total_size());
  GELOGD("[PreDownload][Model] succeeded, model_id = %u, offset = %lu", model_id, req_body.total_size());
  return SUCCESS;
}

Status DeployContext::ParserRankIdFromRankTable(int32_t device_id, const std::string &ipaddr,
                                                const std::string &rank_table_list, std::string &rank_id) {
  std::vector<struct CommDomainNodeInfo> node_list;

  nlohmann::json j;
  try {
    j = nlohmann::json::parse(rank_table_list);
    auto j_rank_table = j.at("node_list");
    node_list = j_rank_table.get<std::vector<struct CommDomainNodeInfo>>();
  } catch (const nlohmann::json::exception &e) {
    GELOGE(FAILED, "[Check][RankTable] Invalid json, rankTable:%s, exception:%s", rank_table_list.c_str(), e.what());
    return FAILED;
  }

  for (const auto &node : node_list) {
    if (node.node_addr != ipaddr) {
      continue;
    }

    for (const auto &rank : node.ranks) {
      if (rank.device_id == std::to_string(device_id)) {
        rank_id = rank.rank_id;
        GELOGD("[RankTable] Parse rank id success.rank id=%s", rank_id.c_str());
        return SUCCESS;
      }
    }
  }
  GELOGE(FAILED, "[RankTable] Parse rank id failed.");
  return FAILED;
}

Status DeployContext::DeployRankTable(const deployer::DeployRankTableRequest &request,
                                      deployer::DeployerResponse &response) {
  const int32_t device_id = request.device_id();
  const auto &rank_table_list = request.rank_table();
  const auto &device_config = Configurations::GetInstance().GetDeviceInfo();
  GELOGI("[Deploy][RankTable] Deploy rank table start, rank table = %s", rank_table_list.c_str());

  std::string rank_id;
  int32_t deviceId = request.device_id();
  GE_CHK_STATUS_RET(ParserRankIdFromRankTable(deviceId, device_config.ipaddr, rank_table_list, rank_id),
                    "[Deploy][RankTable] Failed to get rank id.");

  ExecutorProcess *executor = nullptr;
  ExecutorManager::ExecutorId context = {0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetOrForkExecutorProcess(context, &executor),
                    "[Deploy][RankTable] Failed to get executor");
  deployer::ExecutorRequest executor_request;
  auto exec_req_body = executor_request.mutable_deploy_rank_table_message();
  GE_CHECK_NOTNULL(exec_req_body);
  exec_req_body->set_rank_table(rank_table_list);
  exec_req_body->set_rank_id(rank_id);
  deployer::ExecutorResponse executor_response;
  GE_CHK_STATUS_RET(executor->SendRequest(executor_request, executor_response),
                    "[Deploy][RankTable] Failed to send request");
  GE_CHK_STATUS_RET(SetResponse(executor_response, response),
                    "[Deploy][RankTable] failed, rank table=%s", rank_table_list.c_str());
  GELOGD("[Deploy][RankTable] succssfull, rank id=%s", rank_id.c_str());
  return SUCCESS;
}

Status DeployContext::DownloadModel(const deployer::DeployerRequest &request,
                                    deployer::DeployerResponse &response) {
  const auto &req_body = request.download_model_request();
  const int32_t device_id = req_body.device_id();
  GELOGD("[Download][Model] start, model_id = %u", req_body.model_id());
  ExecutorProcess *executor = nullptr;
  ExecutorManager::ExecutorId context{0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetExecutorProcess(context, &executor),
                    "[Download][Model] Failed to get executor");
  deployer::ExecutorRequest executor_request;
  auto exec_req_body = executor_request.mutable_download_model_message();
  GE_CHECK_NOTNULL(exec_req_body);
  uint32_t model_id = req_body.model_id();
  uint32_t root_model_id = req_body.root_model_id();
  exec_req_body->set_model_id(model_id);
  exec_req_body->set_root_model_id(root_model_id);
  exec_req_body->set_offset(req_body.offset());
  exec_req_body->set_model_data(req_body.om_content());
  deployer::ExecutorResponse executor_response;
  GE_CHK_STATUS_RET(executor->SendRequest(executor_request, executor_response),
                    "[Download][Model] Failed to send request");
  GE_CHK_STATUS_RET(SetResponse(executor_response, response),
                    "[Download][Model] failed, model_id = %u, offset = %lu", model_id, exec_req_body->offset());
  GELOGD("[Download][Model] succeeded, model_id = %u, offset = %lu", model_id, exec_req_body->offset());
  return SUCCESS;
}

Status DeployContext::LoadModel(const deployer::DeployerRequest &request,
                                deployer::DeployerResponse &response) {
  const auto &req_body = request.load_model_request();
  uint32_t sub_model_id = req_body.model_id();
  uint32_t root_model_id = req_body.root_model_id();
  const int32_t device_id = req_body.device_id();
  bool is_unknown_shape = req_body.is_unknown_shape();
  GELOGD("[Load][Model] start, device_id = %d, root_model_id = %u, sub_model_id = %u, is_unknown_shape = %d",
         device_id, root_model_id, sub_model_id, is_unknown_shape);
  std::vector<uint32_t> input_queues;
  std::vector<uint32_t> output_queues;
  DeployTaskId plan_context{device_id, root_model_id, sub_model_id};
  auto ret = GetQueues(plan_context, input_queues, output_queues);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] failed to resolve queue ids, sub_model_id = %u", sub_model_id);
    response.set_error_code(ret);
    response.set_error_message("failed to resolve queue ids");
    return SUCCESS;
  }

  ret = GrantQueuesForCpuSchedule(device_id, is_unknown_shape, input_queues, output_queues);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] failed to grant queue for cpu schedule, device_id = %d", device_id);
    response.set_error_code(ret);
    response.set_error_message("failed to grant queue for cpu schedule");
    return SUCCESS;
  }

  ExecutorProcess *executor = nullptr;
  ExecutorManager::ExecutorId context{0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetExecutorProcess(context, &executor),
                    "[Download][Model] Failed to get executor");
  deployer::ExecutorRequest executor_request;
  auto exec_req_body = executor_request.mutable_load_model_message();
  GE_CHECK_NOTNULL(exec_req_body);
  exec_req_body->set_model_id(sub_model_id);
  exec_req_body->set_root_model_id(root_model_id);
  exec_req_body->mutable_input_queues()->Add(input_queues.begin(), input_queues.end());
  exec_req_body->mutable_output_queues()->Add(output_queues.begin(), output_queues.end());
  deployer::ExecutorResponse executor_response;
  GELOGD("[Load][Model] begin, device_id = %d, root_model_id = %u, sub_model_id = %u, input_queues = %s, "
         "output_queues = %s", device_id, root_model_id, sub_model_id, ToString(input_queues).c_str(),
         ToString(output_queues).c_str());
  GE_CHK_STATUS_RET(executor->SendRequest(executor_request, executor_response),
                    "[Load][Model] Failed to send request");
  GE_CHK_STATUS_RET(SetResponse(executor_response, response),
                    "[Load][Model] failed, request = %s",
                    exec_req_body->DebugString().c_str());
  GELOGD("[Load][Model] succeeded, device_id = %d, root_model_id = %u, sub_model_id = %u, input_queues = %s, "
         "output_queues = %s", device_id, root_model_id, sub_model_id, ToString(input_queues).c_str(),
         ToString(output_queues).c_str());
  return SUCCESS;
}

Status DeployContext::UnloadModel(const deployer::DeployerRequest &request,
                                  deployer::DeployerResponse &response) {
  const auto &req_body = request.unload_model_request();
  uint32_t model_id = req_body.model_id();
  const int32_t device_id = req_body.device_id();
  GELOGD("[Unload][Model] start, device_id = %d, model_id = %u", device_id, model_id);
  ExecutorProcess *executor = nullptr;
  ExecutorManager::ExecutorId context{0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetExecutorProcess(context, &executor),
                    "[Download][Model] Failed to get executor, model_id = %u", model_id);
  deployer::ExecutorRequest executor_request;
  auto exec_req_body = executor_request.mutable_unload_model_message();
  GE_CHECK_NOTNULL(exec_req_body);
  exec_req_body->set_model_id(model_id);
  deployer::ExecutorResponse executor_response;
  auto ret = executor->SendRequest(executor_request, executor_response);
  DeployTaskId plan_context{device_id, model_id, 0};
  (void) FlowRouteManager::GetInstance().UndeployRoute(exchange_routes_[plan_context]);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[Unload][Model] Failed to send request to executor");
    response.set_error_code(FAILED);
    response.set_error_message("Failed to send request to executor");
    return SUCCESS;
  }
  GE_CHK_STATUS_RET(SetResponse(executor_response, response),
                    "[Unload][Model] failed, request = %s",
                    exec_req_body->DebugString().c_str());
  GELOGD("[Unload][Model] succeeded, model_id = %u", model_id);
  return SUCCESS;
}

Status DeployContext::ProcessMultiVarManager(const deployer::MultiVarManagerRequest &request,
                                             deployer::DeployerResponse &response) {
  GELOGI("[process][var_manager] Begin.");
  ExecutorProcess *executor = nullptr;
  const int32_t device_id = request.device_id();
  ExecutorManager::ExecutorId context{0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetOrForkExecutorProcess(context, &executor),
                    "[process][multi-var_manager] Failed to get executor.");
  deployer::ExecutorRequest executor_request;
  auto multi_var_manager = executor_request.mutable_multi_var_manager_info();
  *multi_var_manager = request.multi_var_manager_info();
  deployer::ExecutorResponse executor_response;
  GE_CHK_STATUS_RET(executor->SendRequest(executor_request, executor_response),
                    "[Process][VarManager] Failed to send request.");
  GE_CHK_STATUS_RET(SetResponse(executor_response, response), "[Process][VarManager] failed.");
  GELOGI("[process][var_manager] SUCCESS.");
  return SUCCESS;
}

Status DeployContext::ProcessSharedContent(const deployer::SharedContentDescRequest &request,
                                           deployer::DeployerResponse &response) {
  ExecutorProcess *executor = nullptr;
  const int32_t device_id = request.device_id();
  ExecutorManager::ExecutorId context{0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetExecutorProcess(context, &executor), "Failed to get executor process.");
  deployer::ExecutorRequest executor_request;
  auto shared_content_desc = executor_request.mutable_shared_content_desc();
  *shared_content_desc = request.shared_content_desc();
  deployer::ExecutorResponse executor_response;
  GE_CHK_STATUS_RET(executor->SendRequest(executor_request, executor_response),
                    "[Process][shared_content] Failed to send request.");
  GE_CHK_STATUS_RET(SetResponse(executor_response, response),
                    "[Process][shared_content] Failed.");
  GELOGI("[process][shared_content] SUCCESS.");
  return SUCCESS;
}

Status DeployContext::SetResponse(const deployer::ExecutorResponse &executor_response,
                                  deployer::DeployerResponse &deployer_response) {
  auto ret = executor_response.status_code();
  deployer_response.set_error_code(ret);
  deployer_response.set_error_message(executor_response.error_message());
  return ret;
}

Status DeployContext::GetQueues(const DeployTaskId &client_context,
                                std::vector<uint32_t> &input_queues,
                                std::vector<uint32_t> &output_queues) const {
  const auto &it = submodel_descs_.find(client_context);
  DeployTaskId plan_context{client_context};
  plan_context.sub_model_id = 0;
  const auto &route_it = exchange_routes_.find(plan_context);
  if (it == submodel_descs_.end() || route_it == exchange_routes_.end()) {
    GELOGE(FAILED, "model not pre-deployed, device_id = %d, root_model_id = %u, sub_model_id = %u.",
           client_context.device_id, client_context.root_model_id, client_context.sub_model_id);
    return FAILED;
  }

  const auto &submodel_desc = it->second;
  const auto *exchange_route = FlowRouteManager::GetInstance().QueryRoute(route_it->second);
  GE_CHECK_NOTNULL(exchange_route);
  std::vector<int32_t>
      input_queue_indices(submodel_desc.input_queue_indices().begin(), submodel_desc.input_queue_indices().end());
  GE_CHK_STATUS_RET(exchange_route->GetQueueIds(input_queue_indices, input_queues),
                    "Failed to get input queue ids");
  std::vector<int32_t>
      output_queue_indices(submodel_desc.output_queue_indices().begin(), submodel_desc.output_queue_indices().end());
  GE_CHK_STATUS_RET(exchange_route->GetQueueIds(output_queue_indices, output_queues),
                    "Failed to get output queue ids");
  return SUCCESS;
}

Status DeployContext::GrantQueuesForCpuSchedule(int32_t device_id, bool is_unknown_shape,
                                                const std::vector<uint32_t> &input_queues,
                                                const std::vector<uint32_t> &output_queues) {
  int32_t cpu_schedule_pid = 0U;
  GE_CHK_STATUS_RET(GetCpuSchedulePid(device_id, is_unknown_shape, cpu_schedule_pid),
                    "Failed to get cpu schedule pid, device id = %d, is_unknown_shape = %d",
                    device_id, is_unknown_shape);

  GELOGD("get cpu schedule pid[%d] success, device id = %d, is_unknown_shape = %d",
         cpu_schedule_pid, device_id, is_unknown_shape);

  return DataGwManager::GetInstance().GrantQueueForCpuSchedule(static_cast<uint32_t>(device_id),
                                                               cpu_schedule_pid,
                                                               input_queues,
                                                               output_queues);
}

Status DeployContext::GetCpuSchedulePid(int32_t device_id, bool is_unknown_shape, int32_t &pid) {
  ExecutorProcess *executor = nullptr;
  ExecutorManager::ExecutorId context = {0, device_id};
  GE_CHK_STATUS_RET(executor_manager_.GetExecutorProcess(context, &executor), "[GrantQueues] Failed to get executor");
  const int32_t executor_pid = executor->GetPid();
  if (executor_manager_.IsHost() || is_unknown_shape) {
    pid = executor_pid;
    GELOGD("Get cpu schedule pid = %d success", pid);
    return SUCCESS;
  }

  rtBindHostpidInfo_t info{};
  info.cpType = RT_DEV_PROCESS_CP1;
  info.hostPid = executor_pid;
  info.chipId = static_cast<uint32_t>(device_id);
  rtError_t ret = rtQueryDevPid(&info, &pid);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "Query aicpu schedule failed, deviceId=%d, hostPid=%d.", device_id, executor_pid);
        REPORT_CALL_ERROR("E19999", "Query aicpu schedule failed, deviceId=%d, hostPid=%d.", device_id, executor_pid);
    return FAILED;
  }
  GELOGD("Get cpu schedule pid = %d success", pid);
  return SUCCESS;
}
}  // namespace ge