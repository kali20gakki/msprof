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
#ifndef RUNTIME_DEPLOY_HELPER_DEPLOY_PLANNER_H_
#define RUNTIME_DEPLOY_HELPER_DEPLOY_PLANNER_H_

#include "exec_runtime/deploy/deploy_planner.h"
#include "pne/model/flow_model.h"

namespace ge {
class HelperDeployPlanner : public DeployPlannerBase {
 public:
  HelperDeployPlanner(const FlowModelPtr &flow_model,
                      const std::vector<DeployPlan::DeviceInfo> &device_list) noexcept;

 protected:
  Status PrepareModelsAndRelation(const ModelRelation *&model_relation) override;

 private:
  Status MergeModels(std::map<std::string, PneModelPtr> &name_to_models,
                     const ModelRelation *&model_relation);
  Status PrepareForSingleModel(map<std::string, PneModelPtr> &name_to_models,
                               const ModelRelation *&model_relation);
  Status UnfoldSubModel(const ModelRelation::ModelQueueInfo &model_queue_info,
                        const PneModel &model,
                        std::map<std::string, PneModelPtr> &name_to_models);

  const FlowModelPtr flow_model_;
  std::vector<DeployPlan::DeviceInfo> device_list_;
  ModelRelation merged_model_relation_;
  int32_t dev_count_;
};
}  // namespace ge
#endif  // RUNTIME_DEPLOY_HELPER_DEPLOY_PLANNER_H_
