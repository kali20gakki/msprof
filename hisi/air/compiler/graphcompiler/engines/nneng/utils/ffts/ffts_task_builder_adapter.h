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

#ifndef FUSION_ENGINE_UTILS_FFTS_FFTS_TASK_BUILDER_ADAPTER_H_
#define FUSION_ENGINE_UTILS_FFTS_FFTS_TASK_BUILDER_ADAPTER_H_

#include <string>
#include <mutex>
#include <map>
#include <string>
#include <vector>
#include "common/sgt_slice_type.h"
#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
#include "graph/op_kernel_bin.h"

namespace fe {

struct ThreadParamOffset {
  size_t thread_dim;
  vector<vector<void *>> thread_input_addrs;
  vector<vector<void *>> thread_output_addrs;
  vector<void *> first_thread_input_addrs;
  vector<void *> first_thread_output_addrs;
  vector<vector<void *>> thread_workspace_addrs;
  vector<vector<uint32_t>> thread_workspace_sizes;
  vector<uint64_t> thread_addr_offset;
  vector<int64_t> input_tensor_sizes;
  vector<int64_t> output_tensor_sizes;
};

class FftsTaskBuilderAdapter : public TaskBuilderAdapter {
 public:
  FftsTaskBuilderAdapter(const ge::Node &node, TaskBuilderContext &context);
  ~FftsTaskBuilderAdapter() override;

  /*
   * @ingroup fe
   * @brief   Init TaskBuilderAdapter
   * @return  SUCCESS or FAILED
   */
  Status Init() override;
  void GetThreadParamOffset(ThreadParamOffset &param_offset) const;
  Status CheckInputAndOutputSize();

 protected:
  Status InitInput() override;
  Status InitOutput() override;
  Status InitWorkspace() override;
  Status ThreadInit();
  Status ThreadInitInput(vector<vector<vector<ffts::DimRange>>> &tensor_slice);
  Status ThreadInitOutput(vector<vector<vector<ffts::DimRange>>> &tensor_slice);
  Status RegTbeInfo();
  Status RegisterBinary(void * &bin_handle, std::string& kernel_name);
  void DebugThreadArgs() const;

 private:
  std::string stub_func_;
  ge::OpKernelBinPtr kernel_bin_;
  std::string kernel_name_;
  uint32_t block_dim_;
  ge::Buffer tbe_kernel_buffer_;
  uint32_t tbe_kernel_size_;

  size_t thread_dim_;
  vector<vector<void *>> thread_input_addrs_;
  vector<vector<void *>> thread_output_addrs_;
  vector<void *> first_thread_input_addrs_;
  vector<void *> first_thread_output_addrs_;
  // Workspace
  vector<vector<void *>> thread_workspace_addrs_;
  vector<vector<uint32_t>> thread_workspace_sizes_;

  vector<uint64_t> thread_addr_offset_;
  vector<int64_t> input_tensor_sizes_;
  vector<int64_t> output_tensor_sizes_;

  Status HandleAnchorData(size_t &input_index,
                          size_t &anchor_index, size_t &weight_index);
  void SetInputAddrFromDataBase(int64_t &input_offset);
  Status VerifyWeights();
};

}  // namespace fe

#endif  // FUSION_ENGINE_UTILS_FFTS_FFTS_TASK_BUILDER_ADAPTER_H_