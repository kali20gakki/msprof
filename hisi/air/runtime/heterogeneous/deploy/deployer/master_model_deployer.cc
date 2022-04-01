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
#include "deploy/deployer/master_model_deployer.h"
#include <algorithm>
#include <functional>
#include <fstream>
#include <future>
#include "framework/common/util.h"
#include "framework/common/ge_types.h"
#include "framework/common/helper/model_helper.h"
#include "framework/common/file_constant_util.h"
#include "deploy/flowrm/datagw_manager.h"
#include "deploy/flowrm/network_manager.h"
#include "deploy/flowrm/helper_exchange_deployer.h"
#include "deploy/flowrm/flow_route_manager.h"
#include "deploy/resource/resource_manager.h"
#include "common/data_flow/queue/helper_exchange_service.h"
#include "exec_runtime/execution_runtime.h"
#include "deploy/deployer/helper_deploy_planner.h"
#include "deploy/helper_execution_runtime.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/ge_context.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "securec.h"
#include "deploy/hcom/communication_domain.h"
#include "common/config/configurations.h"

namespace ge {
namespace {
constexpr int32_t kNodeIdLocal = 0;
constexpr int32_t kDeviceIdHost = 0;
constexpr int32_t kEndpointTypeNone = -1;
constexpr int32_t kMaxClusterStrLen = 1024;
}

Status MasterModelDeployer::Initialize(const std::map<std::string, std::string> &options) {
  NodeConfig local_node {};
  local_node.device_id = -1;
  std::vector<NodeConfig> node_config_list {local_node};
  const auto &remote_node_config_list = Configurations::GetInstance().GetRemoteNodeConfigList();
  (void) node_config_list.insert(node_config_list.cend(),
                                 remote_node_config_list.cbegin(),
                                 remote_node_config_list.cend());
  GE_CHK_STATUS_RET(DeviceMaintenanceMasterCfg::InitGlobalMaintenanceConfigs());
  GE_CHK_STATUS_RET(DeployerProxy::GetInstance().Initialize(node_config_list), "Failed to initialize device proxy.");
  GE_CHK_STATUS_RET(ResourceManager::GetInstance().Initialize(), "Failed to initialize ResourceManager");
  return SUCCESS;
}

Status MasterModelDeployer::Finalize() {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto &it : deployed_models_) {
    auto &deployed_model = it.second;
    (void) UndeployModel(deployed_model);
  }

  deployed_models_.clear();
  (void) ResourceManager::GetInstance().Finalize();
  (void) DeployerProxy::GetInstance().Finalize();
  return SUCCESS;
}

Status MasterModelDeployer::DeployModel(const FlowModelPtr &flow_model,
                                        const vector<uint32_t> &input_queue_ids,
                                        const vector<uint32_t> &output_queue_ids,
                                        DeployResult &deploy_result) {
  (void)input_queue_ids;
  (void)output_queue_ids;
  // 0. allocate model_id
  deploy_result.model_id = model_id_gen_++;
  // it'll take 136 years to exhaust model id if load one model per second
  GE_CHECK_LE(deploy_result.model_id, UINT32_MAX - 1U);
  GELOGD("[Deploy][Model] start, model_id = %u", deploy_result.model_id);

  // 1. resolve deploy plan
  std::vector<DeployPlan::DeviceInfo> available_devices;
  GE_CHK_STATUS_RET(GetAvailableDevices(available_devices));
  DeployPlan deploy_plan;
  GE_CHK_STATUS_RET(HelperDeployPlanner(flow_model, available_devices).BuildPlan(deploy_plan),
                    "Failed to build DeployPlan.");
  GELOGD("[Deploy][Model] deploy plan built successfully");

  // 2. deploy submodels
  MasterModelDeployer::DeployedModel deployed_model;
  deployed_model.model_id = deploy_result.model_id;
  GE_DISMISSABLE_GUARD(deployed_model, [&]() {
    GELOGE(FAILED, "Error occurred while deploying model, now start rollback");
    (void) UndeployModel(deployed_model);
  });
  GE_CHK_STATUS_RET_NOLOG(DeploySubmodels(deploy_plan, deployed_model));
  GELOGD("[Deploy][Model] submodels deployed successfully");

  // 3. get input/output queue ids
  const auto *local_route = FlowRouteManager::GetInstance().QueryRoute(deployed_model.route_id);
  GE_CHECK_NOTNULL(local_route);
  GE_CHK_STATUS_RET_NOLOG(GetModelIoQueueIds(deploy_plan, *local_route, deploy_result));
  GELOGD("[Deploy][Model] success, model_id = %u, input queues = %s, output queues = %s",
         deploy_result.model_id,
         ToString(deploy_result.input_queue_ids).c_str(),
         ToString(deploy_result.output_queue_ids).c_str());
  // 4. record DeployInfo
  std::lock_guard<std::mutex> lk(mu_);
  deployed_models_.emplace(deploy_result.model_id, std::move(deployed_model));
  GE_DISMISS_GUARD(deployed_model);
  return SUCCESS;
}

Status MasterModelDeployer::GetAvailableDevices(std::vector<DeployPlan::DeviceInfo> &available_devices) {
  if (GetContext().GetHostExecFlag()) {
    available_devices.emplace_back(DeployPlan::DeviceInfo{0, kNodeIdLocal, 0});
  } else {
    for (const auto &device_info : ResourceManager::GetInstance().GetNpuDeviceInfoList()) {
      available_devices.emplace_back(DeployPlan::DeviceInfo{0, device_info.GetNodeId(), device_info.GetDeviceId()});
    }
  }
  return SUCCESS;
}

Status MasterModelDeployer::Undeploy(uint32_t model_id) {
  MasterModelDeployer::DeployedModel deployed_model;
  {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = deployed_models_.find(model_id);
    if (it == deployed_models_.end()) {
      GELOGE(PARAM_INVALID, "Failed to undeploy model, model id not found, id = %u", model_id);
      return PARAM_INVALID;
    }
    deployed_model = std::move(it->second);
    deployed_models_.erase(it);
  }

  UndeployModel(deployed_model);
  return SUCCESS;
}

void MasterModelDeployer::UndeployModel(MasterModelDeployer::DeployedModel &deployed_model) {
  auto model_id = deployed_model.model_id;
  for (const auto &remote_device : deployed_model.deployed_remote_devices) {
    (void) UndeployRemoteSubmodels(remote_device, deployed_model.model_id);
  }

  (void) UndeployLocalSubmodels(deployed_model);
  GELOGD("[Undeploy][Model] ended, model id = %u", model_id);
}

Status MasterModelDeployer::DeploySubmodels(DeployPlan &deploy_plan,
                                            MasterModelDeployer::DeployedModel &deployed_model) {
  GELOGD("[Deploy][Submodels] started, root_model_id = %u", deployed_model.model_id);
  std::vector<const DeployPlan::SubmodelInfo *> local_models;
  std::map<std::string, std::vector<const DeployPlan::SubmodelInfo *>> remote_models;
  GroupSubmodels(deploy_plan, local_models, remote_models);

  // the creation of hcom handle in master node should precede that of slave node
  deployer::PreDeployModelRequest master_pre_deploy_request;
  DeployPlan::DeviceInfo local_device_info{0, kNodeIdLocal, 0};
  GE_CHK_STATUS_RET(BuildPreDeployRequest(deploy_plan, local_device_info, local_models, master_pre_deploy_request),
                    "[Plan][Route] failed to build local route plan, model_id = %u", deployed_model.model_id);
  GE_CHK_STATUS_RET(MasterDeployRankTable(), "[Deploy][RankTable] failed to deploy rank table.");
  GE_CHK_STATUS_RET(DeployDevMaintenaceCfg(remote_models), "[Deploy][DevMaintenaceCfg] failed");
  auto &exchange_plan = master_pre_deploy_request.exchange_plan();
  HelperExchangeDeployer exchange_deployer(HelperExchangeService::GetInstance(), exchange_plan, kDeviceIdHost);
  GE_CHK_STATUS_RET(exchange_deployer.PreDeploy(),
                    "[Deploy][Route] failed to pre-deploy local route, model_id = %u",
                    deployed_model.model_id);

  GE_CHK_STATUS_RET(DeployRemoteVarManager(deploy_plan, remote_models, deployed_model),
                    "[Deploy][RemoteVarManager] failed");

  // then deploy remote route
  GE_CHK_STATUS_RET(DeployRemoteSubmodels(deploy_plan, remote_models, deployed_model),
                    "[Deploy][RemoteSubmodels] failed");

  // finish the bindings of endpoints in master node
  ExchangeRoute local_route;
  GE_CHK_STATUS_RET(exchange_deployer.Deploy(local_route),
                    "[Deploy][Route] failed to pre-deploy local route, model_id = %u",
                    deployed_model.model_id);
  GE_CHK_STATUS_RET_NOLOG(FlowRouteManager::GetInstance().AddRoute(local_route, deployed_model.route_id));
  GE_CHK_STATUS_RET(DeployLocalSubmodels(local_models, deployed_model), "[Deploy][LocalSubmodels] failed");
  GELOGD("[Deploy][Submodels] succeeded, root_model_id = %u", deployed_model.model_id);
  return SUCCESS;
}

void MasterModelDeployer::GroupSubmodels(const DeployPlan &deploy_plan,
                                         std::vector<const DeployPlan::SubmodelInfo *> &local_models,
                                         std::map<std::string,
                                                  std::vector<const DeployPlan::SubmodelInfo *>> &remote_models) {
  for (const auto &it : deploy_plan.GetSubmodels()) {
    const auto &node_id = it.second.device_info.GetNodeId();
    if (node_id == kNodeIdLocal) {
      local_models.emplace_back(&it.second);
    } else {
      remote_models[it.second.device_info.GetKey()].emplace_back(&it.second);
    }
  }
}

void MasterModelDeployer::SetSubModels(deployer::PreDeployModelRequest &request,
                                       const vector<const DeployPlan::SubmodelInfo *> &submodels) {
  for (const auto &submodel : submodels) {
    auto submodel_desc = request.add_submodels();
    submodel_desc->set_model_name(submodel->model->GetModelName());
    std::for_each(submodel->input_queue_indices.begin(),
                  submodel->input_queue_indices.end(),
                  std::bind(&deployer::SubmodelDesc::add_input_queue_indices, submodel_desc, std::placeholders::_1));
    std::for_each(submodel->control_input_queue_indices.begin(),
                  submodel->control_input_queue_indices.end(),
                  std::bind(&deployer::SubmodelDesc::add_input_queue_indices, submodel_desc, std::placeholders::_1));
    std::for_each(submodel->output_queue_indices.begin(),
                  submodel->output_queue_indices.end(),
                  std::bind(&deployer::SubmodelDesc::add_output_queue_indices, submodel_desc, std::placeholders::_1));
  }
}

Status MasterModelDeployer::BuildPreDeployRequest(const DeployPlan &deploy_plan,
                                                  const DeployPlan::DeviceInfo &target_device_info,
                                                  const vector<const DeployPlan::SubmodelInfo *> &submodels,
                                                  deployer::PreDeployModelRequest &request) {
  auto exchange_plan = request.mutable_exchange_plan();
  GE_CHECK_NOTNULL(exchange_plan);
  GE_CHK_STATUS_RET_NOLOG(ResolveExchangePlan(deploy_plan, target_device_info, *exchange_plan));
  SetSubModels(request, submodels);
  return SUCCESS;
}

Status MasterModelDeployer::GetModelIoQueueIds(const DeployPlan &deploy_plan,
                                               const ExchangeRoute &route,
                                               DeployResult &deploy_result) {
  GE_CHK_STATUS_RET(route.GetQueueIds(deploy_plan.GetInputQueueIndices(), deploy_result.input_queue_ids),
                    "Failed to get model input queue ids");
  GE_CHK_STATUS_RET(route.GetQueueIds(deploy_plan.GetControlInputQueueIndices(), deploy_result.control_input_queue_ids),
                    "Failed to get model control input queue ids");
  GE_CHK_STATUS_RET(route.GetQueueIds(deploy_plan.GetOutputQueueIndices(), deploy_result.output_queue_ids),
                    "Failed to get model output queue ids");
  return SUCCESS;
}

Status MasterModelDeployer::UndeployRemoteSubmodels(const DeployPlan::DeviceInfo &device_info,
                                                    uint32_t root_model_id) {
  const int32_t device_id = device_info.GetNodeId();
  GELOGD("[Undeploy][Submodels] start, device_id = %d, root_model_id = %u.", device_id, root_model_id);
  deployer::DeployerRequest request;
  request.set_type(deployer::kUnloadModel);
  auto unload_model_req = request.mutable_unload_model_request();
  GE_CHECK_NOTNULL(unload_model_req);
  unload_model_req->set_model_id(root_model_id);
  unload_model_req->set_device_id(device_info.GetDeviceId());
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(device_id, request, response),
                    "[Undeploy][Submodels] failed to send request, device_id = %d, root_model_id = %u",
                    device_id, root_model_id);
  if (response.error_code() != SUCCESS) {
    GELOGE(FAILED,
           "[Undeploy][Submodels] failed, device_id = %d, root_model_id = %u, error code = %u error message = %s",
           device_id,
           root_model_id,
           response.error_code(),
           response.error_message().c_str());
    return FAILED;
  }
  GELOGD("[Undeploy][Submodels] succeeded, device_id = %d, root_model_id = %u", device_id, root_model_id);
  return SUCCESS;
}

Status MasterModelDeployer::UndeployLocalSubmodels(MasterModelDeployer::DeployedModel &deployed_model) {
  auto execution_runtime = ExecutionRuntime::GetInstance();
  GE_CHECK_NOTNULL(execution_runtime);
  FlowRouteManager::GetInstance().UndeployRoute(deployed_model.route_id);
  GELOGD("[Undeploy][Route] succeeded, model_id = %u", deployed_model.model_id);
  return SUCCESS;
}

Status MasterModelDeployer::DeployLocalSubmodels(const std::vector<const DeployPlan::SubmodelInfo *> &submodels,
                                                 MasterModelDeployer::DeployedModel &deployed_model) {
  if (submodels.empty()) {
    return SUCCESS;
  }

  GELOGD("[Deploy][LocalSubmodels] start, root_model_id = %u, submodel number = %zu",
         deployed_model.model_id,
         submodels.size());
  DeployPlan::DeviceInfo local_device_info{0, kNodeIdLocal, 0};
  GE_CHK_STATUS_RET_NOLOG(PreDeployLocalSubmodels(deployed_model.model_id, submodels, deployed_model.route_id));
  GE_CHK_STATUS_RET_NOLOG(DoDeploySubmodels(local_device_info, submodels, deployed_model));
  GE_CHK_STATUS_RET_NOLOG(DoLoadSubmodels(local_device_info, submodels, deployed_model));
  GELOGD("[Deploy][LocalSubmodels] success, root_model_id = %u", deployed_model.model_id);
  return SUCCESS;
}

Status MasterModelDeployer::DeployRemoteSubmodels(const DeployPlan &deploy_plan,
                                                  const map<std::string, std::vector<ConstSubmodelInfoPtr>> &models,
                                                  MasterModelDeployer::DeployedModel &deployed_model) {
  for (const auto &it : models) {
    const auto &submodels = it.second;
    const auto &device_info = submodels[0]->device_info;
    GE_CHK_STATUS_RET_NOLOG(PreDeployRemoteSubmodels(deploy_plan,
                                                     device_info,
                                                     deployed_model.model_id,
                                                     submodels));
    GE_CHK_STATUS_RET_NOLOG(DoDeploySubmodels(device_info,
                                              submodels,
                                              deployed_model));
  }

  std::vector<std::future<Status>> futures;
  for (const auto &it : models) {
    const auto &submodels = it.second;
    const auto &device_info = submodels[0]->device_info;
    GELOGD("Begin load remote submodels[%zu]", futures.size());
    auto future = std::async(std::launch::async, [&]() -> Status {
      GE_CHK_STATUS_RET_NOLOG(DoLoadSubmodels(device_info,
                                              submodels,
                                              deployed_model));
      return SUCCESS;
    });
    futures.emplace_back(std::move(future));
  }

  bool has_error = false;
  for (size_t i = 0U; i < futures.size(); ++i) {
    GELOGD("Begin to sync load submodels[%zu]", i);
    const auto ret = futures[i].get();
    if (ret != SUCCESS) {
      GELOGE(ret, "[Check][Result][Submodels: %zu] Failed to load submodels.", i);
      REPORT_INNER_ERROR("E19999", "[Submodels: %zu] Failed to load submodels.", i);
      has_error = true;
    }
  }

  return has_error ? FAILED : SUCCESS;
}

Status MasterModelDeployer::DeployDevMaintenaceCfg(const std::map<std::string,
                                                                  std::vector<ConstSubmodelInfoPtr>> &models) {
  std::set<int32_t> dev_ids;
  for (const auto &it : models) {
    const auto &submodels = it.second;
    const auto device_id = submodels[0]->device_info.GetNodeId();
    (void) dev_ids.emplace(device_id);
  }
  for (const auto &dev_id : dev_ids) {
    GE_CHK_STATUS_RET(DownloadDevMaintenanceCfg(dev_id), "[Download][DevMaintenanceCfg] failed, dev_id[%d].", dev_id);
  }
  return SUCCESS;
}

Status MasterModelDeployer::DeployRemoteVarManager(const DeployPlan &deployPlan,
                                                   const std::map<std::string,
                                                                  std::vector<ConstSubmodelInfoPtr>> &models,
                                                   MasterModelDeployer::DeployedModel &deployed_model) {
  std::map<int32_t, std::set<uint64_t>> sessions;
  std::map<int32_t, std::map<uint64_t, std::set<OpDescPtr>>> node_need_transfer_memory;
  std::map<int32_t, std::set<int32_t>> sub_device_ids;
  for (const auto &it : models) {
    const auto &submodels = it.second;
    const auto device_id = submodels[0]->device_info.GetNodeId();
    GELOGD("[Deploy][RemoteVarManager] started, device_id = %d, submodel cout = %zu.", device_id, submodels.size());
    GE_CHK_STATUS_RET(GetAllRelatedVarManager(submodels[0]->device_info, submodels, sessions,
                                              node_need_transfer_memory, sub_device_ids),
                      "Failed to GetAllRelatedVarManager");
    GELOGD("[Deploy][RemoteVarManager] Success, device_id = %d.", device_id);
  }
  GE_CHK_STATUS_RET(GetVarManagerAndSendToRemote(sub_device_ids, sessions, node_need_transfer_memory),
                    "Failed to GetVarManagerAndSendToRemote.");
  return SUCCESS;
}

Status MasterModelDeployer::CopyOneWeightToTransfer(const SendInfo &send_info, std::istream &input_stream,
                                                    const int64_t file_constant_size, const OpDescPtr &op_desc) {
  int32_t device_id = send_info.device_id;
  int64_t session_id = send_info.session_id;
  GELOGI("Enter to CopyOneWeightToTransfer, file constant size = %ld", file_constant_size);

  const int64_t kBlockSize = 10485760;
  std::string compress_nodes;
  size_t used_memory = 0;
  size_t copy_len_once = 0;
  size_t file_size = static_cast<size_t>(file_constant_size);
  compress_nodes.reserve(kBlockSize);
  auto tensor_desc = op_desc->MutableOutputDesc(0);
  uint8_t *var_logic = nullptr;
  VarManager::Instance(GetContext().SessionId())->GetVarAddr(op_desc->GetName(), *tensor_desc, var_logic);
  rtMemType_t memory_type = RT_MEMORY_HBM;
  auto mem_type = static_cast<uint32_t>(RT_MEMORY_DEFAULT);
  if (AttrUtils::GetInt(op_desc, ATTR_OUTPUT_MEMORY_TYPE, mem_type) && (mem_type == 1)) {  // 1: rdma
    memory_type = RT_MEMORY_RDMA_HBM;
  }
  while ((!input_stream.eof()) && (used_memory != file_size)) {
    input_stream.read(&compress_nodes[0], kBlockSize);
    copy_len_once = input_stream.gcount();
    if (file_constant_size - used_memory < copy_len_once) {
      copy_len_once = file_constant_size - used_memory;
    }

    deployer::DeployerRequest request;
    request.set_type(deployer::kDownloadSharedContent);
    auto shared_content_desc_request = request.mutable_shared_content_desc_request();
    GE_CHECK_NOTNULL(shared_content_desc_request);
    shared_content_desc_request->set_device_id(send_info.sub_device_id);
    auto shared_content_description = shared_content_desc_request->mutable_shared_content_desc();
    GE_CHECK_NOTNULL(shared_content_description);
    shared_content_description->set_session_id(session_id);
    shared_content_description->set_node_name(op_desc->GetName());
    shared_content_description->set_head_offset(reinterpret_cast<uint64_t>(var_logic));
    shared_content_description->set_total_length(file_constant_size);
    shared_content_description->set_current_offset(used_memory);
    shared_content_description->set_mem_type(memory_type);

    proto::TensorDescriptor *tensor_desc_proto = shared_content_description->mutable_tensor_desc();
    GeTensorSerializeUtils::GeTensorDescAsProto(*tensor_desc, tensor_desc_proto);

    shared_content_description->set_om_content(&compress_nodes[0], copy_len_once);
    deployer::DeployerResponse response;
    GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(device_id, request, response),
                      "[Send] [shared_content] failed.");
    used_memory += copy_len_once;
  }

  GELOGI("Out to CopyOneWeightToTransfer, used_memory = %zu, copy_len_once = %zu.", used_memory, copy_len_once);
  return SUCCESS;
}

Status MasterModelDeployer::GetAllRelatedVarManager(
    const DeployPlan::DeviceInfo &device_info,
    const std::vector<const DeployPlan::SubmodelInfo *> &submodels,
    std::map<int32_t, std::set<uint64_t>> &sessions,
    std::map<int32_t, std::map<uint64_t, std::set<OpDescPtr>>> &node_need_transfer_memory,
    std::map<int32_t, std::set<int32_t>> &sub_device_ids) {
  const auto device_id = device_info.GetNodeId();
  for (const auto &submodel : submodels) {
    GeRootModelPtr root_model =  std::dynamic_pointer_cast<GeRootModel>(submodel->model);
    if (root_model == nullptr) {
      continue;
    }
    auto root_graph = root_model->GetRootGraph();
    if (root_model->GetSubgraphInstanceNameToModel().empty()) {
      GELOGE(FAILED, "submodel is empty");
      return FAILED;
    }
    // session id is same for every submodel
    const auto &ge_model = root_model->GetSubgraphInstanceNameToModel().begin()->second;
    int64_t value = 0;
    auto ret = ge::AttrUtils::GetInt(ge_model, ge::MODEL_ATTR_SESSION_ID, value);
    uint64_t session = ret ? static_cast<uint64_t>(value) : 0U;
    auto session_iter = sessions.find(device_id);
    if (session_iter == sessions.end()) {
      std::set<uint64_t> session_vec = {session};
      sessions[device_id] = session_vec;
    } else {
      session_iter->second.emplace(session);
    }
    sub_device_ids[device_id].emplace(device_info.GetDeviceId());
    const Graph &graph = ge_model->GetGraph();
    ComputeGraphPtr compute_graph = GraphUtils::GetComputeGraph(graph);
    GE_CHK_BOOL_RET_STATUS(compute_graph != nullptr, INTERNAL_ERROR, "[Get][ComputeGraph] failed, ret is nullptr.");
    for (const auto &node : compute_graph->GetAllNodes()) {
      const auto &op_desc = node->GetOpDesc();
      GE_CHECK_NOTNULL(op_desc);
      if (op_desc->GetType() != FILECONSTANT) {
        continue;
      }
      auto node_it = node_need_transfer_memory.find(device_id);
      if (node_it == node_need_transfer_memory.end()) {
        std::set<OpDescPtr> op_desc_set = {op_desc};
        std::map<uint64_t, std::set<OpDescPtr>> session_op_desc_map;
        session_op_desc_map[session] = op_desc_set;
        node_need_transfer_memory[device_id] = session_op_desc_map;
      } else {
        auto &map_info = node_it->second;
        auto session_op_desc_map_it = map_info.find(session);
        if (session_op_desc_map_it == map_info.end()) {
          std::set<OpDescPtr> op_desc_set = {op_desc};
          map_info[session] = op_desc_set;
        } else {
          session_op_desc_map_it->second.emplace(op_desc);
        }
      }
    }
  }
  return SUCCESS;
}

Status MasterModelDeployer::GetVarManagerAndSendToRemote(
    const std::map<int32_t, std::set<int32_t>> &sub_device_ids,
    const std::map<int32_t, std::set<uint64_t>> &sessions,
    const std::map<int32_t, std::map<uint64_t, std::set<OpDescPtr>>> &node_need_transfer_memory) {
  for (const auto &session_iter : sessions) {
    auto device_id = session_iter.first;
    auto session_vec = session_iter.second;
    auto dev_iter = sub_device_ids.find(device_id);
    if (dev_iter == sub_device_ids.end()) {
      continue;
    }
    for (const auto sub_device_id : dev_iter->second) {
      deployer::DeployerRequest request;
      request.set_type(deployer::kDownloadVarManager);
      auto multi_var_manger_request = request.mutable_multi_var_manager_request();
      GE_CHECK_NOTNULL(multi_var_manger_request);
      multi_var_manger_request->set_device_id(sub_device_id);
      auto multi_var_manger_info = multi_var_manger_request->mutable_multi_var_manager_info();
      GE_CHECK_NOTNULL(multi_var_manger_info);
      for (const auto &session : session_vec) {
        GELOGI("[VarManager] The Session is %ld to serial.", session);
        auto single_info = multi_var_manger_info->add_var_manager_info();
        GE_CHECK_NOTNULL(single_info);
        VarManager::Instance(session)->VarManagerToSerial(session, *single_info);
      }
      deployer::DeployerResponse response;
      GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(device_id, request, response),
                        "[send][VarManager] failed.");
      if (response.error_code() != SUCCESS) {
        GELOGE(FAILED, "Failed to send var manager info, device id = %d, error code = %u, error message = %s.",
               device_id, response.error_code(), response.error_message().c_str());
        return FAILED;
      }
    }
  }

  std::map<std::string, std::string> file_id_to_path;
  GE_CHK_STATUS_RET(GetFilePathFromOption(file_id_to_path), "Failed to get file path");
  for (const auto &it : node_need_transfer_memory) {
    GELOGI("[VarManager] process shared memory.");
    auto device_id = it.first;
    auto session_op_desc_map = it.second;
    auto dev_iter = sub_device_ids.find(device_id);
    if (dev_iter == sub_device_ids.end()) {
      continue;
    }
    for (const auto &session_iter : session_op_desc_map) {
      auto session_id = session_iter.first;
      auto op_desc_set = session_iter.second;
      for (const auto &op_desc : op_desc_set) {
        int64_t total_length = 0;
        ge::ConstGeTensorDescPtr tensor_desc = op_desc->GetOutputDescPtr(0U);
        if (VarManager::Instance(session_id)->IsVarReady(op_desc->GetName(), *tensor_desc)) {
          continue;
        }

        for (const auto sub_device_id : dev_iter->second) {
          SendInfo send_info = {session_id, device_id, sub_device_id};
          GE_CHECK_NOTNULL(tensor_desc);
          GE_CHK_STATUS_RET(TensorUtils::GetTensorSizeInBytes(*tensor_desc, total_length),
                            "Failed to get size of file constant(%s).", op_desc->GetName().c_str());
          std::string file_path;
          GE_CHK_STATUS_RET(GetFilePath(op_desc, file_id_to_path, file_path), "Failed to get file path.");
          std::unique_ptr<std::istream> input_stream;
          GE_CHK_STATUS_RET_NOLOG(CreateInputStream(file_path, input_stream));
          GE_CHK_STATUS_RET(CopyOneWeightToTransfer(send_info, *input_stream, total_length, op_desc),
                            "Failed to send data.");
        }
        VarManager::Instance(session_id)->SetVarIsReady(op_desc->GetName(), *tensor_desc);
      }
    }
  }
  return SUCCESS;
}

Status MasterModelDeployer::DoDeploySubmodels(const DeployPlan::DeviceInfo &device_info,
                                              const vector<const DeployPlan::SubmodelInfo *> &submodels,
                                              MasterModelDeployer::DeployedModel &deployed_model) {
  auto root_model_id = deployed_model.model_id;
  for (uint32_t submodel_id = 0U; submodel_id < static_cast<uint32_t>(submodels.size()); ++submodel_id) {
    const auto &model = submodels[submodel_id]->model;
    GE_CHK_STATUS_RET_NOLOG(TransferModel(device_info, root_model_id, submodel_id, model));
  }

  return SUCCESS;
}

Status MasterModelDeployer::DoLoadSubmodels(const DeployPlan::DeviceInfo &device_info,
                                            const vector<const DeployPlan::SubmodelInfo *> &submodels,
                                            MasterModelDeployer::DeployedModel &deployed_model) {
  auto root_model_id = deployed_model.model_id;
  for (uint32_t submodel_id = 0U; submodel_id < static_cast<uint32_t>(submodels.size()); ++submodel_id) {
    const auto &model = submodels[submodel_id]->model;
    const ComputeGraphPtr root_graph = model->GetRootGraph();
    GE_CHECK_NOTNULL(root_graph);
    bool is_unknown_shape = false;
    (void) AttrUtils::GetBool(root_graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, is_unknown_shape);
    GE_CHK_STATUS_RET_NOLOG(LoadModel(device_info, root_model_id, submodel_id, is_unknown_shape));
  }

  GELOGD("Remote device [%s] deployed submodels successfully.", device_info.GetDesc().c_str());
  deployed_model.deployed_remote_devices.emplace_back(device_info);
  return SUCCESS;
}

Status MasterModelDeployer::PreDeployLocalSubmodels(uint32_t root_model_id,
                                                    const vector<const DeployPlan::SubmodelInfo *> &submodels,
                                                    int64_t route_id) {
  deployer::DeployerRequest request;
  request.set_type(deployer::kPreDeployModel);
  auto pre_deploy_request = request.mutable_pre_deploy_model_request();
  GE_CHECK_NOTNULL(pre_deploy_request);
  pre_deploy_request->set_root_model_id(root_model_id);
  pre_deploy_request->set_device_id(kDeviceIdHost);
  pre_deploy_request->set_route_id(route_id);
  SetSubModels(*pre_deploy_request, submodels);
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(kNodeIdLocal, request, response),
                    "[PreDeploy] failed to pre-deploy local submodels");
  if (response.error_code() != SUCCESS) {
    GELOGE(FAILED,
           "[PreDeployLocal] failed, error code = %u, error message = %s",
           response.error_code(),
           response.error_message().c_str());
    return FAILED;
  }
  GELOGD("[PreDeployLocal] succeeded, root_model_id = %u", root_model_id);
  return SUCCESS;
}

Status MasterModelDeployer::PreDeployRemoteSubmodels(const DeployPlan &deploy_plan,
                                                     const DeployPlan::DeviceInfo &target_device_info,
                                                     uint32_t root_model_id,
                                                     const vector<const DeployPlan::SubmodelInfo *> &submodels) {
  deployer::DeployerRequest request;
  request.set_type(deployer::kPreDeployModel);
  auto pre_deploy_request = request.mutable_pre_deploy_model_request();
  GE_CHECK_NOTNULL(pre_deploy_request);
  GE_CHK_STATUS_RET_NOLOG(BuildPreDeployRequest(deploy_plan, target_device_info, submodels, *pre_deploy_request));
  pre_deploy_request->set_root_model_id(root_model_id);
  pre_deploy_request->set_device_id(target_device_info.GetDeviceId());
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(target_device_info.GetNodeId(), request, response),
                    "[PreDeploy] failed to send PreDeployRequest, device_id = %d", target_device_info.GetNodeId());
  if (response.error_code() != SUCCESS) {
    GELOGE(FAILED,
           "[PreDeploy] failed, device_id = %d, error code = %u error message = %s",
           target_device_info.GetNodeId(),
           response.error_code(),
           response.error_message().c_str());
    return FAILED;
  }
  GELOGD("[PreDeploy] succeeded, device_id = %d, root_model_id = %u", target_device_info.GetNodeId(), root_model_id);
  return SUCCESS;
}

Status MasterModelDeployer::AddExchangePlanBindings(const DeployPlan &deploy_plan,
                                                    const DeployPlan::DeviceInfo &target_device_info,
                                                    deployer::ExchangePlan &exchange_plan,
                                                    std::set<size_t> &relative_queue_indices) {
  for (auto &binding : deploy_plan.GetQueueBindings()) {
    auto src_q_idx = binding.first;
    auto dst_q_idx = binding.second;
    const DeployPlan::QueueInfo *src_queue_info = nullptr;
    const DeployPlan::QueueInfo *dst_queue_info = nullptr;
    GE_CHK_STATUS_RET_NOLOG(deploy_plan.GetQueueInfo(src_q_idx, src_queue_info));
    GE_CHK_STATUS_RET_NOLOG(deploy_plan.GetQueueInfo(dst_q_idx, dst_queue_info));
    // collect bindings that related to target device
    if (src_queue_info->device_info.GetKey() == target_device_info.GetKey() ||
        dst_queue_info->device_info.GetKey() == target_device_info.GetKey()) {
      relative_queue_indices.emplace(src_q_idx);
      relative_queue_indices.emplace(dst_q_idx);
      if (deploy_plan.IsGroupEndpoint(src_q_idx) || deploy_plan.IsGroupEndpoint(dst_q_idx)) {
        auto new_binding = exchange_plan.add_bindings();
        GE_CHECK_NOTNULL(new_binding);
        new_binding->set_src_queue_index(src_q_idx);
        new_binding->set_dst_queue_index(dst_q_idx);
      }
    }
  }
  return SUCCESS;
}

Status MasterModelDeployer::MasterDeployRankSize() {
  const std::string file_name = "libhcom_graph_adaptor.so";
  std::string path = GetModelPath();
  (void) path.append(file_name);
  const std::string real_path = RealPath(path.c_str());
  if (real_path.empty()) {
    GELOGE(FAILED, "[RankSize][RealPath] Failed to get real path of %s", path.c_str());
    return FAILED;
  }
  GELOGI("[RankSize] Get path [%s] sucess.", path.c_str());
  const auto handle = mmDlopen(real_path.c_str(), static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
      static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
  if (handle == nullptr) {
    const char_t *error = mmDlerror();
    error = (error == nullptr) ? "" : error;
        REPORT_CALL_ERROR("E19999", "mmDlopen failed. path = %s, error = %s", real_path.c_str(), error);
    GELOGE(INTERNAL_ERROR, "[RankSize][DlOpen] failed. path = %s, error = %s", real_path.c_str(), error);
    return INTERNAL_ERROR;
  }
  GE_MAKE_GUARD(not_used_var, [&handle] {
    if (mmDlclose(handle) != 0) {
      GELOGW("[RankSize] Failed to close handle %s", mmDlerror());
    }
  });

  const auto hcom_set_rank_table =
      reinterpret_cast<HcclResult(*)(const char *)>(mmDlsym(handle, "HcomSetRankTable"));
  if (hcom_set_rank_table == nullptr) {
    // wait for hccl submit this api and modify this check
    GELOGI("[RankSize] Symbol HcomSetRankTable is null.");
    return SUCCESS;
  }

  const auto ret = hcom_set_rank_table(CommDomainManager::GetInstance().GetRankTable().c_str());
  if (ret != HCCL_SUCCESS) {
    GELOGE(FAILED, "[RankSize] Hcom set rank table failed, ret:%u", ret);
    return FAILED;
  }
  GELOGD("[RankSize] Set rank table for rank size success.");
  return SUCCESS;
}

Status MasterModelDeployer::DeployRankTableInfo(const DeviceInfo &device_info) {
  const auto &rank_table = CommDomainManager::GetInstance().GetRankTable();
  deployer::DeployerRequest request;
  request.set_type(deployer::kDeployRankTable);
  auto deploy_rank_table = request.mutable_deploy_rank_table_request();
  GE_CHECK_NOTNULL(deploy_rank_table);
  deploy_rank_table->set_device_id(device_info.GetDeviceId());
  deploy_rank_table->set_rank_table(rank_table);
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(device_info.GetNodeId(), request, response),
                    "[RankTable] failed to send request, device id=%d", device_info.GetDeviceId());
  if (response.error_code() != SUCCESS) {
    GELOGE(FAILED, "[RankTable] failed, device id=%d, error code=%u, error message=%s",
           device_info.GetDeviceId(), response.error_code(), response.error_message().c_str());
    return FAILED;
  }
  GELOGD("[RankTable] success, device id=%d", device_info.GetDeviceId());
  return SUCCESS;
}

Status MasterModelDeployer::MasterDeployRankTable() {
  char_t help_cluster[kMaxClusterStrLen];
  auto rank_table = CommDomainManager::GetInstance().GetRankTable();
  if ((!rank_table.empty()) || (mmGetEnv("HELP_CLUSTER", help_cluster, kMaxClusterStrLen) != EN_OK)) {
    GELOGD("[Deploy][RankTable] rank table had been deployed.");
    return SUCCESS;
  }

  std::string master_ip = NetworkManager::GetInstance().GetCtrlPanelIp();
  std::string master_ports = NetworkManager::GetInstance().GetCtrlPanelPorts();
  GELOGD("[Deploy][RankTable] master ip=%s, master port range=%s", master_ip.c_str(), master_ports.c_str());

  int32_t first_npu_node_id = 1;
  auto node_info = DeployerProxy::GetInstance().GetNodeInfo(first_npu_node_id);
  GE_CHECK_NOTNULL(node_info);
  GELOGD("[Deploy][RankTable] device count=%d", node_info->GetDeviceCount());

  auto &resource_manager = ResourceManager::GetInstance();
  const auto &devices = resource_manager.GetNpuDeviceInfoList();
  auto ret = CommDomainManager::GetInstance().Init(master_ip, master_ports, devices);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "Communication domain init failed, ret=%d", ret);
    return FAILED;
  }

  GELOGD("[Deploy][RankTable] rank table=%s", CommDomainManager::GetInstance().GetRankTable().c_str());

  GE_CHK_STATUS_RET(MasterDeployRankSize(), "Failed to deploy rank table for get rank size");

  // multi device send request
  for (const auto &device_info : devices) {
    GE_CHK_STATUS_RET(DeployRankTableInfo(device_info), "Failed to deploy rank table");
  }

  return SUCCESS;
}

Status MasterModelDeployer::ResolveExchangePlan(const DeployPlan &deploy_plan,
                                                const DeployPlan::DeviceInfo &target_device_info,
                                                deployer::ExchangePlan &exchange_plan) {
  std::string master_ip;
  GE_CHK_STATUS_RET_NOLOG(NetworkManager::GetInstance().GetDataPanelIp(master_ip));
  auto master_port = NetworkManager::GetInstance().GetDataPanelPort();
  exchange_plan.set_master_ip(master_ip);
  exchange_plan.set_master_port(master_port);
  GELOGD("master_ip = %s, master_port = %d", master_ip.c_str(), master_port);
  auto &deployer_proxy = DeployerProxy::GetInstance();
  std::set<size_t> relative_queue_indices;
  GE_CHK_STATUS_RET_NOLOG(AddExchangePlanBindings(deploy_plan,
                                                  target_device_info,
                                                  exchange_plan,
                                                  relative_queue_indices));
  // add queue defs that related to target device
  const auto &group_list = deploy_plan.GetGroups();
  const auto &queue_info_list = deploy_plan.GetQueueInfoList();
  for (size_t i = 0; i < queue_info_list.size(); ++i) {
    auto endpoint_index = static_cast<int32_t>(i);
    auto &queue_info = queue_info_list[i];
    const auto &device_info = queue_info.device_info;
    auto queue_desc = exchange_plan.add_queues();
    GE_CHECK_NOTNULL(queue_desc);
    if (device_info.GetKey() == target_device_info.GetKey()) {
      if (deploy_plan.IsGroupEndpoint(endpoint_index)) {
        queue_desc->set_type(ExchangeRoute::kEndpointTypeGroup);
        auto queue_list_iter = group_list.find(endpoint_index);
        if (queue_list_iter == group_list.end()) {
          GELOGE(FAILED, "Get group[%d] info failed.", endpoint_index);
          return FAILED;
        }
        queue_desc->mutable_queue_indices()->Add(queue_list_iter->second.begin(), queue_list_iter->second.end());
        continue;
      }
      if (queue_info.owned) {
        queue_desc->set_type(ExchangeRoute::kEndpointTypeQueue);
      } else {
        queue_desc->set_type(ExchangeRoute::kEndpointTypeExternalQueue);
      }
    } else if (relative_queue_indices.count(i) > 0) {
      queue_desc->set_type(ExchangeRoute::kEndpointTypeTag);
    } else {
      queue_desc->set_type(kEndpointTypeNone);
    }

    queue_desc->set_name(queue_info.name);
    queue_desc->set_depth(queue_info.depth);
    queue_desc->set_key(queue_info.device_info.GetKey());
    if (device_info.GetNodeId() == kNodeIdLocal) {
      queue_desc->set_host_ip(master_ip);  // host is master
      queue_desc->set_port(master_port);
    } else {
      auto dev_info = ResourceManager::GetInstance().GetDeviceInfo(device_info.GetNodeId(), device_info.GetDeviceId());
      GE_CHECK_NOTNULL(dev_info);
      queue_desc->set_host_ip(dev_info->GetHostIp());
      queue_desc->set_port(dev_info->GetDgwPort());
    }
    GELOGD("device = %s, ip = %s, port = %d",
           device_info.GetDesc().c_str(),
           queue_desc->host_ip().c_str(),
           queue_desc->port());
  }

  return SUCCESS;
}

Status MasterModelDeployer::SerializeModel(const PneModelPtr &model, ModelBufferData &model_buff) {
  return model->SerializeModel(model_buff);
}

Status MasterModelDeployer::DeployDevCfg(const int32_t dev_id, DeviceDebugConfig::ConfigType conf_type) {
  std::map<DeviceDebugConfig::ConfigType, deployer::DeviceConfigType> conf_type_map = {
      {DeviceDebugConfig::ConfigType::kLogConfigType, deployer::kLogConfig},
      {DeviceDebugConfig::ConfigType::kDumpConfigType, deployer::kDumpConfig},
      {DeviceDebugConfig::ConfigType::kProfilingConfigType, deployer::kProfilingConfig},
  };
  if (conf_type_map.find(conf_type) == conf_type_map.end()) {
    GELOGW("Init device config failed, can not find config type=%d.", conf_type);
    return SUCCESS;
  }
  std::string conf_data;
  const auto &conf = DeviceMaintenanceCfgManager::GetInstance().GetDevMaintenanceConfig(dev_id);
  if (conf->GetJsonDataByType(conf_type, conf_data) != SUCCESS) {
    GELOGI("Do not have device cfg, cfg type[%d].", conf_type);
    return SUCCESS;
  }
  deployer::DeployerRequest request;
  request.set_type(deployer::kDownloadConf);
  deployer::DeployerResponse response;
  auto download_config_request = request.mutable_download_config_request();
  download_config_request->set_sub_type(conf_type_map[conf_type]);
  download_config_request->set_device_id(dev_id);
  download_config_request->set_config_data(&conf_data[0], conf_data.size());
  if (DeployerProxy::GetInstance().SendRequest(dev_id, request, response) != SUCCESS) {
        REPORT_CALL_ERROR("E19999",
                          "Send disconnect request failed, response info:%s",
                          response.error_message().c_str());
    GELOGE(FAILED, "[Send][Request]Send disconnect request failed, response info:%s", response.error_message().c_str());
    return FAILED;
  }
  auto error_code = response.error_code();
  if (error_code != SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Check response failed. error code =%u, error message=%s", error_code,
                          response.error_message().c_str());
    GELOGE(FAILED, "[Check][Response]Check response failed. error code =%u, error message=%s", error_code,
           response.error_message().c_str());
    return FAILED;
  }
  GELOGI("DeployDevCfg successfully, conf_type=%d, dev_id=%d.", conf_type, dev_id);
  return SUCCESS;
}

Status MasterModelDeployer::DownloadDevMaintenanceCfg(int32_t dev_id) {
  GELOGD("[Download][Device debug Config] start, device id[%d]", dev_id);
  GE_CHK_STATUS_RET(DeviceMaintenanceCfgManager::GetInstance().CreateDevMaintenanceConfig(dev_id),
                    "[Create][MaintenanceCfg] failed, device id[%d].", dev_id);
  GE_CHK_STATUS_RET(DeployDevCfg(dev_id, DeviceDebugConfig::kLogConfigType),
                    "[Download][LogConfig] failed, device id[%d].", dev_id);
  GE_CHK_STATUS_RET(DeployDevCfg(dev_id, DeviceDebugConfig::kDumpConfigType),
                    "[Download][DumpConfig] failed, device id[%d].", dev_id);
  GE_CHK_STATUS_RET(DeployDevCfg(dev_id, DeviceDebugConfig::kProfilingConfigType),
                    "[Download][ProfilingConfig] failed, device id[%d].", dev_id);
  DeviceMaintenanceCfgManager::GetInstance().CloseDevMaintenanceConfig(dev_id);
  return SUCCESS;
}

Status MasterModelDeployer::TransferModel(const DeployPlan::DeviceInfo &device_info,
                                          uint32_t root_model_id,
                                          uint32_t submodel_id,
                                          const PneModelPtr &model) {
  GELOGD("[Transfer][Submodel] start, model_name = [%s]", model->GetModelName().c_str());
  ModelBufferData model_buff;
  GE_CHK_STATUS_RET_NOLOG(SerializeModel(model, model_buff));
  GE_CHK_STATUS_RET_NOLOG(PreDownloadModel(device_info, root_model_id, submodel_id, model_buff.length));
  GE_CHK_STATUS_RET_NOLOG(DownloadModel(device_info, root_model_id, submodel_id, model_buff));
  GELOGD("[Transfer][Submodel] succeeded, model_name = [%s]", model->GetModelName().c_str());
  return SUCCESS;
}

Status MasterModelDeployer::PreDownloadModel(const DeployPlan::DeviceInfo &device_info,
                                             uint32_t root_model_id,
                                             uint32_t submodel_id,
                                             size_t model_size) {
  GELOGD("[PreDownload][Submodel] start, device = %s, root_model_id = %u, submodel_id = %u",
         device_info.GetDesc().c_str(),
         root_model_id,
         submodel_id);
  deployer::DeployerRequest request;
  request.set_type(deployer::kPreDownloadModel);
  auto pre_download_request = request.mutable_pre_download_request();
  GE_CHECK_NOTNULL(pre_download_request);
  pre_download_request->set_model_id(submodel_id);
  pre_download_request->set_root_model_id(root_model_id);
  pre_download_request->set_total_size(model_size);
  pre_download_request->set_device_id(device_info.GetDeviceId());
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(device_info.GetNodeId(), request, response),
                    "[PreDownload] failed to send request, device_id = %d", device_info.GetNodeId());
  if (response.error_code() != SUCCESS) {
    GELOGE(FAILED,
           "[PreDownload] failed, device_id = %d, root_model_id = %u, submodel_id = %u, "
           "error code = %u, error message = %s",
           device_info.GetNodeId(),
           root_model_id,
           submodel_id,
           response.error_code(),
           response.error_message().c_str());
    return FAILED;
  }
  GELOGD("[PreDownload] succeeded, device_id = %d, root_model_id = %u, submodel_id = %u",
         device_info.GetNodeId(),
         root_model_id,
         submodel_id);
  return SUCCESS;
}

Status MasterModelDeployer::DownloadModel(const DeployPlan::DeviceInfo &device_info,
                                          uint32_t root_model_id,
                                          uint32_t submodel_id,
                                          const ModelBufferData &model_buff) {
  GELOGD("[Download] start, device = %s, root_model_id = %u, submodel_id = %u, size = %lu",
         device_info.GetDesc().c_str(), root_model_id, submodel_id, model_buff.length);
  uint64_t offset = 0;
  const uint8_t *model_data = model_buff.data.get();
  uint64_t remaining_size = model_buff.length;
  const uint64_t block_size = 2 * 1024 * 1024;  // 2M
  deployer::DeployerRequest request;
  request.set_type(deployer::kDownloadModel);
  auto download_request = request.mutable_download_model_request();
  GE_CHECK_NOTNULL(download_request);
  download_request->set_root_model_id(root_model_id);
  download_request->set_model_id(submodel_id);
  download_request->set_device_id(device_info.GetDeviceId());
  while (remaining_size > 0) {
    uint64_t size_to_send = std::min(block_size, remaining_size);
    remaining_size -= size_to_send;
    download_request->set_offset(offset);
    download_request->set_om_content(model_data + offset, size_to_send);
    deployer::DeployerResponse response;
    GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(device_info.GetNodeId(), request, response),
                      "[Download] failed to send request, device_id = %d, offset = %lu",
                      device_info.GetNodeId(),
                      offset);
    if (response.error_code() != SUCCESS) {
      GELOGE(FAILED,
             "[Download] failed, device_id = %d, root_model_id = %u, submodel_id = %u, "
             "error code = %u, error message = %s",
             device_info.GetNodeId(),
             root_model_id,
             submodel_id,
             response.error_code(),
             response.error_message().c_str());
      return FAILED;
    }
    offset += size_to_send;
    GELOGD("[Download] succeeded, device_id = %d, root_model_id = %u, submodel_id = %u, progress: %lu/%lu",
           device_info.GetNodeId(), root_model_id, submodel_id, offset, model_buff.length);
  }
  GELOGD("[Download] succeeded, device_id = %d, root_model_id = %u, submodel_id = %u, total size = %lu",
         device_info.GetNodeId(), root_model_id, submodel_id, model_buff.length);
  return SUCCESS;
}

Status MasterModelDeployer::LoadModel(const DeployPlan::DeviceInfo &device_info,
                                      uint32_t root_model_id,
                                      uint32_t submodel_id,
                                      bool is_unknown_shape) {
  deployer::DeployerRequest request;
  request.set_type(deployer::kLoadModel);
  auto load_model_request = request.mutable_load_model_request();
  GE_CHECK_NOTNULL(load_model_request);
  load_model_request->set_root_model_id(root_model_id);
  load_model_request->set_model_id(submodel_id);
  load_model_request->set_device_id(device_info.GetDeviceId());
  load_model_request->set_is_unknown_shape(is_unknown_shape);
  deployer::DeployerResponse response;
  GE_CHK_STATUS_RET(DeployerProxy::GetInstance().SendRequest(device_info.GetNodeId(), request, response),
                    "[Load] failed to send request, device_id = %d", device_info.GetNodeId());
  if (response.error_code() != SUCCESS) {
    GELOGE(FAILED,
           "[Load] failed, device_id = %d, root_model_id = %u, submodel_id = %u, "
           "error code = %u, error message = %s",
           device_info.GetNodeId(),
           root_model_id,
           submodel_id,
           response.error_code(),
           response.error_message().c_str());
    return FAILED;
  }
  GELOGD("[Load] succeeded, device_id = %d, root_model_id = %u, submodel_id = %u",
         device_info.GetNodeId(),
         root_model_id,
         submodel_id);
  return SUCCESS;
}

Status MasterModelDeployer::CreateInputStream(const string &constant_file_path, unique_ptr<std::istream> &in_stream) {
  std::string real_path = RealPath(constant_file_path.c_str());
  auto file_stream = MakeUnique<std::ifstream>(real_path, std::ifstream::binary);
  GE_CHECK_NOTNULL(file_stream);
  if (!file_stream->is_open()) {
    GELOGE(GRAPH_FAILED, "[Open][File] %s failed.", real_path.c_str());
    REPORT_INNER_ERROR("E19999", "open file:%s failed.", real_path.c_str());
    return GRAPH_FAILED;
  }
  in_stream = std::move(file_stream);
  return SUCCESS;
}
}  // namespace ge
