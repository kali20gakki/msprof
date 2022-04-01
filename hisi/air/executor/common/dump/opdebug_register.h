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

#ifndef GE_COMMON_DUMP_OPDEBUG_REGISTER_H_
#define GE_COMMON_DUMP_OPDEBUG_REGISTER_H_

#include <map>
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "common/dump/data_dumper.h"

namespace ge {
class OpDebugTask {
 public:
  OpDebugTask() = default;
  ~OpDebugTask();

 private:
  uint32_t debug_stream_id_ = 0U;
  uint32_t debug_task_id_ = 0U;
  void *op_debug_addr_ = nullptr;
  friend class OpdebugRegister;
};

class OpdebugRegister {
 public:
  OpdebugRegister() = default;
  ~OpdebugRegister();

  Status RegisterDebugForModel(rtModel_t const model_handle, const uint32_t op_debug_mode, DataDumper &data_dumper);
  void UnregisterDebugForModel(rtModel_t const model_handle);

  Status RegisterDebugForStream(rtStream_t const stream, const uint32_t op_debug_mode, DataDumper &data_dumper);
  void UnregisterDebugForStream(rtStream_t const stream);

 private:
  Status MallocMemForOpdebug();
  Status CreateOpDebugTaskByStream(rtStream_t const stream, const uint32_t op_debug_mode);
  Status MallocP2PDebugMem(void * const op_debug_addr);

  void *op_debug_addr_ = nullptr;
  void *p2p_debug_addr_ = nullptr;
  static std::mutex mu_;
  static std::map<rtStream_t, std::unique_ptr<OpDebugTask>> op_debug_tasks_;
  static std::map<rtStream_t, uint32_t> stream_ref_count_;
};
}  // namespace ge
#endif  // GE_COMMON_DUMP_OPDEBUG_REGISTER_H_
