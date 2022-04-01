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

#include "format_selector/manager/format_dtype_querier.h"

namespace fe {
FormatDtypeQuerier::FormatDtypeQuerier(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : FormatDtypeManagerBase(op_store_adapter_manager_ptr) {}

FormatDtypeQuerier::~FormatDtypeQuerier() {}

Status FormatDtypeQuerier::GetSupportFormats(const OpKernelInfoPtr &op_kernel_info_ptr,
                                             const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                             const ge::OpDesc &op_desc, vector<ge::Format> &result) const {
  bool is_dynamic_check = IsOpDynamicImpl(op_desc);
  FormatDtypeSelectorBasePtr selector = GetSelector(op_kernel_info_ptr, is_dynamic_check);
  FE_CHECK_NOTNULL(selector);
  return selector->GetSupportFormats(op_kernel_info_ptr, input_or_output_info_ptr, op_desc, result);
}

Status FormatDtypeQuerier::GetSupportDataTypes(const OpKernelInfoPtr &op_kernel_info_ptr,
                                               const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                               const ge::OpDesc &op_desc, vector<ge::DataType> &result) const {
  bool is_dynamic_check = IsOpDynamicImpl(op_desc);
  FormatDtypeSelectorBasePtr selector = GetSelector(op_kernel_info_ptr, is_dynamic_check);
  FE_CHECK_NOTNULL(selector);
  return selector->GetSupportDataTypes(op_kernel_info_ptr, input_or_output_info_ptr, op_desc, result);
}

Status FormatDtypeQuerier::GetSupportFormatAndDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                    const ge::OpDesc &op_desc,
                                                    const bool &is_dynamic_check,
                                                    map<string, vector<ge::Format>> &format_map,
                                                    map<string, vector<ge::DataType>> &data_type_map) const {
  FormatDtypeSelectorBasePtr selector = GetSelector(op_kernel_info_ptr, is_dynamic_check);
  FE_CHECK_NOTNULL(selector);
  return selector->GetSupportFormatDtype(op_kernel_info_ptr, op_desc, is_dynamic_check, format_map, data_type_map);
}

}  // namespace fe
