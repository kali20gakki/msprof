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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_FACTORY_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_FACTORY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "framework/common/debug/ge_log.h"
#include "common/plugin/ge_util.h"
#include "runtime/rt_model.h"

namespace ge {
class TaskInfo;
using TaskInfoPtr = std::shared_ptr<TaskInfo>;

class TaskInfoFactory {
 public:
  // TaskManagerCreator function def
  using TaskInfoCreatorFun = std::function<TaskInfoPtr(void)>;

  static TaskInfoFactory &Instance() {
    static TaskInfoFactory instance;
    return instance;
  }

  TaskInfoPtr Create(const rtModelTaskType_t task_type) {
    const std::map<rtModelTaskType_t, TaskInfoCreatorFun>::const_iterator iter = creator_map_.find(task_type);
    if (iter == creator_map_.cend()) {
      GELOGW("Cannot find task type %d in inner map.", static_cast<int32_t>(task_type));
      return nullptr;
    }

    return iter->second();
  }

  // TaskInfo registerar
  class Registerar {
   public:
    Registerar(const rtModelTaskType_t type, const TaskInfoCreatorFun func) {
      TaskInfoFactory::Instance().RegisterCreator(type, func);
    }

    ~Registerar() = default;
  };

 private:
  TaskInfoFactory() = default;

  ~TaskInfoFactory() = default;

  // register creator, this function will call in the constructor
  void RegisterCreator(const rtModelTaskType_t type, const TaskInfoCreatorFun func) {
    const std::map<rtModelTaskType_t, TaskInfoCreatorFun>::const_iterator iter = creator_map_.find(type);
    if (iter != creator_map_.cend()) {
      GELOGD("TaskManagerFactory::RegisterCreator: %d creator already exist", static_cast<int32_t>(type));
      return;
    }

    creator_map_[type] = func;
  }

  std::map<rtModelTaskType_t, TaskInfoCreatorFun> creator_map_;
};
}  // namespace ge

#define REGISTER_TASK_INFO(type, clazz)                                                        \
namespace {                                                                                    \
  TaskInfoPtr Creator_Task_Info_##type() {                                                     \
    std::shared_ptr<clazz> ptr = nullptr;                                                      \
    ptr = MakeShared<clazz>();                                                                 \
    return ptr;                                                                                \
  }                                                                                            \
  TaskInfoFactory::Registerar g_Task_Info_Creator_##type(type, &Creator_Task_Info_##type);     \
}  // namespace
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_FACTORY_H_
