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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_OP_FORMAT_DTYPE_UPDATE_DESC_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_OP_FORMAT_DTYPE_UPDATE_DESC_BASE_H_
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "graph/compute_graph.h"
#include "ops_store/op_kernel_info.h"

namespace fe {
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;

struct UpdateInfo {
  const OpKernelInfoPtr &op_kernel_info_ptr;             // op kernel info
  const InputOrOutputInfoPtr &input_or_output_info_ptr;  // tensor kernel info
  const uint32_t &matched_index;                         // mathed index of the op_kernel_info
  const ge::NodePtr &node_ptr;                           // current node pointer
  const uint32_t &index;                                 // the index of the input or output desc
  ge::GeTensorDesc &op_input_or_output_desc;             // the input or output desc of the current node
  const bool &is_input;
};

class OpFormatDtypeUpdateDescBase {
 public:
  explicit OpFormatDtypeUpdateDescBase(FormatDtypeQuerierPtr format_dtype_querier_ptr);
  ~OpFormatDtypeUpdateDescBase();

  /**
   * Get all supported data type and check whether the data types are valid
   * @return SUCCESS or FAIL
   */
  Status GetAndCheckSupportedDtype(const UpdateInfo &update_info, ge::DataType &op_kernel_data_type);

  /**
   * update the data type of the input or output desc of the node
   * @return SUCCESS or FAIL
   */
  Status UpdateDtype(const UpdateInfo &update_info);

  /**
   * Get all supported format and data type from the ops kernel
   * @param input_or_output_info_ptr: op kernel info
   * @param op_kernel_info_ptr: the ops kernel information
   * @param op_desc_ptr: current node's opdesc pointer
   * @param op_kernel_format_vec: output parameter of all supported formats
   * @param op_kernel_data_type_vec: output parameter of all supported dtypes
   * @return SUCCESS or FAIL
   */
  Status GetFormatAndDtypeVec(const OpKernelInfoPtr &op_kernel_info_ptr,
                              const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDescPtr &op_desc_ptr,
                              std::vector<ge::Format> &op_kernel_format_vec,
                              std::vector<ge::DataType> &op_kernel_data_type_vec);

  Status PadNDToOtherFormatAndGetReshapeType(const UpdateInfo &update_info, const ge::Format &op_kernel_format,
                                             ge::Format &ori_format, std::string &reshape_type);

  Status UpdateNewShape(const UpdateInfo& update_info, ge::Format op_kernel_format,
                      ge::DataType op_kernel_dtype, int64_t group, int64_t op_imply_type_input);

  /**
   * Calculate new shape and update format, dtype, shape of specifc tensor
   * @param op_kernel_format_vec: all supported format of this tensor
   * @param op_kernel_data_type_vec: all supported data type of this tensor
   * @return SUCCESS or FAIL
   */
  Status CalcNewShapeAndUpdate(const UpdateInfo &update_info, ge::Format op_kernel_format,
                               ge::DataType op_kernel_dtype);

  /**
   * update the format and data type of the output desc of the node
   * @return SUCCESS or FAIL
   */
  Status UpdateFormat(const UpdateInfo &update_info);

 protected:
  /**
   * get the new shape
   * @param old_shape old shape
   * @param old_format old format
   * @param new_format new format
   * @param op_imply_type the imply type of the op
   * @param current_data_type current data type
   * @param group group for Fractal_Z_G format
   * @return new shape
   */
  ge::GeShape GetNewShape(const ge::OpDescPtr &op_desc_ptr, const ge::GeShape &old_shape, const ge::Format &old_format,
                          const ge::Format &new_format, const int64_t &op_imply_type,
                          const ge::DataType &current_data_type, const int64_t &group) const;
  FormatDtypeQuerierPtr format_dtype_querier_ptr_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_OP_FORMAT_DTYPE_UPDATE_DESC_BASE_H_
