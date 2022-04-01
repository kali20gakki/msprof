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

#include "format_selector/op_customize/format_dtype_op_customize_selector.h"
#include "common/configuration.h"
#include "common/fe_type_utils.h"
#include "common/op_info_common.h"
#include "common/unknown_shape_util.h"

namespace fe {
FormatDtypeOpCustomizeSelector::FormatDtypeOpCustomizeSelector(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : FormatDtypeSelectorBase(op_store_adapter_manager_ptr) {}

FormatDtypeOpCustomizeSelector::~FormatDtypeOpCustomizeSelector() {}
Status FormatDtypeOpCustomizeSelector::GetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                             const ge::OpDesc &op_desc,
                                                             const bool &is_dynamic_check,
                                                             map<string, vector<ge::Format>> &format_map,
                                                             map<string, vector<ge::DataType>> &data_type_map) {
  auto op_name = op_desc.GetName();
  auto op_type = op_desc.GetType();
  FE_LOGD("Start to obtain custom format and data type from op[%s, %s].", op_name.c_str(), op_type.c_str());

  // 1. get all  formats and data_types from the op_desc
  Status get_format_status = GetAllFormatsFromOpDesc(op_desc, format_map);
  Status get_data_type_status = GetAllDataTypesFromOpDesc(op_desc, data_type_map);

  // 2. if failed, GetDynamicFormatDtype
  if (get_format_status != SUCCESS || get_data_type_status != SUCCESS) {
    HeavyFormatInfo heavy_format_info;
    if (GetDynamicFormatDtype(op_kernel_info_ptr, op_desc, is_dynamic_check, heavy_format_info,
                              format_map, data_type_map) != SUCCESS) {
      FE_LOGW("[GraphOpt][Setcheck][GetSptFmtDty][op %s,type %s] Fail to get dynamic format and data type.",
              op_name.c_str(), op_type.c_str());
      return FAILED;
    }
  }

  LogFormatMap(format_map);
  LogDataTypeMap(data_type_map);
  FE_LOGD("Finish obtaining custom format and data type from op[%s, %s].", op_name.c_str(), op_type.c_str());
  return SUCCESS;
}

Status FormatDtypeOpCustomizeSelector::GetSupportFormats(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                         const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                                         const ge::OpDesc &op_desc, vector<ge::Format> &result) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  string unique_name = input_or_output_info_ptr->GetUniqueName();
  if (GetFormatFromOpDescByKey(op_desc, unique_name, result) != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][Setcheck][GetSptFmt][op %s,type %s]: fail to GetSupportFormatsFromOpDesc from op_desc \
        for the input_or_output [%s].",
        op_name.c_str(), op_type.c_str(), unique_name.c_str());
    return FAILED;
  }

  FE_LOGD("Op[name=%s,type=%s]: support formats is [%s] for the input_or_output [%s].", op_name.c_str(),
          op_type.c_str(), GetStrByFormatVec(result).c_str(), unique_name.c_str());
  return SUCCESS;
}

Status FormatDtypeOpCustomizeSelector::GetSupportDataTypes(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                           const InputOrOutputInfoPtr &input_or_output_info_ptr,
                                                           const ge::OpDesc &op_desc, vector<ge::DataType> &result) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  string unique_name = input_or_output_info_ptr->GetUniqueName();
  if (GetDataTypeFromOpDescByKey(op_desc, unique_name, result) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][DtypeJdg][GetDtype] Op[%s,type=%s]: fail to GetSupportDataTypesFromOpDesc for [%s].",
                    op_name.c_str(), op_type.c_str(), unique_name.c_str());

    return FAILED;
  }

  FE_LOGD("Op[name=%s,type=%s]: support data_types is [%s] for the [%s].", op_name.c_str(), op_type.c_str(),
          GetStrByDataTypeVec(result).c_str(), unique_name.c_str());
  return SUCCESS;
}

Status FormatDtypeOpCustomizeSelector::GetDynamicFormatDtype(
    const OpKernelInfoPtr &op_kernel_info_ptr, const ge::OpDesc &op_desc, const bool &is_dynamic_check,
    const HeavyFormatInfo &heavy_format_info, std::map<std::string, vector<ge::Format>> &format_map,
    std::map<std::string, vector<ge::DataType>> &data_type_map) {
  FE_CHECK_NOTNULL(op_kernel_info_ptr);
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();
  FE_LOGD("Op[name=%s,type=%s]: start to get the dynamic format_map and dataTypeMap.", op_name.c_str(),
          op_type.c_str());

  // 1. get the op_store_adapter
  OpStoreAdapterPtr op_store_adapter = nullptr;
  OpImplType impl_type = op_kernel_info_ptr->GetOpStoreImplType();
  if (op_store_adapter_manager_ptr_->GetOpStoreAdapter(impl_type, op_store_adapter) != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][Setcheck][GetDymcFmtDty][op %s,type %s]: fail to get op_store_adapter by op impl type[%d].",
        op_name.c_str(), op_type.c_str(), impl_type);
    return FAILED;
  }

  // 2. call SelectOpFormat
  string op_format_dtype_str;
  if (op_store_adapter->SelectOpFormat(op_desc, op_kernel_info_ptr, is_dynamic_check, heavy_format_info,
                                       op_format_dtype_str) != SUCCESS) {
    FE_LOGW("[GenFormat][SelectOpFormat][Op name=%s,type=%s]: fail to select formats and data_types.",
            op_name.c_str(), op_type.c_str());
    return FAILED;
  }
  if (op_format_dtype_str.empty()) {
    FE_LOGW("[GraphOpt][Setcheck][GetDymcFmtDty][op %s,type %s] The op_format_dtype_str is empty.",
            op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  // 3. parse the op_format_dtype_str
  if (ParseJsonStr(op_kernel_info_ptr, op_desc, op_format_dtype_str, is_dynamic_check, format_map, data_type_map)
                   != SUCCESS) {
    REPORT_FE_ERROR("[GenFormat][ParseFmtJson][Op %s,type=%s]: fail to parse the op_format_dtype_str %s",
                    op_name.c_str(), op_type.c_str(), op_format_dtype_str.c_str());
    return FAILED;
  }

  // 4. filter C04 format
  if (FilterC04Format(format_map, data_type_map) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetDymcFmtDty][op %s,type %s] Fail to filter C04 format.", op_name.c_str(),
                    op_type.c_str());
    return FAILED;
  }
  FE_LOGD("Op[name=%s,type=%s]: end to get the dynamic format_map and data_type_map.", op_name.c_str(),
          op_type.c_str());
  return SUCCESS;
}

Status CheckJsonValid(const nlohmann::json &j, const string &input_or_output_key) {
  if (input_or_output_key.empty()) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][ChkJsonValid] The input_or_output_key is empty.");
    return FAILED;
  }
  if (!j[input_or_output_key].is_object()) {
    REPORT_FE_ERROR(
        "[GraphOpt][Setcheck][ChkJsonValid] The input_or_output is not an object, input_or_output_key is %s.",
        input_or_output_key.c_str());
    return FAILED;
  }

  if (j[input_or_output_key].find(STR_FORMAT) == j[input_or_output_key].end() &&
      j[input_or_output_key].find(STR_UNKNOWN_SHAPE_FORMAT) == j[input_or_output_key].end()) {
    REPORT_FE_ERROR(
        "[GraphOpt][Setcheck][ChkJsonValid] The format or unknown_format of input_or_output_key %s can not be found.",
        input_or_output_key.c_str());
    return FAILED;
  }
  if (j[input_or_output_key].find(STR_DTYPE) == j[input_or_output_key].end()) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][ChkJsonValid] The data_type of input_or_output_key %s can not be found.",
                    input_or_output_key.c_str());
    return FAILED;
  }
  if (j[input_or_output_key].find(STR_NAME) == j[input_or_output_key].end()) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][ChkJsonValid] The name of input_or_output_key %s can not be found.",
                    input_or_output_key.c_str());
    return FAILED;
  }
  return SUCCESS;
}
/**
 * {
 *   "input0":{
 *       "name": "x",
 *       "format": "NCHW",
 *       "dtype": "float"
 *   },
 *   "output0":{
 *       "name": "y",
 *       "format": "NCHW",
 *       "dtype": "float"
 *   }
 * }
 */
Status FormatDtypeOpCustomizeSelector::ParseJsonStr(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                    const ge::OpDesc &op_desc, const string &json_str,
                                                    const bool &is_dynamic_check,
                                                    std::map<std::string, vector<ge::Format>> &format_map,
                                                    std::map<std::string, vector<ge::DataType>> &data_type_map) {
  try {
    nlohmann::json j = nlohmann::json::parse(json_str);
    if (!j.is_object()) {
      REPORT_FE_ERROR("[GraphOpt][Setcheck][PasJsonStr] The json_str is not an object, the json_str is %s.",
                      json_str.c_str());
      return ILLEGAL_JSON;
    }
    uint32_t format_size_of_first_input_or_output = INVALID_DTYPE_AND_FORMAT_SIZE;
    for (auto &input_or_output : j.items()) {
      string input_or_output_key = input_or_output.key();
      if (CheckJsonValid(j, input_or_output_key) != SUCCESS) {
        return FAILED;
      }

      string format_vec_str;
      if (j[input_or_output_key].find(STR_FORMAT) != j[input_or_output_key].end()) {
        format_vec_str = static_cast<string>(j[input_or_output_key].at(STR_FORMAT));
      }

      if (is_dynamic_check) {
        if (j[input_or_output_key].find(STR_UNKNOWN_SHAPE_FORMAT) != j[input_or_output_key].end()) {
          FE_LOGD("Unknown shape op[name:%s,type:%s] support dynamic shape.", op_desc.GetName().c_str(),
                  op_desc.GetType().c_str());
          format_vec_str = static_cast<string>(j[input_or_output_key].at(STR_UNKNOWN_SHAPE_FORMAT));
        }
      }
      string data_type_vec_str = static_cast<string>(j[input_or_output_key].at(STR_DTYPE));
      string name_key = input_or_output_key + "." + static_cast<string>(j[input_or_output_key].at(STR_NAME));
      vector<ge::Format> format_vec;
      vector<ge::DataType> data_type_vec;
      if (ConvertFormatDtype(format_vec_str, data_type_vec_str, format_size_of_first_input_or_output, format_vec,
                             data_type_vec) != SUCCESS) {
        REPORT_FE_ERROR("[GenFormat][ParseFmtJson][Op %s]: tendot %s: fail to convert from json [%s].",
                        op_desc.GetName().c_str(), name_key.c_str(), json_str.c_str());
        return FAILED;
      }
      format_map.insert(pair<string, vector<ge::Format>>(name_key, format_vec));
      data_type_map.insert(pair<string, vector<ge::DataType>>(name_key, data_type_vec));
    }
    return SUCCESS;
  } catch (std::exception &e) {
    REPORT_FE_ERROR("[GraphOpt][ParseFmtJson][Exception]the json_str is %s and the reason is %s", json_str.c_str(),
                    e.what());
    return FAILED;
  }
}

Status FormatDtypeOpCustomizeSelector::ConvertFormatDtype(const string &format_vec_str, const string &data_type_vec_str,
                                                          uint32_t &format_size_of_first_input_or_output,
                                                          vector<ge::Format> &format_vec,
                                                          vector<ge::DataType> &data_type_vec) {
  vector<string> format_str_vec = StringUtils::Split(format_vec_str, ',');
  vector<string> data_type_str_vec = StringUtils::Split(data_type_vec_str, ',');

  // 1. check whether size of dtype is the same as size of format
  uint32_t format_size = format_str_vec.size();
  uint32_t dtype_size = data_type_str_vec.size();
  if (dtype_size != format_size) {
    REPORT_FE_ERROR("[GraphOpt][SetFmt][ConvertFmt]: The format size [%u] and dtype size [%u] is not equal!",
                    format_size, dtype_size);
    return FAILED;
  }

  // 2. check the format size whether is same with other input_or_outputs
  if (format_size_of_first_input_or_output == INVALID_DTYPE_AND_FORMAT_SIZE) {
    format_size_of_first_input_or_output = format_size;
  }
  if (format_size != format_size_of_first_input_or_output) {
    REPORT_FE_ERROR("[GraphOpt][SetFmt][ConvertFmt]: format size %u is invalid, it should be [%u].", format_size,
                    format_size_of_first_input_or_output);
    return FAILED;
  }

  // 3. get the ge::Format and ge::DataType
  for (uint32_t i = 0; i != format_size; i++) {
    string format_str = format_str_vec[i];
    ge::Format format = ge::TypeUtils::SerialStringToFormat(StringUtils::Trim(format_str));
    if (format == ge::FORMAT_RESERVED) {
      REPORT_FE_ERROR("[GraphOpt][SetFmt][ConvertFmt]: Invalid format: %s can not be converted to ge::Format enum.",
                      format_str.c_str());
      return FAILED;
    }

    ge::DataType dtype;
    string dtype_str = data_type_str_vec[i];
    if (String2DataType(StringUtils::Trim(dtype_str), dtype) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][SetFmt][ConvertFmt]: Invalid data type: %s can not be converted.", dtype_str.c_str());
      return FAILED;
    }
    format_vec.push_back(format);
    data_type_vec.push_back(dtype);
  }
  return SUCCESS;
}

Status FormatDtypeOpCustomizeSelector::FilterC04Format(std::map<std::string, vector<ge::Format>> &format_map,
    std::map<std::string, vector<ge::DataType>> &data_type_map) const {
  if (format_map.empty() || data_type_map.empty()) {
    FE_LOGD("The format_map or data_type_map is empty.");
    return SUCCESS;
  }

  set<uint32_t> c04_format_index_set;
  size_t format_vec_size = format_map.begin()->second.size();
  std::map<std::string, vector<ge::Format>>::iterator format_iter;
  for (format_iter = format_map.begin(); format_iter != format_map.end(); ++format_iter) {
    if (format_vec_size != format_iter->second.size()) {
      REPORT_FE_ERROR("[GraphOpt][Setcheck][FltrFmt] the size of format vector is not same.");
      return FAILED;
    }
    for (uint32_t i = 0; i < format_iter->second.size(); i++) {
      bool is_c04_format = std::find(FE_C04_FORMAT_VECTOR.begin(), FE_C04_FORMAT_VECTOR.end(),
                                     format_iter->second[i]) != FE_C04_FORMAT_VECTOR.end();
      if (is_c04_format) {
        c04_format_index_set.insert(i);
      }
    }
  }

  // If c04_format_index_set is empty or all the format is C04
  // that means the remain format will be empty or not change
  // clear all the format is not allowed, so keep all of them
  if (c04_format_index_set.empty() || c04_format_index_set.size() == format_vec_size) {
    return SUCCESS;
  }

  // if enable_small_channel is on, only C04 formats remains
  // if enable_small_channel is off, only non-C04 formats remains
  bool enable_small_channel = Configuration::Instance(AI_CORE_NAME).GetEnableSmallChannel();
  set<uint32_t> remain_index_set;
  if (enable_small_channel) {
    remain_index_set.swap(c04_format_index_set);
  } else {
    for (size_t i = 0; i < format_vec_size; i++) {
      if (std::find(c04_format_index_set.begin(), c04_format_index_set.end(), i) == c04_format_index_set.end()) {
        remain_index_set.insert(i);
      }
    }
  }

  if (UpdateDTypeAndFormat(remain_index_set, format_map, data_type_map) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][FltrFmt] Fail to update data type and format.");
    return FAILED;
  }

  return SUCCESS;
}

Status FormatDtypeOpCustomizeSelector::UpdateDTypeAndFormat(
    set<uint32_t> &remain_index_set, std::map<std::string, vector<ge::Format>> &format_map,
    std::map<std::string, vector<ge::DataType>> &data_type_map) const {
  // after filter format, if remind_index_set is empty, do nothing.
  if (remain_index_set.empty()) {
    FE_LOGD("The remain format is empty. The format remains unchanged.");
    return SUCCESS;
  }

  set<uint32_t>::iterator index_iter;
  std::map<std::string, vector<ge::Format>>::iterator format_iter;
  for (format_iter = format_map.begin(); format_iter != format_map.end(); ++format_iter) {
    if (format_iter->second.empty()) {
      continue;
    }
    vector<ge::Format> remain_format_vec;
    for (index_iter = remain_index_set.begin(); index_iter != remain_index_set.end(); ++index_iter) {
      if (*index_iter < format_iter->second.size()) {
        remain_format_vec.push_back(format_iter->second[*index_iter]);
      }
    }
    format_iter->second.swap(remain_format_vec);
  }

  std::map<std::string, vector<ge::DataType>>::iterator date_type_iter;
  for (date_type_iter = data_type_map.begin(); date_type_iter != data_type_map.end(); ++date_type_iter) {
    if (date_type_iter->second.empty()) {
      continue;
    }
    vector<ge::DataType> remain_data_type_vec;
    for (index_iter = remain_index_set.begin(); index_iter != remain_index_set.end(); ++index_iter) {
      if (*index_iter < date_type_iter->second.size()) {
        remain_data_type_vec.push_back(date_type_iter->second[*index_iter]);
      }
    }
    date_type_iter->second.swap(remain_data_type_vec);
  }
  return SUCCESS;
}
}  // namespace fe
