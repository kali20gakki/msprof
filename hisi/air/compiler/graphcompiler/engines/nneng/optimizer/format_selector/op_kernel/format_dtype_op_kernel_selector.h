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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_OP_KERNEL_FORMAT_DTYPE_OP_KERNEL_SELECTOR_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_OP_KERNEL_FORMAT_DTYPE_OP_KERNEL_SELECTOR_H_

#include "adapter/adapter_itf/op_store_adapter.h"
#include "format_selector/common/format_dtype_selector_base.h"

namespace fe {
class FormatDtypeOpKernelSelector : public FormatDtypeSelectorBase {
 public:
  explicit FormatDtypeOpKernelSelector(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  ~FormatDtypeOpKernelSelector() override;

  Status GetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                               const ge::OpDesc &op_desc,
                               const bool &is_dynamic_check,
                               map<string, vector<ge::Format>> &format_map,
                               map<string, vector<ge::DataType>> &data_type_map) override;

  Status GetSupportFormats(const OpKernelInfoPtr &op_kernel_info_ptr,
                           const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDesc &op_desc,
                           vector<ge::Format> &result) override;
  Status GetSupportDataTypes(const OpKernelInfoPtr &op_kernel_info_ptr,
                             const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDesc &op_desc,
                             vector<ge::DataType> &result) override;

  Status GetDynamicFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                               const ge::OpDesc &op_desc,
                               const bool &is_dynamic_check,
                               const HeavyFormatInfo &heavy_format_info,
                               std::map<std::string, vector<ge::Format>> &format_map,
                               std::map<std::string, vector<ge::DataType>> &data_type_map) override;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_OP_KERNEL_FORMAT_DTYPE_OP_KERNEL_SELECTOR_H_
