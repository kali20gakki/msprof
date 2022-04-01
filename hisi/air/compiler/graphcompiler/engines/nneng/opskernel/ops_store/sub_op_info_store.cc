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

#include "ops_store/sub_op_info_store.h"
#include <dirent.h>
#include <fstream>
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "common/string_utils.h"
#include "common/fe_type_utils.h"
#include "common/configuration.h"
#include "ops_store/ops_kernel_utils.h"
#include "ops_store/ops_kernel_error_codes.h"
#include "ops_store/ops_kernel_constants.h"

namespace fe {
SubOpInfoStore::SubOpInfoStore(const FEOpsStoreInfo &ops_store_info)
    : init_flag_(false), ops_store_info_(ops_store_info) {}
SubOpInfoStore::~SubOpInfoStore() {}

Status SubOpInfoStore::Initialize(const std::string &engine_name) {
  if (init_flag_) {
    return SUCCESS;
  }
  init_flag_ = true;

  if (ops_store_info_.fe_ops_store_name.empty()) {
    FE_LOGW("The name of ops information library is empty.");
    return OP_STORE_CFG_NAME_EMPTY;
  }
  if (ops_store_info_.cfg_file_path.empty()) {
    FE_LOGW("The config file path of op information library[%s] is empty.", ops_store_info_.fe_ops_store_name.c_str());
    return OP_STORE_CFG_FILE_EMPTY;
  }

  std::string cfg_real_path = GetRealPath(ops_store_info_.cfg_file_path);
  if (cfg_real_path.empty()) {
    FE_LOGW("The config file[%s] of op information library[%s] is not existed. ",
            ops_store_info_.cfg_file_path.c_str(), ops_store_info_.fe_ops_store_name.c_str());
    return OP_STORE_CFG_FILE_NOT_EXIST;
  }

  FE_LOGD("Initialize %s SubOpsStore begin.", ops_store_info_.fe_ops_store_name.c_str());
  Status ret_value = LoadOpInfo(cfg_real_path);
  if (ret_value != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Init] Initialize sub op info library:[%s] LoadOpInfo failed! Config file_path:[%s]! \
                    Return value is [%u].", ops_store_info_.fe_ops_store_name.c_str(),
                    ops_store_info_.cfg_file_path.c_str(), ret_value);
    return OP_STORE_READ_CFG_FILE_FAILED;
  }

  std::string modify_mixlist_path;
  Configuration::Instance(engine_name).GetModifyMixlist(modify_mixlist_path);
  if (!modify_mixlist_path.empty()) {
    std::string real_modify_mixlist_path = GetRealPath(modify_mixlist_path);
    if (!real_modify_mixlist_path.empty()) {
      ret_value = LoadCustomJsonFile(real_modify_mixlist_path);
      if (ret_value != SUCCESS) {
        return ret_value;
      }
    } else {
      FE_LOGW("File[%s] is not exist.", modify_mixlist_path.c_str());
    }
  }

  ret_value = ConstructOpKernelInfo(engine_name);
  if (ret_value != SUCCESS) {
    return ret_value;
  }
  FE_LOGI("Size of sub ops store %s is %zu.",
          ops_store_info_.fe_ops_store_name.c_str(), op_kernel_info_map_.size());
  FE_LOGI("Initialize %s SubOpsStore finished.", ops_store_info_.fe_ops_store_name.c_str());
  return SUCCESS;
}

Status SubOpInfoStore::LoadOpInfo(const string &real_path) {
  vector<string> ops_cfg_files;
  DIR *dir;
  struct dirent *dirp = nullptr;
  char *file_suffix = const_cast<char *>(".json");
  dir = opendir(real_path.c_str());
  FE_CHECK(dir == nullptr, FE_LOGE("Fail to open directory %s.", real_path.c_str()), return FAILED);
  while ((dirp = readdir(dir)) != nullptr) {
    if (dirp->d_name[0] == '.') {
      continue;
    }
    if (strlen(dirp->d_name) <= strlen(file_suffix)) {
      continue;
    }
    if (strcmp(&(dirp->d_name)[strlen(dirp->d_name) - strlen(file_suffix)], file_suffix) == 0) {
      ops_cfg_files.push_back(real_path + "/" + dirp->d_name);
    }
  }
  closedir(dir);

  if (ops_cfg_files.empty()) {
    FE_LOGI("There is no json file in path %s.", real_path.c_str());
    return SUCCESS;
  }

  for (std::string json_file_path : ops_cfg_files) {
    if (LoadOpJsonFile(json_file_path) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Init][LoadOpInfo] Fail to load json file[%s].", json_file_path.c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status SubOpInfoStore::LoadOpJsonFile(const std::string &json_file_path) {
  vector<string> op_type_vec;
  nlohmann::json op_json_file;
  FE_LOGD("Begin to load json file[%s].", json_file_path.c_str());
  if (ReadJsonObject(json_file_path, op_json_file) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadOpJsonFile] ReadJsonObject in %s failed.", json_file_path.c_str());
    return FAILED;
  }
  try {
    if (!op_json_file.is_object()) {
      REPORT_FE_ERROR("[GraphOpt][Init][LoadOpJsonFile] Top level of json file should be object, actually is %s.",
                      GetJsonObjectType(op_json_file).c_str());
      return OP_SUB_STORE_ILLEGAL_JSON;
    }
    for (auto &elem : op_json_file.items()) {
      string op_type_temp = elem.key();
      op_type_vec.push_back(StringUtils::Trim(op_type_temp));
    }
    for (auto &op_type : op_type_vec) {
      OpContent op_content;
      op_content.op_type_ = op_type;
      if (!op_json_file[op_type].is_object()) {
        REPORT_FE_ERROR("[GraphOpt][Init][LoadOpJsonFile] Second level of json file should be object, actually is %s.",
                        GetJsonObjectType(op_json_file[op_type]).c_str());
        return OP_SUB_STORE_ILLEGAL_JSON;
      }
      for (auto &elem_out : op_json_file[op_type].items()) {
        map <string, string> map_temp;
        if (!op_json_file[op_type][elem_out.key()].is_object()) {
          REPORT_FE_ERROR("[GraphOpt][Init][LoadOpJsonFile] Third level of json file should be object, actually is %s.",
                          GetJsonObjectType(op_json_file[op_type][elem_out.key()]).c_str());
          return OP_SUB_STORE_ILLEGAL_JSON;
        }
        for (auto &elem_inner : op_json_file[op_type][elem_out.key()].items()) {
          string key_inner = elem_inner.key();
          string value_inner = elem_inner.value();
          map_temp.emplace(std::make_pair(StringUtils::Trim(key_inner), StringUtils::Trim(value_inner)));
        }
        string key_out = elem_out.key();
        op_content.map_kernel_info_.emplace(std::make_pair(StringUtils::Trim(key_out), map_temp));
      }
      op_content_map_.emplace(std::make_pair(op_type, op_content));
    }
  } catch (const std::exception &e) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadOpJsonFile] Fail to convfile[%s] to Json.Error message is %s",
                    json_file_path.c_str(), e.what());
    return OP_SUB_STORE_ILLEGAL_JSON;
  }
  return SUCCESS;
}

Status SubOpInfoStore::ConstructOpKernelInfo(const std::string &engine_name) {
  OpKernelInfoConstructor op_kernel_info_constructor;
  for (auto &op : op_content_map_) {
    OpKernelInfoPtr op_kernel_info_ptr = nullptr;
    FE_MAKE_SHARED(op_kernel_info_ptr = std::make_shared<OpKernelInfo>(op.first), return FAILED);
    FE_CHECK_NOTNULL(op_kernel_info_ptr);
    op_kernel_info_ptr->SetImplType(ops_store_info_.op_impl_type);
    /* Here shared ptr and map/vector will destruct automatically, so not necessary
       to run final for those op_kernel_info which fails to initialize. */
    if (op_kernel_info_constructor.InitializeOpKernelInfo(engine_name, op.second, op_kernel_info_ptr) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Init][ConstructOpKernelInfo] opKernelInfo %s initialize failed.", op.first.c_str());
      return FAILED;
    }

    op_kernel_info_map_.emplace(std::make_pair(op.first, op_kernel_info_ptr));
  }
  return SUCCESS;
}

Status SubOpInfoStore::Finalize() {
  if (!init_flag_) {
    return SUCCESS;
  }
  OpKernelInfoConstructor op_kernel_info_constructor;
  for (auto &el : this->op_kernel_info_map_) {
    if (el.second == nullptr) {
      FE_LOGW("OpKernelInfo pointer[%s] in op_kernel_info_map_ should not be nullptr.",
              el.first.c_str());
      continue;
    }
    op_kernel_info_constructor.FinalizeOpKernelInfo(el.second);
  }
  this->op_kernel_info_map_.clear();
  init_flag_ = false;
  return SUCCESS;
}

const std::map<std::string, OpKernelInfoPtr>& SubOpInfoStore::GetAllOpKernels() {
  return this->op_kernel_info_map_;
}

OpKernelInfoPtr SubOpInfoStore::GetOpKernelByOpType(const std::string &op_type) {
  std::map<std::string, OpKernelInfoPtr>::const_iterator iter = op_kernel_info_map_.find(op_type);
  FE_LOGD("Size of all kernels is %zu.", op_kernel_info_map_.size());
  if (iter == op_kernel_info_map_.end()) {
    FE_LOGD("Op Type[%s] is not existed in op information library[%s].",
            op_type.c_str(), this->ops_store_info_.fe_ops_store_name.c_str());
    return nullptr;
  }
  return iter->second;
}

Status SubOpInfoStore::GetOpContentByOpType(const std::string &op_type, OpContent &op_content) const {
  auto iter = op_content_map_.find(op_type);
  if (iter == op_content_map_.end()) {
    FE_LOGD("Op Type[%s] is not found in op information library[%s].",
            op_type.c_str(), this->ops_store_info_.fe_ops_store_name.c_str());
    return FAILED;
  }
  op_content = iter->second;
  return SUCCESS;
}

Status SubOpInfoStore::SetOpContent(const OpContent &op_content) {
  if (op_content.op_type_.empty()) {
    REPORT_FE_ERROR("[GraphOpt][SetDynamicinfo][SetOpContent] Op type of op_content is empty.");
    return FAILED;
  }
  op_content_map_[op_content.op_type_] = op_content;
  return SUCCESS;
}

const std::string& SubOpInfoStore::GetOpsStoreName() const {
  return this->ops_store_info_.fe_ops_store_name;
}

const OpImplType& SubOpInfoStore::GetOpImplType() const {
  return this->ops_store_info_.op_impl_type;
}
bool IsFilePathValid(std::string file_path, std::string delimiter) {
  if (file_path.empty()) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] File path [%s] is none.", file_path.c_str());
    return false;
  }

  if (file_path.find(delimiter) == std::string::npos) {
    FE_LOGD("[GraphOpt][Init][LoadJson] File path [%s] not contains [%s].", file_path.c_str(), delimiter.c_str());
    file_path = "./" + file_path;
  }

  for (char ch_id : file_path) {
    if (delimiter.find(ch_id) != std::string::npos) {
      continue;
    }
    if (!((ch_id >= '0' && ch_id <= '9') || (ch_id >= 'a' && ch_id <= 'z') || (ch_id >= 'A' && ch_id <= 'Z') ||
        ch_id == '_' || ch_id == '-' || ch_id == '.')) {
      REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] File path [%s] should be '0'~'9' or 'a'~'z' or 'A'~'Z'"
                      "or '_' or '-' or '.'.", file_path.c_str());
      return false;
    }
  }
  return true;
}

bool IsFileEmpty(const std::string &file_name) {
  std::ifstream ifs(file_name);
  if (!ifs.is_open()) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] File [%s] doesn't exist or has been opened.", file_name.c_str());
    return false;
  }
  char ch;
  ifs >> ch;
  if (ifs.eof()) {
    ifs.close();
    return true;
  }
  ifs.close();
  return false;
}

void GroupPrecisionReduceUpdateInfo(nlohmann::json op_json_file, const std::string &list_type,
                                    const std::string &update_type,
                                    std::map<std::string, std::uint8_t> &update_map) {
  for (auto &op_type : op_json_file[list_type][update_type]) {
    uint8_t bitmap = 0;
    if (update_type == "to-remove") {
      bitmap = kPrecisionReduceToRemoveGrayList;
    } else {
      bitmap = kPrecisionReduceToAddGrayList;
    }

    if (list_type == "black-list") {
      bitmap <<= kPrecisionReduceBlackShift;
    } else if (list_type == "white-list") {
      bitmap <<= kPrecisionReduceWhiteShift;
    }

    auto iter = update_map.find(op_type);
    if (iter == update_map.end()) {
      update_map.emplace(std::make_pair(op_type, bitmap));
      continue;
    }
    iter->second |= bitmap;
  }
}

Status SubOpInfoStore::LoadModifyMixlistJson(const std::string &modify_mixlist_path) {
  nlohmann::json op_json_file;
  if (ReadJsonObject(modify_mixlist_path, op_json_file) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] Read modify mixlist json in [%s] fail.", modify_mixlist_path.c_str());
    return OP_SUB_STORE_ILLEGAL_JSON;
  }

  std::map<std::string, std::uint8_t> update_map;
  try {
    if (!op_json_file.is_object()) {
      REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] Top level of json [%s] isn't json object.",
                      GetJsonObjectType(op_json_file).c_str());
      return OP_SUB_STORE_ILLEGAL_JSON;
    }

    for (auto &elem : op_json_file.items()) {
      auto list_type = elem.key();
      if (kPrecisionReduceListType.find(list_type) == kPrecisionReduceListType.end()) {
        REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] Top level of json [%s] should be 'gray-list' or 'white-list' or "
                        "'black-list'.", list_type.c_str());
        return OP_SUB_STORE_ILLEGAL_JSON;
      }

      if (!op_json_file[list_type].is_object()) {
        REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] Second level of json [%s] isn't json object.",
                        GetJsonObjectType(op_json_file[list_type]).c_str());
        return OP_SUB_STORE_ILLEGAL_JSON;
      }
      for (auto &precision_reduce_update_type : op_json_file[list_type].items()) {
        auto update_type = precision_reduce_update_type.key();
        if (kPrecisionReduceUpdateType.find(update_type) == kPrecisionReduceUpdateType.end()) {
          REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] Second level of json [%s] should be 'to-add' or 'to-remove'.",
                          update_type.c_str());
          return OP_SUB_STORE_ILLEGAL_JSON;
        }

        if (!op_json_file[list_type][update_type].is_array()) {
          REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] Third level of json [%s] should be array.",
                          GetJsonObjectType(op_json_file[list_type][update_type]).c_str());
          return OP_SUB_STORE_ILLEGAL_JSON;
        }

        GroupPrecisionReduceUpdateInfo(op_json_file, list_type, update_type, update_map);
      }
    }
  } catch (const std::exception &e) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] Fail to convfile [%s] to json, error message is [%s].",
                    modify_mixlist_path.c_str(), e.what());
    return OP_SUB_STORE_ILLEGAL_JSON;
  }
  return UpdateOpInfoStore(update_map);
};

Status SubOpInfoStore::LoadCustomJsonFile(const std::string &modify_mixlist_path) {
  FE_LOGD("[GraphOpt][Init][LoadJson] Begin to load modify mixlist json file [%s].", modify_mixlist_path.c_str());

  if (!IsFilePathValid(modify_mixlist_path, "/")) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] The file path [%s] is illegal.", modify_mixlist_path.c_str());
    return OP_SUB_STORE_ILLEGAL_JSON;
  }
  if (IsFileEmpty(modify_mixlist_path)) {
    REPORT_FE_ERROR("[GraphOpt][Init][LoadJson] The file path [%s] is empty.", modify_mixlist_path.c_str());
    return OP_SUB_STORE_ILLEGAL_JSON;
  }

  return LoadModifyMixlistJson(modify_mixlist_path);
}

void SubOpInfoStore::UpdateStrToOpContent(OpContent &op_content,
                                          const std::string key1,
                                          const std::string key2,
                                          const std::string &value) {
  const auto &key1_pos = op_content.map_kernel_info_.find(key1);
  if (key1_pos == op_content.map_kernel_info_.end()) {
    if (value.empty()) {
      return;
    }
    std::map<std::string, std::string> map_temp;
    map_temp.emplace(std::make_pair(key2, value));
    op_content.map_kernel_info_.emplace(std::make_pair(key1, map_temp));
    return;
  }

  const auto &key2_pos = key1_pos->second.find(key2);
  if (key2_pos == key1_pos->second.end()) {
    if (value.empty()) {
      return;
    }
    key1_pos->second.emplace(std::make_pair(key2, value));
    return;
  }
  key2_pos->second = value;
}

Status SubOpInfoStore::UpdateOpInfoStore(const std::map<std::string, std::uint8_t> &update_map) {
  for (auto &map : update_map) {
    auto op_type = map.first;
    if (kPrecisionReduceUpdateTemplate.find(map.second) == kPrecisionReduceUpdateTemplate.end()) {
      REPORT_FE_ERROR("[GraphOpt][Init][UpdateOpInfoStore] Op_type [%s] update is invalid, and update map is [0x%x].",
                      op_type.c_str(), map.second);
      return OP_SUB_STORE_ILLEGAL_JSON;
    }

    auto iter = op_content_map_.find(op_type);
    if (iter == op_content_map_.end()) {
      FE_LOGD("[GraphOpt][Init][UpdateOpInfoStore] Op_type [%s] isn't find in op information library.",
              op_type.c_str());
      continue;
    }

    auto op_content = iter->second;
    std::string precision_reduce;
    OpKernelInfoConstructor op_kernel_info_constructor;
    (void) op_kernel_info_constructor.GetStrFromOpContent(op_content, STR_PRECISION_POLICY, STR_FLAG,
                                                          precision_reduce);

    if (((map.second & kPrecisionReduceToRemoveBlackList) && precision_reduce != "false") ||
        ((map.second & kPrecisionReduceToRemoveWhiteList) && precision_reduce != "true") ||
        ((map.second & kPrecisionReduceToRemoveGrayList) && (!precision_reduce.empty()))) {
      REPORT_FE_ERROR("[GraphOpt][Init][UpdateOpInfoStore] The remove list of op_type [%s] is not match with "
                      "precision_reduce [%s], and mapping is [0x%x].",
                      op_type.c_str(), precision_reduce.c_str(), map.second);
      return OP_SUB_STORE_ILLEGAL_JSON;
    }

    std::string value;
    if (map.second & kPrecisionReduceToAddBlackList) {
      value = "false";
    } else if (map.second & kPrecisionReduceToAddWhiteList) {
      value = "true";
    } else {
      if (precision_reduce.empty()) {
        continue;
      }
    }

    FE_LOGD("[GraphOpt][Init][UpdateOpInfoStore] Op_type [%s] will change precision_reduce from [%s] to [%s].",
            op_type.c_str(), precision_reduce.c_str(), value.c_str());
    UpdateStrToOpContent(op_content, STR_PRECISION_POLICY, STR_FLAG, value);
    iter->second = op_content;
  }
  return SUCCESS;
}
}  // namespace fe
