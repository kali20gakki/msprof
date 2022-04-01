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
#include "tensorflow_util.h"
#include "util/log.h"
#include "error_code/error_code.h"
#include "util/util.h"

using domi::tensorflow::AttrValue;
using domi::tensorflow::NodeDef;

using AttrValueMap = google::protobuf::Map<std::string, AttrValue>;

namespace aicpu {
bool TensorFlowUtil::AddNodeAttr(const std::string &attr_name, const AttrValue &attr_value, NodeDef *node_def) {
  AICPU_CHECK_NOTNULL_ERRCODE(node_def, false);
  auto pair = node_def->mutable_attr()->insert(AttrValueMap::value_type(attr_name, attr_value));
  if (!pair.second) {
    AICPUE_LOGD("Insert attr[%s] to op[%s] failed.",
                attr_name.c_str(), node_def->name().c_str());
    return false;
  }
  return true;
}

bool TensorFlowUtil::FindAttrValue(const NodeDef *node_def, const std::string attr_name, AttrValue &attr_value) {
  AICPU_CHECK_NOTNULL_ERRCODE(node_def, false);
  const AttrValueMap &attr = node_def->attr();
  auto iter = attr.find(attr_name);
  if (iter != attr.end()) {
    attr_value = iter->second;
    return true;
  }
  return false;
}
}
