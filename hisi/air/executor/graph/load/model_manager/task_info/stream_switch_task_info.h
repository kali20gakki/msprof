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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_STREAM_SWITCH_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_STREAM_SWITCH_TASK_INFO_H_

#include "graph/load/model_manager/task_info/task_info.h"

namespace ge {
class StreamSwitchTaskInfo : public TaskInfo {
 public:
  StreamSwitchTaskInfo() = default;

  ~StreamSwitchTaskInfo() override {
    input_ptr_ = nullptr;
    value_ptr_ = nullptr;
    true_stream_ = nullptr;
  }

  Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  Status Distribute() override;

  Status CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

 private:
  void SetInputAndValuePtr(const std::vector<void *> &input_data_addrs);

  void *input_ptr_{nullptr};
  rtCondition_t cond_{RT_EQUAL};
  void *value_ptr_{nullptr};
  rtStream_t true_stream_{nullptr};
  uint32_t true_stream_id_{0U};
  rtSwitchDataType_t data_type_{RT_SWITCH_INT32};
  std::vector<int64_t> fixed_addr_offset_;
  uint32_t op_index_{0U};
  DavinciModel *davinci_model_{nullptr};
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_STREAM_SWITCH_TASK_INFO_H_
