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

#include "format_selector/builtin/format_dtype_op_builtin_selector.h"
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "common/configuration.h"
#include "common/fe_log.h"
#include "common/fe_type_utils.h"
#include "common/fe_utils.h"
#include "common/format/axis_name_util.h"
#include "common/op_info_common.h"
#include "common/util/json_util.h"
#include "common/util/platform_info.h"
#include "graph/detail/attributes_holder.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {
FormatDtypeOpBuiltinSelector::FormatDtypeOpBuiltinSelector(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : FormatDtypeSelectorBase(op_store_adapter_manager_ptr) {}

FormatDtypeOpBuiltinSelector::~FormatDtypeOpBuiltinSelector() {}

Status GetFormatDtypeForAllTensors(const vector<InputOrOutputInfoPtr> &all_tensor_info,
                                   map<string, vector<ge::Format>> &format_map, const ge::OpDesc &op_desc,
                                   map<string, vector<ge::Format>> &format_res,
                                   map<string, vector<ge::DataType>> &data_type_res, string &key) {
  for (const auto &input_or_output : all_tensor_info) {
    vector<ge::DataType> old_data_types = input_or_output->GetDataType();
    string unique_name = input_or_output->GetUniqueName();
    key = unique_name;
    if (format_map.find(unique_name) == format_map.end()) {
      REPORT_FE_ERROR(
          "[GraphOpt][FmtJdg][GetFmtDtype] Op[name=%s,type=%s]: fail to find the key[%s] from the formatMap.",
          op_desc.GetName().c_str(), op_desc.GetType().c_str(), unique_name.c_str());
      return FAILED;
    }

    vector<ge::Format> old_formats = format_map[unique_name];
    vector<ge::Format> new_formats;
    vector<ge::DataType> new_data_types;
    if (GenerateUnionFormatDtype(old_formats, old_data_types, new_formats, new_data_types) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][GenFmtUnion][Op%s,type=%s] failed to GenerateUnionFormatDtype",
                      op_desc.GetName().c_str(), op_desc.GetType().c_str());
      return FAILED;
    }
    format_res[input_or_output->GetUniqueName()] = new_formats;
    data_type_res[input_or_output->GetUniqueName()] = new_data_types;
  }
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::GetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                           const ge::OpDesc &op_desc,
                                                           const bool &is_dynamic_check,
                                                           map<string, vector<ge::Format>> &format_res,
                                                           map<string, vector<ge::DataType>> &data_type_res) {
  // 1. get the old_format from op_desc. if failed, get GetDynamicFormatAndDtype
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  FE_LOGD("Op[name=%s,type=%s]: start to GetSupportFormatDtype.", op_name.c_str(), op_type.c_str());
  map<string, vector<ge::Format>> format_map;
  map<string, vector<ge::DataType>> data_type_map;
  Status status = GetAllFormatsFromOpDesc(op_desc, format_map);
  if (status != SUCCESS) {
    HeavyFormatInfo heavy_format_info;
    if (GetDynamicFormatDtype(op_kernel_info_ptr, op_desc, is_dynamic_check, heavy_format_info, format_map,
                              data_type_map) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FmtJdg][GetSptedFmt] Op[name=%s,type=%s]: fail to GetDynamicFormatAndDtype.",
                      op_name.c_str(), op_type.c_str());
      return FAILED;
    }
  }

  // 2. get the data_types from op_kernel, union format and data_type
  string input_key;
  Status ret = GetFormatDtypeForAllTensors(op_kernel_info_ptr->GetAllInputInfo(), format_map, op_desc, format_res,
                                           data_type_res, input_key);
  if (ret != SUCCESS) {
    return ret;
  }

  string output_key;
  ret = GetFormatDtypeForAllTensors(op_kernel_info_ptr->GetAllOutputInfo(), format_map, op_desc, format_res,
                                    data_type_res, output_key);
  if (ret != SUCCESS) {
    return ret;
  }

  if (GetReduceNewFormatDType(op_kernel_info_ptr, op_desc, input_key, output_key, format_res, data_type_res) !=
      SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetSptedFmt] Op[name=%s,type=%s]: fail to GetReduceNewFormatDType.",
                    op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  LogFormatMap(format_res);
  LogDataTypeMap(data_type_res);
  FE_LOGD("Op[name=%s,type=%s]: end to GetSupportFormatDtype.", op_name.c_str(), op_type.c_str());
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::GetSupportFormats(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                       const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                                       const ge::OpDesc &op_desc, vector<ge::Format> &result) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  vector<ge::DataType> new_data_types;
  GetUnionFormatDtype(input_or_output_info_ptr, op_desc, result, new_data_types);

  string unique_name = input_or_output_info_ptr->GetUniqueName();
  if (GetReduceNewFormatDTypeVec(op_kernel_info_ptr, op_desc, unique_name, new_data_types, result) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FmtJdg][GetSptFmt] Op[name=%s,type=%s]: Fail to GetReduceNewFormatDTypeVec.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }
  FE_LOGD("Op[name=%s,type=%s]: support format[%s] for the %s.", op_name.c_str(), op_type.c_str(),
          GetStrByFormatVec(result).c_str(), unique_name.c_str());
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::GetSupportDataTypes(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                         const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                                         const ge::OpDesc &op_desc, vector<ge::DataType> &result) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  vector<ge::Format> new_formats;
  GetUnionFormatDtype(input_or_output_info_ptr, op_desc, new_formats, result);

  string unique_name = input_or_output_info_ptr->GetUniqueName();
  if (GetReduceNewFormatDTypeVec(op_kernel_info_ptr, op_desc, unique_name, result, new_formats) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FmtJdg][GetSptType] Op[name=%s,type=%s]: Fail to GetReduceNewFormatDTypeVec.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  FE_LOGD("Op[name=%s,type=%s]: support data_types[%s] for the %s.", op_name.c_str(), op_type.c_str(),
          GetStrByDataTypeVec(result).c_str(), unique_name.c_str());
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::GetUnionFormatDtype(const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                                         const ge::OpDesc &op_desc, vector<ge::Format> &new_formats,
                                                         vector<ge::DataType> &new_data_types) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  string unique_name = input_or_output_info_ptr->GetUniqueName();

  // 1. get the formats form op_desc
  vector<ge::Format> old_formats;
  if (GetFormatFromOpDescByKey(op_desc, unique_name, old_formats) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FmtJdg][GetUnionFmt][Op %s,type=%s]: fail to get formats by key %s.", op_name.c_str(),
                    op_type.c_str(), unique_name.c_str());
    return FAILED;
  }

  // 2. union format and data_type, return formats
  if (GenerateUnionFormatDtype(old_formats, input_or_output_info_ptr->GetDataType(), new_formats, new_data_types) !=
      SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FmtJdg][GetUnionFmt][Op %s,type=%s]: fail to gen union for %s.", op_name.c_str(),
                    op_type.c_str(), unique_name.c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::GetDynamicFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                           const ge::OpDesc &op_desc,
                                                           const bool &is_dynamic_check,
                                                           const HeavyFormatInfo &heavy_format_info,
                                                           map<string, vector<ge::Format>> &format_res,
                                                           map<string, vector<ge::DataType>> &data_type_res) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  FE_LOGD("Op[name=%s,type=%s]: start to GetDynamicFormatDtype.", op_name.c_str(), op_type.c_str());

  // 1. input_index_map, output_index_map
  IndexNameMap input_index_map;
  IndexNameMap output_index_map;
  Status ret = GetInputOutputNameMap(op_desc, op_kernel_info_ptr, input_index_map, output_index_map);
  if (ret != SUCCESS) {
    return ret;
  }

  // 2. get the origin info
  OriginInfoPtr origin_info_ptr = nullptr;
  FE_MAKE_SHARED(origin_info_ptr = make_shared<OriginInfo>(), return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (InitOriginInfo(op_desc, op_kernel_info_ptr, input_index_map, output_index_map, origin_info_ptr, format_res) !=
      SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GenFmt][Built-In-Selector][Op %s,type=%s]: initialization not success.",
                    op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  FormatProccessArgs args;
  args.op_pattern = op_kernel_info_ptr->GetOpPattern();
  args.origin_info_ptr = origin_info_ptr;
  args.propagat_primary_format = heavy_format_info.expected_heavy_format;
  args.propagat_sub_format = heavy_format_info.sub_format;

  // 3. heavy format
  for (const auto &heavy_format : FE_HEAVY_FORMAT_VECTOR) {
    string heavy_format_str = ge::TypeUtils::FormatToSerialString(heavy_format);
    args.support_format = heavy_format;
    auto processer = BuildFormatProcess(args);
    if (processer == nullptr) {
      FE_LOGD("Op[name=%s,type=%s]: can't find the proccesser of heavy format %s.", op_name.c_str(), op_type.c_str(),
              heavy_format_str.c_str());
      continue;
    }

    FormatProccessResult result;
    FE_LOGD("Op[name=%s,type=%s]: start to process format %s.", op_name.c_str(), op_type.c_str(),
            heavy_format_str.c_str());
    if (processer->Process(op_desc, args, result) == SUCCESS) {
      FE_LOGD("Op[name=%s,type=%s]: end to process format %s, support it.", op_name.c_str(), op_type.c_str(),
              heavy_format_str.c_str());
      if (UpdateFormatMap(op_kernel_info_ptr, result, input_index_map, output_index_map, format_res) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][GenFmt][UptFmtMap][Op %s,type=%s]: update input and output formats not success.",
                        op_name.c_str(), op_type.c_str());
        return FAILED;
      }
    }
  }

  LogFormatMap(format_res);
  FE_LOGD("Op[name=%s,type=%s]: end to GetDynamicFormatDtype.", op_name.c_str(), op_type.c_str());
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::InitOriginInfo(const ge::OpDesc &op_desc,
                                                    const OpKernelInfoPtr &op_kernel_info_ptr,
                                                    const IndexNameMap &input_index_map,
                                                    const IndexNameMap &output_index_map, OriginInfoPtr origin_info_ptr,
                                                    map<string, vector<ge::Format>> &format_res) const {
  vector<ge::Format> input_formats;
  vector<ge::DataType> input_dtypes;
  vector<ge::GeShape> input_shapes;
  vector<ge::GeShape> output_shapes;

  uint32_t index = 0;
  ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_input_desc_ptr = op_desc.GetAllInputsDescPtr();
  for (const auto &input_desc : all_input_desc_ptr) {
    auto origin_format = input_desc->GetOriginFormat();
    auto origin_dtype = input_desc->GetOriginDataType();
    auto origin_shape = input_desc->GetOriginShape();
    input_formats.push_back(origin_format);
    input_dtypes.push_back(origin_dtype);
    input_shapes.push_back(origin_shape);
    FE_LOGD("input[%d]: origin_format=[%s], origin_data_type=[%s], origin_shape=[%s].", index,
            ge::TypeUtils::FormatToSerialString(origin_format).c_str(),
            ge::TypeUtils::DataTypeToSerialString(origin_dtype).c_str(), GetShapeDims(origin_shape).c_str());

    InputOrOutputInfoPtr input_or_output_info_ptr =
        GetInputOrOutputUniqueName(true, input_index_map, index, op_kernel_info_ptr);
    FE_CHECK_NOTNULL(input_or_output_info_ptr);
    string unique_name = input_or_output_info_ptr->GetUniqueName();
    format_res[unique_name] = FE_ORIGIN_FORMAT_VECTOR;
    index++;
  }

  index = 0;
  ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_output_desc_ptr = op_desc.GetAllOutputsDescPtr();
  for (const auto &output_desc : all_output_desc_ptr) {
    output_shapes.push_back(output_desc->GetOriginShape());
    FE_LOGD("output[%d]: origin_shape=[%s].", index, GetShapeDims(output_desc->GetOriginShape()).c_str());
    InputOrOutputInfoPtr input_or_output_info_ptr =
        GetInputOrOutputUniqueName(false, output_index_map, index, op_kernel_info_ptr);
    FE_CHECK_NOTNULL(input_or_output_info_ptr);
    string unique_name = input_or_output_info_ptr->GetUniqueName();
    format_res[unique_name] = FE_ORIGIN_FORMAT_VECTOR;
    index++;
  }
  origin_info_ptr->input_formats = input_formats;
  origin_info_ptr->input_dtypes = input_dtypes;
  origin_info_ptr->input_shapes = input_shapes;
  origin_info_ptr->output_shapes = output_shapes;
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::UpdateFormatMap(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                     const FormatProccessResult &format_proccess_res,
                                                     const IndexNameMap &index_name_map,
                                                     const IndexNameMap &output_index_map,
                                                     map<string, vector<ge::Format>> &format_res) const {
  if (UpdateFormatMap(true, op_kernel_info_ptr, format_proccess_res.input_format_res, index_name_map, format_res) !=
      SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GenFmt][UptFmtMap] Update input formats failed.");
    return FAILED;
  }
  if (UpdateFormatMap(false, op_kernel_info_ptr, format_proccess_res.output_format_res, output_index_map, format_res) !=
      SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GenFmt][UptFmtMap] Update output formats failed");
    return FAILED;
  }
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::UpdateFormatMap(const bool &is_input, const OpKernelInfoPtr &op_kernel_info_ptr,
                                                     const vector<vector<ge::Format>> &formats,
                                                     const IndexNameMap &index_map,
                                                     map<string, vector<ge::Format>> &format_res) const {
  for (const auto &format_vec : formats) {
    for (size_t j = 0; j != format_vec.size(); ++j) {
      InputOrOutputInfoPtr input_or_output_info_ptr =
          GetInputOrOutputUniqueName(is_input, index_map, j, op_kernel_info_ptr);
      FE_CHECK_NOTNULL(input_or_output_info_ptr);
      string unique_name = input_or_output_info_ptr->GetUniqueName();
      if (input_or_output_info_ptr->GetParamType() == DYNAMIC) {
        if (j == 0) {
          format_res[unique_name].push_back(format_vec[j]);
        }
      } else {
        format_res[unique_name].push_back(format_vec[j]);
      }
    }
  }
  return SUCCESS;
}

InputOrOutputInfoPtr FormatDtypeOpBuiltinSelector::GetInputOrOutputUniqueName(
    const bool &is_input, const IndexNameMap &index_name_map, const size_t &index,
    const OpKernelInfoPtr &op_kernel_info_ptr) const {
  InputOrOutputInfoPtr input_or_output_info_ptr = nullptr;
  auto iter = index_name_map.find(index);
  string prefix = is_input ? "input" : "output";
  if (iter == index_name_map.end()) {
    REPORT_FE_ERROR("[GraphOpt][GenFmt][GetInOutNm] the prefix[%s] index [%zu] is not found from the op store.",
                    prefix.c_str(), index);
    return input_or_output_info_ptr;
  }
  if (is_input) {
    if (op_kernel_info_ptr->GetInputInfoByName(iter->second, input_or_output_info_ptr) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][GenFmt][GetInOutNm] the prefix[%s] index[%zu] is not found from the op store.",
                      prefix.c_str(), index);
      return input_or_output_info_ptr;
    }
  } else {
    if (op_kernel_info_ptr->GetOutputInfoByName(iter->second, input_or_output_info_ptr) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][GenFmt][GetInOutNm] the prefix[%s] index[%zu] is not found from the op store.",
                      prefix.c_str(), index);
      return input_or_output_info_ptr;
    }
  }
  return input_or_output_info_ptr;
}

Status FormatDtypeOpBuiltinSelector::GetReduceNewFormatDType(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                             const ge::OpDesc &op_desc, const string input_key,
                                                             const string output_key,
                                                             map<string, vector<ge::Format>> &format_res,
                                                             map<string, vector<ge::DataType>> &data_type_res) const {
  if (op_kernel_info_ptr->GetOpPattern() == OP_PATTERN_REDUCE) {
    FE_LOGD("Op[name=%s,type=%s]: is reduce op, begin to check dtype supported", op_desc.GetName().c_str(),
            op_desc.GetType().c_str());
    if (format_res.size() != 2 || data_type_res.size() != 2) {
      FE_LOGD("Op[name=%s,type=%s]: format size:%zu, datatype size:%zu, not 2.", op_desc.GetName().c_str(),
              op_desc.GetType().c_str(), format_res.size(), data_type_res.size());
      return SUCCESS;
    }

    vector<ge::Format> input_formats = format_res[input_key];
    vector<ge::Format> output_formats = format_res[output_key];
    vector<ge::DataType> input_data_types = data_type_res[input_key];
    vector<ge::DataType> output_data_types = data_type_res[output_key];

    vector<ge::Format> input_new_formats;
    vector<ge::Format> output_new_formats;
    vector<ge::DataType> input_new_data_types;
    vector<ge::DataType> output_new_data_types;

    for (size_t i = 0; i < input_formats.size(); i++) {
      bool is_out_original_format = (find(FE_ORIGIN_FORMAT_VECTOR.begin(), FE_ORIGIN_FORMAT_VECTOR.end(),
                                          output_formats[i]) != FE_ORIGIN_FORMAT_VECTOR.end());
      if (input_formats[i] == ge::FORMAT_NC1HWC0 && is_out_original_format && input_data_types[i] != ge::DT_FLOAT16) {
        FE_LOGD("Op[%s]: input [%zu]'s format is NC1HWC0, output format is ND, and dtype is not float16, clear it.",
                op_desc.GetName().c_str(), i);
        continue;
      }
      /* when format is 5HD, the size of reduce axis and size of not redcued axis
       * must less than ub size. */
      if (input_formats[i] == ge::FORMAT_NC1HWC0) {
        if (!CheckUBSizeEnable(op_desc, input_formats[i], input_data_types[i])) {
          FE_LOGD("Op[%s]: input %zu's format is NC1HWC0, total size needed is greater than ub block size, clear it.",
                  op_desc.GetName().c_str(), i);
          continue;
        }
      }

      input_new_formats.push_back(input_formats[i]);
      input_new_data_types.push_back(input_data_types[i]);
      output_new_formats.push_back(output_formats[i]);
      output_new_data_types.push_back(output_data_types[i]);
    }

    format_res[input_key].swap(input_new_formats);
    format_res[output_key].swap(output_new_formats);
    data_type_res[input_key].swap(input_new_data_types);
    data_type_res[output_key].swap(output_new_data_types);
  }
  return SUCCESS;
}

Status FormatDtypeOpBuiltinSelector::GetReduceNewFormatDTypeVec(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                                const ge::OpDesc &op_desc,
                                                                const string &input_or_out_key,
                                                                vector<ge::DataType> &data_types,
                                                                vector<ge::Format> &formats) {
  if (op_kernel_info_ptr->GetOpPattern() == OP_PATTERN_REDUCE) {
    map<string, vector<ge::Format>> format_res;
    map<string, vector<ge::DataType>> data_type_res;
    bool is_dynamic_check = IsOpDynamicImpl(op_desc);
    if (GetSupportFormatDtype(op_kernel_info_ptr, op_desc, is_dynamic_check, format_res, data_type_res) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][GenFmt][GetRdcNewFmtDtyVec] Op[name=%s,type=%s]: Fail to GetSupportFormatDtype.",
                      op_desc.GetName().c_str(), op_desc.GetType().c_str());
      return FAILED;
    }
    if (!data_types.empty()) {
      data_types.swap(data_type_res[input_or_out_key]);
    }
    if (!formats.empty()) {
      formats.swap(format_res[input_or_out_key]);
    }
  }
  return SUCCESS;
}

uint64_t CalcTotalSize(const vector<int64_t> &axis_new_value, const ge::GeShape &new_shape, const ge::OpDesc &op_desc,
                       int data_size) {
  int64_t axis_value_size = 1;
  int64_t not_reduce_size = 1;
  for (size_t i = 0; i < new_shape.GetDimNum(); i++) {
    auto iter = find(axis_new_value.begin(), axis_new_value.end(), i);
    if (iter != axis_new_value.end()) {
      FE_INT64_MULCHECK(axis_value_size, new_shape.GetDim(i));
      axis_value_size *= new_shape.GetDim(i);
    } else {
      FE_INT64_MULCHECK(not_reduce_size, new_shape.GetDim(i));
      not_reduce_size *= new_shape.GetDim(i);
    }
  }
  /* The min size needed is size of reduce axis plus size not reduce axis multiply coefficients
   * coefficient is the max coefficient of reduce op : ReduceMean whose value is 2. */
  FE_INT64_ADDCHECK(axis_value_size, not_reduce_size);
  uint64_t tmp1 = static_cast<uint64_t>(axis_value_size + not_reduce_size);
  FE_UINT64_MULCHECK(tmp1, data_size);
  FE_UINT64_MULCHECK(tmp1 * data_size, REDUCE_COEFFICIENTS);
  uint64_t total_size = tmp1 * data_size * REDUCE_COEFFICIENTS;
  FE_LOGD("Op[name=%s,type=%s]: min size needed is %ld.", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
          total_size);
  return total_size;
}

bool FormatDtypeOpBuiltinSelector::CheckUBSizeEnable(const ge::OpDesc &op_desc, const ge::Format &check_format,
                                                     const ge::DataType &check_dtype) const {
  vector<int64_t> axis_values;
  (void)ge::AttrUtils::GetListInt(op_desc, AXES_ATTR_NAME, axis_values);
  if (axis_values.empty()) {
    return true;
  }
  // 1. keep_dims must be false
  bool keep_dim = false;
  if (ge::AttrUtils::GetBool(op_desc, KEEP_DIMS, keep_dim) && keep_dim) {
    FE_LOGD("Op[name=%s,type=%s]: the attribute keep_dims is true.", op_desc.GetName().c_str(),
            op_desc.GetName().c_str());
    return true;
  }

  string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  PlatformInfo platform_info;
  OptionalInfo opti_compilation_info;
  if (PlatformInfoManager::Instance().GetPlatformInfo(soc_version, platform_info, opti_compilation_info) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GenFmt][ChkUbSizeEn] Fail to get platform info by soc version[%s].",
                    soc_version.c_str());
    return false;
  }
  uint64_t ub_size = platform_info.ai_core_spec.ub_size;
  FE_LOGD("UB size is %lu.", ub_size);

  int data_size = ge::GetSizeByDataType(check_dtype);
  FE_LOGD("The dtype size to check of op[name:%s,type:%s] is : %d.", op_desc.GetName().c_str(),
          op_desc.GetType().c_str(), data_size);

  int64_t op_imply_type = -1;
  if (!ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, op_imply_type)) {
    FE_LOGW("Op[name=%s,type=%s]: get imply type not success.", op_desc.GetName().c_str(), op_desc.GetType().c_str());
  }

  for (const auto &input_desc : op_desc.GetAllInputsDescPtr()) {
    ge::GeShape origin_shape = input_desc->GetOriginShape();
    ge::Format ori_format = input_desc->GetOriginFormat();
    ge::GeShape new_shape;
    ShapeAndFormat shape_and_format_info = {origin_shape, new_shape,     ori_format,          check_format,
                                            check_dtype,  op_imply_type, GROUPS_DEFAULT_VALUE};
    Status ret = ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(shape_and_format_info);
    if (ret != SUCCESS) {
      FE_LOGW("Get new shape not successful, old format is %u, new format is %u", input_desc->GetOriginFormat(),
              check_format);
      return false;
    }
    // get new axis info
    vector<int64_t> axis_new_value;
    if (AxisNameUtil::GetNewAxisAttributeValue(op_desc, ori_format, check_format, origin_shape, axis_new_value) !=
        SUCCESS) {
      FE_LOGW("Get reduce op [%s] new axis info failed!", op_desc.GetName().c_str());
      return false;
    }

    uint64_t total_size = CalcTotalSize(axis_new_value, new_shape, op_desc, data_size);
    if (total_size >= ub_size) {
      return false;
    }
  }
  return true;
}
}  // namespace fe
