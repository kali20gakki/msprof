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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_H_

#include <vector>
#include <sstream>

#include "common/math/math_util.h"
#include "framework/common/taskdown_common.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/load/model_manager/ts_mem_mall.h"
#include "graph/load/model_manager/task_info/task_info_factory.h"
#include "proto/task.pb.h"
#include "runtime/rt_dfx.h"

namespace ge {
class DavinciModel;
struct MemInfo {
  int64_t memory_size = 0;
  int64_t logic_memory_base = 0;
  uint8_t *memory_base = nullptr;
  uint32_t memory_type = RT_MEMORY_HBM;
  std::string memory_key;

  void *GetMemory(const int64_t offset, const int64_t bytes) const {
    if (bytes <= 0) {
      return nullptr;
    }
    const auto real_offset = offset - logic_memory_base;

    GE_CHK_STATUS_EXEC(CheckInt64AddOverflow(real_offset, bytes), return nullptr,
                       "[Get][Memory] failed,Out of range, total size:%ld, offset:%ld, bytes:%ld.", memory_size,
                       real_offset, bytes);

    if ((real_offset + bytes) <= memory_size) {
      return ValueToPtr(PtrToValue(memory_base) + static_cast<uint64_t>(real_offset));
    }

    REPORT_INNER_ERROR("E19999", "Out of range, total size:%ld, offset:%ld, bytes:%ld.", memory_size, real_offset,
                       bytes);
    GELOGE(OUT_OF_MEMORY, "Out of range, total size:%ld, offset:%ld, bytes:%ld.", memory_size, real_offset, bytes);
    return nullptr;
  }
};

struct RuntimeParam {
  RuntimeParam() {
    ts_mem_mall = MakeUnique<TsMemMall>();
    aicpu_mem_mall = MakeUnique<TsMemMall>(RT_MEMORY_HBM);
  }
  ~RuntimeParam() = default;

  std::string ToString() const {
    std::stringstream ss;
    ss << "session_id:" << session_id << ", stream_num:" << stream_num << ", event_num:" << event_num
       << ", label_num:" << label_num << ", logic_mem_base:" << logic_mem_base
       << ", host_logic_mem_base:" << host_logic_mem_base << ", logic_weight_base:" << logic_weight_base
       << ", logic_var_base:" << logic_var_base << ", memory_size:" << mem_size << ", host_mem_size:" << host_mem_size
       << ", weight_size:" << weight_size << ", var_size:" << var_size << ", zero_copy_size:" << zero_copy_size
       << ", ex_memory_info:";
    for (const auto &it : memory_infos) {
      ss << "[memory_type:" << it.first << ", memory_size:" << it.second.memory_size << "]";
    }
    return ss.str();
  }

  uint64_t mem_size = 0U;
  uint64_t logic_mem_base = 0U;
  uintptr_t mem_base = 0U;
  uint64_t host_mem_size = 0U;
  uint64_t host_logic_mem_base = 0U;
  uintptr_t host_mem_base = 0U;
  uint64_t weight_size = 0U;
  uint64_t logic_weight_base = 0U;
  uintptr_t weight_base = 0U;
  uint64_t var_size = 0U;
  uint64_t logic_var_base = 0U;
  uintptr_t var_base = 0U;
  int64_t zero_copy_size = 0;
  std::map<uint64_t, MemInfo> memory_infos;
  uint32_t batch_num = 0U;
  uint32_t stream_num = 0U;
  uint32_t event_num = 0U;
  uint32_t label_num = 0U;
  uint64_t session_id = 0U;
  uint32_t graph_id = 0U;
  bool is_single_op = false;
  uint32_t root_graph_id = 0U;

  std::unique_ptr<TsMemMall> ts_mem_mall;
  std::unique_ptr<TsMemMall> aicpu_mem_mall;
};

struct FusionOpInfo {
  std::vector<std::string> original_op_names;
  std::string op_name;
  uint32_t op_index = 0U;
  uint32_t task_id = 0U;
  uint32_t stream_id = 0U;
};

class TaskInfo {
 public:
  TaskInfo() : stream_(nullptr) {}

  virtual ~TaskInfo() { stream_ = nullptr; }

  virtual Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) = 0;

  virtual Status Distribute() = 0;

  virtual void PostProcess(const domi::TaskDef &task_def) {
    (void)task_def;
  }

  virtual Status UpdateArgs() { return SUCCESS; }

  virtual Status CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
    (void)task_def;
    (void)davinci_model;
    return SUCCESS;
  }

  virtual Status Release() { return SUCCESS; }

  virtual const ccOpContext *GetCtx() const { return nullptr; }

  virtual uint32_t GetTaskID() const { return 0xFFFFFFFFU; }

  virtual bool CallSaveDumpInfo() const { return false; }

  virtual uint32_t GetStreamId() const { return 0xFFFFFFFFU; }

  virtual uintptr_t GetDumpArgs() const { return 0U; }

  virtual uint32_t GetSktTaskID() const { return 0xFFFFFFFFU; }

  virtual const std::vector<FusionOpInfo> &GetAllFusionOpInfo() const {
    const static std::vector<FusionOpInfo> all_fusion_op_info;
    return all_fusion_op_info;
  }

  virtual uintptr_t GetArgs() const { return 0U; }

 protected:
  TaskInfo(const TaskInfo &) = default;
  TaskInfo &operator=(const TaskInfo &) = default;
  Status SetStream(const uint32_t stream_id, const std::vector<rtStream_t> &stream_list);
  static void SetTaskTag(const char_t *const op_name);

  void *stream_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_H_
