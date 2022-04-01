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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_QUERIER_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_QUERIER_H_

#include "format_selector/manager/format_dtype_manager_base.h"

namespace fe {
class FormatDtypeQuerier : public FormatDtypeManagerBase {
 public:
  explicit FormatDtypeQuerier(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  ~FormatDtypeQuerier() override;

  /**
   * Get the support formats and dtyps from the op_desc, if failed, get the
   * dynamic formats and dtyps. Used for CheckSubStoreSupported.
   * @param op_kernel_info_ptr op kernel info
   * @param op_desc op desc
   * @param format_map formats result
   * @param data_type_map dtypes result
   * @return SUCCESS or FAILED
   */
  Status GetSupportFormatAndDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                  const ge::OpDesc &op_desc,
                                  const bool &is_dynamic_check,
                                  map<string, vector<ge::Format>> &format_map,
                                  map<string, vector<ge::DataType>> &data_type_map) const;

  /**
   * Get the support formats from the op_desc by the input_or_output_info.
   * Used for OpFormatDtypeJudge and HeavyFormatDistribution.
   * @param input_or_output_info_ptr input_or_output_info
   * @param op_desc op desc
   * @param result formats
   * @return SUCCESS or FAILED
   */
  Status GetSupportFormats(const OpKernelInfoPtr &op_kernel_info_ptr,
                           const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDesc &op_desc,
                           vector<ge::Format> &result) const;

  /**
   * Get the support dtypes from the op_desc by the input_or_output_info.
   * Used for OpFormatDtypeJudge and HeavyFormatDistribution.
   * @param input_or_output_info_ptr input_or_output_info
   * @param op_desc op desc
   * @param result d_types
   * @return SUCCESS or FAILED
   */
  Status GetSupportDataTypes(const OpKernelInfoPtr &op_kernel_info_ptr,
                             const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDesc &op_desc,
                             vector<ge::DataType> &result) const;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_QUERIER_H_
