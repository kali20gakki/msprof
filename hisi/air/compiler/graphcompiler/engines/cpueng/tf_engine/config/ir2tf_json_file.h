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
#ifndef AICPU_IR2TF_JSON_FILE_H_
#define AICPU_IR2TF_JSON_FILE_H_

#include <nlohmann/json.hpp>
#include "ir2tf/ir2tf_struct.h"

namespace {
// ir mapping json file configuration item: Source input/output name
const std::string kIrMappingConfigSrcInoutName = "srcInOutputName";

// ir mapping json file configuration item: Target input/output name
const std::string kIrMappingConfigDstInoutName = "dstInOutputName";

// ir mapping json file configuration item: field is ref type or not
const std::string kIrMappingConfigIsRef = "isRef";

// ir mapping json file configuration item: output Ref convert
const std::string kIrMappingConfigOutputRefDesc = "outputRefDesc";

// ir mapping json file configuration item: attr extend convert
const std::string kIrMappingConfigAttrExtDesc = "attrExtDesc";

// ir mapping json file configuration item: field name
const std::string kIrMappingConfigFieldName = "fieldName";

// ir mapping json file configuration item: type of field
const std::string kIrMappingConfigDataType = "dataType";

// ir mapping json file configuration item: default value of field
const std::string kIrMappingConfigDefaultValue = "defaultValue";

// ir mapping json file configuration item: all attrs map
const std::string kIrMappingConfigAttrsDesc = "attrsMapDesc";

// ir mapping json file configuration item: source field name
const std::string kIrMappingConfigSrcFieldName = "srcFieldName";

// ir mapping json file configuration item: target field name
const std::string kIrMappingConfigDstFieldName = "dstFieldName";

// ir mapping json file configuration item: parserExpress
const std::string kIrMappingConfigParseExpress = "parserExpress";

// ir mapping json file configuration item: all dynamic desc
const std::string kIrMappingConfigDynamicDesc = "attrsDynamicDesc";

// ir mapping json file configuration item:  index
const std::string kIrMappingConfigIndex = "index";

// ir mapping json file configuration item: type
const std::string kIrMappingConfigType = "type";

// ir mapping json file configuration item: name
const std::string kIrMappingConfigName = "name";

// ir mapping json file configuration item: source op type
const std::string kIrMappingConfigSrcOpType = "srcOpType";

// ir mapping json file configuration item: target op type
const std::string kIrMappingConfigDstOpType = "dstOpType";

// ir mapping json file configuration item: version tag
const std::string kIrMappingConfigVersion = "version";

// ir mapping json file configuration item: source is ir op,target is tf op
const std::string kIrMappingConfigIr2Tf = "IR2TF";

// ir mapping json file configuration item: source is tf op,target is ir op
const std::string kIrMappingConfigTf2Ir = "TF2IR";

// ir mapping json file configuration item: src atrrs to dest input attr mapping
const std::string kIrMappingConfigAttrsInputMapDesc = "attrsInputMapDesc";

// ir mapping json file configuration item: src input attr to dst attr mapping
const std::string kIrMappingConfigInputAttrMapDesc = "inputAttrMapDesc";

// ir mapping json file configuration item: src atrrs to dest output attr mapping
const std::string kIrMappingConfigAttrsOutputMapDesc = "outputAttrMapDesc";

// ir mapping json file configuration item: src output attr to dst attr mapping
const std::string kIrMappingConfigOutputAttrMapDesc = "outputAttrMapDesc";

// ir mapping json file configuration item: src input attr to dst input attr mapping
const std::string kIrMappingConfigInputRefMapDesc = "inputRefMapDesc";

// ir mapping json file configuration item: op attr blacklist
const std::string kIrMappingConfigAttrsBlacklist = "attrsBlacklist";
}

namespace aicpu {
/**
 * RefTransDesc json to struct object function
 * @param json_read read json handle
 * @param ref_desc for ref convert
 * @return whether read file successfully
 */
void from_json(const nlohmann::json &json_read, RefTransDesc &ref_desc);

/**
 * ParserExpDesc json to struct object function
 * @param json_read read json handle
 * @param parse_desc for op attrs convert
 * @return whether read file successfully
 */
void from_json(const nlohmann::json &json_read, ParserExpDesc &parse_desc);

/**
 * OpMapInfo json to struct object function
 * @param json_read read json handle
 * @param op_map_info all op attrs config
 * @return whether read file successfully
 */
void from_json(const nlohmann::json &json_read, OpMapInfo &op_map_info);

/**
 * IRFMKOpMapLib json to struct object function
 * @param json_read read json handle
 * @param ir_map ir to tf config
 * @return whether read file successfully
 */
void from_json(const nlohmann::json &json_read, IRFMKOpMapLib &ir_map);

/**
 * DynamicExpDesc json to struct object function
 * @param json_read read json handle
 * @param dynamic_desc ir to tf config
 * @return whether read file successfully
 */
void from_json(const nlohmann::json &json_read, DynamicExpDesc &dynamic_desc);

/**
 * ExtendFieldDesc json to struct object function
 * @param json_read read json handle
 * @param ext_desc extend attr config
 * @return whether read file successfully
 */
void from_json(const nlohmann::json &json_read, ExtendFieldDesc &ext_desc);
} // namespace aicpu
#endif //AICPU_IR2TF_JSON_FILE_H_