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
#include "runtime/deploy/local_model_deployer.h"
#include "exec_runtime/execution_runtime.h"
#include "graph/load/model_manager/model_manager.h"
#include "runtime/deploy/local_exchange_deployer.h"
#include "runtime/dev.h"

namespace ge {
Status LocalModelDeployer::Finalize() {
  GELOGI("Finalizing LocalModelDeployer.");
  for (auto &it : deployed_models_) {
    GELOGI("Unload model: %u", it.first);
    (void)UndeployModel(it.second);
  }
  deployed_models_.clear();
  GELOGI("LocalModelDeployer finalized successfully.");
  return SUCCESS;
}

Status LocalModelDeployer::DeployModel(const FlowModelPtr &flow_model,
                                       const std::vector<uint32_t> &input_queue_ids,
                                       const std::vector<uint32_t> &output_queue_ids,
                                       DeployResult &deploy_result) {
  (void)input_queue_ids;
  (void)output_queue_ids;
  GE_CHECK_NOTNULL(flow_model);
  if (flow_model->GetSubmodels().size() != 1U) {
    GELOGE(PARAM_INVALID, "Only support one model, but got %zu", flow_model->GetSubmodels().size());
    return PARAM_INVALID;
  }

  const auto execution_runtime = ExecutionRuntime::GetInstance();
  GE_CHECK_NOTNULL(execution_runtime);
  auto &exchange_service = execution_runtime->GetExchangeService();

  // it'll take 136 years to exhaust model id if load one model per second
  deploy_result.model_id = model_id_gen_++;
  GE_CHECK_LE(deploy_result.model_id, UINT32_MAX - 1U);

  // 1. build deploy plan
  DeployPlanner planner(flow_model);
  DeployPlan deploy_plan;
  GE_CHK_STATUS_RET(planner.BuildPlan(deploy_plan), "Failed to build DeployPlan.");

  // 2. build exchange(create queues, bind queues)
  int32_t device_id = 0;
  GE_CHK_RT_RET(rtGetDevice(&device_id));
  LocalExchangeDeployer ex_deployer(&exchange_service, &deploy_plan, device_id);
  DeployedModel deployed_model;
  GE_CHK_STATUS_RET(ex_deployer.DeployModelExchange(deployed_model.deployed_exchange),
                    "Failed to deploy model exchange");
  deploy_result.input_queue_ids = deployed_model.deployed_exchange.GetInputQueueIds();
  deploy_result.output_queue_ids = deployed_model.deployed_exchange.GetOutputQueueIds();
  GE_DISMISSABLE_GUARD(deployed_model, [&]() {
    (void)UndeployModel(deployed_model);
  });

  // 3. deploy submodels
  GE_CHK_STATUS_RET(LoadSubmodels(deploy_plan, deployed_model), "Failed to deploy model.");

  // 4. record DeployInfo
  const std::lock_guard<std::mutex> lk(mu_);
  (void)deployed_models_.emplace(deploy_result.model_id, std::move(deployed_model));
  GE_DISMISS_GUARD(deployed_model);
  return SUCCESS;
}

Status LocalModelDeployer::Undeploy(const uint32_t model_id) {
  const std::lock_guard<std::mutex> lk(mu_);
  const std::map<uint32_t, DeployedModel>::const_iterator it = deployed_models_.find(model_id);
  if (it == deployed_models_.cend()) {
    GELOGE(GE_EXEC_MODEL_ID_INVALID, "Model not found, id = %u", model_id);
    return GE_EXEC_MODEL_ID_INVALID;
  }

  (void)UndeployModel(it->second);
  (void)deployed_models_.erase(it);
  GELOGI("Model undeployed successfully, model id = %u", model_id);
  return SUCCESS;
}

Status LocalModelDeployer::UndeployModel(const DeployedModel &deployed_model) {
  for (const auto submodel_id : deployed_model.submodel_ids) {
    GE_CHK_STATUS(UnloadSubmodel(submodel_id));
  }
  const auto execution_runtime = ExecutionRuntime::GetInstance();
  GE_CHECK_NOTNULL(execution_runtime);
  auto &exchange_service = execution_runtime->GetExchangeService();
  GE_CHK_STATUS(LocalExchangeDeployer::UndeployModelExchange(&exchange_service, deployed_model.deployed_exchange));
  return SUCCESS;
}

Status LocalModelDeployer::LoadSubmodels(const DeployPlan &deploy_plan,
                                         LocalModelDeployer::DeployedModel &deployed_model) {
  for (const auto &it : deploy_plan.GetSubmodels()) {
    const auto &model_name = it.first;
    const auto &submodel = it.second;
    const auto submodel_queues = deployed_model.deployed_exchange.GetSubmodelQueues(model_name);
    GE_CHECK_NOTNULL(submodel_queues);
    uint32_t submodel_id = 0U;
    GE_CHK_STATUS_RET(LoadSubmodelWithQueue(std::dynamic_pointer_cast<GeRootModel>(submodel.model),
                                            submodel_queues->input_queue_ids,
                                            submodel_queues->output_queue_ids,
                                            submodel_id),
                      "Failed to execute LoadModelWithQ, submodel name = %s", model_name.c_str());
    deployed_model.submodel_ids.emplace_back(submodel_id);
  }
  return SUCCESS;
}

Status LocalModelDeployer::LoadSubmodelWithQueue(const GeRootModelPtr &root_model,
                                                 const std::vector<uint32_t> &input_queue_ids,
                                                 const std::vector<uint32_t> &output_queue_ids,
                                                 uint32_t &model_id) {
  return ModelManager::GetInstance().LoadModelWithQ(model_id, root_model, input_queue_ids, output_queue_ids);
}

Status LocalModelDeployer::UnloadSubmodel(const uint32_t model_id) {
  return ModelManager::GetInstance().Unload(model_id);
}
}  // namespace ge