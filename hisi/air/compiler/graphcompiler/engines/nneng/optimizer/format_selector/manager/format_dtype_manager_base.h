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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_MANAGER_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_MANAGER_BASE_H_

#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "common/string_utils.h"
#include "format_selector/builtin/format_dtype_op_builtin_selector.h"
#include "format_selector/op_customize/format_dtype_op_customize_selector.h"
#include "format_selector/op_kernel/format_dtype_op_kernel_selector.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"

namespace fe {

using FormatDtypeSelectorBasePtr = std::shared_ptr<FormatDtypeSelectorBase>;
using FormatDtypeOpKernelSelectorPtr = std::shared_ptr<FormatDtypeOpKernelSelector>;
using FormatDtypeOpCustomizeSelectorPtr = std::shared_ptr<FormatDtypeOpCustomizeSelector>;
using FormatDtypeOpBuiltinSelectorPtr = std::shared_ptr<FormatDtypeOpBuiltinSelector>;

class FormatDtypeManagerBase {
 public:
  explicit FormatDtypeManagerBase(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  virtual ~FormatDtypeManagerBase();
  FormatDtypeSelectorBasePtr GetSelector(const OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_dynamic_check) const;
  bool IsSelectFormat(const OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_dynamic_check);
  bool IsOpPatternBroadcast(const OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_dynamic_check);
 private:
  FormatDtypeOpKernelSelectorPtr format_dtype_op_kernel_selector_ptr_;
  FormatDtypeOpCustomizeSelectorPtr format_dtype_op_customize_selector_ptr_;
  FormatDtypeOpBuiltinSelectorPtr format_dtype_op_builtin_selector_ptr_;
  /* To check weather format or unknownshape_format is set in op kernel info
   * file, if it is set, then use op kernel info, else, use it by op.pattern
   * value. */
  bool IsOpKernelEnable(const OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_dynamic_check) const;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_MANAGER_FORMAT_DTYPE_MANAGER_BASE_H_
