/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef AIR_CXX_TESTS_ST_STUBS_UTILS_MODEL_FACTORY_H_
#define AIR_CXX_TESTS_ST_STUBS_UTILS_MODEL_FACTORY_H_
#include <string>
#include <unordered_map>
#include "ge_running_env/fake_ns.h"
#include "proto/ge_ir.pb.h"
#include "graph/graph.h"

FAKE_NS_BEGIN

class ModelFactory {
 public:
  static const std::string &GenerateModel_1(bool is_dynamic = true);
  static const std::string &GenerateModel_2(bool is_dynamic = true);
 private:
  static const std::string &SaveAsModel(const std::string &name, const Graph &graph);
  static std::unordered_map<std::string, std::string> model_names_to_path_;
};

FAKE_NS_END

#endif //AIR_CXX_TESTS_ST_STUBS_UTILS_MODEL_FACTORY_H_
