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
#include "deploy/deployer/helper_deploy_planner.h"
#include "graph/ge_context.h"

namespace ge {
namespace {
void RewriteQueueName(const std::string &model_name,
                      const std::map<std::string, std::string> &name_map,
                      std::string &queue_name) {
  const auto &it = name_map.find(queue_name);
  if (it == name_map.end()) {
    // inner queue, add model name to avoid name conflict
    queue_name.insert(0, model_name + ":");
  } else {
    queue_name = it->second;
  }
}

void RewriteQueueNames(const std::string &model_name,
                       const std::map<std::string, std::string> &name_map,
                       std::vector<std::string> &queue_names) {
  std::for_each(queue_names.begin(), queue_names.end(), [&](std::string &queue_name) {
    RewriteQueueName(model_name, name_map, queue_name);
  });
}

Status RewriteQueueNames(const std::string &model_name,
                         const ModelRelation::ModelQueueInfo &model_queue_info,
                         ModelRelation &model_relation,
                         std::vector<ModelRelation::QueueDef> &internal_queue_defs) {
  if (model_relation.root_model_queue_info.input_queue_names.size() != model_queue_info.input_queue_names.size()) {
    GELOGE(PARAM_INVALID, "model input number mismatches, parent input size = %zu, submodel input size = %zu",
           model_queue_info.input_queue_names.size(), model_relation.root_model_queue_info.input_queue_names.size());
    return PARAM_INVALID;
  }
  std::map<std::string, std::string> queue_name_mapping;
  for (size_t i = 0; i < model_relation.root_model_queue_info.input_queue_names.size(); ++i) {
    queue_name_mapping.emplace(model_relation.root_model_queue_info.input_queue_names[i],
                               model_queue_info.input_queue_names[i]);
    model_relation.root_model_queue_info.input_queue_names[i] = model_queue_info.input_queue_names[i];
  }
  for (size_t i = 0; i < model_relation.root_model_queue_info.output_queue_names.size(); ++i) {
    queue_name_mapping.emplace(model_relation.root_model_queue_info.output_queue_names[i],
                               model_queue_info.output_queue_names[i]);
    model_relation.root_model_queue_info.output_queue_names[i] = model_queue_info.output_queue_names[i];
  }

  std::for_each(model_relation.queue_defs.begin(), model_relation.queue_defs.end(),
                [&](ModelRelation::QueueDef &queue_def) {
                  RewriteQueueName(model_name, queue_name_mapping, queue_def.name);
                  if (queue_name_mapping.find(queue_def.name) == queue_name_mapping.end()) {
                    internal_queue_defs.emplace_back(queue_def);
                  }
                });
  for (auto &submodel_it : model_relation.submodel_queue_infos) {
    auto &queue_info = submodel_it.second;
    RewriteQueueNames(model_name, queue_name_mapping, queue_info.input_queue_names);
    RewriteQueueNames(model_name, queue_name_mapping, queue_info.output_queue_names);
  }
  return SUCCESS;
}
}  // namespace

HelperDeployPlanner::HelperDeployPlanner(const FlowModelPtr &flow_model,
                                         const std::vector<DeployPlan::DeviceInfo> &device_list) noexcept
    : DeployPlannerBase(),
      flow_model_(flow_model),
      device_list_(device_list)
    {}

Status HelperDeployPlanner::PrepareModelsAndRelation(const ModelRelation *&model_relation) {
  GE_CHECK_NOTNULL(flow_model_);
  auto num_models = flow_model_->GetSubmodels().size();
  if (num_models == 0U) {
    GELOGE(PARAM_INVALID, "models is empty.");
    return PARAM_INVALID;
  }

  std::map<std::string, PneModelPtr> name_to_models;
  if (num_models == 1U) {
    GELOGD("start to build deploy plan for single model");
    GE_CHK_STATUS_RET(PrepareForSingleModel(name_to_models, model_relation),
                      "Failed to init for single model");
  } else {
    GELOGD("start to build deploy plan for multiply models");
    GE_CHK_STATUS_RET(MergeModels(name_to_models, model_relation),
                      "Failed to merge models by relation");
    GE_CHK_STATUS_RET(ValidateModelAndRelation(name_to_models, *model_relation),
                      "Failed to validate model and relation after merging submodels");
  }

  // device selection is not supported yet, deploy to all given devices
  const std::vector<DeployPlan::DeviceInfo> &targets = device_list_;
  std::map<std::string, std::vector<DeployPlan::DeviceInfo>> target_devices;
  for (const auto &it : name_to_models) {
    target_devices.emplace(it.first, targets);
  }

  // resolve target device after merging
  ModelRelation resolved_model_relation;
  std::map<std::string, DeployPlan::DeviceInfo> model_instance_locations;
  std::map<std::string, PneModelPtr> model_instance_to_models;
  resolved_model_relation.queue_defs = model_relation->queue_defs;
  resolved_model_relation.root_model_queue_info = model_relation->root_model_queue_info;
  for (const auto &it : target_devices) {
    const auto &model_name = it.first;
    for (const auto &target_device : it.second) {
      const auto model_instance_name = model_name + "@" + target_device.GetKey();
      // add submodel
      model_instance_to_models.emplace(model_instance_name, name_to_models[model_name]);
      model_instance_locations.emplace(model_instance_name, target_device);
      resolved_model_relation.submodel_queue_infos.emplace(model_instance_name,
                                                           model_relation->submodel_queue_infos.at(model_name));
      auto &model_queue_info = resolved_model_relation.submodel_queue_infos.at(model_instance_name);
      model_queue_info.model_name = model_name;
      GELOGD("Model[%s] emplace model instance[%s].", model_name.c_str(), model_instance_name.c_str());
    }
  }
  name_to_models = std::move(model_instance_to_models);
  merged_model_relation_ = std::move(resolved_model_relation);
  model_relation = &merged_model_relation_;

  GE_CHK_STATUS_RET_NOLOG(ValidateModelAndRelation(name_to_models, *model_relation));
  for (const auto &it : name_to_models) {
    const auto &model_instance_name = it.first;
    const auto &submodel = it.second;
    auto &submodel_info = MutableSubmodelInfo(model_instance_name);
    submodel_info.model = submodel;
    submodel_info.device_info = model_instance_locations[model_instance_name];
    GELOGD("Model [%s] will be deployed on device [%s]",
           model_instance_name.c_str(),
           submodel_info.device_info.GetDesc().c_str());
  }

  return SUCCESS;
}

Status HelperDeployPlanner::PrepareForSingleModel(std::map<std::string, PneModelPtr> &name_to_models,
                                                  const ModelRelation *&model_relation) {
  const auto &root_model = flow_model_->GetSubmodels().begin()->second;
  GE_CHECK_NOTNULL(root_model);
  if (root_model->GetSubmodels().empty()) {
    GELOGD("Prepare for single model without submodels");
    if (root_model->GetModelRelation() != nullptr) {
      GELOGE(PARAM_INVALID, "Submodels is empty while model relation was presented");
      return PARAM_INVALID;
    }
    const auto &root_graph = root_model->GetRootGraph();
    GE_CHECK_NOTNULL(root_graph);
    GE_CHK_STATUS_RET(ModelRelationBuilder().BuildForSingleModel(*root_graph, merged_model_relation_),
                      "Failed to build model relation");
    name_to_models.emplace(root_graph->GetName(), root_model);
    model_relation = &merged_model_relation_;
  } else {
    GELOGD("Prepare for single model with submodels");
    GE_CHECK_NOTNULL(root_model->GetModelRelation().get());
    model_relation = root_model->GetModelRelation().get();
    name_to_models = root_model->GetSubmodels();
    GE_CHK_STATUS_RET(ValidateModelAndRelation(name_to_models, *model_relation),
                      "Failed to validate model and relation");
  }
  return SUCCESS;
}

Status HelperDeployPlanner::UnfoldSubModel(const ModelRelation::ModelQueueInfo &model_queue_info,
                                           const PneModel &model,
                                           std::map<std::string, PneModelPtr> &name_to_models) {
  GE_CHECK_NOTNULL(model.GetModelRelation());
  GE_CHK_STATUS_RET(ValidateModelAndRelation(model.GetSubmodels(), *model.GetModelRelation()),
                    "Failed to validate model and relation for submodel: %s", model.GetModelName().c_str());
  ModelRelation model_relation = *model.GetModelRelation(); // copy
  GE_CHK_STATUS_RET(RewriteQueueNames(model.GetModelName(),
                                      model_queue_info,
                                      model_relation,
                                      merged_model_relation_.queue_defs),
                    "Failed to rewrite queue names, model name = %s",
                    model.GetModelName().c_str());
  ModelRelationReader reader(model_relation);
  GE_CHK_STATUS_RET_NOLOG(reader.Initialize());
  for (const auto &submodel_it : model.GetSubmodels()) {
    const auto &inner_model_name = submodel_it.first;
    auto queue_info = reader.GetSubmodelQueueInfo(inner_model_name);
    GE_CHECK_NOTNULL(queue_info);
    std::string merged_model_name = model.GetModelName() + ":" + inner_model_name;
    // add inner submodel to merged model relation
    name_to_models.emplace(merged_model_name, submodel_it.second);
    merged_model_relation_.submodel_queue_infos[merged_model_name] = *queue_info;
  }
  return SUCCESS;
}

Status HelperDeployPlanner::MergeModels(std::map<std::string, PneModelPtr> &name_to_models,
                                        const ModelRelation *&model_relation) {
  // initialize merged model relation
  auto root_model_relation = flow_model_->GetModelRelation();
  GE_CHECK_NOTNULL(root_model_relation);
  merged_model_relation_.root_model_queue_info.input_queue_names =
      root_model_relation->root_model_queue_info.input_queue_names;
  merged_model_relation_.root_model_queue_info.output_queue_names =
      root_model_relation->root_model_queue_info.output_queue_names;
  merged_model_relation_.queue_defs.insert(merged_model_relation_.queue_defs.end(),
                                           root_model_relation->queue_defs.begin(),
                                           root_model_relation->queue_defs.end());
  std::map<std::string, PneModelPtr> submodels;
  for (const auto &model : flow_model_->GetSubmodels()) {
    submodels.emplace(model.second->GetModelName(), model.second);
  }
  for (const auto &it : root_model_relation->submodel_queue_infos) {
    const auto &model_name = it.first;
    const auto &model_queue_info = it.second;
    auto submodel = submodels[model_name];
    if (submodel == nullptr) {
      GELOGE(PARAM_INVALID, "Failed to get model, name = %s", model_name.c_str());
      return PARAM_INVALID;
    }
    if (submodel->GetSubmodels().empty()) {
      GELOGD("Submodel [%s] contains no child model", model_name.c_str());
      name_to_models.emplace(model_name, submodel);
      merged_model_relation_.submodel_queue_infos[model_name] = model_queue_info;
    } else {
      GELOGD("Submodel [%s] contains child model, start to unfold them to root relation", model_name.c_str());
      GE_CHK_STATUS_RET(UnfoldSubModel(model_queue_info, *submodel, name_to_models), "Failed to unfold submodels");
    }
  }

  model_relation = &merged_model_relation_;
  return SUCCESS;
}
}  // namespace ge
