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
#ifndef BASE_EXEC_RUNTIME_DEPLOY_LOCAL_EXCHANGE_DEPLOYER_H_
#define BASE_EXEC_RUNTIME_DEPLOY_LOCAL_EXCHANGE_DEPLOYER_H_

#include <map>
#include <memory>
#include "framework/common/ge_types.h"
#include "graph/compute_graph.h"
#include "graph/model.h"
#include "exec_runtime/deploy/deploy_planner.h"
#include "exec_runtime/deploy/exchange_service.h"

namespace ge {
struct QueueDeploymentInfo {
  int32_t device_id = -1;
  uint32_t queue_id = 0U;
  std::string queue_name;
};

struct ModelQueueInfo {
  std::vector<uint32_t> input_queue_ids;
  std::vector<uint32_t> output_queue_ids;
};

class ModelExchangeInfo {
 public:
  /// Get ModelQueueInfo by model name
  /// @param submodel_name        Name of the submodel
  /// @return                     A pointer to ModelQueueInfo, nullptr if submodel name not found
  const ModelQueueInfo *GetSubmodelQueues(const std::string &submodel_name) const;
  const std::vector<uint32_t> &GetInputQueueIds() const;
  const std::vector<uint32_t> &GetOutputQueueIds() const;

  const QueueDeploymentInfo *GetQueueInfo(const int32_t index) const;

 private:
  friend class LocalExchangeDeployer;
  std::vector<QueueDeploymentInfo> deployed_queues_;
  std::vector<std::pair<int32_t, int32_t>> queue_bindings_;
  std::map<std::string, ModelQueueInfo> submodel_queues_;
  std::vector<uint32_t> input_queue_ids_;
  std::vector<uint32_t> output_queue_ids_;
};

/// This class is responsible for handling data exchange
/// 1. Deploying: Creating queues and binding queues by DeployPlan, returning a deployed ModelExchangeInfo
/// 2. Undeploying: Unbinding queues and destroying queues by deployed ModelExchangeInfo
class LocalExchangeDeployer {
 public:
  LocalExchangeDeployer(ExchangeService *const exchange_service, const DeployPlan *const deploy_plan,
                        const int32_t device_id) noexcept;
  virtual ~LocalExchangeDeployer() = default;

  /// Deploy exchange by deploy plan: creating queues, binding queues
  /// Only the queues that belonging to the current device are handled
  /// @param model_exchange_info      deployed exchange info
  /// @return                         SUCCESS if deployed successfully, otherwise returns appropriate error code
  Status DeployModelExchange(ModelExchangeInfo &model_exchange_info);

  /// Undeploy exchange by deployed ModelExchangeInfo. Unbinding queues, destroy queues
  /// @param exchange_service         exchange_service
  /// @param model_exchange_info      deployed ModelExchangeInfo that to be undeployed
  /// @return                         SUCCESS if undeployed successfully, otherwise returns appropriate error code
  static Status UndeployModelExchange(ExchangeService * const exchange_service,
                                      const ModelExchangeInfo &model_exchange_info);

 protected:
  /// Bind queues
  /// @param src_queue_info           source queue info
  /// @param dst_queue_info           destination queue info
  /// @return                         SUCCESS if binding is successful, otherwise returns appropriate error code
  virtual Status DoBindQueue(const QueueDeploymentInfo &src_queue_info, const QueueDeploymentInfo &dst_queue_info);

 private:
  Status Initialize() const;
  static Status ValidateQueueIndices(const std::vector<int32_t> &queue_indices, const int32_t num_queues);
  Status DeployInternal();
  Status BindQueues();
  Status CreateQueues();
  Status SetQueueIdForModels();
  Status ConvertQueueIndexToId(const std::vector<int32_t> &queue_indices,
                               std::vector<uint32_t> &queue_ids) const;

  ExchangeService *exchange_service_;
  const DeployPlan *deploy_plan_;
  ModelExchangeInfo deploying_;
  int32_t device_id_ = -1;
};
}
#endif  // BASE_EXEC_RUNTIME_DEPLOY_LOCAL_EXCHANGE_DEPLOYER_H_
