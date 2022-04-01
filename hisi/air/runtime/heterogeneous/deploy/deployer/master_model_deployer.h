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
#ifndef RUNTIME_DEPLOY_HELPER_MODEL_DEPLOYER_H_
#define RUNTIME_DEPLOY_HELPER_MODEL_DEPLOYER_H_

#include "external/ge/ge_ir_build.h"
#include "common/config/device_debug_config.h"
#include "exec_runtime/deploy/model_deployer.h"
#include "exec_runtime/deploy/deploy_planner.h"
#include "common/plugin/plugin_manager.h"
#include "common/config/device_debug_config.h"
#include "external/ge/ge_ir_build.h"
#include "hccl/hccl_types.h"
#include "deploy/flowrm/helper_exchange_deployer.h"
#include "deploy/deployer/deployer_proxy.h"

namespace ge {
class MasterModelDeployer : public ModelDeployer {
 public:
  MasterModelDeployer() = default;
  ~MasterModelDeployer() override = default;

  Status Initialize(const map<std::string, std::string> &options);

  Status Finalize();

  Status DeployModel(const FlowModelPtr &flow_model,
                     const vector<uint32_t> &input_queue_ids,
                     const vector<uint32_t> &output_queue_ids,
                     DeployResult &deploy_result) override;
  Status Undeploy(const uint32_t model_id) override;

 private:
  using ConstSubmodelInfoPtr = const DeployPlan::SubmodelInfo *;
  struct DeployedModel {
    uint32_t model_id = UINT32_MAX;
    // key: device_id, value: model_name

    std::vector<DeployPlan::DeviceInfo> deployed_remote_devices;
    int64_t route_id = -1;
  };

  static void AddGroupQueueIndices(const DeployPlan &deploy_plan,
                                   const int32_t queue_indice,
                                   const DeployPlan::QueueInfo &queue_info,
                                   std::set<size_t> &relative_queue_indices);

  static Status AddExchangePlanBindings(const DeployPlan &deploy_plan,
                                        const DeployPlan::DeviceInfo &target_device_info,
                                        deployer::ExchangePlan &exchange_plan,
                                        std::set<size_t> &relative_queue_indices);

  static Status ResolveExchangePlan(const DeployPlan &deploy_plan,
                                    const DeployPlan::DeviceInfo &target_device_info,
                                    deployer::ExchangePlan &exchange_plan);

  static Status BuildPreDeployRequest(const DeployPlan &deploy_plan,
                                      const DeployPlan::DeviceInfo &target_device_info,
                                      const std::vector<const DeployPlan::SubmodelInfo *> &submodels,
                                      deployer::PreDeployModelRequest &request);
  static void SetSubModels(deployer::PreDeployModelRequest &request,
                           const std::vector<const DeployPlan::SubmodelInfo *> &submodels);
  static Status GetModelIoQueueIds(const DeployPlan &deploy_plan,
                                   const ExchangeRoute &route,
                                   DeployResult &deploy_result);
  static void GroupSubmodels(const DeployPlan &deploy_plan,
                             std::vector<const DeployPlan::SubmodelInfo *> &local_models,
                             std::map<std::string, std::vector<ConstSubmodelInfoPtr>> &remote_models);
  void UndeployModel(DeployedModel &deployed_model);
  Status DeployRemoteSubmodels(const DeployPlan &deploy_plan,
                               const std::map<std::string, std::vector<ConstSubmodelInfoPtr>> &remote_models,
                               MasterModelDeployer::DeployedModel &deployed_model);

  Status DoDeploySubmodels(const DeployPlan::DeviceInfo &device_info,
                           const std::vector<ConstSubmodelInfoPtr> &submodels,
                           MasterModelDeployer::DeployedModel &deployed_model);

  Status DoLoadSubmodels(const DeployPlan::DeviceInfo &device_info,
                         const std::vector<ConstSubmodelInfoPtr> &submodels,
                         MasterModelDeployer::DeployedModel &deployed_model);

  static Status GetAvailableDevices(std::vector<DeployPlan::DeviceInfo> &available_devices);


  Status DownloadDevMaintenanceCfg(int32_t dev_id);
  Status DeployLocalSubmodels(const std::vector<ConstSubmodelInfoPtr> &submodels,
                              DeployedModel &deployed_model);

  Status UndeployRemoteSubmodels(const DeployPlan::DeviceInfo &device_info, uint32_t root_model_id);
  static Status UndeployLocalSubmodels(MasterModelDeployer::DeployedModel &deployed_model);
  Status PreDeployLocalSubmodels(uint32_t root_model_id,
                                 const vector<const DeployPlan::SubmodelInfo *> &submodels,
                                 int64_t route_id);
  Status PreDeployRemoteSubmodels(const DeployPlan &deploy_plan,
                                  const DeployPlan::DeviceInfo &target_device_info,
                                  uint32_t root_model_id,
                                  const vector<const DeployPlan::SubmodelInfo *> &submodels);
  static Status SerializeModel(const PneModelPtr &model, ModelBufferData &model_buff);
  Status TransferModel(const DeployPlan::DeviceInfo &device_info,
                       uint32_t root_model_id,
                       uint32_t submodel_id,
                       const PneModelPtr &model);
  Status PreDownloadModel(const DeployPlan::DeviceInfo &device_info, uint32_t root_model_id, uint32_t submodel_id, size_t model_size);
  Status DownloadModel(const DeployPlan::DeviceInfo &device_info,
                       uint32_t root_model_id,
                       uint32_t submodel_id,
                       const ModelBufferData &model_buff);
  Status LoadModel(const DeployPlan::DeviceInfo &device_info, uint32_t root_model_id, uint32_t submodel_id,
                   bool is_unknown_shape);

  virtual Status DeploySubmodels(DeployPlan &deploy_plan, DeployedModel &deployed_model);

  virtual Status CreateInputStream(const std::string &constant_file_path, std::unique_ptr<std::istream> &in_stream);

  struct SendInfo {
    uint64_t session_id;
    int32_t device_id;
    int32_t sub_device_id;
  };
  virtual Status DeployRemoteVarManager(const DeployPlan &deploy_plan,
                                        const std::map<std::string, std::vector<ConstSubmodelInfoPtr>> &models,
                                        MasterModelDeployer::DeployedModel &deployedModel);
  Status DeployDevMaintenaceCfg(const std::map<std::string, std::vector<ConstSubmodelInfoPtr>> &models);

  Status GetAllRelatedVarManager(const DeployPlan::DeviceInfo &device_info,
                                 const std::vector<const DeployPlan::SubmodelInfo *> &Submodeles,
                                 std::map<int32_t, std::set<uint64_t>> &sessions,
                                 std::map<int32_t, std::map<uint64_t, std::set<OpDescPtr>>> &node_need_transfer_memory,
                                 std::map<int32_t, std::set<int32_t>> &sub_device_ids);

  Status GetVarManagerAndSendToRemote(
      const std::map<int32_t, std::set<int32_t>> &sub_device_ids,
      const std::map<int32_t, std::set<uint64_t>> &sessions,
      const std::map<int32_t, std::map<uint64_t, std::set<OpDescPtr>>> &node_need_transfer_memory);

  Status CopyOneWeightToTransfer(const SendInfo &send_info, std::istream &input_stream, const int64_t file_const_size,
                                 const OpDescPtr &op_desc);

  Status MasterDeployRankSize();

  Status MasterDeployRankTable();

  Status DeployRankTableInfo(const DeviceInfo &device_info);
  Status DeployDevCfg(const int32_t dev_id, DeviceDebugConfig::ConfigType conf_type);

  std::mutex mu_;
  std::map<uint32_t, DeployedModel> deployed_models_;
  std::atomic<std::uint32_t> model_id_gen_{};
};
}  // namespace ge
#endif  // RUNTIME_DEPLOY_HELPER_MODEL_DEPLOYER_H_
