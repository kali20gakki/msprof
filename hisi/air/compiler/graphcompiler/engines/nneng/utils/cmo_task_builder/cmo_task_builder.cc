/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "cmo_task_builder.h"

namespace fe {
CMOTaskBuilder::CMOTaskBuilder() {}
CMOTaskBuilder::~CMOTaskBuilder() {}

Status CMOTaskBuilder::GenerateCMOTask(const ge::Node &node, std::vector<domi::TaskDef> &task_defs,
                                       const int32_t &stream_id, TaskBuilderContext context, const bool &pre_cmo_task) {
  ge::OpDescPtr op_desc = node.GetOpDesc();
  CmoExtraAttr cmo_ext_attr;
  cmo_ext_attr = op_desc->TryGetExtAttr(kOpExtattrNameCmo, cmo_ext_attr);

  Status status;
  for (auto &cmo_attr : cmo_ext_attr) {
    GenerateCMOTaskBasePtr cmo_task_ptr = GetCMOTaskType(node, context, cmo_attr.first, pre_cmo_task);
    if (cmo_task_ptr == nullptr) {
      CM_LOGD("No necessary to generate [%s] cmo task %s node[%s].",
              cmo_attr.first.c_str(), GetPosition(pre_cmo_task).c_str(), node.GetName().c_str());
      continue;
    }
    status = cmo_task_ptr->GenerateTask(task_defs, stream_id, cmo_attr.second);
    if (status != SUCCESS) {
      CM_LOGW("Generate [%s] CMO task for node[%s] failed.", cmo_attr.first.c_str(), node.GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

GenerateCMOTaskBasePtr CMOTaskBuilder::GetCMOTaskType(const ge::Node &node, TaskBuilderContext &context,
                                                      const std::string &task_type, const bool &pre_cmo_task) {
  if (pre_cmo_task) {
    if (task_type == kCmoPrefetch) {
      CM_MAKE_SHARED(generate_cmo_prefetch_task_ptr_ = std::make_shared<GenerateCMOPrefetchTask>(node, context),
                     return nullptr);
      return generate_cmo_prefetch_task_ptr_;
    } else if (task_type == kCmoBarrier) {
      CM_MAKE_SHARED(generate_cmo_barrier_task_ptr_ = std::make_shared<GenerateCMOBarrierTask>(node, context),
                     return nullptr);
      return generate_cmo_barrier_task_ptr_;
    }
  } else {
    if (task_type == kCmoInvalid) {
      CM_MAKE_SHARED(generate_cmo_invalid_task_ptr_ = std::make_shared<GenerateCMOInvalidTask>(node, context),
                     return nullptr);
      return generate_cmo_invalid_task_ptr_;
    }  else if (task_type == kCmoWriteback) {
      CM_MAKE_SHARED(generate_cmo_writeback_task_ptr_ = std::make_shared<GenerateCMOWritebackTask>(node, context),
                     return nullptr);
      return generate_cmo_writeback_task_ptr_;
    }
  }
  return nullptr;
}
std::string CMOTaskBuilder::GetPosition(const bool &pre_task) const {
  if (pre_task) {
    return "before";
  } else {
    return "after";
  }
}
}
