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

#ifndef FUSION_ENGINE_UTILS_ADAPTER_FACTORY_TASK_BUILDER_ADAPTER_FACTORY_H_
#define FUSION_ENGINE_UTILS_ADAPTER_FACTORY_TASK_BUILDER_ADAPTER_FACTORY_H_

#include "common/aicore_util_types.h"
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "graph/node.h"

namespace fe {
class TaskBuilderAdapterFactory {
 public:
  TaskBuilderAdapterFactory(const TaskBuilderAdapterFactory &) = delete;
  TaskBuilderAdapterFactory &operator=(const TaskBuilderAdapterFactory &) = delete;

  /**
   * Get the singleton Instance of TaskBuilderAdapterFactory
   * @return Instance
   */
  static TaskBuilderAdapterFactory &Instance();

  /**
   * Create task builder adapter pointer
   * @param op_impl_type
   * @return
   */
  TaskBuilderAdapterPtr CreateTaskBuilderAdapter(const OpImplType &op_impl_type, const ge::Node &node,
                                                 TaskBuilderContext &context) const;

 private:
  TaskBuilderAdapterFactory();
  ~TaskBuilderAdapterFactory();
  static const std::vector<OpImplType> TBE_IMPL_TYPE_VEC;
};
}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_ADAPTER_FACTORY_TASK_BUILDER_ADAPTER_FACTORY_H_
