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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_FORMAT_DTYPE_OP_BUILTIN_SELECTOR_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_FORMAT_DTYPE_OP_BUILTIN_SELECTOR_H_

#include <map>
#include <string>
#include <vector>
#include "adapter/adapter_itf/op_store_adapter.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"
#include "common/string_utils.h"
#include "format_selector/builtin/process/format_process_registry.h"
#include "format_selector/common/format_dtype_selector_base.h"
#include "graph/compute_graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/type_utils.h"

namespace fe {
using IndexNameMap = map<uint32_t, string>;

class FormatDtypeOpBuiltinSelector : public FormatDtypeSelectorBase {
 public:
  explicit FormatDtypeOpBuiltinSelector(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  ~FormatDtypeOpBuiltinSelector() override;

  Status GetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                               const ge::OpDesc &op_desc,
                               const bool &is_dynamic_check,
                               map<string, vector<ge::Format>> &format_res,
                               map<string, vector<ge::DataType>> &data_type_res) override;

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
                               map<string, vector<ge::Format>> &format_res,
                               map<string, vector<ge::DataType>> &data_type_res) override;

 private:
  Status GetUnionFormatDtype(const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDesc &op_desc,
                             vector<ge::Format> &new_formats, vector<ge::DataType> &new_data_types);
  Status InitOriginInfo(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                        const IndexNameMap &input_index_map, const IndexNameMap &output_index_map,
                        OriginInfoPtr origin_info_ptr, map<string, vector<ge::Format>> &format_res) const;

  Status UpdateFormatMap(const OpKernelInfoPtr &op_kernel_info_ptr, const FormatProccessResult &format_proccess_res,
                         const IndexNameMap &index_name_map, const IndexNameMap &output_index_map,
                         map<string, vector<ge::Format>> &format_res) const;

  Status UpdateFormatMap(const bool &is_input, const OpKernelInfoPtr &op_kernel_info_ptr,
                         const vector<vector<ge::Format>> &formats, const IndexNameMap &index_map,
                         map<string, vector<ge::Format>> &format_res) const;

  InputOrOutputInfoPtr GetInputOrOutputUniqueName(const bool &is_input, const IndexNameMap &index_name_map,
                                                  const size_t &index, const OpKernelInfoPtr &op_kernel_info_ptr) const;

  Status GetReduceNewFormatDType(const OpKernelInfoPtr &op_kernel_info_ptr, const ge::OpDesc &op_desc,
                                 const string input_key, const string output_key,
                                 map<string, vector<ge::Format>> &format_res,
                                 map<string, vector<ge::DataType>> &data_type_res) const;

  /* if reduce size greater than ub block size, can not support the format */
  bool CheckUBSizeEnable(const ge::OpDesc &op_desc, const ge::Format &check_format, const ge::DataType &check_dtype) const;

  Status GetReduceNewFormatDTypeVec(const OpKernelInfoPtr &op_kernel_info_ptr, const ge::OpDesc &op_desc,
                                    const string &input_or_out_key, vector<ge::DataType> &data_types,
                                    vector<ge::Format> &formats);
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_FORMAT_DTYPE_OP_BUILTIN_SELECTOR_H_
