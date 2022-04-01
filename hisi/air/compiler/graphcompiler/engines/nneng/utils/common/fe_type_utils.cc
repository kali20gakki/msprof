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

#include "common/fe_type_utils.h"
#include <vector>
#include <sstream>
#include <climits>
#include "graph/debug/ge_attr_define.h"
#include "framework/common/types.h"
#include "common/comm_log.h"
#include "common/constants_define.h"
#include "common/string_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/attr_utils.h"
#include "common/util/error_manager/error_manager.h"

namespace fe {
std::string GetRealPath(const std::string &path) {
  if (path.empty()) {
    CM_LOGI("path string is nullptr.");
    return "";
  }
  if (path.size() >= PATH_MAX) {
    CM_LOGI("file path %s is too long! ", path.c_str());
    return "";
  }

  // PATH_MAX is the system marco，indicate the maximum length for file path
  // pclint check one param in stack can not exceed 1K bytes
  char resoved_path[PATH_MAX] = {0x00};

  std::string res;

  // path not exists or not allowed to read return nullptr
  // path exists and readable, return the resoved path
  if (realpath(path.c_str(), resoved_path) != nullptr) {
    res = resoved_path;
  } else {
    CM_LOGI("path %s is not exist.", path.c_str());
  }
  return res;
}

void ReportErrorMessage(std::string error_code, const std::map<std::string, std::string> &args_map) {
  int result = ErrorManager::GetInstance().ReportErrMessage(error_code, args_map);
  if (result != 0) {
    CM_LOGI("Faild to call ErrorManager::GetInstance(). ReportErrMessage");

  }
}

Status String2DataType(std::string dtype_str, ge::DataType &dtype) {
  std::string dtype_str_trim = StringUtils::Trim(dtype_str);
  if (STR_DTYPE_MAP.end() == STR_DTYPE_MAP.find(dtype_str_trim)) {
    CM_LOGE("Not found this dtype %s in struct STR_DTYPE_MAP.", dtype_str.c_str());
    return fe::FAILED;
  }
  dtype = STR_DTYPE_MAP.at(dtype_str_trim);
  return fe::SUCCESS;
}

std::string GetStrByFormatVec(const std::vector<ge::Format>& format_vec) {
  string result;
  size_t size = format_vec.size();
  for (size_t i = 0; i < size; ++i) {
    string format = ge::TypeUtils::FormatToSerialString(format_vec[i]);
    result += ge::TypeUtils::FormatToSerialString(format_vec[i]);
    if (i != size - 1) {
      result += ",";
    }
  }
  return result;
}

std::string GetStrByDataTypeVec(const std::vector<ge::DataType>& data_type_vec) {
  std::string result;
  size_t size = data_type_vec.size();
  for (size_t i = 0; i < size; ++i) {
    std::string data_type = ge::TypeUtils::DataTypeToSerialString(data_type_vec[i]);
    result += data_type;
    if (i != size - 1) {
      result += ",";
    }
  }
  return result;
}

std::string GetOpPatternString(OpPattern op_pattern) {
  auto iter = OP_PATTERN_STRING_MAP.find(op_pattern);
  if (iter == OP_PATTERN_STRING_MAP.end()) {
    return "unknown-op-pattern";
  } else {
    return iter->second;
  }
}

std::string GetPrecisionPolicyString(PrecisionPolicy precision_policy) {
  auto iter = PRECISION_POLICY_STRING_MAP.find(precision_policy);
  if (iter == PRECISION_POLICY_STRING_MAP.end()) {
    return "unknown-precision-policy";
  } else {
    return iter->second;
  }
}

bool IsMemoryEmpty(const ge::GeTensorDesc &tensor_desc) {
    auto memory_size_calc_type = static_cast<int64_t>(ge::MemorySizeCalcType::NORMAL);
    (void)ge::AttrUtils::GetInt(tensor_desc, ge::ATTR_NAME_MEMORY_SIZE_CALC_TYPE, memory_size_calc_type);
    return memory_size_calc_type == static_cast<int64_t>(ge::MemorySizeCalcType::ALWAYS_EMPTY);
}

bool IsSubGraphData(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr || op_desc_ptr->GetType() != "Data") {
    return false;
  }
  return op_desc_ptr->HasAttr(ge::ATTR_NAME_PARENT_NODE_INDEX);
}

bool IsSubGraphNetOutput(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr || op_desc_ptr->GetType() != "NetOutput") {
    return false;
  }
  for (auto &tensor : op_desc_ptr->GetAllInputsDescPtr()) {
    if (ge::AttrUtils::HasAttr(tensor, ge::ATTR_NAME_PARENT_NODE_INDEX)) {
      return true;
    }
  }
  return false;
}
}  // namespace fe
