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
#ifndef BASE_EXEC_RUNTIME_DEPLOY_DEPLOY_PLANNER_H_
#define BASE_EXEC_RUNTIME_DEPLOY_DEPLOY_PLANNER_H_
#include <map>
#include <set>
#include <vector>
#include "common/model/ge_root_model.h"

namespace ge {
/**
 * Deploy plan for GeRootModel
 */
class DeployPlan {
 public:
  class DeviceInfo {
   public:
    DeviceInfo() = default;
    DeviceInfo(const int32_t type, const int32_t node_id, const int32_t device_id) noexcept;
    int32_t GetType() const;
    int32_t GetNodeId() const;
    int32_t GetDeviceId() const;
    const std::string &GetKey() const;
    const std::string &GetDesc() const;

   private:
    std::string key_;
    int32_t type_ = 0;
    int32_t node_id_ = -1;
    int32_t device_id_ = 0;
  };

  struct QueueInfo {
    DeviceInfo device_info;
    uint32_t depth = 2U; // minimal queue depth
    std::string name;
    bool owned = true;
    bool is_control = false;
  };

  struct SubmodelInfo {
    DeviceInfo device_info;
    PneModelPtr model;
    std::vector<int32_t> input_queue_indices;
    std::vector<int32_t> control_input_queue_indices;
    std::vector<int32_t> output_queue_indices;
  };

  /// Get QueueInfo by queue_index
  /// @param queue_index      queue index
  /// @param queue_info       queue info
  /// @return                 SUCCESS if got successfully, otherwise returns appropriate error code
  Status GetQueueInfo(const int32_t queue_index, const DeployPlan::QueueInfo *&queue_info) const;

  /// getters and setters
  const std::vector<QueueInfo> &GetQueueInfoList() const;
  const std::vector<std::pair<int32_t, int32_t>> &GetQueueBindings() const;
  const std::vector<int32_t> &GetInputQueueIndices() const;
  const std::vector<int32_t> &GetControlInputQueueIndices() const;
  std::vector<int32_t> GetAllInputQueueIndices() const;
  const std::vector<int32_t> &GetOutputQueueIndices() const;
  const std::map<std::string, SubmodelInfo> &GetSubmodels() const;
  const std::map<int32_t, std::vector<int32_t>> &GetGroups() const;
  bool IsGroupEndpoint(const int32_t queue_index) const;

 private:
  friend class DeployPlannerBase;
  std::string model_name_;
  std::vector<QueueInfo> queues_;
  std::vector<std::pair<int32_t, int32_t>> queue_bindings_;
  SubmodelInfo root_model_info_;
  // key: submodel_name
  std::map<std::string, SubmodelInfo> submodels_;
  // key is group queue index, value is sub queue index list
  std::map<int32_t, std::vector<int32_t>> groups_;
};

class DeployPlannerBase {
 public:
  DeployPlannerBase() = default;
  GE_DELETE_ASSIGN_AND_COPY(DeployPlannerBase);
  virtual ~DeployPlannerBase() = default;

  /// Build DeployPlan
  /// @param deploy_plan      output DeployPlan
  /// @return                 SUCCESS if built successfully, otherwise returns appropriate error code
  Status BuildPlan(DeployPlan &deploy_plan);

  struct ModelQueueIndex {
    std::string model_name;
    size_t id;
    bool operator < (const ModelQueueIndex &other) const {
      if (model_name != other.model_name) {
        return model_name < other.model_name;
      } else {
        return id < other.id;
      }
    }
  };

 protected:
  virtual Status PrepareModelsAndRelation(const ModelRelation *&model_relation) = 0;
  DeployPlan::SubmodelInfo& MutableSubmodelInfo(const std::string &name);
  static Status ValidateModelAndRelation(const std::map<std::string, PneModelPtr> &models,
                                         const ModelRelation &model_relation);

 private:
  Status Initialize();
  // methods for parsing model relation
  Status ParseModelRelation();
  void UpdateRelationForControlIo();
  Status ResolveReusableQueues();
  Status AssignEnqueueQueues();
  Status AssignDequeueQueues();
  Status AssignDequeueQueue(const ModelRelation::QueueDef &queue_def,
                            const DeployPlan::DeviceInfo &device_info,
                            int32_t &queue_index);
  Status AssignEndpointGroups();
  Status AddQueueToCreate(const ModelRelation::QueueDef &queue_def,
                          const DeployPlan::DeviceInfo &device_info,
                          int32_t &queue_idx);
  Status AddGroupToCreate(const DeployPlan::DeviceInfo &device_info,
                          const std::vector<int32_t> &queue_indices,
                          int32_t &group_index);
  Status AddEnqueueGroup(std::vector<std::pair<int32_t, int32_t>> &bindings);
  Status AddDequeueGroup(std::vector<std::pair<int32_t, int32_t>> &bindings);
  static void AddInputQueue(DeployPlan::SubmodelInfo &submodel_info, const bool is_control, const int32_t index);

  DeployPlan deploy_plan_;
  const ModelRelation *model_relation_ = nullptr;
  ModelRelation model_relation_with_control_queues_;
  std::unique_ptr<ModelRelationReader> relation_reader_;
  // enqueue name is same in multi-model instance
  std::multimap<std::string, int32_t> enqueue_queues_;
  std::set<std::string> reusable_queues_;
  std::map<int32_t, ModelQueueIndex> model_queue_indices_;
  std::map<int32_t, std::vector<int32_t>> enqueue_to_dequeue_;
  std::map<int32_t, std::vector<int32_t>> dequeue_to_enqueue_;
};

class DeployPlanner : public DeployPlannerBase {
 public:
  explicit DeployPlanner(const PneModelPtr &root_model);
  ~DeployPlanner() override = default;

 protected:
  Status PrepareModelsAndRelation(const ModelRelation *&model_relation) override;

 private:
  const PneModelPtr root_model_;
};
}  // namespace ge
#endif  // BASE_EXEC_RUNTIME_DEPLOY_DEPLOY_PLANNER_H_
