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

#ifndef AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_GENERATE_CMO_TASK_BASE_H_
#define AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_GENERATE_CMO_TASK_BASE_H_

#include "common/cmo_id_gen_strategy.h"
#include "common/aicore_util_attr_define.h"
#include "common/aicore_util_types.h"
#include "external/graph/types.h"
#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
#include "runtime/rt_model.h"
#include "graph/utils/tensor_utils.h"
#include "proto/task.pb.h"

namespace fe {
enum class rtCMOType {
  rtCMOBarrier = 0,
  rtCMOInvalid = 1,
  rtCMOPrefetch = 2,
  rtCMOWriteBack = 3,
};

const std::map<ge::DataType, uint8_t> DATA_TYPE_CODE = {
        {ge::DT_INT8, 0x0},
        {ge::DT_INT16, 0x1},
        {ge::DT_INT32, 0x2},
        {ge::DT_FLOAT16, 0x6},
        {ge::DT_FLOAT, 0x7},
};

class GenerateCMOTaskBase {
 public:
  GenerateCMOTaskBase(const ge::Node &node, TaskBuilderContext &context);
  virtual ~GenerateCMOTaskBase();

  virtual Status GenerateTask(std::vector<domi::TaskDef> &task_defs, const int32_t &stream_id,
                              const std::vector<CmoAttr> &cmo_attrs);
  Status ParseTensorInfo(const CmoAttr &cmo_attr, TaskArgs &task_args, ge::DataType &data_type,
                         uint64_t &source_addr, uint32_t &length_inner) const;
  Status InitCmoNodeAddrs(const ge::NodePtr &node, TaskArgs &task_args);

 protected:
  const ge::Node &node_;
  TaskBuilderContext context_;
};
}  // namespace fe
#endif  // AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_UTILS_CMO_TASK_BUILDER_CMO_TASK_GENERATE_CMO_TASK_BASE_H_
