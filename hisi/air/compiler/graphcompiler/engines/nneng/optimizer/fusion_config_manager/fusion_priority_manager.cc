/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "common/configuration.h"
#include "fusion_priority_manager.h"

namespace fe {
static const std::vector<GraphFusionPassType> GRAPH_FUSION_PASS_TYPE_AICORE_VEC = {
    BUILT_IN_GRAPH_PASS,
    SECOND_ROUND_BUILT_IN_GRAPH_PASS,
    BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS,
    BUILT_IN_PREPARE_GRAPH_PASS,
    BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS};

static const std::vector<GraphFusionPassType> GRAPH_FUSION_PASS_TYPE_VECTORCORE_VEC = {BUILT_IN_VECTOR_CORE_GRAPH_PASS};


bool GraphFusionPrioritySort(const FusionPassOrRule &fusion_pass_or_rule1,
                             const FusionPassOrRule &fusion_pass_or_rule2) {
  return fusion_pass_or_rule1.priority < fusion_pass_or_rule2.priority;
}

bool BufferFusionPrioritySort(const BufferFusionInfo &buffer_fusion_info1,
                              const BufferFusionInfo &buffer_fusion_info2) {
  return buffer_fusion_info1.priority < buffer_fusion_info2.priority;
}

FusionPriorityManager::FusionPriorityManager(std::string engine_name, FusionPassManagerPtr fusion_pass_mgr_ptr,
                                             FusionRuleManagerPtr fusion_rule_mgr_ptr)
    : engine_name_(std::move(engine_name)),
      pass_manager_ptr_(std::move(fusion_pass_mgr_ptr)),
      rule_manager_ptr_(std::move(fusion_rule_mgr_ptr)),
      has_configured_custom_priority_(false),
      has_graph_fusion_inited_(false),
      has_buffer_fusion_inited_(false) {}

FusionPriorityManager::~FusionPriorityManager() = default;

Status FusionPriorityManager::Initialize() {
  FE_MAKE_SHARED(fusion_config_parser_ptr_ = std::make_shared<FusionConfigParser>(engine_name_), return FAILED);
  return fusion_config_parser_ptr_->ParseFusionConfigFile();
}

Status FusionPriorityManager::SortGraphFusion() {
  FE_LOGD("SortGraphFusion start.");
  FE_TIMECOST_START(SortGraphFusion);
  if (has_graph_fusion_inited_) {
    FE_LOGD("SortGraphFusion has been inited.");
    return SUCCESS;
  }
  // 1.load configured priority from configuration,
  // init configured_graph_fusion_priority_map_ and configured_buffer_fusion_priority_map_
  if (LoadGraphPriorityCfg() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FusionConfig][SortGphFus] Failed to load configured fusion priority for engine:%s.",
                    engine_name_.c_str());
    return FAILED;
  }

  // 2.init sorted_custom_pass_or_rule_vector_ and sorted_built_in_pass_or_rule_vector_
  if (InitPassesAndRules() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FusionConfig][SortGphFus] Failed to init passes and rules for engine:%s.",
                    engine_name_.c_str());
    return FAILED;
  }

  // 3.find the configured fusion in sorted_custom_pass_or_rule_vector_ and sorted_built_in_pass_or_rule_vector_
  // then change the priority
  if (!configured_graph_fusion_priority_map_.empty()) {
    FindAndModifyPriority();
  }

  // 4.sort sorted_custom_pass_or_rule_vector_ and sorted_built_in_pass_or_rule_vector_ by priority,
  // then combine them to init sorted_graph_fusion_vector_
  SortFusionPassOrRule();
  has_graph_fusion_inited_ = true;
  FE_LOGD("SortGraphFusion success.");
  FE_TIMECOST_END(SortGraphFusion, "FusionPriorityManager::SortGraphFusion");
  return SUCCESS;
}

Status FusionPriorityManager::SortBufferFusion() {
  if (has_buffer_fusion_inited_) {
    FE_LOGD("SortBufferFusion has been inited.");
    return SUCCESS;
  }
  // 1.Load ub priority config
  if (LoadBufferPriorityCfg() != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][FusionConfig][SortBufFus] Failed to load configured buffer fusion priority from \
        configuration for engine:%s.",
        engine_name_.c_str());
    return FAILED;
  }
  sorted_buffer_fusion_vector_.clear();
  vector<BufferFusionInfo> sorted_build_in_buffer_fusion_info_vector;
  // 2.Get ub fusion pass from register
  if (engine_name_ == fe::AI_CORE_NAME) {
    // init built-in ai core pass
    if (GetBufferFusionPassInfosByType(BUILT_IN_AI_CORE_BUFFER_FUSION_PASS,
                                       sorted_build_in_buffer_fusion_info_vector) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FusionConfig][SortGphFus] Failed to get buffer pass from this engine [%s]",
                      engine_name_.c_str());
      return FAILED;
    }
  } else {
    // init built-in vector pass
    if (GetBufferFusionPassInfosByType(BUILT_IN_VECTOR_CORE_BUFFER_FUSION_PASS,
                                       sorted_build_in_buffer_fusion_info_vector) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FusionConfig][SortGphFus] Failed to get buffer pass from this engine [%s]",
                      engine_name_.c_str());
      return FAILED;
    }
  }
  sorted_buffer_fusion_vector_.insert(sorted_buffer_fusion_vector_.cend(),
                                      sorted_build_in_buffer_fusion_info_vector.cbegin(),
                                      sorted_build_in_buffer_fusion_info_vector.cend());
  // 3.Find the configured fusion in sorted_buffer_fusion_info_vector_ and configured_buffer_fusion_priority_map_
  // then change the priority
  for (auto &iter : configured_buffer_fusion_priority_map_) {
    auto buffer_fusion_iter = find_if(sorted_buffer_fusion_vector_.begin(), sorted_buffer_fusion_vector_.end(),
                                      BufferFusionFinder(iter.first));
    if (buffer_fusion_iter != sorted_buffer_fusion_vector_.end()) {
      buffer_fusion_iter->priority = AdjustDownStagePriority(iter.second);
      FE_LOGD("The priority of fusion:%s has been set to %d", iter.first.c_str(), iter.second);
      continue;
    }
    FE_LOGW("Could not find this buffer fusion: %s in engine:%s.", iter.first.c_str(), engine_name_.c_str());
  }
  FE_LOGD("End to add priority and register buffer fusion size: %zu", sorted_buffer_fusion_vector_.size());
  sort(sorted_buffer_fusion_vector_.begin(), sorted_buffer_fusion_vector_.end(), BufferFusionPrioritySort);
  has_buffer_fusion_inited_ = true;
  FE_LOGD("End to order buffer pass.");
  return SUCCESS;
}

Status fe::FusionPriorityManager::LoadGraphPriorityCfg() {
  return fusion_config_parser_ptr_->GetFusionPriorityByFusionType(GRAPH_FUSION, configured_graph_fusion_priority_map_);
}

Status fe::FusionPriorityManager::LoadBufferPriorityCfg() {
  return fusion_config_parser_ptr_->GetFusionPriorityByFusionType(UB_FUSION, configured_buffer_fusion_priority_map_);
}

Status FusionPriorityManager::InitPassesAndRules() {
  // init custom pass
  vector<FusionPassOrRule> custom_graph_fusion_pass_infos;
  sorted_graph_fusion_vector_.clear();
  sorted_custom_pass_or_rule_vector_.clear();
  sorted_built_in_pass_or_rule_vector_.clear();

  if (engine_name_ == fe::AI_CORE_NAME) {
    if (GetGraphFusionPassInfosByType(CUSTOM_AI_CORE_GRAPH_PASS, custom_graph_fusion_pass_infos) != SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][FusionConfig][InitPassRule] Failed to init sorted custom graph fusion passes for engine:%s",
          engine_name_.c_str());
      return FAILED;
    }
  } else {
    if (GetGraphFusionPassInfosByType(CUSTOM_VECTOR_CORE_GRAPH_PASS, custom_graph_fusion_pass_infos) != SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][FusionConfig][InitPassRule] Failed to init sorted custom graph fusion passes for engine:%s",
          engine_name_.c_str());
      return FAILED;
    }
  }
  sorted_custom_pass_or_rule_vector_.insert(sorted_custom_pass_or_rule_vector_.cend(),
                                            custom_graph_fusion_pass_infos.cbegin(),
                                            custom_graph_fusion_pass_infos.cend());
  // init custom rule
  vector<FusionPassOrRule> custom_graph_fusion_rule_infos;
  if (GetGraphFusionRuleInfosByType(RuleType::CUSTOM_GRAPH_RULE, custom_graph_fusion_rule_infos)) {
    REPORT_FE_ERROR(
        "[GraphOpt][FusionConfig][InitPassRule] Failed to init sorted custom graph fusion rules for engine:%s",
        engine_name_.c_str());
    return FAILED;
  }
  sorted_custom_pass_or_rule_vector_.insert(sorted_custom_pass_or_rule_vector_.cend(),
                                            custom_graph_fusion_rule_infos.cbegin(),
                                            custom_graph_fusion_rule_infos.cend());
  // init built-in pass
  vector<FusionPassOrRule> built_in_graph_fusion_pass_infos;
  vector<GraphFusionPassType> pass_type_vec;
  if (engine_name_ == fe::AI_CORE_NAME) {
    pass_type_vec = GRAPH_FUSION_PASS_TYPE_AICORE_VEC;
  } else if (engine_name_ == fe::VECTOR_CORE_NAME) {
    pass_type_vec = GRAPH_FUSION_PASS_TYPE_VECTORCORE_VEC;
  }
  for (auto pass_type : pass_type_vec) {
    if (GetGraphFusionPassInfosByType(pass_type, built_in_graph_fusion_pass_infos) != SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][FusionConfig][InitPassRule] Failed to init sorted built-in graph fusion passes for engine:%s",
          engine_name_.c_str());
      return FAILED;
    }
  }
  sorted_built_in_pass_or_rule_vector_.insert(sorted_built_in_pass_or_rule_vector_.cend(),
                                              built_in_graph_fusion_pass_infos.cbegin(),
                                              built_in_graph_fusion_pass_infos.cend());
  // init built-in rule
  vector<FusionPassOrRule> built_in_graph_fusion_rule_infos;
  if (GetGraphFusionRuleInfosByType(RuleType::BUILT_IN_GRAPH_RULE, built_in_graph_fusion_rule_infos)) {
    REPORT_FE_ERROR(
        "[GraphOpt][FusionConfig][InitPassRule] Failed to init sorted built-in graph fusion rules for engine:%s",
        engine_name_.c_str());
    return FAILED;
  }
  sorted_built_in_pass_or_rule_vector_.insert(sorted_built_in_pass_or_rule_vector_.cend(),
                                              built_in_graph_fusion_rule_infos.cbegin(),
                                              built_in_graph_fusion_rule_infos.cend());
  return SUCCESS;
}

Status FusionPriorityManager::GetGraphFusionPassInfosByType(GraphFusionPassType pass_type,
                                                            vector<FusionPassOrRule> &graph_fusion_pass_vector) {
  string pass_type_str = GetPassTypeString(pass_type);
  std::map<string, FusionPassRegistry::CreateFn> create_fns =
      FusionPassRegistry::GetInstance().GetCreateFnByType(pass_type);
  if (create_fns.empty()) {
    FE_LOGD("No registered graph fusion pass was found, type[%s], engine[%s].", pass_type_str.c_str(),
            engine_name_.c_str());
    return SUCCESS;
  }
  std::string method;
  int32_t priority;
  switch (pass_type) {
    case CUSTOM_AI_CORE_GRAPH_PASS:
    case CUSTOM_VECTOR_CORE_GRAPH_PASS:
      method = PASS_METHOD;
      priority = CUSTOM_PASS_PRIORITY_MIN;
      break;

    case BUILT_IN_PREPARE_GRAPH_PASS:
    case BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS:
    case BUILT_IN_GRAPH_PASS:
    case BUILT_IN_VECTOR_CORE_GRAPH_PASS:
    case SECOND_ROUND_BUILT_IN_GRAPH_PASS:
    case BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS:
      method = PASS_METHOD;
      priority = BUILT_IN_PASS_PRIORITY_MIN;
      break;

    default:
      REPORT_FE_ERROR("[GraphOpt][FusionConfig][GetGphFusPassInfo] The pass type[%s] does not support priority order.",
                      pass_type_str.c_str());
      return FAILED;
  }
  for (const auto &iter : create_fns) {
    std::string pass_name = iter.first;
    if (!fusion_config_parser_ptr_->GetFusionSwitchByName(pass_name, GRAPH_FUSION)) {
      FE_LOGD("The graph fusion pass[%s] switch is off.", pass_name.c_str());
      continue;
    }
    FE_LOGD("Load registered graph fusion pass(switch on): %s", pass_name.c_str());
    graph_fusion_pass_vector.emplace_back(pass_name, static_cast<int32_t>(pass_type), method, priority, iter.second);
    priority++;
  }
  FE_LOGI("The total number of pass(switch on) for type[%s] is %zu.", pass_type_str.c_str(),
          graph_fusion_pass_vector.size());
  return SUCCESS;
}

Status FusionPriorityManager::GetGraphFusionRuleInfosByType(RuleType type,
                                                            vector<FusionPassOrRule> &graph_fusion_rule_vector) {
  FE_CHECK_NOTNULL(rule_manager_ptr_);
  std::vector<FusionRulePatternPtr> graph_fusion_rules;
  if (rule_manager_ptr_->GetFusionRulesByRuleType(type, graph_fusion_rules) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FusionConfig][GetGphFusRuleInfo] Failed to obtain graph fusion rules for type[%s].",
                    GetRuleTypeString(type).c_str());
    return FAILED;
  }
  if (graph_fusion_rules.empty()) {
    FE_LOGD("No graph fusion rule was found, type[%s], engine[%s].", GetRuleTypeString(type).c_str(),
            engine_name_.c_str());
    return SUCCESS;
  }
  std::string method;
  int32_t priority;
  switch (type) {
    case RuleType::CUSTOM_GRAPH_RULE:
      method = RULE_METHOD;
      priority = CUSTOM_RULE_PRIORITY_MIN;
      break;
    case RuleType::BUILT_IN_GRAPH_RULE:
      method = RULE_METHOD;
      priority = BUILT_IN_RULE_PRIORITY_MIN;
      break;
    default:
      REPORT_FE_ERROR("[GraphOpt][FusionConfig][GetGphFusRuleInfo] The rule type[%s] does not support priority order.",
                      GetRuleTypeString(type).c_str());
      return FAILED;
  }
  for (const FusionRulePatternPtr &one_rule : graph_fusion_rules) {
    std::string name = one_rule->GetRuleName();
    if (!fusion_config_parser_ptr_->GetFusionSwitchByName(name, GRAPH_FUSION)) {
      FE_LOGD("The graph fusion rule[%s] switch is off.", name.c_str());
      continue;
    }
    FE_LOGD("Load graph fusion rule(switch on): %s", name.c_str());
    graph_fusion_rule_vector.emplace_back(name, static_cast<int32_t>(type), method, priority, nullptr);
    priority++;
  }
  FE_LOGD("The total number of rule(switch on) for type[%s] is %zu.", GetRuleTypeString(type).c_str(),
          graph_fusion_rule_vector.size());
  return SUCCESS;
}

Status FusionPriorityManager::GetBufferFusionPassInfosByType(BufferFusionPassType pass_type,
                                                             vector<BufferFusionInfo> &buffer_fusion_pass_infos) {
  std::map<string, std::shared_ptr<BufferFusionPassBase>> buffer_fusion_pass_map;

  string pass_type_str = GetBufferFusionPassTypeString(pass_type);
  std::map<string, BufferFusionPassRegistry::CreateFn> create_fns =
      BufferFusionPassRegistry::GetInstance().GetCreateFnByType(pass_type);
  if (create_fns.empty()) {
    FE_LOGD("GetUbFusion-PassType[%s]: registered buffer fusion passes are empty.", pass_type_str.c_str());
    return SUCCESS;
  }
  int64_t priority = BUILT_IN_PASS_PRIORITY_MIN;
  for (const auto &iter : create_fns) {
    std::string pass_name = iter.first;
    if (!fusion_config_parser_ptr_->GetFusionSwitchByName(pass_name, UB_FUSION)) {
      FE_LOGD("The ub fusion pass [%s] switch is off.", pass_name.c_str());
      continue;
    }
    FE_LOGD("Start to load registered buffer fusion passes (on) : %s", pass_name.c_str());
    buffer_fusion_pass_infos.emplace_back(pass_name, priority, iter.second);
    priority++;
  }
  FE_LOGD("GetUbFusion-PassType[%s]: end to get BufferFusionPass.", pass_type_str.c_str());
  return SUCCESS;
}

void FusionPriorityManager::FindAndModifyPriority() {
  for (auto iter = configured_graph_fusion_priority_map_.begin(); iter != configured_graph_fusion_priority_map_.end();
       iter++) {
    auto fusion_info_iter = find_if(sorted_custom_pass_or_rule_vector_.begin(),
                                    sorted_custom_pass_or_rule_vector_.end(), FusionPassOrRuleFinder(iter->first));
    if (fusion_info_iter != sorted_custom_pass_or_rule_vector_.end()) {
      has_configured_custom_priority_ = true;
      fusion_info_iter->priority = AdjustDownStagePriority(iter->second);
      FE_LOGD("The priority of fusion:%s has been set to %d", iter->first.c_str(), iter->second);
      continue;
    }
    fusion_info_iter = find_if(sorted_built_in_pass_or_rule_vector_.begin(), sorted_built_in_pass_or_rule_vector_.end(),
                               FusionPassOrRuleFinder(iter->first));
    if (fusion_info_iter != sorted_built_in_pass_or_rule_vector_.end()) {
      fusion_info_iter->priority = AdjustDownStagePriority(iter->second);
      FE_LOGD("The priority of fusion:%s has been set to %d", iter->first.c_str(), iter->second);
      continue;
    }
    FE_LOGW("Could not find fusion:%s in engine:%s.", iter->first.c_str(), engine_name_.c_str());
  }
}

void FusionPriorityManager::SortFusionPassOrRule() {
  if (!has_configured_custom_priority_) {
    // if the configuration file only has priority for built-in fusion,
    // only sort the built-in vector and insert the custom vector before the built-in vector.
    sort(sorted_built_in_pass_or_rule_vector_.begin(), sorted_built_in_pass_or_rule_vector_.end(),
         GraphFusionPrioritySort);
    sorted_graph_fusion_vector_.insert(sorted_graph_fusion_vector_.cend(), sorted_custom_pass_or_rule_vector_.cbegin(),
                                       sorted_custom_pass_or_rule_vector_.cend());
    sorted_graph_fusion_vector_.insert(sorted_graph_fusion_vector_.cend(),
                                       sorted_built_in_pass_or_rule_vector_.cbegin(),
                                       sorted_built_in_pass_or_rule_vector_.cend());
  } else {
    // if the configuration file has priority both for built-in fusion and custom fusion,
    // combine the built-in vector and the custom vector, then sort it.
    sorted_graph_fusion_vector_.insert(sorted_graph_fusion_vector_.cend(), sorted_custom_pass_or_rule_vector_.cbegin(),
                                       sorted_custom_pass_or_rule_vector_.cend());
    sorted_graph_fusion_vector_.insert(sorted_graph_fusion_vector_.cend(),
                                       sorted_built_in_pass_or_rule_vector_.cbegin(),
                                       sorted_built_in_pass_or_rule_vector_.cend());
    sort(sorted_graph_fusion_vector_.begin(), sorted_graph_fusion_vector_.end(), GraphFusionPrioritySort);
  }
}

int32_t FusionPriorityManager::AdjustDownStagePriority(int32_t priority) const {
  int32_t adjusted_priority = priority;
  if ((priority >= CUSTOM_CFG_DOWN_PRIORITY_MIN && priority < BUILT_IN_CFG_TOP_PRIORITY_MIN) ||
      (priority >= BUILT_IN_CFG_DOWN_PRIORITY_MIN && priority < CUSTOM_PASS_PRIORITY_MIN)) {
    adjusted_priority += RESERVED_FOR_DOWN_PRIORITY;
  }
  return adjusted_priority;
}

int32_t FusionPriorityManager::GetRealPriority(int32_t priority) {
  int32_t real_priority = priority;
  if (priority > RESERVED_FOR_DOWN_PRIORITY) {
    real_priority -= RESERVED_FOR_DOWN_PRIORITY;
  }
  return real_priority;
}

const FusionConfigParserPtr &FusionPriorityManager::GetFusionConfigParserPtr() const {
  return fusion_config_parser_ptr_;
}
}  // namespace fe
