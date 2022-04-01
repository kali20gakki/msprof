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

#include "framework/common/file_constant_util.h"
#include <fstream>
#include "framework/common/debug/log.h"
#include "ge/ge_api_types.h"
#include "ge_call_wrapper.h"
#include "graph/def_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_context.h"
#include "graph/ge_tensor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/file_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "runtime/mem.h"

namespace ge {
namespace {
const int64_t kBlockSize = 10485760;
const std::string kBinFileValues = "value_bins";
const std::string kBinIdValue = "value_bin_id";
const std::string kBinFilePathValue = "value_bin_file";
const std::int32_t kFirstElementIndex = 0;
const std::int32_t kIndentWidth = 2;
const std::string kAttrNameFilePath = "file_path";
}

void from_json(const nlohmann::json &j, FileConstantInfo &info) {
  const auto id = j.find(kBinIdValue);
  if (id != j.end()) {
    info.value_bin_file_id = id->get<std::string>();
  }

  const auto file_path = j.find(kBinFilePathValue);
  if (file_path != j.end()) {
    info.value_bin_file_path = file_path->get<std::string>();
  }
}

void from_json(const nlohmann::json &j, OptionInfo &option_info) {
  const auto it = j.find(kBinFileValues);
  if (it != j.end()) {
    option_info = it->get<OptionInfo>();
  }
}

Status GetFilePathFromOption(std::map<std::string, std::string> &file_id_and_path_map) {
  std::string opt;
  (void)GetContext().GetOption(FILE_CONSTANT_PATH, opt);
  if (opt.empty()) {
    GELOGW("[Check][Param] Failed to get file constant path.");
    return SUCCESS;
  }
  GELOGI("source string = %s.", opt.c_str());

  nlohmann::json options;
  try {
    options = nlohmann::json::parse(opt);
  } catch (nlohmann::json::exception &ex) {
    REPORT_CALL_ERROR("E19999", "Failed to parse option FILE_CONSTANT_PATH, which [%s] is invalid", opt.c_str());
    GELOGE(GRAPH_FAILED, "Failed to parse option FILE_CONSTANT_PATH, which [%s] is invalid", opt.c_str());
    return GRAPH_FAILED;
  }

  for (const nlohmann::json &single_json : options) {
    GELOGD("Parsing op[%d], jsonStr = %s.", kFirstElementIndex, single_json.dump(kIndentWidth).c_str());
    std::vector<FileConstantInfo> multi_info;
    multi_info = single_json.get<std::vector<FileConstantInfo>>();
    for (const auto &single_info : multi_info) {
      GELOGD("get single info, file id is %s, file path is %s.", single_info.value_bin_file_id.c_str(),
             single_info.value_bin_file_path.c_str());
      (void)file_id_and_path_map.insert(
          std::pair<std::string, std::string>(single_info.value_bin_file_id, single_info.value_bin_file_path));
    }
  }
  return SUCCESS;
}

Status CopyOneWeightFromFile(const void *const curr_dev_ptr, const std::string &value, const size_t file_constant_size,
                             size_t &left_size) {
  if (left_size < file_constant_size) {
    GELOGE(GRAPH_FAILED, "Failed to copy data to device, free memory is %zu, need copy size = %ld.", left_size,
           file_constant_size);
    return GRAPH_FAILED;
  }
  const std::string real_path = RealPath(value.c_str());
  std::ifstream ifs(real_path, std::ifstream::binary);
  if (!ifs.is_open()) {
    GELOGE(GRAPH_FAILED, "[Open][File] %s failed.", real_path.c_str());
    REPORT_CALL_ERROR("E19999", "open file:%s failed.", real_path.c_str());
    return GRAPH_FAILED;
  }
  size_t used_memory = 0U;
  std::string compress_nodes;
  compress_nodes.reserve(static_cast<size_t>(kBlockSize));
  Status ret = SUCCESS;
  while ((!ifs.eof()) && (used_memory != file_constant_size)) {
    (void) ifs.read(&compress_nodes[0U], kBlockSize);
    size_t copy_len_once = static_cast<size_t>(ifs.gcount());
    if ((file_constant_size - used_memory) < copy_len_once) {
      copy_len_once = file_constant_size - used_memory;
    }
    if (left_size < (used_memory + copy_len_once)) {
      GELOGE(GRAPH_FAILED, "copy failed for lack memory, free size is %zu, need memroy is %zu.", left_size,
             used_memory + copy_len_once);
      REPORT_CALL_ERROR("E19999", "copy failed for lack memory, free size is %zu, need memroy is %zu.", left_size,
                        used_memory + copy_len_once);
      ret = FAILED;
      break;
    }

    GELOGI("copy %zu bytes to memory.", copy_len_once);
    void *const cur_dev_ptr = reinterpret_cast<void *>(PtrToValue(curr_dev_ptr) + used_memory);
    const rtError_t rts_error =
        rtMemcpy(cur_dev_ptr, left_size - used_memory, &compress_nodes[0U], copy_len_once, RT_MEMCPY_HOST_TO_DEVICE);
    if (rts_error != RT_ERROR_NONE) {
      GELOGE(GRAPH_FAILED, "copy failed, result code = %d.", rts_error);
      REPORT_CALL_ERROR("E19999", "copy failed, result code = %d.", rts_error);
      ret = RT_ERROR_TO_GE_STATUS(rts_error);
      break;
    }
    used_memory += copy_len_once;
  }
  ifs.close();
  left_size -= used_memory;
  GELOGI("used memory is %zu.", used_memory);
  return ret;
}

Status GetFilePath(const OpDescPtr &op_desc, const std::map<std::string, std::string> &file_id_and_path_map,
                   std::string &file_path) {
  std::string file_path_attr;
  (void)AttrUtils::GetStr(op_desc, kAttrNameFilePath, file_path_attr);
  if (!file_path_attr.empty()) {
    file_path = file_path_attr;
    return SUCCESS;
  }
  std::string file_id;
  file_path = "";
  GE_CHK_BOOL_RET_STATUS(AttrUtils::GetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID, file_id), FAILED,
                         "Failed to get filed id from attr");
  GE_CHK_BOOL_RET_STATUS(!file_id.empty(), FAILED, "The file id is empty.");
  GELOGI("copy data to the device memory of file constant, file id is %s.", file_id.c_str());
  const auto it = file_id_and_path_map.find(file_id);
  if (it == file_id_and_path_map.end()) {
    REPORT_INNER_ERROR("E19999", "Failed to get file path.");
    GELOGE(FAILED, "[Check][Param] Failed to get file path.");
    return FAILED;
  }
  GE_CHK_BOOL_RET_STATUS(!(it->second.empty()), FAILED, "File path is empty.");
  file_path = it->second;
  return SUCCESS;
}
}  // namespace ge
