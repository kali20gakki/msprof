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

#ifndef FUSION_ENGINE_OPTIMIZER_FUSION_CONFIG_MANAGER_FUSION_PRIORITY_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_FUSION_CONFIG_MANAGER_FUSION_PRIORITY_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include "fusion_config_manager/fusion_config_parser.h"
#include "fusion_rule_manager/fusion_rule_manager.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_registry.h"
#include "register/graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"
#include "common/util/error_manager/error_manager.h"
#include "common/fe_error_code.h"

namespace fe {
const string PASS_METHOD = "PASS_METHOD";
const string RULE_METHOD = "RULE_METHOD";

using FusionPassManagerPtr = std::shared_ptr<FusionPassManager>;
using FusionRuleManagerPtr = std::shared_ptr<FusionRuleManager>;
using FusionConfigParserPtr = std::shared_ptr<FusionConfigParser>;

struct FusionPassOrRule {
  std::string name;
  int32_t type; // different pass and rule may have same value of type
  std::string method;
  int32_t priority;
  FusionPassRegistry::CreateFn fusion_pass_create_fn;

  FusionPassOrRule(std::string name, int32_t type,
                   std::string method, int32_t priority,
                   FusionPassRegistry::CreateFn fusion_pass_create_fn)
    : name(std::move(name)),
      type(type),
      method(std::move(method)),
      priority(priority),
      fusion_pass_create_fn(fusion_pass_create_fn) {}
};

struct FusionPassOrRuleFinder {
  std::string fusion_name;

  explicit FusionPassOrRuleFinder(std::string name) : fusion_name(std::move(name)) {}

  bool operator()(const vector<FusionPassOrRule>::value_type &fusion_pass_or_rule) {
    return fusion_pass_or_rule.name == fusion_name;
  }
};

struct BufferFusionInfo {
  std::string name;
  int32_t priority;
  BufferFusionPassRegistry::CreateFn buffer_fusion_pass_create_fn;

  BufferFusionInfo(std::string name, int32_t priority,
                   BufferFusionPassRegistry::CreateFn buffer_fusion_pass_create_fn)
    : name(name),
      priority(priority),
      buffer_fusion_pass_create_fn(buffer_fusion_pass_create_fn) {}
};

struct BufferFusionFinder {
  std::string fusion_name;

  explicit BufferFusionFinder(std::string name) : fusion_name(std::move(name)) {}

  bool operator()(const vector<BufferFusionInfo>::value_type &BufferFusionInfo) {
    return BufferFusionInfo.name == fusion_name;
  }
};

class FusionPriorityManager {
 public:
  explicit FusionPriorityManager(std::string engine_name, FusionPassManagerPtr fusion_pass_mgr_ptr,
                                 FusionRuleManagerPtr fusion_rule_mgr_ptr);
  virtual ~FusionPriorityManager();

  FusionPriorityManager(const FusionPriorityManager &) = delete;
  FusionPriorityManager &operator=(const FusionPriorityManager &) = delete;

  Status Initialize();

  Status SortGraphFusion();

  Status SortBufferFusion();

  static int32_t GetRealPriority(int32_t priority);

  const FusionConfigParserPtr &GetFusionConfigParserPtr() const;

  std::vector<FusionPassOrRule> sorted_graph_fusion_vector_;
  std::vector<BufferFusionInfo> sorted_buffer_fusion_vector_;

 private:
  Status LoadGraphPriorityCfg();

  Status LoadBufferPriorityCfg();

  Status InitPassesAndRules();

  void FindAndModifyPriority();

  void SortFusionPassOrRule();

  Status GetGraphFusionPassInfosByType(GraphFusionPassType pass_type, vector<FusionPassOrRule> &graph_fusion_pass_vector);

  Status GetGraphFusionRuleInfosByType(RuleType type, vector<FusionPassOrRule> &graph_fusion_rule_vector);

  Status GetBufferFusionPassInfosByType(BufferFusionPassType pass_type, vector<BufferFusionInfo> &buffer_fusion_pass_infos);

  int32_t AdjustDownStagePriority(int32_t priority) const;

  std::string engine_name_;
  FusionPassManagerPtr pass_manager_ptr_{nullptr};
  FusionRuleManagerPtr rule_manager_ptr_{nullptr};
  FusionConfigParserPtr fusion_config_parser_ptr_{nullptr};
  bool has_configured_custom_priority_;
  bool has_graph_fusion_inited_;
  bool has_buffer_fusion_inited_;
  std::map<std::string, int32_t> configured_graph_fusion_priority_map_;
  std::map<std::string, int32_t> configured_buffer_fusion_priority_map_;
  std::vector<FusionPassOrRule> sorted_custom_pass_or_rule_vector_;
  std::vector<FusionPassOrRule> sorted_built_in_pass_or_rule_vector_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FUSION_CONFIG_MANAGER_FUSION_PRIORITY_MANAGER_H_
