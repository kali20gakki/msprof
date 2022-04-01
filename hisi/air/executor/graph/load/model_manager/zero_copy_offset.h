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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_ZERO_COPY_OFFSET_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_ZERO_COPY_OFFSET_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "graph/load/model_manager/zero_copy_task.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "external/ge/ge_api_error_codes.h"
#include "graph/debug/ge_attr_define.h"

namespace ge {
class ZeroCopyOffset {
 public:
  ZeroCopyOffset();
  ~ZeroCopyOffset();

  Status InitInputDataInfo(const int64_t output_size, void *virtual_addr, const OpDescPtr &op_desc, bool &fusion_flag);
  void SetInputOutsideAddrs(const int64_t output_offset, const uintptr_t addr_val, const bool fusion_flag,
                            std::set<const void *> &real_virtual_addrs);

  static void IsL2Fusion(const std::vector<int64_t> &fusion_basic_addrs, const int64_t &tensor_addr, bool &fusion_flag);
  Status InitOutputDataInfo(const std::vector<int64_t> &input_size_list, const std::vector<void *> &virtual_addr_list,
                            const OpDescPtr &op_desc, const size_t &idx, bool &fusion_flag);
  void SetOutputOutsideAddrs(const int64_t input_offset, const bool fusion_flag, const uintptr_t addr_val,
                             std::vector<const void *> &tensor_addrs);
  void SetOutsideAddrsValue(ZeroCopyTask &zero_copy_task, const uintptr_t outside_addr, const uintptr_t args_base,
                            const size_t offset);

  void SetLogicalOutsideAddrs(const uintptr_t logical_addr, const uintptr_t device_addr);

  // basic_addr of l2-fusion
  void *GetBasicAddr() const { return basic_addr_; }
  // total num of out_of_data/in_of_phonyconcat
  size_t GetDataCount() const { return data_count_; }
  size_t GetAddrCount() const { return addr_count_; }
  // value of *data_info_ from davinci_model
  const std::vector<std::pair<int64_t, void *>> &GetDataInfo() const { return data_info_; }
  // relative_offset from zero_copy_relative_offset_
  const std::vector<int64_t> &GetRelativeOffset() const { return relative_offset_; }
  // data_size of Data/Netoutput
  int64_t GetDataSize() const { return data_size_; }
  // value of *outside_addrs_ from davinci_model
  const std::vector<std::map<uintptr_t, std::vector<uintptr_t>>> &GetOutsideAddrs() const { return outside_addrs_; }
  // name of op
  const std::string &GetOpName() const { return op_name_; }
  bool IsRelativeOffsetValid() const { return valid_relative_offset_; }

 private:
  void *basic_addr_ = nullptr;
  std::string op_name_;
  size_t data_count_ = 0U;
  std::vector<std::pair<int64_t, void *>> data_info_;
  std::vector<int64_t> relative_offset_;
  int64_t data_size_ = 0;
  size_t addr_count_ = 0U;
  std::vector<std::map<uintptr_t, std::vector<uintptr_t>>> outside_addrs_;

  std::vector<int64_t> zero_copy_basic_offset_;
  std::vector<int64_t> zero_copy_relative_offset_;
  bool valid_relative_offset_ = false;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_ZERO_COPY_OFFSET_H_
