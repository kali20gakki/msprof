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

#include "format_selector/op_kernel/format_dtype_op_kernel_selector.h"
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/unknown_shape_util.h"
#include "common/util/json_util.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"

namespace fe {
FormatDtypeOpKernelSelector::FormatDtypeOpKernelSelector(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : FormatDtypeSelectorBase(op_store_adapter_manager_ptr) {}
FormatDtypeOpKernelSelector::~FormatDtypeOpKernelSelector() {}

Status FormatDtypeOpKernelSelector::GetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                          const ge::OpDesc &op_desc,
                                                          const bool &is_dynamic_check,
                                                          std::map<std::string, vector<ge::Format>> &format_map,
                                                          std::map<std::string, vector<ge::DataType>> &data_type_map) {
  for (InputOrOutputInfoPtr input_or_output_info_ptr : op_kernel_info_ptr->GetAllInputInfo()) {
    string unique_name = input_or_output_info_ptr->GetUniqueName();
    if (is_dynamic_check) {
      format_map[unique_name] = input_or_output_info_ptr->GetUnknownShapeFormat();
      data_type_map[unique_name] = input_or_output_info_ptr->GetUnknownShapeDataType();
    } else {
      format_map[unique_name] = input_or_output_info_ptr->GetFormat();
      data_type_map[unique_name] = input_or_output_info_ptr->GetDataType();
    }
  }

  for (InputOrOutputInfoPtr input_or_output_info_ptr : op_kernel_info_ptr->GetAllOutputInfo()) {
    string unique_name = input_or_output_info_ptr->GetUniqueName();
    if (is_dynamic_check) {
      format_map[unique_name] = input_or_output_info_ptr->GetUnknownShapeFormat();
      data_type_map[unique_name] = input_or_output_info_ptr->GetUnknownShapeDataType();
    } else {
      format_map[unique_name] = input_or_output_info_ptr->GetFormat();
      data_type_map[unique_name] = input_or_output_info_ptr->GetDataType();
    }
  }
  return SUCCESS;
}

Status FormatDtypeOpKernelSelector::GetSupportFormats(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                      const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                                      const ge::OpDesc &op_desc, vector<ge::Format> &result) {
  if (IsOpDynamicImpl(op_desc)) {
    result = input_or_output_info_ptr->GetUnknownShapeFormat();
  } else {
    result = input_or_output_info_ptr->GetFormat();
  }
  return SUCCESS;
}

Status FormatDtypeOpKernelSelector::GetSupportDataTypes(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                        const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                                        const ge::OpDesc &op_desc, vector<ge::DataType> &result) {
  if (IsOpDynamicImpl(op_desc)) {
    result = input_or_output_info_ptr->GetUnknownShapeDataType();
  } else {
    result = input_or_output_info_ptr->GetDataType();
  }
  return SUCCESS;
}

Status FormatDtypeOpKernelSelector::GetDynamicFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                          const ge::OpDesc &op_desc,
                                                          const bool &is_dynamic_check,
                                                          const HeavyFormatInfo &heavy_format_info,
                                                          std::map<std::string, vector<ge::Format>> &format_map,
                                                          std::map<std::string, vector<ge::DataType>> &data_type_map) {
  return SUCCESS;
}
}  // namespace fe
