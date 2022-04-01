/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "ir2tf_json_file.h"
#include <fstream>
#include "util/log.h"
#include "util/tf_util.h"

namespace aicpu {
void from_json(const nlohmann::json &json_read, RefTransDesc &ref_desc) {
  auto iter = json_read.find(kIrMappingConfigSrcInoutName);
  if (iter != json_read.end()) {
    ref_desc.src_inout_name = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigDstInoutName);
  if (iter != json_read.end()) {
    ref_desc.dst_inout_name = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigIsRef);
  if (iter != json_read.end()) {
    ref_desc.is_ref = iter.value().get<bool>();
  }
}

void from_json(const nlohmann::json &json_read, ParserExpDesc &parse_desc) {
  auto iter = json_read.find(kIrMappingConfigSrcFieldName);
  if (iter != json_read.end()) {
    parse_desc.src_field_name = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigDstFieldName);
  if (iter != json_read.end()) {
    parse_desc.dst_field_name = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigParseExpress);
  if (iter != json_read.end()) {
    parse_desc.parser_express = iter.value().get<std::string>();
  }
}

void from_json(const nlohmann::json &json_read, OpMapInfo &op_map_info) {
  auto iter = json_read.find(kIrMappingConfigSrcOpType);
  if (iter != json_read.end()) {
    op_map_info.src_op_type = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigDstOpType);
  if (iter != json_read.end()) {
    op_map_info.dst_op_type = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigAttrsBlacklist);
  if (iter != json_read.end()) {
    op_map_info.attrs_blacklist = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigAttrsDesc);
  if (iter != json_read.end()) {
    op_map_info.attrs_map_desc = iter.value().get<std::vector<ParserExpDesc>>();
  }

  iter = json_read.find(kIrMappingConfigAttrsInputMapDesc);
  if (iter != json_read.end()) {
    op_map_info.attrs_input_map_desc = iter.value().get<std::vector<ParserExpDesc>>();
  }

  iter = json_read.find(kIrMappingConfigInputAttrMapDesc);
  if (iter != json_read.end()) {
    op_map_info.input_attr_map_desc = iter.value().get<std::vector<ParserExpDesc>>();
  }

  iter = json_read.find(kIrMappingConfigInputRefMapDesc);
  if (iter != json_read.end()) {
    op_map_info.input_ref_map_desc = iter.value().get<std::vector<RefTransDesc>>();
  }

  iter = json_read.find(kIrMappingConfigAttrsOutputMapDesc);
  if (iter != json_read.end()) {
    op_map_info.attrs_output_map_desc = iter.value().get<std::vector<ParserExpDesc>>();
  }

  iter = json_read.find(kIrMappingConfigDynamicDesc);
  if (iter != json_read.end()) {
    op_map_info.attrs_dynamic_desc = iter.value().get<std::vector<DynamicExpDesc>>();
  }

  iter = json_read.find(kIrMappingConfigOutputAttrMapDesc);
  if (iter != json_read.end()) {
    op_map_info.output_attr_map_desc = iter.value().get<std::vector<ParserExpDesc>>();
  }

  iter = json_read.find(kIrMappingConfigOutputRefDesc);
  if (iter != json_read.end()) {
    op_map_info.output_ref_desc = iter.value().get<std::vector<RefTransDesc>>();
  }

  iter = json_read.find(kIrMappingConfigAttrExtDesc);
  if (iter != json_read.end()) {
    op_map_info.attr_ext_desc = iter.value().get<std::vector<ExtendFieldDesc>>();
  }
}

void from_json(const nlohmann::json &json_read, ExtendFieldDesc &ext_desc) {
  auto iter = json_read.find(kIrMappingConfigFieldName);
  if (iter != json_read.end()) {
    ext_desc.field_name = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigDataType);
  if (iter != json_read.end()) {
    ext_desc.data_type = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigDefaultValue);
  if (iter != json_read.end()) {
    ext_desc.default_value = iter.value().get<std::string>();
  }
}

void from_json(const nlohmann::json &json_read, DynamicExpDesc &dynamic_desc) {
  auto iter = json_read.find(kIrMappingConfigIndex);
  if (iter != json_read.end()) {
    dynamic_desc.index = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigType);
  if (iter != json_read.end()) {
    dynamic_desc.type = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigName);
  if (iter != json_read.end()) {
    dynamic_desc.name = iter.value().get<std::string>();
  }
}


void from_json(const nlohmann::json &json_read, IRFMKOpMapLib &ir_map) {
  auto iter = json_read.find(kIrMappingConfigVersion);
  if (iter != json_read.end()) {
    ir_map.version = iter.value().get<std::string>();
  }

  iter = json_read.find(kIrMappingConfigIr2Tf);
  if (iter != json_read.end()) {
    ir_map.ir2tf = iter.value().get<std::vector<OpMapInfo>>();
  }

  iter = json_read.find(kIrMappingConfigTf2Ir);
  if (iter != json_read.end()) {
    ir_map.tf2ir = iter.value().get<std::vector<OpMapInfo>>();
  }
}
} // namespace aicpu
