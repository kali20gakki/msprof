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

#include "common/op_info_common.h"
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "common/aicore_util_attr_define.h"
#include "common/configuration.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_type_utils.h"
#include "common/graph/fe_graph_utils.h"
#include "common/format/axis_name_util.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {
const std::string FULLYCONNECTION = "FullyConnection";
const std::string FULLCONNECTION = "FullConnection";
const std::string FLATTEN = "Flatten";
const std::string MATMUL = "MatMul";
const std::unordered_set<ge::Format> kSupportedTransType = {ge::FORMAT_ND, ge::FORMAT_FRACTAL_NZ,
                                                            ge::FORMAT_ND_RNN_BIAS, ge::FORMAT_FRACTAL_ZN_RNN};
static const uint32_t CHECK_WEIGHT_MAX_COUNT = 100;

bool CheckInputSubString(const std::string& op_desc_input_name, const std::string& info_input_name) {
  size_t length_of_info_input_name = info_input_name.length();
  size_t length_of_op_desc_input_name = op_desc_input_name.length();
  if (length_of_info_input_name > length_of_op_desc_input_name) {
    return false;
  } else {
    /* LengthOfInfoInputName less than length_of_op_desc_input_name */
    if (op_desc_input_name.substr(0, length_of_info_input_name) == info_input_name) {
      /* Get from the first char after "infoInputName"
       * to the end of op_desc_input_name */
      std::string rest = op_desc_input_name.substr(length_of_info_input_name);
      if (rest.empty()) {
        return true;
      }
      if (StringUtils::IsInteger(rest)) {
        return true;
      } else {
        /* In other cases, we consider this input name of op_desc is illegal.
         * Digits should only appears at the end of name
         * as index. */
        FE_LOGW("Illegal input name [%s] in Opdesc during comparison with inputname [%s].", op_desc_input_name.c_str(),
                info_input_name.c_str());
        return false;
      }
    } else {
      return false;
    }
  }
}

void CheckSpecialCases(const std::vector<InputOrOutputInfoPtr>& input_or_output_info, IndexNameMap& index_name_map,
                       uint32_t index, uint32_t op_desc_input_or_output_size, bool& has_found) {
  if (input_or_output_info.size() == 1 && input_or_output_info[0]->GetParamType() == DYNAMIC) {
    has_found = true;
    index_name_map[index] = input_or_output_info[0]->GetName();
  }
  if (!has_found) {
    size_t optional_count = 0;
    // Find the count of input or output whose parameter type is optional
    for (size_t loop = 0; loop < input_or_output_info.size(); loop++) {
      if (input_or_output_info[loop]->GetParamType() == OPTIONAL) {
        if (CheckSizetAddOverFlow(optional_count, 1) != SUCCESS) {
          REPORT_FE_ERROR("[GraphOpt][Setcheck][ChkSplCases] The addition between [%lu] and 1 is overflow.",
                          optional_count);
          return;
        }
        optional_count++;
      }
    }
    // If more than one optional input or output is found, can not decide which
    // one should be choose.
    if ((op_desc_input_or_output_size >= input_or_output_info.size() - optional_count) &&
        (op_desc_input_or_output_size <= input_or_output_info.size())) {
      for (auto const& ele : input_or_output_info) {
        uint32_t index_in_op_kernel = ele->GetIndex();
        if (index == index_in_op_kernel) {
          has_found = true;
          index_name_map[index] = ele->GetName();
          break;
        }
      }
    }
  }
}

Status GetInputIndexNameMap(const ge::OpDesc &op_desc, const OpKernelInfo &op_kernel_info,
                            IndexNameMap &input_map) {
  const std::vector<InputOrOutputInfoPtr>& input_info = op_kernel_info.GetAllInputInfo();
  size_t input_size_in_op_kernel = input_info.size();
  if (!input_size_in_op_kernel) {
    return fe::SUCCESS;
  }
  auto input_desc_size = op_desc.GetAllInputsSize();
  for (size_t i = 0; i < input_desc_size; i++) {
    std::string op_desc_input_name = op_desc.GetInputNameByIndex(i);
    FE_LOGD("Op[name:%s,type:%s] op desc index is %zu, desc name is %s",
            op_desc.GetName().c_str(),
            op_desc.GetType().c_str(), i, op_desc_input_name.c_str());
    bool has_found = false;

    for (auto const& ele : input_info) {
      std::string info_input_name = ele->GetName();
      if (CheckInputSubString(op_desc_input_name, info_input_name)) {
        has_found = true;
        input_map[i] = info_input_name;
        break;
      }
    }
    // Now op node info is not created by IR, so many node name is none or is
    // wrong. Fix this problem by match the input count with the input count in op kernel info
    bool input_size_match_flag =
        (input_desc_size == input_size_in_op_kernel ||
         (input_desc_size == input_size_in_op_kernel - 1 && op_kernel_info.GetOpStoreImplType() == EN_IMPL_PLUGIN_TBE));
    if (!has_found && (input_size_match_flag)) {
      for (auto const& ele : input_info) {
        uint32_t index = ele->GetIndex();
        if (index == i) {
          has_found = true;
          input_map[i] = ele->GetName();
          break;
        }
      }
    }
    if (!has_found) {
      CheckSpecialCases(input_info, input_map, i, op_desc.GetAllInputsSize(), has_found);
    }
    if (!has_found) {
      FE_LOGI("Input name[%s] index %zu is not found in kernel of [%s]. Size in Opdesc is [%zu] and in kernel is [%zu]",
              op_desc_input_name.c_str(), i, op_kernel_info.GetOpType().c_str(), op_desc.GetAllInputsSize(),
              input_info.size());
      return fe::FAILED;
    }
  }
  return fe::SUCCESS;
}

Status GetOutputIndexNameMap(const ge::OpDesc &op_desc, const OpKernelInfo &op_kernel_info, IndexNameMap &output_map) {
  const std::vector<InputOrOutputInfoPtr>& output_info = op_kernel_info.GetAllOutputInfo();
  size_t output_size_in_op_kernel = output_info.size();
  if (!output_size_in_op_kernel) {
    return fe::SUCCESS;
  }
  for (size_t i = 0; i < op_desc.GetAllOutputsDescSize(); i++) {
    std::string op_desc_output_name = op_desc.GetOutputNameByIndex(i);
    bool has_found = false;
    auto output0_op_kernel_info = output_info[0];
    if (output_size_in_op_kernel == 1 && output_info[0]->GetParamType() == DYNAMIC) {
      has_found = true;
      output_map[i] = output0_op_kernel_info->GetName();
      continue;
    }
    for (auto const& ele : output_info) {
      std::string info_output_name = ele->GetName();

      if (CheckInputSubString(op_desc_output_name, info_output_name)) {
        has_found = true;
        output_map[i] = info_output_name;
        break;
      }
    }
    // Now op node info is not created by IR, so many node name is none or is wrong.
    // Fix this problem by match the input count with the input count in op kernel info
    if (!has_found && op_desc.GetOutputsSize() == output_info.size()) {
      for (auto const& ele : output_info) {
        uint32_t index = ele->GetIndex();
        if (index == i) {
          has_found = true;
          output_map[i] = ele->GetName();
          break;
        }
      }
    }
    if (!has_found) {
      CheckSpecialCases(output_info, output_map, i, op_desc.GetAllOutputsDescSize(), has_found);
    }
    if (!has_found) {
      FE_LOGI("Output name[%s] index %zu is not found in kernel of [%s]. Size in Opdesc is [%u] and in kernel is [%zu]",
              op_desc_output_name.c_str(), i, op_kernel_info.GetOpType().c_str(), op_desc.GetAllOutputsDescSize(),
              output_info.size());
      return fe::FAILED;
    }
  }
  return fe::SUCCESS;
}

Status GetInputOutputNameMap(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                             IndexNameMap &input_map, IndexNameMap &output_map) {
  // feed all inputs to TbeOpInfo
  FE_CHECK(op_kernel_info_ptr == nullptr,
           REPORT_FE_ERROR("[GraphOpt][Setcheck][GetInOutNm] opKernelInfoPtr is nullptr."),
           return FAILED);

  if (GetInputIndexNameMap(op_desc, *op_kernel_info_ptr, input_map) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetInOutNm] Failed to get input index name map for op %s.",
                    op_desc.GetName().c_str());
    return FAILED;
  }

  if (GetOutputIndexNameMap(op_desc, *op_kernel_info_ptr, output_map) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetInOutNm] Failed to get output index name map for op %s.",
                    op_desc.GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status GetOutputNameMap(const ge::OpDesc& op_desc, const OpKernelInfoPtr& op_kernel_info_ptr,
                        IndexNameMap& output_map) {
  FE_CHECK(op_kernel_info_ptr == nullptr, REPORT_FE_ERROR("[GraphOpt][Setcheck][GetOutNm] opKernelInfoPtr is nullptr."),
           return FAILED);
  if (GetOutputIndexNameMap(op_desc, *op_kernel_info_ptr, output_map) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Setcheck][GetOutNm] Failed to GetOutputIndexNameMap for op %s.",
                    op_desc.GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

bool GetInputOutputNameMap(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                           IndexNameMap &input_map, IndexNameMap &output_map, UnSupportedReason &reason) {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  FE_CHECK(op_kernel_info_ptr == nullptr, FE_LOGI("opKernelInfoPtr must not be nullptr."), return false);
  if (GetInputIndexNameMap(op_desc, *(op_kernel_info_ptr.get()), input_map) != SUCCESS) {
    reason.reason = "the inputs in op desc and inputs in op information library are not matched";
    reason.reason_id = static_cast<uint64_t>(OpNotSupportedReasonID::EN_INPUTS_NUM_NOT_MATCH);
    FE_LOGW("[ChkSpt][GetInputName]Failed to GetInputIndexNameMap for op %s.", op_desc.GetName().c_str());
    return false;
  }

  if (GetOutputIndexNameMap(op_desc, *(op_kernel_info_ptr.get()), output_map) != SUCCESS) {
    reason.reason = "the outputsin op desc and outputs in op information library are not matched";
    reason.reason_id = static_cast<uint64_t>(OpNotSupportedReasonID::EN_OUTPUTS_NUM_NOT_MATCH);
    FE_LOGW("[ChkSpt][GetOutputName]Failed to GetOutputIndexNameMap for op %s.", op_desc.GetName().c_str());
    return false;
  }
  return true;
}

bool IsExpandNecessary(std::vector<int64_t>& dims, const ge::Format& original_format, const ge::Format& final_format,
                       const std::string& reshape_type, size_t& full_size) {
  /* 1. Check whether the old dim size is full. Full size is not necessary for expand. */
  auto old_dims_size = static_cast<uint32_t>(dims.size());
  auto iter_full_size = kFullSizeOfFormat.find(original_format);
  if (iter_full_size == kFullSizeOfFormat.end()) {
    FE_LOGW("Original Format %u is invalid", original_format);
    return false;
  } else {
    if (old_dims_size >= iter_full_size->second) {
      return false;
    }
  }
  /* 2. Check whether the final format does not need expanding dimension. */
  bool no_need_reshape_flag = reshape_type == kReshapeTypeForbidden;
  if (no_need_reshape_flag) {
    return false;
  }
  /* check whether support the transdate from ND to final_format */
  if (IsSupportedTransType(original_format, final_format)) {
    return false;
  }
  full_size = iter_full_size->second;
  return true;
}

Status GetDefaultReshapeType(const ge::Format& original_format, size_t old_dims_size, std::string& reshape_type) {
  auto rsp_tp_all_format = kDefaultReshapeType.find(old_dims_size);
  if (rsp_tp_all_format == kDefaultReshapeType.end()) {
    FE_LOGW("dim size %zu is invalid.", old_dims_size);
    return FAILED;
  }

  auto iter_rsp_tp = rsp_tp_all_format->second.find(original_format);
  if (iter_rsp_tp == rsp_tp_all_format->second.end()) {
    FE_LOGW("Cannot find default reshape type for %u.", original_format);
    return FAILED;
  }

  reshape_type = iter_rsp_tp->second;
  return SUCCESS;
}

void ExpandByReshapeType(std::vector<int64_t>& dims, const std::string& op_name,
                         const ge::Format& original_format,
                         size_t full_size, const uint32_t& tensor_index,
                         const std::string& reshape_type) {
  FE_LOGD("Expand tensor %u of %s by reshape type %s.", tensor_index, op_name.c_str(), reshape_type.c_str());
  auto old_dims_size = dims.size();
  if (reshape_type == "CN") {
    /* If the reshape_type is set as CN, we will consider the original format
     * is HWCN. */
    vector<int64_t> new_dims;
    if (old_dims_size < SIZE_OF_CN) {
      FE_LOGW("oldDimsSize %zu is less than 2. Reshape type is %s", dims.size(), reshape_type.c_str());
      return;
    }
    new_dims.push_back(1);
    new_dims.push_back(1);
    new_dims.push_back(dims[0]);
    new_dims.push_back(dims[1]);
    dims.swap(new_dims);
    /* In this case the final format must be HWCN,
     * we just return SUCCESS */
    return;
  } else {
    /* Build a array with all 1 of full size. Then we will substitute some of the 1 with the original axis value. */
    vector<int64_t> new_dims;
    for (size_t i = 0; i < full_size; i++) {
      new_dims.emplace_back(1);
    }

    auto iter_axis_name_index = FE_AXIS_INDEX_OF_FORMAT.find(original_format);
    if (iter_axis_name_index == FE_AXIS_INDEX_OF_FORMAT.end()) {
      FE_LOGW("Cannot find axis index name map value of original format %u of tensor %u of %s.", original_format,
              tensor_index, op_name.c_str());
      return;
    }
    for (size_t i = 0; i < old_dims_size; i++) {
      /* The length of reshape type is larger than the dims. */
      string axis_str(1, reshape_type.at(i));
      auto iter_axis_index = iter_axis_name_index->second.find(axis_str);
      if (iter_axis_index == iter_axis_name_index->second.end()) {
        FE_LOGW("Invalid reshape type %s for tensor %u of %s.", reshape_type.c_str(), tensor_index, op_name.c_str());
        return;
      }
      int32_t index = iter_axis_index->second;
      if ((index < 0) || (index >= static_cast<int32_t>(full_size))) {
        FE_LOGW("Index of %s is %d which is larger than the full size %zu", axis_str.c_str(), index, full_size);
        return;
      }
      new_dims[index] = dims[i];
    }
    dims.swap(new_dims);
  }
}

Status ExpandDimension(std::vector<int64_t>& dims, const std::string& op_name, const ge::Format& original_format,
                       const ge::Format& final_format, const uint32_t& tensor_index, const std::string& reshape_type) {
  /* 1. Check expanding necessary. */
  size_t full_size = 0;
  if (!IsExpandNecessary(dims, original_format, final_format, reshape_type, full_size)) {
    return NOT_CHANGED;
  }

  /* 2. Check whether the reshape type is consistent with the original format.
   * If not consistent, just return and report a warning. */
  string valid_reshape_type = reshape_type;
  size_t old_dims_size = dims.size();
  auto iter_format = kAllValidReshapeType.find(original_format);
  if (iter_format != kAllValidReshapeType.end()) {
    auto iter_reshape_type = iter_format->second.find(reshape_type);
    if (iter_reshape_type == iter_format->second.end()) {
      if (GetDefaultReshapeType(original_format, old_dims_size, valid_reshape_type) != SUCCESS) {
        return SUCCESS;
      }
      FE_LOGI("Get default reshape type %s for op %s's tensor %u ori format %u is invalid.",
              valid_reshape_type.c_str(), op_name.c_str(), tensor_index, original_format);
    }
  }

  /* 3. Check whether the dimension of original shape is less than or equal to
   * the length of reshape type. If the the dimension of original shape is larger,
   * we cannot find suitable position for all axis in original shape and we just return. */
  if (old_dims_size > valid_reshape_type.length()) {
    FE_LOGW("Dimension %zu of tensor %u of %s is larger than the length of reshape type which is %zu.",
            old_dims_size, tensor_index, op_name.c_str(), valid_reshape_type.length());
    return SUCCESS;
  }

  /* 4. Expand dimension. */
  ExpandByReshapeType(dims, op_name, original_format, full_size, tensor_index, valid_reshape_type);
  return SUCCESS;
}

string GetShapeDims(const ge::GeShape &shape) {
  string dim_string;
  std::vector<int64_t> dims = shape.GetDims();
  size_t size = dims.size();
  for (size_t i = 0; i != size; ++i) {
    dim_string += std::to_string(dims[i]);
    if (i != size - 1) {
      dim_string += ", ";
    }
  }
  return dim_string;
}

void CheckStridedReadInConv2d(const vector<ge::NodePtr> &conv_nodes, vector<ge::NodePtr> &fusion_nodes) {
  for (const auto &fusion_node : fusion_nodes) {
    if (fusion_node->GetType() == STRIDEDREAD) {
      return;
    }
  }
  for (const auto &conv2d_node : conv_nodes) {
    auto conv2d_in_nodes = conv2d_node->GetInDataNodes();
    if (!conv2d_in_nodes.empty()) {
      if (conv2d_in_nodes.at(0)->GetType() == STRIDEDREAD) {
        fusion_nodes.emplace_back(conv2d_in_nodes.at(0));
      }
    }
  }
}

/*
 * @brief: check if is TVM type op
 * @param [in] node: node
 * @return bool: check result
 */
bool IsTbeOp(ge::NodePtr node) {
  FE_CHECK((node == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][UbFusion][IsTbeOP] null node in judging AICoreOp"), return false);
  int64_t type = 0;
  (void)ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, type);
  const bool res = (type == static_cast<int64_t>(domi::ImplyType::TVM));
  if (!res) {
    FE_LOGD("op [%s] is not tbe op", node->GetName().c_str());
    return false;
  }
  return true;
}

bool IsPlaceOrEnd(const std::string& op_type) {
  return PLACE_OR_END_SET.count(op_type) > 0 ? true : false;
}

bool CheckVirtualOp(const ge::OpDescPtr op_desc_ptr) {
  bool no_task = false;
  (void)ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_NAME_NOTASK, no_task);
  string op_type = op_desc_ptr->GetType();
  return no_task && (op_type == CONCATD || op_type == CONCATV2D || op_type == SPLITD || op_type == SPLITVD);
}

bool IsNd(const ge::Format& format) { return format == ge::FORMAT_ND; }

bool IsSupportedTransType(const ge::Format& ori_format, const ge::Format& final_format) {
  if (kSupportedTransType.count(final_format) != 0 ||
      (ori_format == ge::FORMAT_ND && final_format == ge::FORMAT_FRACTAL_Z)) {
    return true;
  }
  return false;
}

bool IsOpTranspose(const std::string &op_type) {
  if (op_type == TRANSPOSE) {
    return true;
  }

  return false;
}

bool CheckOpConstOrVariableInOriGraph(const ge::OpDescPtr &op_desc) {
  string op_type = op_desc->GetType();
  if (op_type == fe::CONSTANT || op_type == fe::CONSTANTOP || op_type == fe::VARIABLE) {
    return true;
  }

  return false;
}

ge::Format GetCurOpOriginFormat(const ge::GeTensorDesc& cur_tensor_desc) {
  auto cur_format = static_cast<ge::Format>(ge::GetPrimaryFormat(cur_tensor_desc.GetFormat()));
  if (cur_format == ge::FORMAT_ND) {
    return cur_tensor_desc.GetOriginFormat();
  } else {
    return cur_format;
  }
}

ge::GeShape GetCurOpOriginShape(const ge::GeTensorDesc& cur_tensor_desc) {
  ge::Format cur_format = static_cast<ge::Format>(ge::GetPrimaryFormat(cur_tensor_desc.GetFormat()));
  ge::GeShape cur_shape = cur_tensor_desc.GetShape();
  if (cur_format == ge::FORMAT_ND) {
    return cur_tensor_desc.GetOriginShape();
  } else {
    return cur_shape;
  }
}

void LogFormatMap(const map<string, vector<ge::Format>>& format_map) {
  string result = "";
  map<string, vector<ge::Format>>::const_iterator iter;
  for (iter = format_map.begin(); iter != format_map.end(); ++iter) {
    vector<ge::Format> format_vec = iter->second;
    string str = GetStrByFormatVec(format_vec);
    FE_LOGD("name=[%s], formats=[%s]", iter->first.c_str(), str.c_str());
  }
}

void LogDataTypeMap(const map<string, vector<ge::DataType>>& data_type_map) {
  string result = "";
  map<string, vector<ge::DataType>>::const_iterator iter;
  for (iter = data_type_map.begin(); iter != data_type_map.end(); ++iter) {
    vector<ge::DataType> data_type_vec = iter->second;
    string str = GetStrByDataTypeVec(data_type_vec);
    FE_LOGD("Data type of [%s] is [%s]", iter->first.c_str(), str.c_str());
  }
}

Status GenerateUnionFormatDtype(const vector<ge::Format>& old_formats, const vector<ge::DataType>& old_data_types,
                                vector<ge::Format>& new_formats, vector<ge::DataType>& new_data_types) {
  size_t old_formats_size = old_formats.size();
  size_t old_dtypes_size = old_data_types.size();
  if (old_formats.empty() || old_data_types.empty()) {
    FE_LOGI("The old_formats_size [%zu] is 0 or the old_dtypes_size [%zu] is 0.", old_formats_size, old_dtypes_size);
    if (!old_formats.empty()) {
      new_formats = old_formats;
    }
    if (!old_data_types.empty()) {
      new_data_types = old_data_types;
    }
    return SUCCESS;
  }

  for (size_t i = 0; i != old_formats_size; i++) {
    new_formats.insert(new_formats.cend(), old_dtypes_size, old_formats[i]);
    new_data_types.insert(new_data_types.cend(), old_data_types.cbegin(), old_data_types.cend());
  }
  size_t new_formats_size = new_formats.size();
  size_t new_dtypes_size = new_data_types.size();
  if (new_formats_size != new_dtypes_size) {
    REPORT_FE_ERROR(
        "[GraphOpt][FmtJdg][GenFmtUnion] The new_formats_size [%zu] is not same with the new_dtypes_size [%zu].",
        new_formats_size, new_dtypes_size);
    return FAILED;
  }
  return SUCCESS;
}

Status GetAllInputAndOutputKernelInfo(const OpKernelInfoPtr& op_kernel_info_ptr, const ge::NodePtr& current_node,
                                      const std::vector<IndexNameMap>& tensor_map,
                                      std::vector<std::vector<InputOrOutputInfoPtr>>& input_and_output_kernel) {
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  auto all_input_tensor = op_desc_ptr->GetAllInputsDescPtr();
  if (all_input_tensor.empty()) {
    FE_LOGW("Input tensor vector of node %s is empty", current_node->GetName().c_str());
    return FAILED;
  }

  if (input_and_output_kernel.size() != INPUT_OUTPUT_INDEX_BOTTOM) {
    FE_LOGW("Size of input kernel vector %zu is not correct for node %s.", input_and_output_kernel.size(),
            current_node->GetName().c_str());
    return FAILED;
  }

  for (size_t index = 0; index < current_node->GetOpDesc()->GetInputsSize(); index++) {
    InputOrOutputInfoPtr input_info;
    auto iter = tensor_map[INPUT_INDEX].find(index);
    if (iter == tensor_map[INPUT_INDEX].end()) {
      FE_LOGW("Can not find input %zu in tensor map!", index);
      continue;
    }
    Status ret = op_kernel_info_ptr->GetInputInfoByName(iter->second, input_info);

    if (ret == FAILED) {
      return FAILED;
    }
    input_and_output_kernel[INPUT_INDEX].emplace_back(input_info);
  }

  for (size_t index = 0; index < current_node->GetOpDesc()->GetOutputsSize(); index++) {
    InputOrOutputInfoPtr output_info;
    auto iter = tensor_map[OUTPUT_INDEX].find(index);
    if (iter == tensor_map[OUTPUT_INDEX].end()) {
      FE_LOGW("Can not find output %zu in tensor map!", index);
      continue;
    }
    Status ret = op_kernel_info_ptr->GetOutputInfoByName(iter->second, output_info);
    if (ret == FAILED) {
      return FAILED;
    }
    input_and_output_kernel[OUTPUT_INDEX].emplace_back(output_info);
  }
  return SUCCESS;
}

bool IsScalarInput(const ge::GeShape& shape) {
  return shape.IsScalar() || (shape.GetDimNum() == 1 && shape.GetDim(0) == 1);
}
bool IsSameShape(const ge::GeShape& first_shape, const ge::GeShape& second_shape) {
  if (first_shape.GetDimNum() != second_shape.GetDimNum()) {
    return false;
  }
  size_t dim_value = first_shape.GetDimNum();
  for (size_t i = 0; i < dim_value; i++) {
    if (first_shape.GetDim(i) == SHAPE_UNKNOWN_DIM && second_shape.GetDim(i) == SHAPE_UNKNOWN_DIM) {
      return false;
    }
    if (first_shape.GetDim(i) != second_shape.GetDim(i)) {
      return false;
    }
  }
  return true;
}

bool CheckOriginShapesDimNum(const vector<ge::GeShape>& shapes, const size_t& dim_min) {
  for (auto shape : shapes) {
    if (!CheckOriginShapeDimNum(shape, dim_min)) {
      return false;
    }
  }
  return true;
}

bool CheckAccuracyOriginShapesDimNum(const vector<ge::GeShape>& shapes, const size_t& dim_size) {
  for (auto shape : shapes) {
    if (shape.GetDimNum() != dim_size) {
      return false;
    }
  }
  return true;
}

bool CheckOriginShapeDimNum(const ge::GeShape& shape, const size_t& dim_min) {
  if (shape.GetDimNum() < dim_min) {
    FE_LOGD("The dim_num [%zu] is less than dim_min[%zu].", shape.GetDimNum(), dim_min);
    return false;
  }
  return true;
}

bool CheckOriginFormatsIdentifiable(const vector<ge::Format>& formats) {
  for (auto format : formats) {
    FE_LOGD("The format %s.", ge::TypeUtils::FormatToSerialString(format).c_str());
    if (!CheckOriginFormatIdentifiable(format)) {
      return false;
    }
  }
  return true;
}

bool CheckOriginFormatIdentifiable(const ge::Format& format) {
  if (std::find(FE_IDENTIFIABLE_ORIGIN_FORMAT_VECTOR.begin(), FE_IDENTIFIABLE_ORIGIN_FORMAT_VECTOR.end(), format) ==
      FE_IDENTIFIABLE_ORIGIN_FORMAT_VECTOR.end()) {
    FE_LOGD("The format %s is not identifiable.", ge::TypeUtils::FormatToSerialString(format).c_str());
    return false;
  }
  return true;
}

bool IsEsBoard() {
  string soc_version = fe::Configuration::Instance(fe::AI_CORE_NAME).GetSocVersion();
  return (soc_version == SOC_VERSION_HI3796CV300ES || soc_version == SOC_VERSION_HI3796CV300CS ||
          soc_version == SOC_VERSION_SD3403);
}

/* If the Cast node is connected directly to the NetOutput on ES board,
 * we need to skip the opjudge of this Cast */
bool IsSpecialCast(const ge::NodePtr& node_ptr) {
  if (IsEsBoard() && IsDtypeSensitiveOp(node_ptr->GetType())) {
    auto input_anchor = node_ptr->GetInDataAnchor(0);
    if (input_anchor == nullptr || input_anchor->GetPeerOutAnchor() == nullptr ||
        input_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr) {
      return false;
    }
    auto peer_node = input_anchor->GetPeerOutAnchor()->GetOwnerNode();
    auto output_0 = peer_node->GetOpDesc()->GetOutputDescPtr(0);
    if (output_0 == nullptr) {
      return false;
    }
    auto peer_out_format =
        static_cast<ge::Format>(ge::GetPrimaryFormat(output_0->GetFormat()));
    if (peer_out_format != ge::FORMAT_NC1HWC0 && peer_out_format != ge::FORMAT_FRACTAL_NZ) {
      return false;
    }

    auto output_anchor = node_ptr->GetOutDataAnchor(0);
    if (output_anchor == nullptr || output_anchor->GetPeerInDataAnchors().size() != 1 ||
        output_anchor->GetPeerInDataAnchors().at(0) == nullptr ||
        output_anchor->GetPeerInDataAnchors().at(0)->GetOwnerNode() == nullptr) {
      return false;
    }
    peer_node = output_anchor->GetPeerInDataAnchors().at(0)->GetOwnerNode();
    return peer_node->GetType() == NETOUTPUT;
  } else {
    return false;
  }
}

int32_t GetAxisIndexByFormat(const ge::Format& format, const string& axis) {
  auto iter = FE_AXIS_INDEX_OF_FORMAT.find(format);
  if (iter != FE_AXIS_INDEX_OF_FORMAT.end()) {
    auto iter2 = iter->second.find(axis);
    if (iter2 != iter->second.end()) {
      return iter2->second;
    } else {
      FE_LOGW("Do not support this axis %s", axis.c_str());
      return -1;
    }
  } else {
    FE_LOGW("Do not support this format %s", ge::TypeUtils::FormatToSerialString(format).c_str());
    return -1;
  }
}

bool GetDimValueByFormatAndShape(const ge::Format& format, const ge::GeShape& shape, string axis, int64_t& dim_value) {
  if (shape.GetDimNum() != NCHW_DIMENSION_NUM) {
    REPORT_INNER_ERROR(EM_INNER_ERROR, "[SubGraphOpt][Compile][WeightCmprs] shape dim is not equal to 4.");
    return false;
  }
  int32_t dim_index = GetAxisIndexByFormat(format, axis);
  if (dim_index < 0) {
    REPORT_INNER_ERROR(EM_INNER_ERROR, "[SubGraphOpt][Compile][WeightCmprs] dim_index is less than 0.");
    return false;
  }

  dim_value = shape.GetDim(dim_index);
  return true;
}

Status GetGroupAttributeWithVerify(ge::OpDescPtr op_desc_ptr, int64_t& group) {
  if (op_desc_ptr == nullptr) {
    return FAILED;
  }

  group = GROUPS_DEFAULT_VALUE;
  if (!ge::AttrUtils::GetInt(op_desc_ptr, ATTR_NAME_GROUPS, group)) {
    FE_LOGD("Op Desc[%s, %s] does not have groups attribute, take default value [1].",
            op_desc_ptr->GetName().c_str(),
            op_desc_ptr->GetType().c_str());
    return SUCCESS;
  }
  if (group < GROUPS_DEFAULT_VALUE) {
    REPORT_FE_ERROR(
        "[GraphOpt][Setcheck][GetGrpAttr] The group value of op desc[%s, %s] shoule be equal to or greater \
        than one, but [%ld].", op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), group);
    return FAILED;
  }

  return SUCCESS;
}

std::string GetRealNodeType(ge::OpDescPtr OpDescPtr) {
  if (OpDescPtr->GetType() == FRAMEWORKOP) {
    string op_type = "";
    if (!ge::AttrUtils::GetStr(OpDescPtr, ge::ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, op_type)) {
      REPORT_FE_ERROR("[GraphOpt][Setcheck][GetRealNodeType] Get original_type for op[%s] fail!",
                      OpDescPtr->GetName().c_str());
      return FRAMEWORKOP;
    }
    return op_type;
  } else {
    return OpDescPtr->GetType();
  }
}

bool CheckWeightTypeQualifiedWithCount(const ge::NodePtr& weight_node, const string& expected_type,
                                       uint32_t& recursion_count) {
  recursion_count++;
  if (recursion_count > CHECK_WEIGHT_MAX_COUNT) {
    FE_LOGD("The count of CheckWeightTypeQualified recursion has reached %d, now stop recursion.", recursion_count);
    return false;
  }

  if (weight_node == nullptr) {
    FE_LOGW("Node is nullptr!");
    return false;
  }

  if (weight_node->GetType() == expected_type) {
    return true;
  } else if (FeGraphUtils::IsSubGraphData(weight_node->GetOpDesc())) {
    ge::OutDataAnchorPtr pre_out_data_anchor_ptr = nullptr;
    if (FeGraphUtils::GetPreOutAnchorOfSubData(weight_node, pre_out_data_anchor_ptr) != SUCCESS) {
      FE_LOGW("Parent node of sub graph is not found.");
      return false;
    }
    ge::NodePtr parent_node = pre_out_data_anchor_ptr->GetOwnerNode();
    if (parent_node == nullptr) {
      FE_LOGW("Parent node of sub graph is null.");
      return false;
    }
    return CheckWeightTypeQualifiedWithCount(parent_node, expected_type, recursion_count);
  } else {
    auto input_anchors = weight_node->GetAllInDataAnchors();
    if (input_anchors.empty()) {
      return false;
    }

    size_t unsupported_count = 0;
    for (const auto& in_anchor : input_anchors) {
      if (in_anchor == nullptr || in_anchor->GetPeerOutAnchor() == nullptr) {
        unsupported_count++;
        continue;
      }

      if (CheckWeightTypeQualifiedWithCount(in_anchor->GetPeerOutAnchor()->GetOwnerNode(), expected_type,
                                            recursion_count)) {
        /* We need to check all top inputs of the Conv2D's weight. */
        continue;
      } else {
        /* If one of the top inputs of Conv2D's weight is not const, we consider this is not the
         * inference network. */
        return false;
      }
    }

    return unsupported_count != input_anchors.size();
  }
}

bool CheckWeightTypeQualified(const ge::NodePtr& weight_node, const string& expected_type) {
  uint32_t recursion_count = 0;
  return CheckWeightTypeQualifiedWithCount(weight_node, expected_type, recursion_count);
}

void CheckHasNoFather(bool is_input, int32_t index, const ge::NodePtr& node, ge::InDataAnchorPtr& in_data_anchor,
                      bool& has_no_father) {
  if (is_input) {
    in_data_anchor = node->GetInDataAnchor(index);
    has_no_father = (in_data_anchor == nullptr || in_data_anchor->GetPeerOutAnchor() == nullptr ||
                     in_data_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr ||
                     in_data_anchor->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc() == nullptr);
  } else {
    has_no_father = true;
  }
}

// If a subgraph has been optimized by L2fusion, some nodes in the subgraph will have the lx_fusion_pass attribute
// if a node in the subgraph has attr lx_fusion_pass and the value is true,
// that means this subgraph should do lxfusion
bool CheckL2FusionFusionStrategy(const ge::ComputeGraph& graph) {
  bool lx_fusion_pass = false;
  for (auto node : graph.GetDirectNode()) {
    (void)ge::AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_LX_FUSION_PASS, lx_fusion_pass);
    if (lx_fusion_pass) {
      return true;
    }
  }
  return false;
}

// If a subgraph has been optimized by L2fusion, some nodes in the subgraph will have the lx_fusion_pass attribute
// if a node in the subgraph has attr lx_fusion_pass and the value is true,
// that means this subgraph should do lxfusion
bool CheckL2BufferFusionStrategy(ge::ComputeGraph& graph) {
  bool lx_fusion_pass = false;
  for (auto node : graph.GetDirectNode()) {
    (void)ge::AttrUtils::GetBool(node->GetOpDesc(), "lx_fusion_pass", lx_fusion_pass);
    if (lx_fusion_pass) {
      return false;
    }
  }
  return true;
}

bool IsNeedReshape(const ge::OpDescPtr& op_desc_ptr) {
  bool need_reshape = true;
  std::string op_name = op_desc_ptr->GetName();
  std::string op_type = op_desc_ptr->GetType();
  std::vector<ge::GeShape> input_shapes;
  for (auto input : op_desc_ptr->GetAllInputsDescPtr()) {
    auto shape = input->GetOriginShape();
    if (IsScalarInput(shape)) {
      FE_LOGD("op[name:%s,type:%s] input has scalar, do not need to reshape.", op_name.c_str(), op_type.c_str());
      need_reshape = false;
    }
    input_shapes.push_back(shape);
  }

  if (need_reshape) {
    ge::GeShape first_shape = input_shapes.front();
    for (auto iter = input_shapes.begin(); iter != input_shapes.end(); iter++) {
      if (!IsSameShape(first_shape, *iter)) {
        FE_LOGD("op[name:%s,type:%s] shape is not same, need to reshape.", op_name.c_str(), op_type.c_str());
        return true;
      }
    }
    need_reshape = false;
  }

  FE_LOGD("op[name:%s,type:%s] reshape flag is %d.", op_name.c_str(), op_type.c_str(), need_reshape);
  return need_reshape;
}

void CopyWeightAttrToPlaceHolder(ge::NodePtr& node) {
  if (node == nullptr || node->GetOpDesc() == nullptr) {
    return;
  }

  if (node->GetType() != OP_TYPE_PLACE_HOLDER) {
    return;
  }

  ge::NodePtr parent_node = nullptr;
  parent_node = node->GetOpDesc()->TryGetExtAttr(ATTR_NAME_PARENT_NODE, parent_node);
  if (parent_node == nullptr || parent_node->GetOpDesc() == nullptr) {
    return;
  }

  if (parent_node->GetType() != CONSTANT && parent_node->GetType() != CONSTANTOP) {
    FE_LOGD("Op type[%s] of parent node[%s] is not const or constant.", parent_node->GetType().c_str(),
            parent_node->GetName().c_str());
    return;
  }

  ge::GeTensorPtr weight = nullptr;
  bool find_weight = ge::AttrUtils::MutableTensor(parent_node->GetOpDesc(), ge::ATTR_NAME_WEIGHTS, weight);
  if (!find_weight || weight == nullptr) {
    FE_LOGD("Can not find attr ATTR_NAME_WEIGHTS for node:%s.", parent_node->GetName().c_str());
    return;
  }

  ge::GeTensor copy_weight = weight->Clone();
  if (!ge::AttrUtils::SetTensor(node->GetOpDesc(), ge::ATTR_NAME_WEIGHTS, copy_weight)) {
    FE_LOGW("Fail to set weight attr value for place holder node[%s].", node->GetName().c_str());
    return;
  }
  FE_LOGD("Clone ATTR_NAME_WEIGHTS for node:%s success.", node->GetName().c_str());
}

bool InvalidMemType(const ge::OpDescPtr& node_desc) {
  std::vector<uint32_t> input_mem_type;
  (void)ge::AttrUtils::GetListInt(node_desc, ge::ATTR_NAME_INPUT_MEM_TYPE_LIST, input_mem_type);
  std::vector<uint32_t> output_mem_type;
  (void)ge::AttrUtils::GetListInt(node_desc, ge::ATTR_NAME_OUTPUT_MEM_TYPE_LIST, output_mem_type);
  for (auto mem_type : input_mem_type) {
    if (mem_type == RT_MEMORY_L1 || mem_type == RT_MEMORY_L2) {
      FE_LOGD("Node [%s] has lx addr input, not optimize.", node_desc->GetName().c_str());
      return true;
    }
  }
  for (auto mem_type : output_mem_type) {
    if (mem_type == RT_MEMORY_L1 || mem_type == RT_MEMORY_L2) {
      FE_LOGD("Node [%s] has lx addr output, not optimize.", node_desc->GetName().c_str());
      return true;
    }
  }
  return false;
}

bool HasFusionScopeAttr(const ge::OpDescPtr &op_desc) {
  if (op_desc == nullptr) {
    return false;
  }
  return op_desc->HasAttr(L1_SCOPE_ID_ATTR) || op_desc->HasAttr(SCOPE_ID_ATTR);
}

bool GetFusionScopeAttr(const ge::OpDescPtr &op_desc, int64_t &scope_id) {
  bool is_l1_fusion = false;
  return GetFusionScopeAttr(op_desc, scope_id, is_l1_fusion);
}

bool GetFusionScopeAttr(const ge::OpDescPtr &op_desc, int64_t &scope_id, bool &is_l1_fusion) {
  if (op_desc == nullptr) {
    return false;
  }
  if (ge::AttrUtils::GetInt(op_desc, L1_SCOPE_ID_ATTR, scope_id)) {
    is_l1_fusion = true;
    return true;
  }
  if (ge::AttrUtils::GetInt(op_desc, SCOPE_ID_ATTR, scope_id)) {
    is_l1_fusion = false;
    return true;
  }
  return false;
}

void RemoveL1FusionScopeAttr(const ge::OpDescPtr &op_desc) {
  int64_t scope_id = 0;
  if (ge::AttrUtils::GetInt(op_desc, L1_SCOPE_ID_ATTR, scope_id)) {
    (void)ge::AttrUtils::SetInt(op_desc, FAILED_L1_SCOPE_ID_ATTR, scope_id);
    (void)op_desc->DelAttr(L1_SCOPE_ID_ATTR);
  }
}

bool IsOpDynamicImpl(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr) {
    return false;
  }
  bool is_dynamic_impl = false;
  return ge::AttrUtils::GetBool(op_desc_ptr, ATTR_NAME_IS_OP_DYNAMIC_IMPL, is_dynamic_impl) && is_dynamic_impl;
}

bool IsOpDynamicImpl(const ge::OpDesc &op_desc) {
  bool is_dynamic_impl = false;
  return ge::AttrUtils::GetBool(op_desc, ATTR_NAME_IS_OP_DYNAMIC_IMPL, is_dynamic_impl) && is_dynamic_impl;
}


bool IsZeroShapeTensor(const ge::GeTensorDescPtr &tensor) {
  if (tensor == nullptr) {
    return false;
  }
  auto &shape = tensor->MutableShape();
  for (size_t i = 0; i < shape.GetDimNum(); i++) {
    if (shape.GetDim(i) == 0) {
      return true;
    }
  }
  return false;
}

bool IsStaticZeroShapeOp(const ge::OpDescPtr &op_desc) {
  for (size_t i = 0; i < op_desc->GetAllInputsSize(); i++) {
    auto tensor = op_desc->MutableInputDesc(i);
    if (tensor == nullptr) {
      continue;
    }

    if (IsZeroShapeTensor(tensor) &&
        !tensor->MutableShape().IsUnknownShape()) {
      return true;
    }
  }

  for (size_t i = 0; i < op_desc->GetAllOutputsDescSize(); i++) {
    auto tensor = op_desc->MutableOutputDesc(i);
    if (tensor == nullptr) {
      continue;
    }

    if (IsZeroShapeTensor(tensor) &&
        !tensor->MutableShape().IsUnknownShape()) {
      return true;
    }
  }
  return false;
}

bool IsLifeCycleEnd(const ge::Node &node, const ge::GeTensorDescPtr &input_desc,
                    int input_idx) {
  auto op_desc = node.GetOpDesc();
  auto op_name = op_desc->GetName();
  auto op_type = op_desc->GetType();

  bool is_life_cycle_end = false;
  if (ge::AttrUtils::HasAttr(input_desc, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE)) {
    (void)ge::AttrUtils::GetBool(input_desc, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE, is_life_cycle_end);
    FE_LOGD("Op[name=%s,type=%s,input=%d]: has attr %s, the life_cycle is %s.", op_name.c_str(), op_type.c_str(),
            input_idx, ge::ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE.c_str(), is_life_cycle_end ? "end" : "not end");
    return is_life_cycle_end;
  }
  return is_life_cycle_end;
}

bool IsAiCoreOp(const ge::NodePtr &node) {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  OpImplType op_impl_type;
  uint32_t imply_value = 0;
  (void)ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, imply_value);
  op_impl_type = static_cast<OpImplType>(imply_value);
  bool is_aicore_op = false;
  is_aicore_op = op_impl_type == EN_IMPL_CUSTOM_TIK || op_impl_type == EN_IMPL_CUSTOM_TBE ||
                 op_impl_type == EN_IMPL_HW_TIK || op_impl_type == EN_IMPL_HW_TBE;
  return is_aicore_op;
}

bool CheckTransFormatValid(const ge::NodePtr &node) {
  if (node == nullptr) {
    return false;
  }

  ge::Format input_format = node->GetOpDesc()->MutableInputDesc(0)->GetFormat();
  ge::Format output_format = node->GetOpDesc()->MutableOutputDesc(0)->GetFormat();
  if (kValidTransFormat.count(input_format) && kValidTransFormat.at(input_format).count(output_format)) {
    return true;
  }
  return false;
}

void CheckFusionTransNode(ge::NodePtr &cube_node, std::vector<ge::NodePtr> &trans_nodes,
                          std::vector<ge::NodePtr> &fusion_nodes) {
  if (cube_node == nullptr) {
    return;
  }
  if (!trans_nodes.empty() && !CheckTransFormatValid(trans_nodes[0])) {
    auto iter = std::find(fusion_nodes.begin(), fusion_nodes.end(), trans_nodes[0]);
    if (iter != fusion_nodes.end()) {
      fusion_nodes.erase(iter);
    }
  }

  int64_t group = 1;
  const size_t kMinInputNum = 2;
  (void)ge::AttrUtils::GetInt(cube_node->GetOpDesc(), "groups", group);
  bool no_group = (group == 1);
  auto in_data_nodes = cube_node->GetInDataNodes();
  if (no_group && in_data_nodes.size() >= kMinInputNum) {
    ge::NodePtr weight_node = in_data_nodes.at(1);
    if (weight_node == nullptr) {
      return;
    }
    if (weight_node->GetType() == TRANSDATA && CheckTransFormatValid(weight_node)) {
      fusion_nodes.emplace_back(weight_node);
      return;
    }
  }
  return;
}
}  // namespace fe
