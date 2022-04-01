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

#include "tbe_json_parse_impl.h"
#include <fstream>
#include <thread>
#include <ext/stdio_filebuf.h>
#include <sys/file.h>
#include "common/fe_inner_attr_define.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/util/json_util.h"
#include "tensor_engine/fusion_api.h"

using nlohmann::json;

namespace fe {
const std::string JSON_PARAM_COMPRESS_PARAMETERS = "compress_parameters";
const std::string JSON_PARAM_WEIGHT_REPEAT = "weight_repeat";

const int32_t kMaxBlockDim = 65535;
const uint32_t kLockRecursiveIntvl = 10; // 10 milliseconds
const uint32_t kLockRecursiveCntMax = 6000; // 1mins 6000*10 milliseconds

using JsonExpcetion = nlohmann::json::exception;

/*
*  @ingroup fe
*  @brief  parse the block_dim info in handle
*  @param   [in] handle
*  @param   [out] op_desc_ set block_dim according to the block_dim info in handle
*  @return SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::ParseTvmBlockDim(int32_t &block_dim) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);
  try {
    block_dim = (j->find("blockDim") == j->end()) ? 1 : static_cast<int32_t>(j->at("blockDim"));
    if (((block_dim) < -1) || (block_dim > kMaxBlockDim)) {
      FE_LOGE("blockDim[%d] is out of range, range is (-1, 65535).", block_dim);
      return FAILED;
    }

    if (block_dim == -1) {
      block_dim = 1;
    }
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the blockDim failed, exception:%s.", e.what());
    return FAILED;
  }
  return SUCCESS;
}

Status TbeJsonFileParseImpl::ParseOpKBHitrate(int32_t &op_hitrate) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);
  try {
    op_hitrate = (j->find(kKBHit) == j->end()) ? -1 : static_cast<int32_t>(j->at(kKBHit));
    if (((op_hitrate) < -1) || (op_hitrate > 1)) {
      FE_LOGE("op_hitrate[%d] is out of range, range is (-1, 1).", op_hitrate);
      return FAILED;
    }
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the op_hitrate failed, exception:%s.", e.what());
    return FAILED;
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the batch_bind_only info in handle
*  @param   [in] handle
*  @param   [out] op_desc_ set batch_bind_only according to
*   the batch_bind_only info in handle
*  @return SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::ParseBatchBindOnly(uint32_t &batch_bind_only) {
  const json *json_file = static_cast<json*>(handle_);
  FE_CHECK(json_file == nullptr, FE_LOGE("json is nullptr."), return FAILED);

  try {
    batch_bind_only = (json_file->find("batchBindOnly") == json_file->end())
                      ? 0
                      : static_cast<uint32_t>(json_file->at("batchBindOnly"));
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the batch_bind_only failed, exception:%s.", e.what());
    return FAILED;
  }

  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief parse the magic info in handle
*  @param   [in] handle
*  @param   [out] op_desc_ set magic according to magic info in handle
*  @return SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::ParseTvmMagic(std::string &magic) {
  try {
    // check magic key value
    if (j_.find("magic") == j_.end()) {
      FE_LOGE("Data error! 'magic' is necessary.");
      return FAILED;
    }
    json j_magic = j_.at("magic");
    magic = j_magic.get<std::string>();
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the magic failed, exception:%s.", e.what());
    magic = "RT_DEV_BINARY_MAGIC_ELF";
    return SUCCESS;
  }

  return SUCCESS;
}

Status TbeJsonFileParseImpl::ParseTvmCoreType(std::string &core_type) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);

  try {
    // check core_type key value, NOT required
    if (j->find("core_type") == j->end()) {
      return SUCCESS;
    }

    json j_core_type = j->at("core_type");
    core_type = j_core_type.get<std::string>();
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the core_type failed, exception:%s.", e.what());
    return FAILED;
  }

  return SUCCESS;
}

Status TbeJsonFileParseImpl::ParseTvmTaskRatio(uint32_t &task_ratio) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);

  try {
    // check core_type key value, NOT required
    if (j->find("taskRation") == j->end()) {
      return SUCCESS;
    }

    json j_task_ratio = j->at("taskRation");
    task_ratio = j_task_ratio.get<std::uint32_t>();
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the taskRation failed, exception:%s.", e.what());
    return FAILED;
  }
  return SUCCESS;
}

Status TbeJsonFileParseImpl::ParseModeInArgsFirstField(uint32_t &mode) {
  const json *json_file = static_cast<json*>(handle_);
  FE_CHECK(json_file == nullptr, FE_LOGE("json is nullptr."), return FAILED);

  try {
    mode = (json_file->find("modeInArgsFirstField") == json_file->end())
                      ? 0
                      : static_cast<uint32_t>(json_file->at("modeInArgsFirstField"));
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the modeInArgsFirstField failed, exception:%s.", e.what());
    return FAILED;
  }

  return SUCCESS;
}

Status TbeJsonFileParseImpl::ParseTvmKernelList(std::string &kernel_list_first) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);

  if (j->find("kernelList") != j->end()) {
    try {
      json kernel_list = j->at("kernelList");
      if (kernel_list.empty()) {
        FE_LOGD("kernel list is empty.");
        return SUCCESS;
      }
      try {
        json first_name = kernel_list.begin()->at("kernelName");
        kernel_list_first = first_name.get<std::string>();
        FE_LOGI("kernel list first node name is %s.", kernel_list_first.c_str());
      } catch (const JsonExpcetion &e) {
        FE_LOGE("ParseTvmKernelList get kernelName Exception:%s.", e.what());
      }
    } catch (const JsonExpcetion &e) {
      FE_LOGE("ParseTvmKernelList get kernelList Exception:%s.", e.what());
    }
  }
  return SUCCESS;
}

int32_t TransGeWorkspaceType(int32_t type) {
  int32_t res_type = 0;
  switch (type) {
    case te::TBE_MEMORY_DDR:
      res_type = RT_MEMORY_HBM;
      break;

    case te::TBE_MEMORY_L1:
      res_type = RT_MEMORY_L1;
      break;

    case te::TBE_MEMORY_L2:
      res_type = RT_MEMORY_L2;
      break;

    default:
      res_type = RT_MEMORY_HBM;
      break;
  }
  return res_type;
}

/* OpKernelBinPtr
 *  @ingroup fe
 *  @brief  parse the workspace info in handle
 *  @param   [in] handle
 *  @param   [out] op_desc_, tvm_workspace_sizes_  set workspace according to
 * block_dim info in handle
 *  @return SUCCESS or FAILED
 */
Status TbeJsonFileParseImpl::ParseTvmWorkSpace(vector<int64_t> &tvm_workspace_sizes,
                                               vector<int64_t> &tvm_workspace_types) {
  const json *j = static_cast<json *>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);
  if (j->find("workspace") == j->end()) {
    return SUCCESS;
  }

  try {
    json j_workspace = j->at("workspace");

    // make sure that both num and size exist
    if ((j_workspace.find("num") == j_workspace.end() || j_workspace.find("size") == j_workspace.end())) {
      FE_LOGE("Data error! 'num', 'size' and 'type' are necessary.");
      return FAILED;
    }

    // get number of workspace
    uint32_t workspace_num = j_workspace.at("num").get<uint32_t>();

    // get size and type of workspace
    json j_workspace_sizes = j_workspace.at("size");

    if (!j_workspace_sizes.is_array()) {
      FE_LOGE("Format error! 'size' should be array.");
      return FAILED;
    }

    if ((workspace_num != static_cast<uint32_t>(j_workspace_sizes.size()))) {
      FE_LOGE("Data error! 'num' is error,not equal to 'size' or 'type'.");
      return FAILED;
    }

    if (j_workspace.find("type") != j_workspace.end()) {
      json j_workspace_types = j_workspace.at("type");
      if (!j_workspace_types.is_array()) {
        FE_LOGE("Format error! 'type' should be array.");
        return FAILED;
      }
      if (workspace_num != static_cast<uint32_t>(j_workspace_types.size())) {
        FE_LOGE("Data error! 'num' is error, not equal to 'type'.");
        return FAILED;
      }
      for (auto const &type : j_workspace_types) {
        tvm_workspace_types.push_back(TransGeWorkspaceType(type.get<int32_t>()));
      }
    }

    for (auto const &size : j_workspace_sizes) {
      // add 32B memory for workspace, the memory obtained from the json file is
      // not enough
      tvm_workspace_sizes.push_back(size.get<int64_t>() + DATA_MEMORY_ALIGN_SIZE);
    }
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the workspace failed, exception:%s.", e.what());
    return FAILED;
  }

  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the parameters info in handle
*  @param   [in] handle
*  @param   [out] op_desc_, set output_index and workspace
*  according to parameters info in handle
*  @return SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::ParseTvmParameters(std::vector<int64_t> &parameters_index) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);
  if (j->find("parameters") == j->end()) {
    return SUCCESS;
  }

  try {
    json j_parameters = j->at("parameters");

    if (!j_parameters.is_array()) {
      FE_LOGE("Format error! 'parameters' should be array.");
      return FAILED;
    }

    // get parameters value
    for (auto it = j_parameters.begin(); it != j_parameters.end(); ++it) {
      json flag = *it;
      parameters_index.push_back(flag.get<int64_t>());
    }
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the parameters failed, exception:%s.", e.what());
    return FAILED;
  }

  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the meta_data info in handle
*  @param   [in] handle
*  @param   [out] op_desc_, set meta_data according to meta_data info in handle
*  @return SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::ParseTvmMetaData(std::string &meta_data) {
// 1. get bin_file_suffix
  std::string binfile_suffix = GetStrValueFromJson("binFileSuffix");

  // do not register MetaData when bin_file_suffix is not .so
  if (binfile_suffix != ".so") {
    FE_LOGD("'BinFileSuffix' is not '.so', skip.");
    return SUCCESS;
  }

  // 2. get bin_file_name
  std::string binfile_name = GetStrValueFromJson("binFileName");
  // check kernel name
  if (binfile_name.empty()) {
    FE_LOGE("BinFileName is empty. please check json file.");
    return FAILED;
  }

  // sha256, set sha256 to be null string when it is not specifed in json file
  std::string sha256 = GetStrValueFromJson("sha256");
  if (sha256.empty()) {
    FE_LOGE("BinFileSuffix is so but json info is not enough. please check json file.");
    return FAILED;
  }

  // 3. concat meta data
  meta_data.append(binfile_name);
  meta_data.append(binfile_suffix);
  meta_data.append(", version, ");
  meta_data.append(sha256);
  meta_data.append(", shared");
  FE_LOGD("Add meta data: %s.", meta_data.c_str());
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the kernel_name info in handle
*  @param   [in] handle
*  @param   [out] op_desc_, add kernel_name to op_desc_.name according to
* kernel_name info in handle
*  @return SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::ParseTvmKernelName(std::string &kernel_name) {
  kernel_name = GetStrValueFromJson("kernelName");

  if (kernel_name.empty()) {
    FE_LOGE("KernelName is empty. please check json file.");
    return FAILED;
  }

  return SUCCESS;
}

Status TbeJsonFileParseImpl::ParseConvCompressParameters(std::vector<int64_t> &compress_param_vec) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);
  if (j->find(JSON_PARAM_COMPRESS_PARAMETERS) == j->end()) {
    return SUCCESS;
  }

  try {
    json json_compress = j->at(JSON_PARAM_COMPRESS_PARAMETERS);
    if (!json_compress.is_array()) {
      FE_LOGE("Format error! compress_parameters should be array.");
      return FAILED;
    }

    for (auto it = json_compress.begin(); it != json_compress.end(); it++) {
      json flag = *it;
      compress_param_vec.push_back(flag.get<int64_t>());
    }
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the compress_parameters failed, exception:%s.", e.what());
    return FAILED;
  }

  return SUCCESS;
}

Status TbeJsonFileParseImpl::ParseWeightRepeat(int64_t &weight_repeat) {
  const json *j = static_cast<json *>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);
  if (j->find(JSON_PARAM_WEIGHT_REPEAT) == j->end()) {
    return SUCCESS;
  }

  try {
    json json_weight_repeat = j->at(JSON_PARAM_WEIGHT_REPEAT);
    weight_repeat = json_weight_repeat.get<int64_t>();
    return SUCCESS;
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the weight_repeat failed, exception:%s.", e.what());
    return FAILED;
  }
}

Status TbeJsonFileParseImpl::ParseOpParaSize(int64_t &op_para_size) {
  const json *j = static_cast<json *>(handle_);
  FE_CHECK(j == nullptr, FE_LOGE("json is nullptr."), return FAILED);
  ;
  try {
    op_para_size = (j->find("opParaSize") == j->end()) ? 0 : (uint32_t)j->at("opParaSize");
  } catch (const JsonExpcetion &e) {
    FE_LOGE("get the opParaSize failed, exception:%s.", e.what());
    return FAILED;
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  get the str value from handle based on key
*  @param   [in] handle, key
*  @return j->at(key)
*/
std::string TbeJsonFileParseImpl::GetStrValueFromJson(const std::string& key) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGD("json is nullptr."), return "");
  if (j->find(key) == j->end()) {
    return "";
  }

  try {
    return j->at(key);
  } catch (const JsonExpcetion &e) {
    FE_LOGD("get the %s failed, exception:%s.", key.c_str(), e.what());
  }
  return "";
}

/*
*  @ingroup fe
*  @brief  set the str value from handle based on key to graph and op_desc_ptr
*  @param   [in] graph, op_desc_ptr
*  @return status
*/
Status TbeJsonFileParseImpl::ParseGlobleWorkspaceStatus(ge::ComputeGraphPtr &graph, ge::OpDescPtr &op_desc_ptr) {
  const json *j = static_cast<json*>(handle_);
  FE_CHECK(j == nullptr, FE_LOGD("json is nullptr."), return FAILED);
  if (j->find(GLOBALWORKSPACE_STATUS) == j->end()) {
    FE_LOGD("[SubGraphOpt][Compile][ParseGlobalWorkspaceStatus] cannot get %s graph[%s] op[%s].",
            GLOBALWORKSPACE_STATUS.c_str(), graph->GetName().c_str(), op_desc_ptr->GetName().c_str());
    return SUCCESS;
  }
  int64_t globalworkspace_status_value;
  try {
    json global_work = j->at(GLOBALWORKSPACE_STATUS);
    if (global_work.find("size") == global_work.end()) {
      FE_LOGE("[SubGraphOpt][Compile][ParseGlobalWorkspaceStatus] data error! 'size' is necessary.");
      return FAILED;
    }
    globalworkspace_status_value = global_work.at("size").get<int64_t>();
  } catch (const JsonExpcetion &e) {
    FE_LOGE("[SubGraphOpt][Compile][ParseGlobalWorkspaceStatus] get the %s failed, exception:%s.",
            GLOBALWORKSPACE_STATUS.c_str(), e.what());
    return FAILED;
  }
  if (!ge::AttrUtils::HasAttr(graph, GLOBALWORKSPACE_STATUS_BYTES)) {
    (void)ge::AttrUtils::SetInt(graph, GLOBALWORKSPACE_STATUS_BYTES, globalworkspace_status_value);
    FE_LOGD("[SubGraphOpt][Compile][ParseGlobalWorkspaceStatus] graph[%s] op[%s] set [%s:%ld] success",
            graph->GetName().c_str(), op_desc_ptr->GetName().c_str(),
            GLOBALWORKSPACE_STATUS_BYTES.c_str(), globalworkspace_status_value);
  }
  if (!ge::AttrUtils::HasAttr(graph, GLOBALWORKSPACE_STATUS)) {
    (void)ge::AttrUtils::SetInt(graph, GLOBALWORKSPACE_STATUS, 0);
  }
  (void)ge::AttrUtils::SetStr(op_desc_ptr, GLOBALWORKSPACE_REF, GLOBALWORKSPACE_STATUS);
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  joint the path of bin file, if success, renew the op_desc.name
*  @param   [in] handle
*  @param   [out] op_desc_->name
*  @return  SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::PackageTvmBinFile(std::string &bin_file_path, vector<char> &buffer) {
  std::string binfile_suffix = GetStrValueFromJson("binFileSuffix");
  std::string binfile_name = GetStrValueFromJson("binFileName");

  if (binfile_suffix.empty() || binfile_name.empty()) {
    FE_LOGE("BinFileName or binfile_suffix is empty. please check json file.");
    return FAILED;
  }

  bin_file_path.append("/");
  bin_file_path.append(binfile_name);
  bin_file_path.append(binfile_suffix);

  bool ret = ReadBytesFromBinaryFile(bin_file_path, buffer);
  if (ret != SUCCESS) {
    FE_LOGE("Read bin file failed, file path is %s.", bin_file_path.c_str());
    return PARAM_INVALID;
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief   package the json info together
*  @param   [in]  info
*  @param   [out] tvm_file_path_
*  @return  SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::Initialize(const std::string &json_file_path) {
  FE_LOGD("Start to parse op_json_file %s", json_file_path.c_str());
  if (ReadJsonFileByLock(json_file_path, j_) != SUCCESS) {
    FE_LOGE("ReadJsonFile failed.");
    return FAILED;
  }
  if (json_file_path.find("_mix") != string::npos) {
    if (json_file_path.find("_mix_aic") != string::npos) {
      attr_prefix_ = "_mix_aic";
    } else if (json_file_path.find("_mix_aiv") != string::npos) {
      attr_prefix_ = "_mix_aiv";
    } else {
      REPORT_FE_ERROR("Json file path[%s] is wrong.", json_file_path.c_str());
      return FAILED;
    }
  }

  handle_ = &j_;
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief   reading binary files
*  @param   [in]  file_name(or path), buffer, length
*  @param   [out] length
*  @return  SUCCESS or FAILED
*/
Status TbeJsonFileParseImpl::ReadBytesFromBinaryFile(const string& file_name, std::vector<char>& buffer) {
  if ((file_name.empty())) {
    FE_LOGE("incorrect parameter. file path is null");
    return FAILED;
  }

  string real_path = RealPath(file_name);
  if (real_path.empty()) {
    FE_LOGE("File path '%s' not valid.", file_name.c_str());
    return FAILED;
  }

  std::ifstream if_stream(real_path.c_str(), std::ios::binary | std::ios::ate);
  if (!if_stream.is_open()) {
    FE_LOGE("read file %s failed.", file_name.c_str());
    return FAILED;
  }
  try {
    uint32_t recursive_cnt = 0;
    int fd = static_cast<__gnu_cxx::stdio_filebuf<char> *const>(if_stream.rdbuf())->fd();
    do {
      if (FcntlLockFile(file_name, fd, F_RDLCK, recursive_cnt) == FAILED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kLockRecursiveIntvl));
      } else {
        FE_LOGD("Lock file(%s).", file_name.c_str());
        break;
      }
      if (recursive_cnt == kLockRecursiveCntMax) {
        FE_LOGE("Lock file(%s) failed, try %u times.", file_name.c_str(), kLockRecursiveCntMax);
        if_stream.close();
        return FAILED;
      }
      recursive_cnt++;
    } while (true);

    std::streamsize size = if_stream.tellg();

    if (size <= 0) {
      (void)FcntlLockFile(file_name, fd, F_UNLCK, 0);
      if_stream.close();
      FE_LOGE("file length <= 0, not valid.");
      return FAILED;
    }

    if (size > MAX_FILE_SIZE_LIMIT) {
      (void)FcntlLockFile(file_name, fd, F_UNLCK, 0);
      if_stream.close();
      FE_LOGE("File size %ld is out of limit: %d.", size, MAX_FILE_SIZE_LIMIT);
      return FAILED;
    }
    if_stream.seekg(0, std::ios::beg);

    buffer.resize(size);
    if_stream.read(&buffer[0], size);
    (void)FcntlLockFile(file_name, fd, F_UNLCK, 0);
    FE_LOGD("Release lock file(%s).", real_path.c_str());
    if_stream.close();
    FE_LOGD("Read size:%ld.", size);
  } catch (const ifstream::failure& e) {
    FE_LOGE("Fail to read file %s. Exception: %s.", file_name.c_str(), e.what());
    if_stream.close();
    return FAILED;
  }
  return SUCCESS;
}
std::string TbeJsonFileParseImpl::GetAttrPrefix() {
  return attr_prefix_;
}
}  // namespace fe
