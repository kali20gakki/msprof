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

#ifndef FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_MANAGER_H_

#include <memory>
#include <string>
#include <vector>
#include "fusion_rule_manager/fusion_rule_data/fusion_rule_pattern.h"
#include "fusion_rule_manager/fusion_rule_parser/fusion_rule_pattern_constructor.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

namespace fe {

using FEOpsKernelInfoStorePtr = std::shared_ptr<FEOpsKernelInfoStore>;

struct FusionRuleFinder {
  std::string rule_name;

  explicit FusionRuleFinder(std::string name) : rule_name(std::move(name)) {}

  bool operator()(const vector<FusionRulePatternPtr>::value_type &rule_pattern_ptr) {
    return rule_pattern_ptr->GetRuleName() == rule_name;
  }
};

/** @brief implement the manage of fusion rules module.
*        Provide the following methods:
*        initializing the management of fusion rules,
*        finalizing the management of fusion rules,
*        and obtaining fusion rules based on rule types. */
class FusionRuleManager {
 public:
  explicit FusionRuleManager(FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr);

  virtual ~FusionRuleManager();

  /*
   * obtain fusion rules from jsons
   * sort fusion rules based on the number of nodes
   */
  Status Initialize(const std::string &engine_name);

  Status Finalize();

  /*
   * obtain fusion rules based on rule types
   * param[in] RuleType ruletype, which is defined by the users
   * param[out] vector<FusionRulePattern> &out_fusion_rules, which is get from the
   *            initialize func
   * return Status
   */
  Status GetFusionRulesByRuleType(RuleType rule_type, vector<FusionRulePatternPtr> &out_fusion_rules);

  Status RunGraphFusionRuleByType(ge::ComputeGraph &graph, RuleType rule_type, const std::string &rule_name);

 private:
  FusionRuleManager(const FusionRuleManager &) = delete;

  FusionRuleManager &operator=(const FusionRuleManager &) = delete;

  Status InitGraphRules(const std::string &engine_name);
  /*
   * Parse and load each fusion rule from json file to fusion_rule_patterns's
   * element
   */
  static Status LoadFusionRule(const std::string &file_path, std::vector<FusionRulePatternPtr> &fusion_rule_patterns,
                               FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr);

  /*
   * Try to open json file, and split whole file to single fusion rules
   */
  static Status OpenJsonFileToItems(const std::string &file_path,
                                    std::vector<nlohmann::json> &fusion_rule_json_objects);

  Status MatchAndReplaceByRules(ge::ComputeGraph &graph, const std::string &rule_name,
                                const vector<FusionRulePatternPtr>::iterator &rule_pattern_ptr_iter);
  /* Store op info */
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;

  bool init_flag_;
  /* Save graph rules loaded from json */
  vector<FusionRulePatternPtr> graph_rule_vector_;
  /* Save custom rules loaded from json */
  vector<FusionRulePatternPtr> custom_rule_vector_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_MANAGER_H_
