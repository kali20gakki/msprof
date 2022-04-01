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

#include "hybrid/node_executor/rts/rts_task_factory.h"

namespace ge {
namespace hybrid {
RtsNodeTaskPtr RtsTaskFactory::Create(const std::string &task_type) const {
  const auto it = creators_.find(task_type);
  if (it == creators_.end()) {
    GELOGW("Cannot find task type %s in inner map.", task_type.c_str());
    return nullptr;
  }

  return it->second();
}

void RtsTaskFactory::RegisterCreator(const std::string &task_type, const RtsTaskCreatorFun &creator) {
  if (creator == nullptr) {
    GELOGW("Register %s creator is null", task_type.c_str());
    return;
  }

  const std::map<std::string, RtsTaskCreatorFun>::const_iterator it = creators_.find(task_type);
  if (it != creators_.cend()) {
    GELOGW("Task %s creator already exist", task_type.c_str());
    return;
  }

  creators_[task_type] = creator;
}
}  // namespace hybrid
}  // namespace ge
