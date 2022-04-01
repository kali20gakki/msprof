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

#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task_factory.h"

namespace ge {
namespace hybrid {
FftsPlusSubTaskFactory &FftsPlusSubTaskFactory::Instance() {
  static FftsPlusSubTaskFactory instance;
  return instance;
}

FftsPlusSubTaskPtr FftsPlusSubTaskFactory::Create(const std::string &core_type) const {
  const auto it = creators_.find(core_type);
  if (it == creators_.end()) {
    GELOGW("Cannot find creator for core type: %s.", core_type.c_str());
    return nullptr;
  }

  return it->second();
}

void FftsPlusSubTaskFactory::RegisterCreator(const std::string &core_type, const FftsPlusSubTaskCreatorFun &creator) {
  if (creator == nullptr) {
    GELOGW("Register null creator for core type: %s", core_type.c_str());
    return;
  }

  const std::map<std::string, FftsPlusSubTaskCreatorFun>::const_iterator it = creators_.find(core_type);
  if (it != creators_.cend()) {
    GELOGW("Creator already exist for core type: %s", core_type.c_str());
    return;
  }

  GELOGI("Register creator done.");
  creators_[core_type] = creator;
}
} // namespace hybrid
} // namespace ge
