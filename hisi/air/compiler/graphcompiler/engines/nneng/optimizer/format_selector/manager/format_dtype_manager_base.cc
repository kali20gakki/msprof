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

#include "format_selector/manager/format_dtype_manager_base.h"
#include "common/unknown_shape_util.h"

namespace fe {
FormatDtypeManagerBase::FormatDtypeManagerBase(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr) {
  FE_MAKE_SHARED(format_dtype_op_kernel_selector_ptr_ =
      std::make_shared<FormatDtypeOpKernelSelector>(op_store_adapter_manager_ptr), return);
  FE_MAKE_SHARED(format_dtype_op_customize_selector_ptr_ =
      std::make_shared<FormatDtypeOpCustomizeSelector>(op_store_adapter_manager_ptr), return);
  FE_MAKE_SHARED(format_dtype_op_builtin_selector_ptr_ =
      std::make_shared<FormatDtypeOpBuiltinSelector>(op_store_adapter_manager_ptr), return);
}

FormatDtypeManagerBase::~FormatDtypeManagerBase() {}

FormatDtypeSelectorBasePtr FormatDtypeManagerBase::GetSelector(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                               const bool &is_dynamic_check) const {
  if (IsOpKernelEnable(op_kernel_info_ptr, is_dynamic_check)) {
    return format_dtype_op_kernel_selector_ptr_;
  }
  OpPattern pattern = op_kernel_info_ptr->GetOpPattern();
  if (pattern == OP_PATTERN_OP_CUSTOMIZE) {
    return format_dtype_op_customize_selector_ptr_;
  } else if (pattern == OP_PATTERN_OP_KERNEL || pattern == OP_PATTERN_FORMAT_AGNOSTIC) {
    return format_dtype_op_kernel_selector_ptr_;
  }
  return format_dtype_op_builtin_selector_ptr_;
}

bool FormatDtypeManagerBase::IsOpKernelEnable(const OpKernelInfoPtr &op_kernel_info_ptr,
                                              const bool &is_dynamic_check) const {
  for (auto input_info_ptr : op_kernel_info_ptr->GetAllInputInfo()) {
    if (is_dynamic_check && !input_info_ptr->GetUnknownShapeFormat().empty()) {
      return true;
    }
    if (!is_dynamic_check && !input_info_ptr->GetFormat().empty()) {
      return true;
    }
  }
  return false;
}

bool FormatDtypeManagerBase::IsSelectFormat(const OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_dynamic_check) {
  return GetSelector(op_kernel_info_ptr, is_dynamic_check) == format_dtype_op_customize_selector_ptr_;
}

bool FormatDtypeManagerBase::IsOpPatternBroadcast(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                  const bool &is_dynamic_check) {
  OpPattern pattern = op_kernel_info_ptr->GetOpPattern();
  return GetSelector(op_kernel_info_ptr, is_dynamic_check) == format_dtype_op_builtin_selector_ptr_ &&
         pattern == OP_PATTERN_BROADCAST;
}
}  // namespace fe
