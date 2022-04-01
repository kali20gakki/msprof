/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "graph/load/model_manager/aipp_utils.h"

#include <sstream>
#include "framework/common/debug/log.h"
#include "framework/common/op/ge_op_utils.h"

namespace ge {
namespace {
const size_t kInvalidIdx = 0xFFFFFFFFU;
const std::string kDataModeStatic = "static_aipp";
const std::string kDataModeDynamic = "dynamic_aipp";
const std::string kDataModeDynamicConf = "dynamic_aipp_conf";
}

void AippUtils::SetMatrixInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info) {
  if (aipp_params.matrix_r0c0_size() > 0) {
    aipp_info.matrix_r0c0 = aipp_params.matrix_r0c0(0);
  }
  if (aipp_params.matrix_r0c1_size() > 0) {
    aipp_info.matrix_r0c1 = aipp_params.matrix_r0c1(0);
  }
  if (aipp_params.matrix_r0c2_size() > 0) {
    aipp_info.matrix_r0c2 = aipp_params.matrix_r0c2(0);
  }
  if (aipp_params.matrix_r1c0_size() > 0) {
    aipp_info.matrix_r1c0 = aipp_params.matrix_r1c0(0);
  }
  if (aipp_params.matrix_r1c1_size() > 0) {
    aipp_info.matrix_r1c1 = aipp_params.matrix_r1c1(0);
  }
  if (aipp_params.matrix_r1c2_size() > 0) {
    aipp_info.matrix_r1c2 = aipp_params.matrix_r1c2(0);
  }
  if (aipp_params.matrix_r2c0_size() > 0) {
    aipp_info.matrix_r2c0 = aipp_params.matrix_r2c0(0);
  }
  if (aipp_params.matrix_r2c1_size() > 0) {
    aipp_info.matrix_r2c1 = aipp_params.matrix_r2c1(0);
  }
  if (aipp_params.matrix_r2c2_size() > 0) {
    aipp_info.matrix_r2c2 = aipp_params.matrix_r2c2(0);
  }
}

void AippUtils::SetBiasInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info) {
  if (aipp_params.output_bias_0_size() > 0) {
    aipp_info.output_bias_0 = aipp_params.output_bias_0(0);
  }
  if (aipp_params.output_bias_1_size() > 0) {
    aipp_info.output_bias_1 = aipp_params.output_bias_1(0);
  }
  if (aipp_params.output_bias_2_size() > 0) {
    aipp_info.output_bias_2 = aipp_params.output_bias_2(0);
  }
  if (aipp_params.input_bias_0_size() > 0) {
    aipp_info.input_bias_0 = aipp_params.input_bias_0(0);
  }
  if (aipp_params.input_bias_1_size() > 0) {
    aipp_info.input_bias_1 = aipp_params.input_bias_1(0);
  }
  if (aipp_params.input_bias_2_size() > 0) {
    aipp_info.input_bias_2 = aipp_params.input_bias_2(0);
  }
}

void AippUtils::SetChnInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info) {
  aipp_info.mean_chn_0 = aipp_params.mean_chn_0();
  aipp_info.mean_chn_1 = aipp_params.mean_chn_1();
  aipp_info.mean_chn_2 = aipp_params.mean_chn_2();
  aipp_info.mean_chn_3 = aipp_params.mean_chn_3();
  aipp_info.min_chn_0 = aipp_params.min_chn_0();
  aipp_info.min_chn_1 = aipp_params.min_chn_1();
  aipp_info.min_chn_2 = aipp_params.min_chn_2();
  aipp_info.min_chn_3 = aipp_params.min_chn_3();
  if (aipp_params.var_reci_chn_0_size() > 0) {
    aipp_info.var_reci_chn_0 = aipp_params.var_reci_chn_0(0);
  }
  if (aipp_params.var_reci_chn_1_size() > 0) {
    aipp_info.var_reci_chn_1 = aipp_params.var_reci_chn_1(0);
  }
  if (aipp_params.var_reci_chn_2_size() > 0) {
    aipp_info.var_reci_chn_2 = aipp_params.var_reci_chn_2(0);
  }
  if (aipp_params.var_reci_chn_3_size() > 0) {
    aipp_info.var_reci_chn_3 = aipp_params.var_reci_chn_3(0);
  }
}

Status AippUtils::ConvertAippParams2AippInfo(const domi::AippOpParams &aipp_params, AippConfigInfo &aipp_info) {
  aipp_info.aipp_mode = static_cast<int8_t>(aipp_params.aipp_mode());
  aipp_info.input_format = static_cast<int8_t>(aipp_params.input_format());
  aipp_info.related_input_rank = aipp_params.related_input_rank();
  aipp_info.src_image_size_w = aipp_params.src_image_size_w();
  aipp_info.src_image_size_h = aipp_params.src_image_size_h();
  aipp_info.crop = static_cast<int8_t>(aipp_params.crop());
  aipp_info.load_start_pos_w = aipp_params.load_start_pos_w();
  aipp_info.load_start_pos_h = aipp_params.load_start_pos_h();
  aipp_info.crop_size_w = aipp_params.crop_size_w();
  aipp_info.crop_size_h = aipp_params.crop_size_h();
  aipp_info.resize = static_cast<int8_t>(aipp_params.resize());
  aipp_info.resize_output_w = aipp_params.resize_output_w();
  aipp_info.resize_output_h = aipp_params.resize_output_h();
  aipp_info.padding = static_cast<int8_t>(aipp_params.padding());
  aipp_info.left_padding_size = aipp_params.left_padding_size();
  aipp_info.right_padding_size = aipp_params.right_padding_size();
  aipp_info.top_padding_size = aipp_params.top_padding_size();
  aipp_info.bottom_padding_size = aipp_params.bottom_padding_size();
  aipp_info.csc_switch = static_cast<int8_t>(aipp_params.csc_switch());
  aipp_info.rbuv_swap_switch = static_cast<int8_t>(aipp_params.rbuv_swap_switch());
  aipp_info.ax_swap_switch = static_cast<int8_t>(aipp_params.ax_swap_switch());
  aipp_info.single_line_mode = static_cast<int8_t>(aipp_params.single_line_mode());
  aipp_info.support_rotation = static_cast<int8_t>(aipp_params.support_rotation());
  aipp_info.max_src_image_size = aipp_params.max_src_image_size();

  SetMatrixInfo(aipp_params, aipp_info);
  SetBiasInfo(aipp_params, aipp_info);
  SetChnInfo(aipp_params, aipp_info);

  return SUCCESS;
}

Status AippUtils::SetAippInfoAndTypeFromOpDesc(const std::map<std::string, uint32_t> &data_index_map,
                                               const OpDescPtr &op_desc, const uint32_t index,
                                               std::map<uint32_t, AippConfigInfo> &aipp_infos,
                                               std::map<uint32_t, std::pair<InputAippType, size_t>> &aipp_types) {
  GE_CHECK_NOTNULL(op_desc);
  NamedAttrs aipp_attr;
  const bool get_attr_aipp = AttrUtils::GetNamedAttrs(op_desc, ATTR_NAME_AIPP, aipp_attr);
  std::string data_mode;
  const bool get_attr_mode = AttrUtils::GetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, data_mode);
  if ((!get_attr_aipp) && (!get_attr_mode)) {
    GELOGD("There is not AIPP related with op:%s, index:%u.", op_desc->GetName().c_str(), index);
    return SUCCESS;
  }
  if ((!get_attr_aipp) || (!get_attr_mode)) {
    std::stringstream error_message;
    error_message << "Both ATTR_NAME_AIPP and ATTR_DATA_RELATED_AIPP_MODE attributes are needed on the data node, "
                     "but only " << (get_attr_aipp ? "ATTR_NAME_AIPP" : "ATTR_DATA_RELATED_AIPP_MODE")
                  << " is obtained at the time.";
    REPORT_INNER_ERROR("E19999", "Op:%s, index:%u, error message:%s", op_desc->GetName().c_str(), index,
                       error_message.str().c_str());
    GELOGE(INTERNAL_ERROR, "[Get][Attrs]Op:%s, index:%u, error message:%s", op_desc->GetName().c_str(), index,
           error_message.str().c_str());
    return INTERNAL_ERROR;
  }

  auto ret = SetAippInfoImpl(aipp_attr, op_desc, index, aipp_infos);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][AippInfo]Failed to set aipp info, op:%s, index:%u.", op_desc->GetName().c_str(), index);
    return ret;
  }
  ret = SetAippTypeImpl(data_index_map, data_mode, op_desc, index, aipp_types);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][AippType]Failed to set aipp type, op:%s, index:%u.", op_desc->GetName().c_str(), index);
    return ret;
  }
  return SUCCESS;
}

Status AippUtils::SetAippInfoImpl(const NamedAttrs &aipp_attr, const OpDescPtr &op_desc, const uint32_t index,
                                  std::map<uint32_t, AippConfigInfo> &aipp_infos) {
  domi::AippOpParams aipp_params;
  Status ret = OpUtils::ConvertAippParams(aipp_attr, aipp_params);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Failed to convert aipp params, op:%s.", op_desc->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[Convert][AippParams] Failed to convert aipp params, op:%s.", op_desc->GetName().c_str());
    return INTERNAL_ERROR;
  }
  GELOGI("Add aipp info for node:%s, type:%s, current index:%u, current node related input rank:%u",
         op_desc->GetName().c_str(), op_desc->GetType().c_str(), index, aipp_params.related_input_rank());

  AippConfigInfo aipp_info;
  ret = AippUtils::ConvertAippParams2AippInfo(aipp_params, aipp_info);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Failed to convert params to info, op:%s.", op_desc->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[Convert][AippInfo]Failed to convert params to info, op:%s.", op_desc->GetName().c_str());
    return INTERNAL_ERROR;
  }
  aipp_infos[index] = aipp_info;
  return SUCCESS;
}

Status AippUtils::SetAippTypeImpl(const std::map<std::string, uint32_t> &data_index_map,
                                  const std::string &data_mode, const OpDescPtr &op_desc, const uint32_t index,
                                  std::map<uint32_t, std::pair<InputAippType, size_t>> &aipp_types) {
  InputAippType aipp_type = DATA_WITHOUT_AIPP;
  if (data_mode == kDataModeStatic) {
    aipp_type = DATA_WITH_STATIC_AIPP;
  } else if (data_mode == kDataModeDynamic) {
    aipp_type = DATA_WITH_DYNAMIC_AIPP;
  } else if (data_mode == kDataModeDynamicConf) {
    aipp_type = DYNAMIC_AIPP_NODE;
  } else {
    REPORT_INNER_ERROR("E19999", "Get invalid mode:%s, op:%s, type:%s.", data_mode.c_str(),
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Get][Attr]Get invalid mode:%s, op:%s, type:%s.", data_mode.c_str(),
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return INTERNAL_ERROR;
  }

  size_t aipp_data_index = kInvalidIdx;
  if (aipp_type == DATA_WITH_DYNAMIC_AIPP) {
    std::string releated_name;
    const bool ret = AttrUtils::GetStr(op_desc, ATTR_DATA_AIPP_DATA_NAME_MAP, releated_name);
    if (!ret) {
      REPORT_INNER_ERROR("E19999", "Failed to get attr:%s, op:%s, type:%s.", ATTR_DATA_AIPP_DATA_NAME_MAP.c_str(),
                         op_desc->GetName().c_str(), op_desc->GetType().c_str());
      GELOGE(INTERNAL_ERROR, "[Get][Attr]Failed to get attr:%s, op:%s, type:%s.", ATTR_DATA_AIPP_DATA_NAME_MAP.c_str(),
             op_desc->GetName().c_str(), op_desc->GetType().c_str());
      return INTERNAL_ERROR;
    }
    const auto iter = data_index_map.find(releated_name);
    if (iter != data_index_map.end()) {
      aipp_data_index = iter->second;
      GELOGI("Find AippData:%s of index:%zu for op:%s, index:%u", releated_name.c_str(), aipp_data_index,
             op_desc->GetName().c_str(), index);
    } else {
      REPORT_INNER_ERROR("E19999", "Can not find AippData node for index:%u, op:%s", index, op_desc->GetName().c_str());
      GELOGE(INTERNAL_ERROR, "[Find][AippData]Can not find AippData node for index:%u, op:%s",
             index, op_desc->GetName().c_str());
      return INTERNAL_ERROR;
    }
  }
  aipp_types[index] = {aipp_type, aipp_data_index};
  return SUCCESS;
}

Status AippUtils::GetAippInfo(const std::map<uint32_t, AippConfigInfo> &aipp_infos, const uint32_t index,
                              AippConfigInfo &aipp_info) {
  const auto it = aipp_infos.find(index);
  if (it == aipp_infos.end()) {
    GELOGD("There is not AIPP related info with index:%u", index);
    return ACL_ERROR_GE_AIPP_NOT_EXIST;
  }
  aipp_info = it->second;
  return SUCCESS;
}

Status AippUtils::GetAippType(const std::map<uint32_t, std::pair<InputAippType, size_t>> &aipp_types,
                              const uint32_t index, InputAippType &aipp_type, size_t &aipp_data_index) {
  const auto it = aipp_types.find(index);
  if (it == aipp_types.end()) {
    GELOGD("There is not AIPP releated type with index:%u, return default value.", index);
    aipp_type = DATA_WITHOUT_AIPP;
    aipp_data_index = kInvalidIdx;
    return SUCCESS;
  }
  aipp_type = it->second.first;
  aipp_data_index = it->second.second;
  return SUCCESS;
}
}  // namespace ge
