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

#ifndef GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_TASK_FACTORY_H_
#define GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_TASK_FACTORY_H_

#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task.h"

namespace ge {
namespace hybrid {
using FftsPlusSubTaskPtr = std::shared_ptr<FftsPlusSubTask>;
using FftsPlusSubTaskCreatorFun = std::function<FftsPlusSubTaskPtr()>;

class FftsPlusSubTaskFactory {
 public:
  static FftsPlusSubTaskFactory &Instance();

  /**
   * Get FFTS Plus sub node task by core type.
   * @param core_type: core type of Node
   * @return FFTS Plus sub node task instance.
   */
  FftsPlusSubTaskPtr Create(const std::string &core_type) const;

  class FftsPlusSubTaskRegistrar {
   public:
    FftsPlusSubTaskRegistrar(const std::string &core_type, const FftsPlusSubTaskCreatorFun &creator) {
      FftsPlusSubTaskFactory::Instance().RegisterCreator(core_type, creator);
    }
    ~FftsPlusSubTaskRegistrar() = default;
  };

 private:
  FftsPlusSubTaskFactory() = default;
  ~FftsPlusSubTaskFactory() = default;

  /**
   * Register FFTS Plus sub node task creator.
   * @param core_type: core type of Node
   * @param creator: FFTS Plus sub node task creator.
   */
  void RegisterCreator(const std::string &core_type, const FftsPlusSubTaskCreatorFun &creator);

  std::map<std::string, FftsPlusSubTaskCreatorFun> creators_;
};
} // namespace hybrid
} // namespace ge

#define REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(core_type, task_clazz)                    \
namespace {                                                                           \
  FftsPlusSubTaskPtr Create_##task_clazz##_##core_type() {                            \
    return MakeShared<task_clazz>();                                                  \
  }                                                                                   \
  const FftsPlusSubTaskFactory::FftsPlusSubTaskRegistrar g##task_clazz##_##core_type( \
      (core_type), &Create_##task_clazz##_##core_type);                               \
}
#endif // GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_TASK_FACTORY_H_
