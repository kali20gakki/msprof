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

#include "adapter/factory/task_builder_adapter_factory.h"
#include "common/comm_log.h"
#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
#include "common/fe_log.h"

namespace fe {
const std::vector<OpImplType> TaskBuilderAdapterFactory::TBE_IMPL_TYPE_VEC = {
    EN_IMPL_HW_TBE, EN_IMPL_CUSTOM_TBE, EN_IMPL_PLUGIN_TBE, EN_IMPL_VECTOR_CORE_HW_TBE,
    EN_IMPL_VECTOR_CORE_CUSTOM_TBE, EN_IMPL_NON_PERSISTENT_CUSTOM_TBE
};

TaskBuilderAdapterFactory &TaskBuilderAdapterFactory::Instance() {
  static TaskBuilderAdapterFactory task_builder_adapter_factory;
  return task_builder_adapter_factory;
}

TaskBuilderAdapterPtr TaskBuilderAdapterFactory::CreateTaskBuilderAdapter(const OpImplType &op_impl_type,
                                                                          const ge::Node &node,
                                                                          TaskBuilderContext &context) const {
  TaskBuilderAdapterPtr task_builder_adapter_ptr = nullptr;
  if (std::find(TBE_IMPL_TYPE_VEC.begin(), TBE_IMPL_TYPE_VEC.end(), op_impl_type) != TBE_IMPL_TYPE_VEC.end()) {
    FE_MAKE_SHARED(task_builder_adapter_ptr = std::make_shared<TbeTaskBuilderAdapter>(node, context), return nullptr);
    if (task_builder_adapter_ptr == nullptr) {
      CM_LOGW("Fail to create TbeTaskBuilderAdapter pointer for node[%s].", node.GetName().c_str());
    }
  }
  return task_builder_adapter_ptr;
}

TaskBuilderAdapterFactory::TaskBuilderAdapterFactory() {}
TaskBuilderAdapterFactory::~TaskBuilderAdapterFactory() {}
}  // namespace fe
