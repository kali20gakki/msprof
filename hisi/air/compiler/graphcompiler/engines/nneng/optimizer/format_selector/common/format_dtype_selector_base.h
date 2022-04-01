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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_COMMON_FORMAT_DTYPE_SELECTOR_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_COMMON_FORMAT_DTYPE_SELECTOR_BASE_H_

#include <map>
#include <string>
#include <vector>
#include "adapter/common/op_store_adapter_manager.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "common/string_utils.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/type_utils.h"
#include "ops_store/op_kernel_info.h"

namespace fe {
class FormatDtypeSelectorBase {
 public:
  explicit FormatDtypeSelectorBase(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);
  virtual ~FormatDtypeSelectorBase();

  /**
   * Get the support formats and dtyps from the op_desc, if failed, get the
   * dynamic formats and dtyps. Used for CheckSubStoreSupported.
   * @param op_kernel_info_ptr op kernel info
   * @param op_desc op desc
   * @param format_map formats result
   * @param data_type_map dtypes result
   * @return SUCCESS or FAILED
   */
  virtual Status GetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                       const ge::OpDesc &op_desc,
                                       const bool &is_dynamic_check,
                                       map<string, vector<ge::Format>> &format_map,
                                       map<string, vector<ge::DataType>> &data_type_map) = 0;

  /**
   * Get the support formats from the op_desc by the input_or_output_info.
   * Used for OpFormatDtypeJudge and HeavyFormatDistribution.
   * @param input_or_output_info_ptr input_or_output_info
   * @param op_desc op desc
   * @param result formats
   * @return SUCCESS or FAILED
   */
  virtual Status GetSupportFormats(const OpKernelInfoPtr &op_kernel_info_ptr,
                                   const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDesc &op_desc,
                                   vector<ge::Format> &result) = 0;

  /**
   * Get the support dtypes from the op_desc by the input_or_output_info.
   * Used for OpFormatDtypeJudge and HeavyFormatDistribution.
   * @param input_or_output_info_ptr input_or_output_info
   * @param op_desc op desc
   * @param result d_types
   * @return SUCCESS or FAILED
   */
  virtual Status GetSupportDataTypes(const OpKernelInfoPtr &op_kernel_info_ptr,
                                     const InputOrOutputInfoPtr &input_or_output_info_ptr, const ge::OpDesc &op_desc,
                                     vector<ge::DataType> &result) = 0;

  /**
   * Get the dynmaic formats and dtypes by infering or caliing the
   * SelectTbeOpFormat function of TeFusion.
   * @param op_kernel_info_ptr op kernel info
   * @param op_desc op desc
   * @param format_map formats result
   * @param data_type_map dtypes result
   * @return SUCCESS or FAILED
   */
  virtual Status GetDynamicFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                       const ge::OpDesc &op_desc,
                                       const bool &is_dynamic_check,
                                       const HeavyFormatInfo &heavy_format_info,
                                       std::map<std::string, vector<ge::Format>> &format_map,
                                       std::map<std::string, vector<ge::DataType>> &data_type_map) = 0;

  /**
   * Set the support formats and dtypes for the op_desc.
   * @param op_kernel_info_ptr op kernel info
   * @param op_desc op desc
   * @return SUCCESS or FAILED
   */
  Status SetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr, const HeavyFormatInfo &heavy_format_info,
                               ge::OpDesc &op_desc, const bool &is_dynamic_check);

 protected:
  Status GetAllFormatsFromOpDesc(const ge::OpDesc &op_desc, map<string, vector<ge::Format>> &result);
  Status GetAllDataTypesFromOpDesc(const ge::OpDesc &op_desc, map<string, vector<ge::DataType>> &result);
  Status GetFormatFromOpDescByKey(const ge::OpDesc &op_desc, const string &key, vector<ge::Format> &result);
  Status GetDataTypeFromOpDescByKey(const ge::OpDesc &op_desc, const string &key, vector<ge::DataType> &result);
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;

 private:
  /**
   * Save the format_map and data_type_map to the ext attribute of the op_desc.
   * @param format_map format map
   * @param data_type_map data_type map
   * @param op_desc  op desc
   * @return SUCCESS or FAILED
   */
  Status SaveDynamicFormatDtype(const std::map<std::string, vector<ge::Format>> &format_map,
                                const std::map<std::string, vector<ge::DataType>> &data_type_map, ge::OpDesc &op_desc);

  const string EXT_DYNAMIC_FORMAT = "ext_dynamic_format";
  const string EXT_DYNAMIC_DATATYPE = "ext_dynamic_datatype";
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_COMMON_FORMAT_DTYPE_SELECTOR_BASE_H_
