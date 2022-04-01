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

#include "ops_kernel_store/sub_ops_store.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>
#include <cmath>
#include "common/fe_inner_error_codes.h"
#include "common/graph/fe_graph_utils.h"
#include "common/string_utils.h"
#include "graph/ge_tensor.h"
#include "common/configuration.h"

using ge::GeAttrValue;
using std::map;
using std::string;
using std::vector;

namespace fe {
using fe::StringUtils;

/*
 *  @namespace fe
 *  @brief   check specified GeAttrValue::ValueType of op_desc_attr.Value() is in info_op_attr.vector<Values>
 *  @param   [in] value      : used to specified GeAttrValue::ValueType for template
 *  @param   [in] op_desc_attr : GeAttrValue from OpDesc
 *  @param   [in] info_op_attr : vector<GeAttrValue> from OpKernelInfo
 *  @return  true or false
 */
template <typename T>
bool FindValueInVector(T &value, const GeAttrValue &op_desc_attr, const vector<GeAttrValue> &info_op_attr);

SubOpsStore::SubOpsStore(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : init_flag_(false),
      sub_store_info_(),
      sub_store_type_(),
      format_dtype_querier_ptr_(nullptr),
      op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr) {}

SubOpsStore::~SubOpsStore() {}

void SubOpsStore::SetSubStoreInfo(const FEOpsStoreInfo &sub_store_info) { sub_store_info_ = (sub_store_info); }

void SubOpsStore::SetSubStoreType(const string &sub_store_type) { sub_store_type_ = (sub_store_type); }

void SubOpsStore::GetSubStoreType(string &sub_store_type) const { sub_store_type = sub_store_type_; }

Status SubOpsStore::InitializeSubStore(std::string engine_name) {
  if (init_flag_) {
    FE_LOGD("SubOpsStore has already been initialized,directly return.");
    return SUCCESS;
  }

  FE_LOGD("Initialize %s SubOpsStore begin.", sub_store_info_.fe_ops_store_name.c_str());

  FE_MAKE_SHARED(format_dtype_querier_ptr_ = std::make_shared<FormatDtypeQuerier>(op_store_adapter_manager_ptr_),
      return FAILED);
  FE_CHECK_NOTNULL(format_dtype_querier_ptr_);

  engine_name_ = engine_name;
  FE_LOGI("Initialize %s SubOpsStore finished.", sub_store_info_.fe_ops_store_name.c_str());

  init_flag_ = true;
  return SUCCESS;
}

Status SubOpsStore::FinalizeSubStore() {
  FE_LOGD("Finalizing the SubOpsKernelInfoStore begin.");
  if (!init_flag_) {
    FE_LOGD("SubOpsStore has not been initialized, Finalize is not allowed,directly return.");
    return SUCCESS;
  }
  sub_store_type_ = "";
  init_flag_ = false;
  return SUCCESS;
}

bool SubOpsStore::CheckDtypeSupported(const ge::NodePtr &node, const ge::GeTensorDescPtr &tensor_desc_ptr,
                                      InputOrOutputInfoPtr input_or_output_info_ptr,
                                      const vector<ge::DataType> &support_data_types,
                                      const OpKernelInfoPtr &op_kernel_info_ptr) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  FE_CHECK(tensor_desc_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkTensor]cTensorDescPtr is nullptr."), return false);
  FE_CHECK(input_or_output_info_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkTensor]inputOrOutputInfoPtr is nullptr."), return false);
  ge::DataType desc_dtype = tensor_desc_ptr->GetDataType();
  /* For those configurated as keep-dtype, we will not allow precision loss. */
  int64_t keep_dtype = 0;
  (void)ge::AttrUtils::GetInt(op_desc, KEEP_DTYPE, keep_dtype);
  auto precision_mode = Configuration::Instance(engine_name_).GetPrecisionModeStr();
  bool black_list_op = op_kernel_info_ptr->GetOpStoreInfo().precision_policy == BLACK;
  bool forbid_precision_reduce = (keep_dtype) || (precision_mode == MUST_KEEP_ORIGIN_DTYPE) ||
                                  (((precision_mode == FORCE_FP32) || (precision_mode == ALLOW_MIX_PRECISION)) &&
                                  black_list_op);

  bool allow_precision_reduce = !forbid_precision_reduce;

  if (Configuration::Instance(engine_name_).GetPrecisionModeStr() == ALLOW_FP32_TO_FP16) {
    FE_LOGD("Current precision mode is allow_fp32_tofp16, start checking.");
    if (tensor_desc_ptr->GetDataType() == ge::DT_FLOAT &&
        IsAllDtypeExpected(ge::DT_FLOAT16, ge::DT_FLOAT, support_data_types)) {
      FE_LOGD("Op[name=%s, type=%s]:Current precision mode is allow_fp32_tofp16, input_or_output_name[%s] dtype \
              supported in op store include fp16 without fp32, need to update dtype when op checksupport.",
              op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(),
              input_or_output_info_ptr->GetName().c_str());
      (void)ge::AttrUtils::SetBool(tensor_desc_ptr, NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT, true);
    } else {
      (void)ge::AttrUtils::SetBool(tensor_desc_ptr, NEED_UPDATE_DTYPE_WHEN_OP_CHECKSUPPORT, false);
    }
  }
  bool is_check_true = false;
  for (auto &store_dtype : support_data_types) {
    is_check_true = (desc_dtype == store_dtype) || (desc_dtype == ge::DT_FLOAT16 && store_dtype == ge::DT_FLOAT) ||
      (desc_dtype == ge::DT_BF16 && store_dtype == ge::DT_FLOAT) ||
      (desc_dtype == ge::DT_FLOAT && store_dtype == ge::DT_FLOAT16 && allow_precision_reduce);
    if (is_check_true) {
      return true;
    }
  }
  return false;
}

bool SubOpsStore::CheckAccuracyDtypeSupported(ge::ConstGeTensorDescPtr c_tensor_desc_ptr,
                                              uint32_t type_index, InputOrOutputInfoPtr input_or_output_info_ptr,
                                              const vector<ge::DataType> &support_data_types) const {
  FE_CHECK(c_tensor_desc_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkAccurDtypeSup] cTensorDescPtr is nullptr."), return false);
  FE_CHECK(input_or_output_info_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkAccurDtypeSup] inputOrOutputInfoPtr is nullptr."), return false);
  ge::DataType desc_dtype = c_tensor_desc_ptr->GetDataType();
  if (support_data_types.size() < type_index + 1) {
    FE_LOGW("supportDataTypes.size() less than type_index.");
    return false;
  }

  auto desc_dtype_position = DATATYPE_PRIORITY_MAP.find(desc_dtype);
  if (desc_dtype_position == DATATYPE_PRIORITY_MAP.end()) {
    FE_LOGW("Data type %u in OpDesc is invalid.", desc_dtype);
    return false;
  }

  auto store_dtype_position = DATATYPE_PRIORITY_MAP.find(support_data_types[type_index]);
  if (store_dtype_position == DATATYPE_PRIORITY_MAP.end()) {
    FE_LOGW("Data type %u in op kernel is invalid.", support_data_types[type_index]);
    return false;
  }
  if (desc_dtype == support_data_types[type_index]) {
    return true;
  } else {
    return false;
  }
}

bool SubOpsStore::CheckFormatSupported(const ge::NodePtr &node, ge::ConstGeTensorDescPtr c_tensor_desc_ptr,
                                       InputOrOutputInfoPtr input_or_output_info_ptr,
                                       const vector<ge::Format> &support_formats) const {
  FE_CHECK(c_tensor_desc_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkFmtSpt] cTensorDescPtr of %s is nullptr.",
                           node->GetName().c_str()), return false);
  FE_CHECK(input_or_output_info_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkFmtSpt] inputOrOutputInfoPtr of %s is nullptr.",
                           node->GetName().c_str()), return false);
  uint32_t tensor_index = input_or_output_info_ptr->GetIndex();
  // TBE plugin: the op_info_format_vec may be empty
  if (support_formats.empty()) {
    FE_LOGI("The op_info_format is empty, check success, return true.");
    return true;
  }

  ge::Format desc_format = c_tensor_desc_ptr->GetOriginFormat();
  if (!IsNd(desc_format)) {
    return true;
  }

  FE_LOGD("The format in tensor [named %s, index %u] of node %s is %s",
          input_or_output_info_ptr->GetName().c_str(), tensor_index,
          node->GetName().c_str(), FormatToStr(desc_format).c_str());
  for (auto &op_info_format : support_formats) {
    if (IsSupportedTransType(desc_format, op_info_format)) {
      FE_LOGD("Check format successfully! Formats %s in op info lib supports this tensor [named %s, index %u].",
              ConstFormatToStr(op_info_format).c_str(), input_or_output_info_ptr->GetName().c_str(), tensor_index);
      return true;
    }
  }
  FE_LOGD("Formats in op info lib does not match with format in tensor.");
  return false;
}

bool SubOpsStore::CheckAccuracyFormatSupported(ge::ConstGeTensorDescPtr c_tensor_desc_ptr,
                                               uint32_t type_index, InputOrOutputInfoPtr input_or_output_info_ptr,
                                               const vector<ge::Format> &support_formats) const {
  FE_CHECK(c_tensor_desc_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkAccurFmtSpt] cTensorDescPtr is nullptr."), return false);
  FE_CHECK(input_or_output_info_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkAccurFmtSpt] inputOrOutputInfoPtr is nullptr."), return false);
  ge::Format desc_format = static_cast<ge::Format>(ge::GetPrimaryFormat(c_tensor_desc_ptr->GetFormat()));
  if (support_formats.size() >= type_index + 1 &&
      (support_formats[type_index] == ge::FORMAT_ND || desc_format == support_formats[type_index])) {
    return true;
  }
  return false;
}

bool CompareValue(const float &a, const float &b) {
  return std::fabs(a - b) < 1e-6;
}

bool CompareValue(const double &a, const double &b) {
  return std::fabs(a - b) < 1e-9;
}

template <typename T>
bool CompareValue(const T &a, const T &b) {
  return a == b;
}


template <typename T>
bool FindValueInVector(T &value, const GeAttrValue &op_desc_attr, const vector<GeAttrValue> &info_op_attr) {
  if (op_desc_attr.GetValue<T>(value) != ge::GRAPH_SUCCESS) {
    FE_LOGD("Can not get value from OpDescAttr.");
    return false;
  }

  for (auto &info_op_attr_value : info_op_attr) {
    T info_value;
    if (info_op_attr_value.GetValue<T>(info_value) != ge::GRAPH_SUCCESS) {
      FE_LOGD("Get value from OpDescAttr not success");
      return false;
    }
    if (CompareValue(value, info_value)) {
      return true;
    }
  }
  return false;
}

bool SubOpsStore::CheckAttrValue(const GeAttrValue &op_desc_attr, const vector<GeAttrValue> &info_op_attr,
                                 GeAttrValue::ValueType attr_type) const {
  switch (attr_type) {
    case GeAttrValue::VT_INT: {
      int64_t attr_value;
      return FindValueInVector(attr_value, op_desc_attr, info_op_attr);
    }
    case GeAttrValue::VT_FLOAT: {
      float attr_value;
      return FindValueInVector(attr_value, op_desc_attr, info_op_attr);
    }
    case GeAttrValue::VT_BOOL: {
      bool attr_value = false;
      return FindValueInVector(attr_value, op_desc_attr, info_op_attr);
    }
    case GeAttrValue::VT_STRING: {
      string attr_value;
      bool ret = FindValueInVector(attr_value, op_desc_attr, info_op_attr);
      return ret;
    }
    case GeAttrValue::VT_LIST_INT: {
      vector<int64_t> attr_value;
      return FindValueInVector(attr_value, op_desc_attr, info_op_attr);
    }
    case GeAttrValue::VT_LIST_FLOAT: {
      vector<float> attr_value;
      return FindValueInVector(attr_value, op_desc_attr, info_op_attr);
    }
    case GeAttrValue::VT_LIST_BOOL: {
      vector<bool> attr_value;
      return FindValueInVector(attr_value, op_desc_attr, info_op_attr);
    }
    case GeAttrValue::VT_LIST_STRING: {
      vector<string> attr_value;
      return FindValueInVector(attr_value, op_desc_attr, info_op_attr);
    }
    default:
      FE_LOGW("Not supported ValueType %d.", attr_type);
      break;
  }
  return false;
}

bool SubOpsStore::CheckAttrSupport(const ge::NodePtr &node, const OpKernelInfo &op_kernel_info,
                                   std::string &reason) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  // get all attrs from input OpDesc
  map<string, GeAttrValue> op_desc_attrs = op_desc.GetAllAttrs();

  // get all attributes information from op kernel
  const vector<AttrInfoPtr> attrs_info = op_kernel_info.GetVecAttrInfo();
  if (attrs_info.empty()) {
    FE_LOGD("The Op: [%s] contains no Attritbue.", op_desc.GetName().c_str());
    return true;
  }

  std::ostringstream reason_oss;
  // check attr support, check each OpKernelInfo specified Attr should in OpDesc
  for (auto &iter : attrs_info) {
    string key = iter->GetAttrName();
    // check OpDesc should have each attr specified in OpKernelInfo
    if (op_desc_attrs.count(key) == 0) {
      if (iter->GetIsRequired()) {
        reason_oss << ("the attribute[" + key + "] which is required is not found in op information library");
        reason = reason_oss.str();
        FE_LOGW("Can not find AttrValue:[%s] in OpDesc (name %s)", key.c_str(), op_desc.GetName().c_str());
        return false;
      } else {
        continue;
      }
    }
    // if attr "isSupportAllValue_" flag is true, then pass this attr value check
    if (iter->GetSupportAllValue()) {
      continue;
    }

    // check attr value match
    // get Attr Value in OpDesc: GeAttrValue
    map<string, GeAttrValue>::const_iterator op_desc_attr_iter = op_desc_attrs.find(key);
    if (op_desc_attr_iter == op_desc_attrs.cend()) {
      if (iter->GetIsRequired()) {
        reason_oss << ("the attribute[" + key + "] which is required is not found in op information library");
        reason = reason_oss.str();
        FE_LOGW("Can not find Attr Value (key [%s]) in OpDesc (name [%s]) and it is required!", key.c_str(),
                op_desc.GetName().c_str());
        return false;
      } else {
        continue;
      }
    }
    GeAttrValue op_desc_attr_value = op_desc_attr_iter->second;

    // get Attr Value Vector in Op Kernel Info: vector<GeAttrValue>
    vector<GeAttrValue> attr_value_vector;
    uint32_t ret = op_kernel_info.GetAttrValueByAttrName(key, attr_value_vector);
    if (ret != SUCCESS) {
      reason_oss << ("the value of attribute[" + key + "] is not found in op information library");
      reason = reason_oss.str();
      FE_LOGW("Can not find Attr Value (key [%s]) in OpKernelInfo of op (name [%s]). Ret [%u]", key.c_str(),
              op_desc.GetName().c_str(), ret);
      return false;
    }

    // get GeAttrValue::ValueType of Attr from op_kernel_info
    GeAttrValue::ValueType attr_value_type;
    ret = op_kernel_info.GetAttrTypeByAttrName(key, attr_value_type);
    if (ret != SUCCESS) {
      reason_oss << ("the data type of attribute[" + key + "] is not found in op information library");
      reason = reason_oss.str();
      FE_LOGW("Can not find the attr value type of Attr [%s] of op (name [%s]) in OpKernelInfo; Ret value [%u]",
              key.c_str(), op_desc.GetName().c_str(), ret);
      if (ret == OP_ATTR_EMPTY_IN_OP_KERNEL_INFO) {
        return true;
      }
      return false;
    }

    // check if op_desc_attr_value is in info_attr_value_list
    if (!CheckAttrValue(op_desc_attr_value, attr_value_vector, attr_value_type)) {
      reason_oss << ("the matched value of attribute[" + key + "] is not found in op desc");
      reason = reason_oss.str();
      FE_LOGD("Match attr value of attr name %s type [%u] from OpKernelInfo not success.", key.c_str(),
              attr_value_type);
      return false;
    }
  }
  FE_LOGD("check OpDesc: %s attr value supported", op_desc.GetName().c_str());
  return true;
}

bool SubOpsStore::CheckParamType(const ge::NodePtr &node, OpKernelInfoPtr op_kernel_info_ptr,
                                 const map<uint32_t, string> &input_index_name_map,
                                 const map<uint32_t, string> &output_index_name_map, std::string &reason) const {
  FE_CHECK(op_kernel_info_ptr == nullptr,
           REPORT_FE_ERROR("[ChkSpt][FEChk][ChkParmType] opKernelInfoPtr is nullptr."), return false);
  vector<InputOrOutputInfoPtr> all_input_info_vector = op_kernel_info_ptr->GetAllInputInfo();
  vector<InputOrOutputInfoPtr> all_output_info_vector = op_kernel_info_ptr->GetAllOutputInfo();
  if (!CheckAllTensorsParamType(node, true, all_input_info_vector, input_index_name_map, reason)) {
    return false;
  }

  return CheckAllTensorsParamType(node, false, all_output_info_vector, output_index_name_map, reason);
}

bool SubOpsStore::CheckAllTensorsParamType(const ge::NodePtr &node, bool is_input,
                                           const vector<InputOrOutputInfoPtr> &all_tesnors_info,
                                           const map<uint32_t, string> &index_name_map, std::string &reason) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  int index = 0;
  std::ostringstream reason_oss;
  string in_or_out = IS_INPUT_TO_STRING(is_input);
  for (auto &tensor_info : all_tesnors_info) {
    int32_t counter = 0;
    for (auto &index_name : index_name_map) {
      if (index_name.second == tensor_info->GetName()) {
        counter++;
      }
    }
    if ((tensor_info->GetParamType() == REQUIRED) && (counter != 1)) {
      reason_oss << "the quantity of required input[" << tensor_info->GetName() << "] is not one";
      reason = reason_oss.str();
      FE_LOGD("%s [%u] which named [%s] is required but its quantity is [%d] instead of 1. node is [%s] [%s])",
              in_or_out.c_str(), index, tensor_info->GetName().c_str(), counter, op_desc.GetName().c_str(),
              op_desc.GetType().c_str());
      return false;
    }
    if ((tensor_info->GetParamType() == DYNAMIC) && (counter == 0)) {
      reason_oss << "the quantity of dynamic input[" << tensor_info->GetName() << "] should be larger than zero";
      reason = reason_oss.str();
      FE_LOGD("%s [%u] which named [%s] is dynamic but its quantity is 0 (should be larger than 0). node is [%s] [%s])",
              in_or_out.c_str(), index, tensor_info->GetName().c_str(), op_desc.GetName().c_str(),
              op_desc.GetType().c_str());
      return false;
    }
    index++;
  }
  return true;
}

void SubOpsStore::LogAllFormatAndDtype(const SupportedFormatAndDtype &info, const string &tensor_name,
                                       std::ostringstream &reason_oss, string &reason) const {
  auto all_data_type = StringUtils::IntegerVecToString(info.support_data_types_map.at(tensor_name));
  auto all_formats = StringUtils::IntegerVecToString(info.suppport_formats_map.at(tensor_name));
  reason_oss << "All supported data type and format of tensor " << tensor_name << " is: " << endl
             << "Data Type: " << all_data_type << endl
             << "Format:" << all_formats <<endl;
  reason = reason_oss.str();
}

bool SubOpsStore::CheckFormatAndDtypeNormalMode(const ge::NodePtr &node, const string &name,
                                                const ge::GeTensorDescPtr &tensor_desc,
                                                const SupportedFormatAndDtype &info, string &reason) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  std::ostringstream reason_oss;
  InputOrOutputInfoPtr tensor_kernel;
  string in_or_out = IS_INPUT_TO_STRING(info.is_input);
  if (info.op_kernel_info_ptr->GetTensorInfoByName(info.is_input, name, tensor_kernel) != SUCCESS) {
    reason_oss << "the detailed info of " << in_or_out << " [" << name
               << "] of this op is not found in op information library.";
    reason = reason_oss.str();
    FE_LOGW("Get %s kernel info of Op (name [%s] type [%s] not success!", in_or_out.c_str(), op_desc.GetName().c_str(),
            op_desc.GetType().c_str());
    return false;
  }

  string tensor_name = tensor_kernel->GetUniqueName();
  auto iter_types_map = info.support_data_types_map.find(tensor_name);
  if (iter_types_map == info.support_data_types_map.end()) {
    REPORT_FE_ERROR("[ChkSpt][FEChk][ChkTensor][Node %s, type %s] fail to find the support data_types for the %s [%s].",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str(), in_or_out.c_str(), tensor_name.c_str());
    return false;
  }

  if (!CheckDtypeSupported(node, tensor_desc, tensor_kernel, iter_types_map->second, info.op_kernel_info_ptr)) {
    string dtype = ge::TypeUtils::DataTypeToSerialString(tensor_desc->GetDataType());
    reason_oss << "data type " << dtype << " of " << in_or_out << " [" << name << "] is not supported. ";

    FE_LOGD("[ChkSpt][FEChk][ChkTensor][Node %s, type %s] %s %s's data type [%s] is not supported by ops kernel.",
            op_desc.GetName().c_str(), op_desc.GetType().c_str(), in_or_out.c_str(), tensor_name.c_str(),
            dtype.c_str());
    LogAllFormatAndDtype(info, tensor_name, reason_oss, reason);
    return false;
  }

  auto iter_formats_map = info.suppport_formats_map.find(tensor_name);
  if (iter_formats_map == info.suppport_formats_map.end()) {
    REPORT_FE_ERROR("[ChkSpt][FEChk][ChkTensor][Node %s, type %s] fail to find the support formats for the %s [%s].",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str(), in_or_out.c_str(), tensor_name.c_str());
    return false;
  }

  if (!CheckFormatSupported(node, tensor_desc, tensor_kernel, iter_formats_map->second)) {
    string format = ge::TypeUtils::FormatToSerialString(tensor_desc->GetFormat());
    reason_oss << "format " << format << " of " << in_or_out << " [" << name << "] is not supported. ";

    FE_LOGD("[ChkSpt][FEChk][ChkTensor][Node %s, type %s] %s %s's format [%s] is not supported by ops kernel.",
            op_desc.GetName().c_str(), op_desc.GetType().c_str(), in_or_out.c_str(), tensor_name.c_str(),
            format.c_str());
    LogAllFormatAndDtype(info, tensor_name, reason_oss, reason);
    return false;
  }

  return true;
}

bool SubOpsStore::CheckInputConstValueDepend(SupportedFormatAndDtype &info, const string &input_name,
                                             const bool &is_input_const) const {
  std::ostringstream reason_oss;
  InputOrOutputInfoPtr tensor_kernel;
  if (info.op_kernel_info_ptr->GetTensorInfoByName(info.is_input, input_name, tensor_kernel) != SUCCESS) {
    reason_oss << "the detail info of input[" << input_name << "] of this op is not found in op information library";
    info.reason = reason_oss.str();
    FE_LOGW("Fail to get the tensor kernel of input[%s].", input_name.c_str());
    return false;
  }

  if (tensor_kernel->GetConstValueDepend() == CONST_REQUIRED && !is_input_const) {
    reason_oss << "The value depend of input[" << input_name << "] is required,";
    reason_oss << " but this input does not linked to a const or constant node";
    info.reason = reason_oss.str();
    FE_LOGI("%s", info.reason.c_str());
    return false;
  }
  return true;
}

void SubOpsStore::GetIsInputConstNodeVec(const ge::NodePtr &node, vector<bool> &is_input_const_vec) const {
  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    if (in_anchor == nullptr) {
      is_input_const_vec.push_back(false);
      continue;
    }
    auto out_anchor = in_anchor->GetPeerOutAnchor();
    if (out_anchor == nullptr || out_anchor->GetOwnerNode() == nullptr) {
      is_input_const_vec.push_back(false);
      continue;
    }
    auto nod = out_anchor->GetOwnerNode();
    std::string node_type = nod->GetType();
    if (node_type == CONSTANT || node_type == CONSTANTOP) {
      is_input_const_vec.push_back(true);
    } else if (FeGraphUtils::IsSubGraphData(nod->GetOpDesc())) {
      bool is_const_node = FeGraphUtils::CheckSubGraphDataForConstValue(nod);
      is_input_const_vec.push_back(is_const_node);
    } else {
      is_input_const_vec.push_back(false);
    }
  }
}

bool SubOpsStore::CheckInputSupported(const ge::NodePtr &node, uint32_t input_size,
                                      SupportedFormatAndDtype &info) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  std::ostringstream reason_oss;
  // check input%d.dtype & input%d.shape supported or not
  vector<bool> is_input_const_vec;
  GetIsInputConstNodeVec(node, is_input_const_vec);

  for (uint32_t in_index = 0; in_index < input_size; ++in_index) {
    auto tensor_desc = op_desc.MutableInputDesc(in_index);
    if (tensor_desc == nullptr) {
      continue;
    }
    IndexNameMap::const_iterator input_index_name_map_iter = info.input_index_name_map.find(in_index);
    if (input_index_name_map_iter == info.input_index_name_map.cend()) {
      reason_oss << "the name of input[" << in_index << "] is not found";
      info.reason = reason_oss.str();
      REPORT_FE_ERROR("[ChkSpt][FEChk][ChkInSpt] Can not find the input index %d in map whose size is %zu",
                      in_index, info.input_index_name_map.size());
      return false;
    }
    string input_name = input_index_name_map_iter->second;
    info.is_input = true;
    if (!CheckFormatAndDtypeNormalMode(node, input_name, tensor_desc, info, info.reason)) {
      return false;
    }
    bool is_input_const = false;
    if (ge::AttrUtils::HasAttr(tensor_desc, ge::ATTR_NAME_VALUE)) {
      // fuzz build, GE set const value to op_desc by tensor attr, change const node to data node
      FE_LOGD("The index[%u] of node[%s] is const input.", in_index, op_desc.GetName().c_str());
      is_input_const = true;
    } else {
      if (in_index < is_input_const_vec.size()) {
        is_input_const = is_input_const_vec[in_index];
      } else {
        FE_LOGW(
            "The index[%u] is greater than the size of is_input_const_vec[%zu]."
            "The is_input_const of this input will be set as [false].",
            in_index, is_input_const_vec.size());
      }
    }

    if (!CheckInputConstValueDepend(info, input_name, is_input_const)) {
      return false;
    }
  }
  return true;
}

bool SubOpsStore::CheckOutputSupported(const ge::NodePtr &node, uint32_t output_size,
                                       SupportedFormatAndDtype &info) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  std::ostringstream reason_oss;
  // check output%d.dtype & output%d.shape supported or not
  for (uint32_t out_index = 0; out_index < output_size; ++out_index) {
    ge::GeTensorDescPtr output_desc = op_desc.MutableOutputDesc(out_index);
    if (output_desc == nullptr) {
      REPORT_FE_ERROR("[ChkSpt][FEChk][ChkOutSpt] output_desc is null.");
      return false;
    }
    IndexNameMap::const_iterator output_index_name_map_iter = info.output_index_name_map.find(out_index);
    if (output_index_name_map_iter == info.output_index_name_map.cend()) {
      reason_oss << "the name of output[" << out_index << "] is not found";
      info.reason = reason_oss.str();
      REPORT_FE_ERROR("[ChkSpt][FEChk][ChkOutSpt] Can not find the output index %u in map.", out_index);
      return false;
    }
    string output_name = output_index_name_map_iter->second;
    info.is_input = false;
    if (!CheckFormatAndDtypeNormalMode(node, output_name, output_desc, info, info.reason)) {
      return false;
    }
  }
  return true;
}

bool GetTensorName(SupportedFormatAndDtype &info, const string &input_or_output, uint32_t index,
                   const IndexNameMap &index_name_map, string &name) {
  std::ostringstream reason_oss;
  auto index_name_map_iter = index_name_map.find(index);
  if (index_name_map_iter == index_name_map.end()) {
    reason_oss << "the name of " << input_or_output << " [" << index << "] is not found";
    info.reason = reason_oss.str();
    REPORT_FE_ERROR("[ChkSpt][FEChk][GetTensorNm]Can not find the %s index %d in map whose size is %zu",
                    input_or_output.c_str(), index, info.input_index_name_map.size());
    return false;
  }
  name = index_name_map_iter->second;
  return true;
}

bool SubOpsStore::CheckSingleTensorSupportedAccurateMode(const ge::NodePtr &node, uint32_t index, uint32_t type_index,
                                                         SupportedFormatAndDtype &info, bool &check_flag) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  string input_or_output = IS_INPUT_TO_STRING(info.is_input);
  std::ostringstream reason_oss;
  ge::ConstGeTensorDescPtr tensor_desc;
  string name;
  if (info.is_input) {
    tensor_desc = op_desc.GetInputDescPtr(index);
    if (!GetTensorName(info, input_or_output, index, info.input_index_name_map, name)) {
      return false;
    }
  } else {
    tensor_desc = op_desc.GetOutputDescPtr(index);
    if (!GetTensorName(info, input_or_output, index, info.output_index_name_map, name)) {
      return false;
    }
  }
  if (tensor_desc == nullptr) {
    REPORT_FE_ERROR("[ChkSpt][FEChk][ChkSigTensSptAccrMode] tensor_desc is null.");
    return false;
  }
  InputOrOutputInfoPtr tensor_info_ptr;
  if (info.op_kernel_info_ptr->GetTensorInfoByName(info.is_input, name, tensor_info_ptr) != SUCCESS) {
    reason_oss << "the detail info of " << input_or_output << " [" << name
               << "] of this op is not found in op information library";
    info.reason = reason_oss.str();
    FE_LOGW("Get %s kernel info of Op (name [%s] type [%s] not success!", input_or_output.c_str(),
            op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return false;
  }

  string unique_name = tensor_info_ptr->GetUniqueName();
  if (info.support_data_types_map.find(unique_name) == info.support_data_types_map.end()) {
    REPORT_FE_ERROR("[ChkSpt][FEChk][ChkSigTensSptAccrMode][Op %s,type %s] fail to find the support data_types for \
                    the %s [%s].", op_desc.GetName().c_str(),
                    op_desc.GetType().c_str(), input_or_output.c_str(), unique_name.c_str());
    return false;
  }
  if (info.suppport_formats_map.find(unique_name) == info.suppport_formats_map.end()) {
    REPORT_FE_ERROR("[ChkSpt][FEChk][ChkSigTensSptAccrMode][Op %s,type %s] fail to find the support formats for the \
                    output [%s].", op_desc.GetName().c_str(), op_desc.GetType().c_str(), unique_name.c_str());
    return false;
  }

  check_flag = !CheckAccuracyDtypeSupported(tensor_desc, type_index, tensor_info_ptr,
                                            info.support_data_types_map.at(unique_name)) ||
               !CheckAccuracyFormatSupported(tensor_desc, type_index, tensor_info_ptr,
                                             info.suppport_formats_map.at(unique_name));
  return true;
}

bool SubOpsStore::CheckSingleTensorAccurateMode(uint32_t size, uint32_t type_index, const ge::NodePtr &node,
                                                SupportedFormatAndDtype &info, bool &need_continue) const {
  for (uint32_t in_index = 0; in_index < size; ++in_index) {
    bool check_flag = false;
    if (!CheckSingleTensorSupportedAccurateMode(node, in_index, type_index, info, check_flag)) {
      return false;
    }
    if (check_flag) {
      need_continue = true;
      break;
    }
  }
  return true;
}

bool SubOpsStore::CheckAllTensorsSupportedAccurateMode(const ge::NodePtr &node, SupportedFormatAndDtype &info) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_input_desc_ptr_vistor = op_desc.GetAllInputsDescPtr();
  ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_output_desc_ptr_vistor = op_desc.GetAllOutputsDescPtr();
  auto input_size = static_cast<uint32_t>(all_input_desc_ptr_vistor.size());
  auto output_size = static_cast<uint32_t>(all_output_desc_ptr_vistor.size());

  UnSupportedReason reason_tmp;
  bool ret = GetInputOutputNameMap(node, info.op_kernel_info_ptr, info.input_index_name_map,
                                   info.output_index_name_map, reason_tmp);
  if (!ret) {
    info.reason = reason_tmp.reason;
    return false;
  }

  if (info.op_kernel_info_ptr->GetAllInputInfo().size() == 0) {
    info.reason = "inputs size of op_kernel_info is 0";
    return false;
  }

  uint32_t type_size = static_cast<uint32_t>(info.support_data_types_map.cbegin()->second.size());
  for (uint32_t type_index = 0; type_index < type_size; type_index++) {
    bool need_continue = false;
    need_continue = false;
    // check input%d.dtype & input%d.shape supported or not
    info.is_input = true;
    if (!CheckSingleTensorAccurateMode(input_size, type_index, node, info, need_continue)) {
      return false;
    }
    if (need_continue) {
      continue;
    }
    info.is_input = false;
    // check output%d.dtype & output%d.shape supported or not
    if (!CheckSingleTensorAccurateMode(output_size, type_index, node, info, need_continue)) {
      return false;
    }
    if (need_continue) {
      continue;
    }
    return true;
  }
  info.reason = "The format and dtype is not precisely equivalent to format and dtype in op information library";
  return false;
}

void SetReason(string reason_str, OpNotSupportedReasonID id, UnSupportedReason &reason) {
  reason.reason += reason_str;
  reason.reason_id = static_cast<uint64_t>(id);
}

bool SubOpsStore::CheckOpSupported(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr,
                                   const bool &is_dynamic_impl, UnSupportedReason &reason) const {
  if (!op_kernel_info_ptr->GetNeedCheckSupportFlag()) {
    FE_LOGD("[ChkSpt][OpChk][Node %s, type %s] This op is not configure to check it's implementation.",
            node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
    return true;
  }
  OpStoreAdapterPtr op_store_adapter = nullptr;
  FE_CHECK(op_store_adapter_manager_ptr_ == nullptr,
           REPORT_FE_ERROR("[GraphOpt][SubChk][GetTbeAdap] op_store_adapter_manager_ptr_ is null."), return FAILED);
  if (op_store_adapter_manager_ptr_->GetOpStoreAdapter(sub_store_info_.op_impl_type, op_store_adapter) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][SubChk][GetTbeAdap] Failed to get op adapter by op impl type[%d].",
                    sub_store_info_.op_impl_type);
    return false;
  }

  std::ostringstream reason_oss;
  std::string unsupported_reason_in_op = te::NO_NEED_REASON;
  if (!op_store_adapter->CheckSupport(node_ptr, op_kernel_info_ptr, is_dynamic_impl, unsupported_reason_in_op)) {
    OpNotSupportedReasonID reason_id = OpNotSupportedReasonID::EN_OPERATOR_NOT_SUPPORT;
    FE_LOGI("[ChkSpt][OpChk][Node %s, type %s] This op is not supported inside its implementation. Reason is %s.",
            node_ptr->GetName().c_str(), node_ptr->GetType().c_str(), unsupported_reason_in_op.c_str());
    reason_oss << "Op " << node_ptr->GetName() << " type " << node_ptr->GetType().c_str()
               << " is not supported in the operator's implementation. Reason is: " << unsupported_reason_in_op << ".";
    uint64_t offset_num = sub_store_info_.op_impl_type * NOT_SUPPORTED_REASON_OFFSET_BIT_NUM;
    reason.reason_id += (static_cast<uint64_t>(reason_id) << offset_num);
    reason.reason += reason_oss.str();
    return false;
  }

  return true;
}

bool SubOpsStore::CheckSubStoreSupported(const ge::NodePtr &node, OpKernelInfoPtr &op_kernel_info_ptr,
                                         UnSupportedReason &reason, const CheckSupportMode &check_mode,
                                         const bool &check_unknown_shape) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  string check_type = check_unknown_shape ? "[Dynamic shape check]: " : "[Static shape check]:";
  reason.reason += check_type;
  FE_LOGI("[ChkSpt][FEChk][SubChk][Node %s, type %s] %s in lib %s by check_mode %u.", op_desc.GetName().c_str(),
          op_desc.GetType().c_str(), check_type.c_str(), sub_store_info_.fe_ops_store_name.c_str(),
          static_cast<uint32_t>(check_mode));

  ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_input_desc_ptr_vistor = op_desc.GetAllInputsDescPtr();
  ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_output_desc_ptr_vistor = op_desc.GetAllOutputsDescPtr();
  auto input_size = static_cast<uint32_t>(all_input_desc_ptr_vistor.size());
  auto output_size = static_cast<uint32_t>(all_output_desc_ptr_vistor.size());

  SupportedFormatAndDtype info(op_kernel_info_ptr, "");
  if (!GetInputOutputNameMap(node, op_kernel_info_ptr, info.input_index_name_map, info.output_index_name_map,
                             reason)) {
    return false;
  }

  if (format_dtype_querier_ptr_->GetSupportFormatAndDtype(op_kernel_info_ptr, op_desc, check_unknown_shape,
                                                          info.suppport_formats_map, info.support_data_types_map)
      != SUCCESS) {
    FE_LOGW("[GraphOpt][Setcheck][CheckSubSupt] Fail to get the GetSupportFormatAndDtype, return false.");
    return false;
  }

  if (check_mode == CheckSupportMode::ACCURACY_MODE) {
    if (!CheckAllTensorsSupportedAccurateMode(node, info)) {
      SetReason(info.reason, OpNotSupportedReasonID::EN_INPUTS_AND_OUTPUTS_NOT_ACCURACY_SUPPORT, reason);
      return false;
    }
  } else {
    if (!CheckInputSupported(node, input_size, info)) {
      SetReason(info.reason, OpNotSupportedReasonID::EN_INPUTS_NOT_SUPPORT, reason);
      FE_LOGD("Node %s, type %s's input is not supported in %s.", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
              sub_store_info_.fe_ops_store_name.c_str());
      return false;
    }
    if (!CheckOutputSupported(node, output_size, info)) {
      SetReason(info.reason, OpNotSupportedReasonID::EN_OUTPUTS_NOT_SUPPORT, reason);
      FE_LOGD("Node %s, type %s's output is not supported in %s.", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
              sub_store_info_.fe_ops_store_name.c_str());
      return false;
    }
  }
  // check attrs are supported or not
  if (!CheckAttrSupport(node, *(op_kernel_info_ptr.get()), reason.reason)) {
    reason.reason_id = static_cast<uint64_t>(OpNotSupportedReasonID::EN_ATTRS_NOT_SUPPORT);
    FE_LOGD("Attr is not supported in %s.", sub_store_info_.fe_ops_store_name.c_str());
    return false;
  }

  // input or output dynamic can have more than 1, required must equal 1,optional not check;
  if (!CheckParamType(node, op_kernel_info_ptr, info.input_index_name_map, info.output_index_name_map,
                      reason.reason)) {
    reason.reason_id = static_cast<uint64_t>(OpNotSupportedReasonID::EN_PARAM_TYPE_NOT_SUPPORT);
    FE_LOGD("Parameter type is not supported in %s.", sub_store_info_.fe_ops_store_name.c_str());
    return false;
  }

  // check op supported
  if (!CheckOpSupported(node, op_kernel_info_ptr, check_unknown_shape, reason)) {
    return false;
  }

  // set is_ge_op attr
  if (op_kernel_info_ptr->GetOpInfo().opFileName == NULL_OP_FILE ||
      op_kernel_info_ptr->GetOpInfo().opFileName == AICORE_NULL) {
    (void)ge::AttrUtils::SetBool(op_desc_ptr, IS_GE_OP, true);
  }

  FE_LOGI("[ChkSpt][FEChk][%s][Node %s, type %s] is support by op store [%s] by check_mode %u.",
          check_type.c_str(), op_desc.GetName().c_str(), op_desc.GetType().c_str(),
          sub_store_info_.fe_ops_store_name.c_str(), static_cast<uint32_t>(check_mode));
  return true;
}

bool SubOpsStore::IsAllDtypeExpected(ge::DataType dtype_expected, ge::DataType dtype_unexpected,
                                     const vector<ge::DataType> &dtypes) const {
  for (const auto &dtype : dtypes) {
    if (dtype == dtype_unexpected) {
      return false;
    }
  }
  for (const auto &dtype : dtypes) {
    if (dtype == dtype_expected) {
      return true;
    }
  }
  return false;
}
}  // namespace fe
