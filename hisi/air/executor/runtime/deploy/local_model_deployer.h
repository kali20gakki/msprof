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
#ifndef EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_DEPLOY_LOCAL_MODEL_DEPLOYER_H_
#define EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_DEPLOY_LOCAL_MODEL_DEPLOYER_H_

#include <atomic>
#include "exec_runtime/deploy/deploy_planner.h"
#include "exec_runtime/deploy/model_deployer.h"
#include "local_exchange_deployer.h"

namespace ge {
class LocalModelDeployer : public ModelDeployer {
 public:
  Status Finalize();

  Status DeployModel(const FlowModelPtr &flow_model,
                     const std::vector<uint32_t> &input_queue_ids,
                     const std::vector<uint32_t> &output_queue_ids,
                     DeployResult &deploy_result) override;

  Status Undeploy(const uint32_t model_id) override;

 protected:
  virtual Status LoadSubmodelWithQueue(const GeRootModelPtr &root_model,
                                       const std::vector<uint32_t> &input_queue_ids,
                                       const std::vector<uint32_t> &output_queue_ids,
                                       uint32_t &model_id);

  virtual Status UnloadSubmodel(const uint32_t model_id);

 private:
  struct DeployedModel {
    ModelExchangeInfo deployed_exchange;
    std::vector<uint32_t> submodel_ids;
  };

  Status UndeployModel(const DeployedModel &deployed_model);

  Status LoadSubmodels(const DeployPlan &deploy_plan, DeployedModel &deployed_model);

  std::mutex mu_;
  std::map<uint32_t, DeployedModel> deployed_models_;
  std::atomic<uint32_t> model_id_gen_{0U};
};
}  // namespace ge
#endif  // EXECUTOR_GRAPH_LOAD_MODEL_MANAGER_DEPLOY_LOCAL_MODEL_DEPLOYER_H_
