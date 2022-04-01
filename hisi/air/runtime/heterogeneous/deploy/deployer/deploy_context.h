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

#ifndef AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOY_CONTEXT_H_
#define AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOY_CONTEXT_H_

#include <cstdint>
#include <map>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>
#include "deploy/flowrm/helper_exchange_deployer.h"
#include "common/config/device_debug_config.h"
#include "deploy/execfwk/executor_manager.h"
#include "ge/ge_api_error_codes.h"
#include "proto/deployer.pb.h"
#include "proto/var_manager.pb.h"

namespace ge {
class DeployContext {
 public:
  explicit DeployContext(bool is_local = false);
  ~DeployContext() = default;
  /*
   *  @ingroup ge
   *  @brief   record client
   *  @param   [in]  None
   *  @return  None
   */
  void Initialize();

  /*
   *  @ingroup ge
   *  @brief   finalize context
   *  @param   [in]  None
   *  @return  None
   */
  void Finalize() const;

  /*
   *  @ingroup ge
   *  @brief   download device debug config
   *  @param   [in]  const DeployerRequest *
   *  @param   [in]  DeployerResponse *
   *  @return  SUCCESS or FAILED
   */
  Status DownloadDevMaintenanceCfg(const deployer::DeployerRequest &request,
                                   deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   pre-download model
   *  @param   [in]  const DeployerRequest *
   *  @param   [in]  DeployerResponse *
   *  @return  SUCCESS or FAILED
   */
  Status PreDownloadModel(const deployer::DeployerRequest &request,
                          deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   download model
   *  @param   [in]  const DeployerRequest *
   *  @param   [in]  DeployerResponse *
   *  @return  SUCCESS or FAILED
   */
  Status DownloadModel(const deployer::DeployerRequest &request,
                       deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   load model
   *  @param   [in]  const DeployerRequest *
   *  @param   [in]  DeployerResponse *
   *  @return  SUCCESS or FAILED
   */
  Status LoadModel(const deployer::DeployerRequest &request,
                   deployer::DeployerResponse &response);

  Status UnloadModel(const deployer::DeployerRequest &request,
                     deployer::DeployerResponse &response);

  void PreDeployModel(const deployer::PreDeployModelRequest &request,
                      deployer::DeployerResponse &response);

  Status ProcessMultiVarManager(const deployer::MultiVarManagerRequest &request,
                                deployer::DeployerResponse &response);

  Status ProcessSharedContent(const deployer::SharedContentDescRequest &request,
                              deployer::DeployerResponse &response);

  Status DeployRankTable(const deployer::DeployRankTableRequest &request,
                         deployer::DeployerResponse &response);

  void SetName(const std::string &name);

 private:
  struct DeployTaskId {
    int32_t device_id;
    uint32_t root_model_id;
    uint32_t sub_model_id;
    bool operator < (const DeployTaskId &other) const {
      if (device_id != other.device_id) {
        return device_id < other.device_id;
      } else if (root_model_id != other.root_model_id) {
        return root_model_id < other.root_model_id;
      } else {
        return sub_model_id < other.sub_model_id;
      }
    }
  };

  static Status ParserRankIdFromRankTable(int32_t device_id, const std::string &ipaddr,
                                          const std::string &rank_table_list, std::string &rank_id);

  static Status SetResponse(const deployer::ExecutorResponse &executor_response,
                            deployer::DeployerResponse &deployer_response);

  Status GetQueues(const DeployTaskId &client_context, std::vector<uint32_t> &input_queues,
                   std::vector<uint32_t> &output_queues) const;

  Status GrantQueuesForCpuSchedule(int32_t device_id, bool is_unknown_shape,
                                   const std::vector<uint32_t> &input_queues,
                                   const std::vector<uint32_t> &output_queues);

  Status GetCpuSchedulePid(int32_t device_id, bool is_unknown_shape, int32_t &pid);

  std::mutex mu_;
  std::string name_ = "unnamed";
  bool is_executing_ = false;
  std::map<DeployTaskId, int64_t> exchange_routes_;
  std::map<DeployTaskId, deployer::SubmodelDesc> submodel_descs_;
  ExecutorManager executor_manager_;
  DeviceMaintenanceClientCfg dev_maintenance_cfg_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_HETEROGENEOUS_DEPLOY_DEPLOYER_DEPLOY_CONTEXT_H_
