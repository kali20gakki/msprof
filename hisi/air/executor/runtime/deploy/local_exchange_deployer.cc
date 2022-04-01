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
#include "local_exchange_deployer.h"
#include "runtime/rt_mem_queue.h"

namespace ge {
const ModelQueueInfo *ModelExchangeInfo::GetSubmodelQueues(const std::string &submodel_name) const {
  const auto it = submodel_queues_.find(submodel_name);
  if (it == submodel_queues_.end()) {
    GELOGE(PARAM_INVALID, "Failed to find submodel queue by name: %s", submodel_name.c_str());
    return nullptr;
  }
  return &it->second;
}

const std::vector<uint32_t> &ModelExchangeInfo::GetInputQueueIds() const {
  return input_queue_ids_;
}

const std::vector<uint32_t> &ModelExchangeInfo::GetOutputQueueIds() const {
  return output_queue_ids_;
}

const QueueDeploymentInfo *ModelExchangeInfo::GetQueueInfo(const int32_t index) const {
  if ((index < 0) || (static_cast<size_t>(index) >= deployed_queues_.size())) {
    GELOGE(PARAM_INVALID, "Queue index (%d) out of range [0, %zu)", index, deployed_queues_.size());
    return nullptr;
  }
  return &deployed_queues_[static_cast<size_t>(index)];
}

LocalExchangeDeployer::LocalExchangeDeployer(ExchangeService *const exchange_service,
                                             const DeployPlan *const deploy_plan,
                                             const int32_t device_id) noexcept
    : exchange_service_(exchange_service), deploy_plan_(deploy_plan), device_id_(device_id) {
}

Status LocalExchangeDeployer::DeployModelExchange(ModelExchangeInfo &model_exchange_info) {
  GE_CHK_STATUS_RET(Initialize(), "Failed to validate params.");
  const auto ret = DeployInternal();
  if (ret != SUCCESS) {
    GELOGI("Failed to deploy model exchange, start rollback");
    (void)LocalExchangeDeployer::UndeployModelExchange(exchange_service_, deploying_);
    return ret;
  }
  GELOGI("Deploy model exchange successfully.");
  model_exchange_info = std::move(deploying_);
  return SUCCESS;
}

Status LocalExchangeDeployer::DeployInternal() {
  GE_CHK_STATUS_RET(CreateQueues(), "Failed to create queues.");
  GE_CHK_STATUS_RET(BindQueues(), "Failed to bind queues.");
  GE_CHK_STATUS_RET(SetQueueIdForModels(), "Failed to set queue id for submodels.");
  return SUCCESS;
}

Status LocalExchangeDeployer::Initialize() const {
  GE_CHECK_NOTNULL(deploy_plan_);
  GE_CHECK_NOTNULL(exchange_service_);
  GE_CHECK_LE(deploy_plan_->GetQueueInfoList().size(), UINT32_MAX);
  const auto num_queues = static_cast<int32_t>(deploy_plan_->GetQueueInfoList().size());
  for (const auto &it : deploy_plan_->GetSubmodels()) {
    GE_CHK_STATUS_RET_NOLOG(ValidateQueueIndices(it.second.input_queue_indices, num_queues));
    GE_CHK_STATUS_RET_NOLOG(ValidateQueueIndices(it.second.output_queue_indices, num_queues));
  }
  GE_CHK_STATUS_RET_NOLOG(ValidateQueueIndices(deploy_plan_->GetAllInputQueueIndices(), num_queues));
  GE_CHK_STATUS_RET_NOLOG(ValidateQueueIndices(deploy_plan_->GetOutputQueueIndices(), num_queues));
  return SUCCESS;
}

Status LocalExchangeDeployer::ValidateQueueIndices(const std::vector<int32_t> &queue_indices,
                                                   const int32_t num_queues) {
  for (const auto queue_index : queue_indices) {
    if ((queue_index < 0) || (queue_index >= num_queues)) {
      GELOGE(PARAM_INVALID, "queue index out of range, index = %d, num_queues = %d", queue_index, num_queues);
      return PARAM_INVALID;
    }
  }
  return SUCCESS;
}

Status LocalExchangeDeployer::CreateQueues() {
  GE_CHECK_LE(deploy_plan_->GetQueueInfoList().size(), UINT32_MAX);
  const auto num_queues = static_cast<int32_t>(deploy_plan_->GetQueueInfoList().size());
  for (int32_t queue_index = 0; queue_index < num_queues; ++queue_index) {
    const auto &queue_info = deploy_plan_->GetQueueInfoList()[static_cast<size_t>(queue_index)];
    QueueDeploymentInfo queue_deployment_info;
    GE_CHK_STATUS_RET(exchange_service_->CreateQueue(device_id_,
                                                     queue_info.name,
                                                     queue_info.depth,
                                                     static_cast<uint32_t>(RT_MQ_MODE_PULL),
                                                     queue_deployment_info.queue_id),
                      "Failed to create queue, device = %d, name = %s",
                      device_id_, queue_info.name.c_str());
    GELOGD("Queue created successfully, device id = %d, name = %s, index = %d, queue id = %u",
           device_id_, queue_info.name.c_str(), queue_index, queue_deployment_info.queue_id);
    queue_deployment_info.device_id = device_id_;
    queue_deployment_info.queue_name = queue_info.name;
    deploying_.deployed_queues_.emplace_back(queue_deployment_info);
  }
  return SUCCESS;
}

Status LocalExchangeDeployer::BindQueues() {
  if (deploy_plan_->GetQueueBindings().empty()) {
    GELOGD("No queues for binding.");
    return SUCCESS;
  }

  for (const auto &queue_binding : deploy_plan_->GetQueueBindings()) {
    auto src_queue_index = queue_binding.first;
    const QueueDeploymentInfo *const src_queue_info = deploying_.GetQueueInfo(src_queue_index);
    GE_CHECK_NOTNULL(src_queue_info);
    auto dst_queue_index = queue_binding.second;
    const QueueDeploymentInfo *const dst_queue_info = deploying_.GetQueueInfo(dst_queue_index);
    GE_CHECK_NOTNULL(dst_queue_info);

    GE_CHK_STATUS_RET_NOLOG(DoBindQueue(*src_queue_info, *dst_queue_info));
    GELOGD("Bind queues successfully, queue name = %s, src queue_id = %u, dst queue id = %u",
           src_queue_info->queue_name.c_str(),
           src_queue_info->queue_id,
           dst_queue_info->queue_id);
    deploying_.queue_bindings_.emplace_back(src_queue_index, dst_queue_index); // record for undeploying
  }
  GELOGD("All queues bound successfully.");
  return SUCCESS;
}

Status LocalExchangeDeployer::UndeployModelExchange(ExchangeService * const exchange_service,
                                                    const ModelExchangeInfo &model_exchange_info) {
  GE_CHECK_NOTNULL(exchange_service);
  for (const auto &queue_info : model_exchange_info.deployed_queues_) {
    (void)exchange_service->DestroyQueue(queue_info.device_id, queue_info.queue_id);
  }
  return SUCCESS;
}

Status LocalExchangeDeployer::SetQueueIdForModels() {
  for (const auto &it : deploy_plan_->GetSubmodels()) {
    const auto &model_name = it.first;
    const auto &submodel = it.second;
    auto &submodel_queues = deploying_.submodel_queues_[model_name];
    GE_CHK_STATUS_RET_NOLOG(ConvertQueueIndexToId(submodel.input_queue_indices, submodel_queues.input_queue_ids));
    GE_CHK_STATUS_RET_NOLOG(ConvertQueueIndexToId(submodel.output_queue_indices, submodel_queues.output_queue_ids));
  }
  GE_CHK_STATUS_RET_NOLOG(ConvertQueueIndexToId(deploy_plan_->GetInputQueueIndices(), deploying_.input_queue_ids_));
  GE_CHK_STATUS_RET_NOLOG(ConvertQueueIndexToId(deploy_plan_->GetOutputQueueIndices(), deploying_.output_queue_ids_));
  return SUCCESS;
}

Status LocalExchangeDeployer::ConvertQueueIndexToId(const std::vector<int32_t> &queue_indices,
                                                    std::vector<uint32_t> &queue_ids) const {
  for (const auto queue_index : queue_indices) {
    const auto *const queue_info = deploying_.GetQueueInfo(queue_index);
    GE_CHECK_NOTNULL(queue_info);
    queue_ids.emplace_back(queue_info->queue_id);
  }
  return SUCCESS;
}

Status LocalExchangeDeployer::DoBindQueue(const QueueDeploymentInfo &src_queue_info,
                                          const QueueDeploymentInfo &dst_queue_info) {
  (void)src_queue_info;
  (void)dst_queue_info;
  GELOGE(UNSUPPORTED, "Binding queues are not yet supported.");
  return UNSUPPORTED;
}
}  // namespace ge
