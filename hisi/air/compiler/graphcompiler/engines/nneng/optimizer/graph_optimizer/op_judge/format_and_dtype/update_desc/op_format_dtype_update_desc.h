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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_OP_FORMAT_DTYPE_UPDATE_DESC_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_OP_FORMAT_DTYPE_UPDATE_DESC_H_

#include "graph_optimizer/op_judge/format_and_dtype/update_desc/op_format_dtype_update_desc_base.h"
#include "ops_store/op_kernel_info.h"

namespace fe {

using OpFormatDtypeUpdateTensorDescBasePtr = std::shared_ptr<OpFormatDtypeUpdateDescBase>;

class OpFormatDtypeUpdateDesc {
 public:
  explicit OpFormatDtypeUpdateDesc(FormatDtypeQuerierPtr format_dtype_querier_ptr);

  virtual ~OpFormatDtypeUpdateDesc();

  Status Initialize();
  /**
   * update the format and data type of the input desc of the node
   * @param op_kernel_info_ptr op kernel information
   * @param matched_index: matched index of the op kernel information
   * @param tensor_index_name_map: the map of tensor index and its name
   * in the op kernel
   * @param node_ptr: current node, we will update it's tensor desc according
   * to the op kernel
   * @return SUCCESS or FAIL
   */
  Status UpdateTensorDescInfo(const OpKernelInfoPtr& op_kernel_info_ptr, const uint32_t& matched_index,
                              const IndexNameMap& tensor_index_name_map, const bool& is_input, ge::NodePtr& node_ptr);

 private:
  OpFormatDtypeUpdateTensorDescBasePtr op_format_dtype_update_tensor_desc_ptr_;

  FormatDtypeQuerierPtr format_dtype_querier_ptr_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_OP_FORMAT_DTYPE_UPDATE_DESC_H_
