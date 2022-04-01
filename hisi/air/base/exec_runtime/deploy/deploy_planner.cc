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
#include "exec_runtime/deploy/deploy_planner.h"

namespace ge {
namespace {
constexpr int32_t kLocalNodeId = 0;
constexpr uint32_t kDepDefQueDepth = 8U;
const DeployPlan::DeviceInfo kLocalDeviceInfo{0, kLocalNodeId, 0};
}  // namespace

const std::vector<DeployPlan::QueueInfo> &DeployPlan::GetQueueInfoList() const {
  return queues_;
}

const std::vector<std::pair<int32_t, int32_t>> &DeployPlan::GetQueueBindings() const {
  return queue_bindings_;
}

const std::vector<int32_t> &DeployPlan::GetInputQueueIndices() const {
  return root_model_info_.input_queue_indices;
}

const std::vector<int32_t> &DeployPlan::GetOutputQueueIndices() const {
  return root_model_info_.output_queue_indices;
}

const std::map<std::string, DeployPlan::SubmodelInfo> &DeployPlan::GetSubmodels() const {
  return submodels_;
}

const std::map<int32_t, std::vector<int32_t>> &DeployPlan::GetGroups() const {
  return groups_;
}

bool DeployPlan::IsGroupEndpoint(const int32_t queue_index) const {
  return groups_.find(queue_index) != groups_.end();
}

Status DeployPlan::GetQueueInfo(const int32_t queue_index, const DeployPlan::QueueInfo *&queue_info) const {
  if ((queue_index < 0) || (static_cast<size_t>(queue_index) >= queues_.size())) {
    GELOGE(PARAM_INVALID, "Queue index(%d) out of range: [0, %zu)", queue_index, queues_.size());
    return PARAM_INVALID;
  }
  queue_info = &queues_[static_cast<size_t>(queue_index)];
  return SUCCESS;
}

std::vector<int32_t> DeployPlan::GetAllInputQueueIndices() const {
  auto all_indices = root_model_info_.input_queue_indices;
  (void) all_indices.insert(all_indices.cend(),
                            root_model_info_.control_input_queue_indices.cbegin(),
                            root_model_info_.control_input_queue_indices.cend());
  return all_indices;
}

const std::vector<int32_t> &DeployPlan::GetControlInputQueueIndices() const {
  return root_model_info_.control_input_queue_indices;
}

DeployPlanner::DeployPlanner(const PneModelPtr &root_model)
    : DeployPlannerBase(), root_model_(root_model) {
}

Status DeployPlannerBase::BuildPlan(DeployPlan &deploy_plan) {
  GE_CHK_STATUS_RET(Initialize(), "Failed to initialize deploy planner.");
  GE_CHK_STATUS_RET(ParseModelRelation(), "Failed to parse model relation.");
  deploy_plan = std::move(deploy_plan_);
  return SUCCESS;
}

Status DeployPlannerBase::ValidateModelAndRelation(const std::map<std::string, PneModelPtr> &models,
                                                   const ModelRelation &model_relation) {
  // check all model in model_relation exist in RootModel
  for (const auto &it : model_relation.submodel_queue_infos) {
    const auto &model_name = it.first;
    const auto &submodel = models.find(model_name);
    if (submodel == models.end()) {
      GELOGE(PARAM_INVALID, "model exists in ModelRelation bot not found in RootModel, name = %s", model_name.c_str());
      return PARAM_INVALID;
    }
  }
  return SUCCESS;
}

Status DeployPlannerBase::ParseModelRelation() {
  GE_CHK_STATUS_RET(AssignEnqueueQueues(), "Failed to assign enqueue queues");
  GE_CHK_STATUS_RET(ResolveReusableQueues(), "Failed to resolve reusable queues");
  GE_CHK_STATUS_RET(AssignDequeueQueues(), "Failed to assign dequeue queues");
  GE_CHK_STATUS_RET(AssignEndpointGroups(), "Failed to assign endpoint groups");
  return SUCCESS;
}

Status DeployPlannerBase::AssignEnqueueQueues() {
  for (const auto &queue_def : relation_reader_->GetInputQueueDefs()) {
    const auto &queue_name = queue_def->name;
    int32_t queue_index = -1;
    GE_CHK_STATUS_RET_NOLOG(AddQueueToCreate(*queue_def, kLocalDeviceInfo, queue_index));
    (void)enqueue_queues_.emplace(queue_name, queue_index);
    AddInputQueue(deploy_plan_.root_model_info_, queue_def->is_control_, queue_index);
    GELOGD("Queue index assigned to root model input successfully, queue name = %s, queue index = %d",
           queue_def->name.c_str(), queue_index);
  }

  for (const auto &ext_queue_name : model_relation_->root_model_queue_info.external_queue_names) {
    ModelRelation::QueueDef queue_def;
    queue_def.name = ext_queue_name;
    int32_t queue_index = -1;
    GE_CHK_STATUS_RET_NOLOG(AddQueueToCreate(queue_def, kLocalDeviceInfo, queue_index));
    deploy_plan_.queues_[static_cast<size_t>(queue_index)].owned = false;
    (void)enqueue_queues_.emplace(ext_queue_name, queue_index);
  }

  for (const auto &it : model_relation_->submodel_queue_infos) {
    const auto &model_name = it.first;
    std::vector<const ModelRelation::QueueDef *> model_output_queues;
    GE_CHK_STATUS_RET_NOLOG(relation_reader_->BatchGetQueueDefs(it.second.output_queue_names, model_output_queues));
    const auto &device_info = deploy_plan_.submodels_[model_name].device_info;
    auto &output_queue_indices = deploy_plan_.submodels_[model_name].output_queue_indices;
    for (size_t output_idx = 0U; output_idx < model_output_queues.size(); ++output_idx) {
      const auto * const queue_def = model_output_queues[output_idx];
      GE_CHECK_NOTNULL(queue_def);
      const auto &queue_name = queue_def->name;
      int32_t queue_index = -1;
      GE_CHK_STATUS_RET_NOLOG(AddQueueToCreate(*queue_def, device_info, queue_index));
      (void)enqueue_queues_.emplace(queue_name, queue_index);
      output_queue_indices.emplace_back(queue_index);
      model_queue_indices_[queue_index] = {it.second.model_name, output_idx};
      GELOGD("Queue index assigned successfully, model = %s, model instance = %s, output index = %zu, "
             "queue_name = %s, queue index = %d",
             it.second.model_name.c_str(), model_name.c_str(), output_idx, queue_name.c_str(), queue_index);
    }
  }
  return SUCCESS;
}

Status DeployPlannerBase::ResolveReusableQueues() {
  // 1. collect dequeue operations
  // <queue_name, <device_id>>
  std::map<std::string, std::vector<DeployPlan::DeviceInfo>> queue_refs;
  for (const auto * const queue_def : relation_reader_->GetOutputQueueDefs()) {
    queue_refs[queue_def->name].emplace_back(kLocalDeviceInfo);
  }
  for (const auto &it : model_relation_->submodel_queue_infos) {
    const auto &model_name = it.first;
    const auto &device_info = deploy_plan_.submodels_[model_name].device_info;
    for (const auto &queue_name : it.second.input_queue_names) {
      queue_refs[queue_name].emplace_back(device_info);
    }
  }

  for (const auto &it : queue_refs) {
    const auto &queue_name = it.first;
    if (it.second.size() > 1U) {  // has multiple dequeue op
      GELOGD("Queue[%s] has one-to-many relation", queue_name.c_str());
      continue;
    }

    const std::multimap<std::string, int32_t>::const_iterator &src_dev_it = enqueue_queues_.find(queue_name);
    if (src_dev_it == enqueue_queues_.end()) {
      GELOGE(PARAM_INVALID, "Failed to find enqueue operation for queue [%s]", queue_name.c_str());
      return PARAM_INVALID;
    }

    if (enqueue_queues_.count(queue_name) > 1) {
      GELOGD("Queue [%s] has multi instance", queue_name.c_str());
      continue;
    }

    const auto enqueue_queue_index = src_dev_it->second;
    const auto enqueue_device = deploy_plan_.queues_[static_cast<size_t>(enqueue_queue_index)].device_info;
    const auto dequeue_device = it.second.front();
    if (enqueue_device.GetKey() != dequeue_device.GetKey()) {
      GELOGD("Queue [%s], enqueue device = %s, dequeue device = %s",
             queue_name.c_str(), enqueue_device.GetDesc().c_str(), dequeue_device.GetDesc().c_str());
      continue;
    }

    GELOGD("Queue [%s] has one-to-one relation and in same device: %s",
           queue_name.c_str(), enqueue_device.GetKey().c_str());
    (void)reusable_queues_.emplace(queue_name);
  }
  return SUCCESS;
}

Status DeployPlannerBase::AssignDequeueQueue(const ModelRelation::QueueDef &queue_def,
                                             const DeployPlan::DeviceInfo &device_info,
                                             int32_t &queue_index) {
  const auto &queue_name = queue_def.name;
  const std::multimap<std::string, int32_t>::const_iterator &src_queue_it = enqueue_queues_.find(queue_name);
  if (src_queue_it == enqueue_queues_.end()) {
    GELOGE(PARAM_INVALID, "Failed to find enqueue operation for queue [%s]", queue_name.c_str());
    return PARAM_INVALID;
  }

  if (reusable_queues_.find(queue_name) != reusable_queues_.end()) {
    queue_index = src_queue_it->second;
    GELOGD("Reuse enqueue queue, queue name = %s, queue index = %d", queue_name.c_str(), queue_index);
  } else {
    GE_CHK_STATUS_RET_NOLOG(AddQueueToCreate(queue_def, device_info, queue_index));
    const auto iter_range = enqueue_queues_.equal_range(queue_name);
    // In the multi-instance scenario, the input index is multiple
    for (auto iter = iter_range.first; iter != iter_range.second; ++iter) {
      auto &dequeues = enqueue_to_dequeue_[iter->second];
      dequeues.emplace_back(queue_index);
      auto &enqueues = dequeue_to_enqueue_[queue_index];
      enqueues.emplace_back(iter->second);
      deploy_plan_.queue_bindings_.emplace_back(iter->second, queue_index);
    }
  }
  return SUCCESS;
}

Status DeployPlannerBase::AssignDequeueQueues() {
  for (const auto &it : model_relation_->submodel_queue_infos) {
    const auto &model_name = it.first;
    std::vector<const ModelRelation::QueueDef *> model_queues;
    GE_CHK_STATUS_RET_NOLOG(relation_reader_->BatchGetQueueDefs(it.second.input_queue_names, model_queues));
    auto &submodel_info = deploy_plan_.submodels_[model_name];
    const auto &device_info = submodel_info.device_info;
    for (size_t input_idx = 0U; input_idx < model_queues.size(); ++input_idx) {
      const auto * const queue_def = model_queues[input_idx];
      int32_t queue_index = -1;
      GE_CHK_STATUS_RET(AssignDequeueQueue(*queue_def, device_info, queue_index),
                        "Failed to assign input queue, model = %s, input index = %zu, queue name = %s",
                        model_name.c_str(), input_idx, queue_def->name.c_str());
      AddInputQueue(submodel_info, queue_def->is_control_, queue_index);
      model_queue_indices_[queue_index] = {it.second.model_name, input_idx};
      GELOGD("Queue index assigned successfully, model = %s, model instance = %s, input index = %zu, "
             "queue name = %s, queue index = %d",
             it.second.model_name.c_str(), model_name.c_str(), input_idx, queue_def->name.c_str(), queue_index);
    }

    for (const auto &ext_queue_name : it.second.external_queue_names) {
      ModelRelation::QueueDef queue_def;
      queue_def.name = ext_queue_name;
      queue_def.depth = kDepDefQueDepth;
      int32_t queue_index = -1;
      GE_CHK_STATUS_RET(AssignDequeueQueue(queue_def, device_info, queue_index),
                        "Failed to assign external input queue, model = %s, queue name = %s",
                        model_name.c_str(), queue_def.name.c_str());
      GELOGD("External queue index assigned successfully, model = %s, queue name = %s, queue index = %d",
             model_name.c_str(), queue_def.name.c_str(), queue_index);
    }
  }

  auto &output_queue_indices = deploy_plan_.root_model_info_.output_queue_indices;
  for (const auto * const queue_def : relation_reader_->GetOutputQueueDefs()) {
    int32_t queue_index = -1;
    GE_CHK_STATUS_RET(AssignDequeueQueue(*queue_def, kLocalDeviceInfo, queue_index),
                      "Failed to assign output queue for root model, queue name = %s",
                      queue_def->name.c_str());
    output_queue_indices.emplace_back(queue_index);
    GELOGD("Queue index assigned to root model output successfully, queue name = %s, queue index = %d",
           queue_def->name.c_str(), queue_index);
  }
  return SUCCESS;
}

Status DeployPlannerBase::AddGroupToCreate(const DeployPlan::DeviceInfo &device_info,
                                           const std::vector<int32_t> &queue_indices,
                                           int32_t &group_index) {
  if (queue_indices.empty()) {
    GELOGW("Queue indices is empty, no need to create group.");
    return SUCCESS;
  }
  ModelRelation::QueueDef queue_def;
  const size_t queue_index = static_cast<size_t>(queue_indices[0U]);
  queue_def.name = deploy_plan_.queues_[queue_index].name;
  queue_def.depth = deploy_plan_.queues_[queue_index].depth;
  GE_CHK_STATUS_RET_NOLOG(AddQueueToCreate(queue_def, device_info, group_index));
  deploy_plan_.groups_[group_index] = queue_indices;
  return SUCCESS;
}

Status DeployPlannerBase::AddEnqueueGroup(std::vector<std::pair<int32_t, int32_t>> &bindings) {
  for (const auto &dequeue_iter : enqueue_to_dequeue_) {
    const auto enqueue_index = dequeue_iter.first;
    const auto &dequeue_list = dequeue_iter.second;
    std::map<ModelQueueIndex, std::vector<int32_t>> groups;
    const auto queue_name = deploy_plan_.queues_[static_cast<size_t>(enqueue_index)].name;
    const auto device_info = deploy_plan_.queues_[static_cast<size_t>(enqueue_index)].device_info;
    for (const auto dequeue_index : dequeue_list) {
      // group dequeue indices
      groups[model_queue_indices_[dequeue_index]].emplace_back(dequeue_index);
      GELOGD("Dequeue index[%d] is multi instance, enqueue index[%d], queue name[%s].",
             dequeue_index, enqueue_index, queue_name.c_str());
    }

    for (const auto &group_iter : groups) {
      const auto &group = group_iter.second;
      int32_t group_index = -1;
      GE_CHK_STATUS_RET_NOLOG(AddGroupToCreate(device_info, group, group_index));
      bindings.emplace_back(enqueue_index, group_index);
      GELOGD("Create group success, group index[%d], add relation[%d->%d], queue name[%s].",
             group_index, enqueue_index, group_index, queue_name.c_str());
    }
  }
  return SUCCESS;
}

Status DeployPlannerBase::AddDequeueGroup(std::vector<std::pair<int32_t, int32_t>> &bindings) {
  for (const auto &enqueue_list_iter : dequeue_to_enqueue_) {
    const auto dequeue_indice = enqueue_list_iter.first;
    const auto device_info = deploy_plan_.queues_[static_cast<size_t>(dequeue_indice)].device_info;
    int32_t group_index = -1;
    GE_CHK_STATUS_RET_NOLOG(AddGroupToCreate(device_info, enqueue_list_iter.second, group_index));
    bindings.emplace_back(group_index, dequeue_indice);
    const auto queue_name = deploy_plan_.queues_[static_cast<size_t>(group_index)].name;
    GELOGD("Create group success, group indice[%d], add relation[%d->%d], queue name[%s].",
           group_index, group_index, dequeue_indice, queue_name.c_str());
  }
  return SUCCESS;
}

Status DeployPlannerBase::AssignEndpointGroups() {
  GE_CHK_STATUS_RET_NOLOG(AddEnqueueGroup(deploy_plan_.queue_bindings_));
  GE_CHK_STATUS_RET_NOLOG(AddDequeueGroup(deploy_plan_.queue_bindings_));
  return SUCCESS;
}

Status DeployPlannerBase::AddQueueToCreate(const ModelRelation::QueueDef &queue_def,
                                           const DeployPlan::DeviceInfo &device_info,
                                           int32_t &queue_idx) {
  const auto queue_size = deploy_plan_.queues_.size();
  GE_CHECK_LE(queue_size, static_cast<size_t>(INT32_MAX));
  DeployPlan::QueueInfo queue_info;
  queue_info.device_info = device_info;
  queue_info.depth = queue_def.depth;
  queue_info.name = queue_def.name;
  queue_info.is_control = queue_def.is_control_;
  deploy_plan_.queues_.emplace_back(queue_info);
  queue_idx = static_cast<int32_t>(queue_size);
  return SUCCESS;
}

DeployPlan::SubmodelInfo &DeployPlannerBase::MutableSubmodelInfo(const std::string &name) {
  return deploy_plan_.submodels_[name];
}

Status DeployPlannerBase::Initialize() {
  GE_CHK_STATUS_RET(PrepareModelsAndRelation(model_relation_), "Failed to prepare");
  UpdateRelationForControlIo();  // add control input/output for submodels if needed
  relation_reader_ = MakeUnique<ModelRelationReader>(*model_relation_);
  GE_CHECK_NOTNULL(relation_reader_);
  GE_CHK_STATUS_RET(relation_reader_->Initialize(), "Failed to initialize model relation reader");
  return SUCCESS;
}

void DeployPlannerBase::UpdateRelationForControlIo() {
  std::vector<std::string> models_without_input;
  for (const auto &it : model_relation_->submodel_queue_infos) {
    const auto &submodel_queue_info = it.second;
    if (submodel_queue_info.input_queue_names.empty() && submodel_queue_info.external_queue_names.empty()) {
      // need control input queue
      // all empty goes to LoadModelWithoutQ for now
      if (!submodel_queue_info.output_queue_names.empty()) {
        GELOGI("submodel [%s] needs control input", it.first.c_str());
        models_without_input.emplace_back(it.first);
      }
    }
  }

  if (!models_without_input.empty()) {
    model_relation_with_control_queues_ = *model_relation_;
    const std::string control_input_queue_name = "__control_input";
    ModelRelation::QueueDef queue_def{};
    queue_def.name = control_input_queue_name;
    queue_def.depth = kDepDefQueDepth;
    queue_def.is_control_ = true;
    model_relation_with_control_queues_.queue_defs.emplace_back(queue_def);
    model_relation_with_control_queues_.root_model_queue_info.input_queue_names.emplace_back(control_input_queue_name);
    for (const auto &model_name : models_without_input) {
      model_relation_with_control_queues_.submodel_queue_infos[model_name].input_queue_names.emplace_back(control_input_queue_name);
    }

    model_relation_ = &model_relation_with_control_queues_;
  }
}

void DeployPlannerBase::AddInputQueue(DeployPlan::SubmodelInfo &submodel_info,
                                      const bool is_control,
                                      const int32_t index) {
  if (is_control) {
    submodel_info.control_input_queue_indices.emplace_back(index);
  } else {
    submodel_info.input_queue_indices.emplace_back(index);
  }
}

Status DeployPlanner::PrepareModelsAndRelation(const ModelRelation *&model_relation) {
  auto pne_root_model = root_model_->GetSubmodels().begin()->second;
  GE_CHECK_NOTNULL(pne_root_model->GetModelRelation());
  model_relation = pne_root_model->GetModelRelation().get();
  auto &name_to_models = pne_root_model->GetSubmodels();
  GE_CHK_STATUS_RET_NOLOG(ValidateModelAndRelation(name_to_models, *model_relation));
  for (const auto &it : name_to_models) {
    const auto &model_name = it.first;
    const auto &submodel = it.second;
    auto &submodel_info = MutableSubmodelInfo(model_name);
    submodel_info.model = submodel;
    submodel_info.device_info = DeployPlan::DeviceInfo(0, kLocalNodeId, 0);
    GELOGD("Model [%s] will be deployed on device [%d]", model_name.c_str(), submodel_info.device_info.GetNodeId());
  }
  return SUCCESS;
}

DeployPlan::DeviceInfo::DeviceInfo(const int32_t type, const int32_t node_id, const int32_t device_id) noexcept
    : type_(type), node_id_(node_id), device_id_(device_id) {
  key_ = std::to_string(type) + "_" + std::to_string(node_id) + "_" + std::to_string(device_id);
}

int32_t DeployPlan::DeviceInfo::GetType() const {
  return type_;
}

int32_t DeployPlan::DeviceInfo::GetNodeId() const {
  return node_id_;
}

int32_t DeployPlan::DeviceInfo::GetDeviceId() const {
  return device_id_;
}

const string &DeployPlan::DeviceInfo::GetKey() const {
  return key_;
}

const std::string &DeployPlan::DeviceInfo::GetDesc() const {
  return key_;
}
}  // namespace ge
