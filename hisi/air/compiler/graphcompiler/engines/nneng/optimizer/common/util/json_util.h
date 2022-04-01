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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_JSON_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_JSON_UTIL_H_

#include <nlohmann/json.hpp>
#include <string>
#include "graph_optimizer/graph_optimize_register_error_codes.h"

using JsonHandle = void*;
namespace fe {

std::string RealPath(const std::string& path);
Status FcntlLockFile(const std::string &file, int fd, int type, uint32_t recursive_cnt);
Status ReadJsonFile(const std::string& file, nlohmann::json& json_obj);
Status ReadJsonFileByLock(const std::string &file, nlohmann::json &json_obj);
std::string GetSuffixJsonFile(const std::string &json_file_path, const std::string &suffix);
std::string GetJsonType(const nlohmann::json &json_object);
}
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_JSON_UTIL_H_

