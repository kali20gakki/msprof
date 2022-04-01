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

#ifndef GE_COMMON_DUMP_EXCEPTION_DUMPER_H_
#define GE_COMMON_DUMP_EXCEPTION_DUMPER_H_

#include <vector>

#include "graph/op_desc.h"
#include "framework/common/ge_types.h"
#include "graph/load/model_manager/task_info/task_info.h"

namespace ge {
struct ExtraOpInfo {
  std::string node_info;
  std::string tiling_data;
  uint32_t tiling_key = 0U;
  uintptr_t args = 0U;
  std::vector<void *> input_addrs;
  std::vector<void *> output_addrs;
};

class ExceptionDumper {
 public:
  ExceptionDumper() = default;
  ~ExceptionDumper();

  void SaveDumpOpInfo(const OpDescPtr &op, const uint32_t task_id, const uint32_t stream_id,
                      const ExtraOpInfo &extra_op_info);
  void SaveDumpOpInfo(const RuntimeParam &model_param, const OpDescPtr &op, const uint32_t task_id,
                      const uint32_t stream_id);
  Status DumpExceptionInfo(const std::vector<rtExceptionInfo> &exception_infos) const;
  void LogExceptionTvmOpInfo(const OpDescInfo &op_desc_info) const;
  bool GetOpDescInfo(const uint32_t stream_id, const uint32_t task_id, OpDescInfo &op_desc_info) const;
  OpDescInfo *MutableOpDescInfo(const uint32_t task_id, const uint32_t stream_id);

  static Status DumpDevMem(const ge::char_t * const file, const void * const addr, const int64_t size);

  static void Reset(ExtraOpInfo &extra_op_info);
 private:
  void RefreshAddrs(OpDescInfo &op_desc_info) const;
  void SaveOpDescInfo(const OpDescPtr &op, const uint32_t task_id, const uint32_t stream_id,
                      OpDescInfo &op_desc_info) const;
  Status DumpExceptionInput(const OpDescInfo &op_desc_info, const std::string &dump_file) const;
  Status DumpExceptionOutput(const OpDescInfo &op_desc_info, const std::string &dump_file) const;

  std::vector<OpDescInfo> op_desc_info_;
};
}  // namespace ge

#endif // GE_COMMON_DUMP_EXCEPTION_DUMPER_H_
