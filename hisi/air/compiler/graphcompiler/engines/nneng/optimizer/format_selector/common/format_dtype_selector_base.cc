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

#include "format_selector/common/format_dtype_selector_base.h"

namespace fe {
FormatDtypeSelectorBase::FormatDtypeSelectorBase(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr) {}
FormatDtypeSelectorBase::~FormatDtypeSelectorBase() {}

Status FormatDtypeSelectorBase::GetAllFormatsFromOpDesc(const ge::OpDesc &op_desc,
                                                        map<string, vector<ge::Format>> &result) {
  result = op_desc.TryGetExtAttr(EXT_DYNAMIC_FORMAT, result);
  if (result.empty()) {
    FE_LOGD("Op[name=%s,type=%s]: the %s attribute is empty.", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
            EXT_DYNAMIC_FORMAT.c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status FormatDtypeSelectorBase::GetAllDataTypesFromOpDesc(const ge::OpDesc &op_desc,
                                                          map<string, vector<ge::DataType>> &result) {
  result = op_desc.TryGetExtAttr(EXT_DYNAMIC_DATATYPE, result);
  if (result.empty()) {
    FE_LOGD("Op[name=%s,type=%s]: the %s attribute is empty.", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
            EXT_DYNAMIC_DATATYPE.c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status FormatDtypeSelectorBase::GetFormatFromOpDescByKey(const ge::OpDesc &op_desc, const string &key,
                                                         vector<ge::Format> &result) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();

  // 1. get all formats from the op_desc
  map<string, vector<ge::Format>> format_map;
  if (GetAllFormatsFromOpDesc(op_desc, format_map) != SUCCESS) {
    FE_LOGD("Op[name=%s,type=%s]: fail to GetAllFormatsFromOpDesc by the key [%s].", op_name.c_str(), op_type.c_str(),
            key.c_str());
    return FAILED;
  }

  // 2. find the key from format_map
  if (format_map.find(key) == format_map.end()) {
    FE_LOGD("Op[name=%s,type=%s]: fail to find the formats from the formatMap by the key [%s].", op_name.c_str(),
            op_type.c_str(), key.c_str());
    return FAILED;
  }
  result = format_map[key];
  return SUCCESS;
}

Status FormatDtypeSelectorBase::GetDataTypeFromOpDescByKey(const ge::OpDesc &op_desc, const string &key,
                                                           vector<ge::DataType> &result) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();

  // 1. get all data_types from the op_desc
  std::map<std::string, vector<ge::DataType>> data_type_map;
  if (GetAllDataTypesFromOpDesc(op_desc, data_type_map) != SUCCESS) {
    FE_LOGD("Op[name=%s,type=%s]: fail to GetAllDataTypesFromOpDesc by the key [%s].", op_name.c_str(), op_type.c_str(),
            key.c_str());
    return FAILED;
  }

  // 2. find the key from the data_type_map
  if (data_type_map.find(key) == data_type_map.end()) {
    FE_LOGD("Op[name=%s,type=%s]: fail to find the data_types from the dataTypeMap by the key [%s].", op_name.c_str(),
            op_type.c_str(), key.c_str());
    return FAILED;
  }
  result = data_type_map[key];
  return SUCCESS;
}

Status FormatDtypeSelectorBase::SetSupportFormatDtype(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                      const HeavyFormatInfo &heavy_format_info,
                                                      ge::OpDesc &op_desc,
                                                      const bool &is_dynamic_check) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();

  std::map<std::string, vector<ge::Format>> format_map;
  std::map<std::string, vector<ge::DataType>> data_type_map;
  FE_LOGD("Op[name=%s,type=%s]: propagat_primary_format=%s, propagat_sub_format=%d.", op_name.c_str(), op_type.c_str(),
          ge::TypeUtils::FormatToSerialString(heavy_format_info.expected_heavy_format).c_str(),
          heavy_format_info.sub_format);

  if (GetDynamicFormatDtype(op_kernel_info_ptr, op_desc, is_dynamic_check, heavy_format_info,
                            format_map, data_type_map) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][SetSptFmtDtype] Op[name=%s,type=%s]: fail to GetDynamicFormatDtype.",
                    op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  if (SaveDynamicFormatDtype(format_map, data_type_map, op_desc) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][SetSptFmtDtype] Op[name=%s,type=%s]: fail to SaveFormatAndDtype.",
                    op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status FormatDtypeSelectorBase::SaveDynamicFormatDtype(const std::map<std::string, vector<ge::Format>> &format_map,
                                                       const std::map<std::string, vector<ge::DataType>> &data_type_map,
                                                       ge::OpDesc &op_desc) {
  string op_name = op_desc.GetName();
  string op_type = op_desc.GetType();

  if (!format_map.empty()) {
    FE_LOGI("Op[name=%s,type=%s]: the format_map is not empty, set it to the attribute [%s].", op_name.c_str(),
            op_type.c_str(), EXT_DYNAMIC_FORMAT.c_str());
    if (!op_desc.SetExtAttr(EXT_DYNAMIC_FORMAT, format_map)) {
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][SvDymcFmtDty][name %s,type %s]: fail to set the format_map to the attribute  [%s].",
          op_name.c_str(), op_type.c_str(), EXT_DYNAMIC_FORMAT.c_str());
      return FAILED;
    }
  }

  if (!data_type_map.empty()) {
    FE_LOGI("Op[name=%s,type=%s]: the data_type_map is not empty, set it to the attribute [%s].", op_name.c_str(),
            op_type.c_str(), EXT_DYNAMIC_DATATYPE.c_str());
    if (!op_desc.SetExtAttr(EXT_DYNAMIC_DATATYPE, data_type_map)) {
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][SvDymcFmtDty][name %s,type %s]: fail to set the data_type_map to the attribute [%s].",
          op_name.c_str(), op_type.c_str(), EXT_DYNAMIC_FORMAT.c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

}  // namespace fe
