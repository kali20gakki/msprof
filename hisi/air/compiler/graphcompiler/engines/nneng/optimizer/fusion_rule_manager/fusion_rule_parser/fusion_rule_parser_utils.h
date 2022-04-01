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

#ifndef FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_PARSER_FUSION_RULE_PARSER_UTILS_H_
#define FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_PARSER_FUSION_RULE_PARSER_UTILS_H_

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include "common/fe_utils.h"
#include "graph/utils/attr_utils.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

namespace fe {

using FEOpsKernelInfoStorePtr = std::shared_ptr<FEOpsKernelInfoStore>;

Status GetFromJson(const nlohmann::json &json_object, std::string &str);

bool AnalyseCheckMap(const std::map<std::string, bool> &check_map);

/*
 * @brief: Split input string by format "key:value"
 */
Status SplitKeyValueByColon(const std::string &str, std::string &key, std::string &value);

/*
 * @brief: Split input string by format "key.value"
 */
Status SplitKeyValueByDot(const std::string &str, std::string &key, std::string &value);

Status StringToInt(const std::string &str, int &value);

Status StringToInt64(const std::string &str, int64_t &value);

Status StringToFloat(const std::string &str, float &value);
/*
 * @brief: Get value stored in GeAttrValue, and convert it to string.
 */
std::string GetStrFromAttrValue(ge::GeAttrValue &attr_value);

class FusionRuleParserUtils {
 public:
  FusionRuleParserUtils() : ops_kernel_info_store_ptr_(nullptr) {}

  ~FusionRuleParserUtils() {}

  static FusionRuleParserUtils *Instance() {
    static FusionRuleParserUtils instance;
    return &instance;
  }

  void SetValue(FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr) {
    std::lock_guard<std::mutex> lock_guard(ops_kernel_info_mutex_);
    ops_kernel_info_store_ptr_ = ops_kernel_info_store_ptr;
  }

  FEOpsKernelInfoStorePtr GetValue() {
    std::lock_guard<std::mutex> lock_guard(ops_kernel_info_mutex_);
    return ops_kernel_info_store_ptr_;
  }

 private:
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
  std::mutex ops_kernel_info_mutex_;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_PARSER_FUSION_RULE_PARSER_UTILS_H_
