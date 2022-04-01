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

#ifndef AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_GENERATE_CMO_INVALID_TASK_H_
#define AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_GENERATE_CMO_INVALID_TASK_H_
#include "generate_cmo_task_base.h"

namespace fe {
class GenerateCMOInvalidTask : public GenerateCMOTaskBase {
 public:
  GenerateCMOInvalidTask(const ge::Node &node, TaskBuilderContext &context);
  ~GenerateCMOInvalidTask() override;

  Status GenerateTask(std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                      const std::vector<CmoAttr> &cmo_attrs) override;
  Status ParseTensorInfo(const CmoAttr &cmo_attr, const TaskArgs &task_args, ge::DataType &data_type,
                         int64_t &complex_cmo_id, uint64_t &source_addr, uint32_t &length_inner) const;
};
}  // namespace fe
#endif  // AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_GENERATE_CMO_INVALID_TASK_H_
