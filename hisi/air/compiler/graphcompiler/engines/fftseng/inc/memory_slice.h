/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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
#ifndef FFTS_ENGINE_TASK_BUILDER_MODE_MEMORY_SLICE_H_
#define FFTS_ENGINE_TASK_BUILDER_MODE_MEMORY_SLICE_H_
#include "common/sgt_slice_type.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_log.h"
#include "graph/compute_graph.h"
#include "nneng/inc/common/string_utils.h"

namespace ffts {
struct DataContextParam {
  int64_t num_inner;
  int64_t len_inner;
  int64_t stride_inner;
  int64_t num_outter;
  int64_t stride_outter;
  int64_t base_addr_offset;
  uint32_t index;
};

const int kBlockDim = 3;
struct Block {
  int64_t dim[kBlockDim];
  int64_t dim_stride[kBlockDim];
  int64_t offset;
  int64_t count;
  int64_t stride;
};

class MemorySlice {
 public:
  MemorySlice();
  ~MemorySlice();

  static Status GenerateManualDataCtxParam(const ge::NodePtr &node, int index, bool is_input, int64_t burst_len,
                                           std::vector<DataContextParam> &data_ctx);

  static Status GenerateAutoDataCtxParam(const ge::NodePtr &node, int index, bool is_input, int64_t burst_len,
                                         std::vector<DataContextParam> &param_nontail_tail);

  static Status GenerateDataCtxParam(const std::vector<int64_t> &shape, const std::vector<DimRange> &slice,
                                     ge::DataType dtype, int64_t burst_len, std::vector<DataContextParam> &data_ctx);

 private:
  MemorySlice(const MemorySlice &builder) = delete;
  MemorySlice &operator=(const MemorySlice &builder) = delete;
};
Status GetPeerNodeAndOutIdx(const ge::NodePtr &ori_node, int ori_index,
                            ge::NodePtr &peer_node, int &peer_index);
}  // namespace ffts
#endif // FFTS_ENGINE_TASK_BUILDER_MODE_MEMORY_SLICE_H_
