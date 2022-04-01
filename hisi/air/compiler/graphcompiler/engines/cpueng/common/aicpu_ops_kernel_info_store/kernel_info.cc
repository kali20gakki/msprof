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
#include "kernel_info.h"
#include "config/ops_json_file.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "util/log.h"
#include "util/util.h"
#include "util/constant.h"
#include "proto/cpu_node_def.pb.h"
#include "cpu_engine_util.h"

using namespace std;
using namespace ge;
using namespace ::aicpu::FWKAdapter;

namespace aicpu {
Status KernelInfo::Initialize(const map<string, string> &options) {
  AICPU_CHECK_RES(Finalize());
  // read kernel info json file
  if (!ReadOpInfoFromJsonFile()) {
    AICPU_REPORT_CALL_ERROR(
        "Call ReadOpInfoFromJsonFile to read kernel info from json file failed.");
    return LOAD_CONFIG_JSON_FILE_FAILED;
  }
  AICPU_IF_BOOL_EXEC(
      ((op_info_json_file_.find(kKernelConfigOpInfos) == op_info_json_file_.end()) ||
       (op_info_json_file_.find(kKernelConfigLibName) == op_info_json_file_.end())),
      AICPUE_LOGW("Json file does not have op_infos or lib_name.") return SUCCESS)
  try {
    OpInfoDescs info_desc = op_info_json_file_;
    AICPUE_LOGI("Read json file, op size is: %lu.", info_desc.opInfos.size());
    return FillOpInfos(info_desc);
  } catch (const nlohmann::json::exception &e) {
    AICPU_REPORT_INNER_ERROR("Parse json file[%s] failed, %s.",
        op_info_json_file_.dump().c_str(), e.what());
    return LOAD_CONFIG_JSON_FILE_FAILED;
  }
}

ge::Status KernelInfo::Finalize() {
  infos_.clear();
  return SUCCESS;
}

Status KernelInfo::FillOpInfos(OpInfoDescs &info_desc) {
  for (const auto &op_desc : info_desc.opInfos) {
    AICPU_IF_BOOL_EXEC(op_desc.opName.empty(), continue)

    auto ret = infos_.emplace(
        pair<string, aicpu::OpFullInfo>(op_desc.opName, op_desc.opInfo));
    if (!ret.second) {
      AICPUE_LOGW("Insert a pair of op[%s] and OpInfo failed.",
                  op_desc.opName.c_str());
    }
    AICPUE_LOGD("Read json file, op_name: %s.", op_desc.opName.c_str());
  }
  return SUCCESS;
}

const string KernelInfo::GetOpsPath(const void *instance) const {
  char resoved_path[PATH_MAX] = {0x00};
  string real_file_path;
  string path_base = GetSoPath(instance) + "../../../..";
  AICPU_IF_BOOL_EXEC(realpath(path_base.c_str(), resoved_path) == nullptr,
      AICPU_REPORT_INNER_ERROR("realpath [%s] failed, %s.",
          path_base.c_str(), strerror(errno));
      return real_file_path);
  real_file_path = resoved_path;
  return real_file_path;
}

Status KernelInfo::GetOpInfos(map<string, aicpu::OpFullInfo> &op_infos) const {
  op_infos = infos_;
  return SUCCESS;
}

Status KernelInfo::GetOpInfo(string &op_type, OpFullInfo &op_info) const {
  auto iter = infos_.find(op_type);
  if (iter != infos_.end()) {
    op_info = iter->second;
    return SUCCESS;
  }
  return OP_NOT_EXIST_IN_KERNEL_LIBS;
}

/**
 * For ops in AIcore, Call CompileOp before Generate task in AICPU
 * @param node Node information
 * @return status whether operation successful
 */
Status KernelInfo::CompileOp(ge::NodePtr &node) {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  AICPU_CHECK_NOTNULL(op_desc_ptr)
  std::string op_type = op_desc_ptr->GetType();

  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES(GetOpInfos(all_op_info));
  std::string kernel_lib_name = GetKernelLibNameByOpType(op_type, all_op_info);
  auto iter = all_op_info.find(op_type);
  if (iter == all_op_info.end()) {
    AICPU_REPORT_INNER_ERROR("Can't find op type[%s] in KernelInfo, op[%s].", op_type.c_str(), node->GetName().c_str());
    return ErrorCode::CREATE_NODEDEF_FAILED;
  }

  OpFullInfo op_full_info = iter->second;
  std::string kernel_so = op_full_info.kernelSo;
  std::string func_name = op_full_info.functionName;
  bool async_flag = op_full_info.flagAsync;
  int workspace_size = op_full_info.workspaceSize;
  FWKAdapter::FWKExtTopicType topic_type = op_full_info.topicType;
  std::string resource = op_full_info.resource;
  bool support_block_flag = op_full_info.flagSupportBlockDim;
  int block_dim_index = op_full_info.blockDimByIndex;

  if (kernel_lib_name != kHostCpuKernelInfoChoice) {
      (void)ge::AttrUtils::SetStr(op_desc_ptr, kKernelSo, kernel_so);
      (void)ge::AttrUtils::SetStr(op_desc_ptr, kFuncName, func_name);
  }
  (void)ge::AttrUtils::SetBool(op_desc_ptr, kAsyncFlag, async_flag);
  (void)ge::AttrUtils::SetInt(op_desc_ptr, kWorkspaceSize, workspace_size);
  (void)ge::AttrUtils::SetStr(op_desc_ptr, kOpKernelLib, kernel_lib_name);
  (void)ge::AttrUtils::SetInt(op_desc_ptr, kTopicType, topic_type);
  (void)ge::AttrUtils::SetBool(op_desc_ptr, kCustAicpuFlag, false);
  (void)ge::AttrUtils::SetStr(op_desc_ptr, kResource, resource);
  (void)ge::AttrUtils::SetBool(op_desc_ptr, kSupportBlockDim, support_block_flag);
  (void)ge::AttrUtils::SetInt(op_desc_ptr, kBlockDimByIndex, block_dim_index);

  AICPU_CHECK_RES_WITH_LOG(CheckAndSetUnknowType(op_desc_ptr, all_op_info),
      "Call CheckAndSetUnknowType function failed. op[%s].", node->GetName().c_str())

  aicpuops::NodeDef node_def;
  Status status = BuildAicpuNodeDef(op_desc_ptr, node_def);
  if (status != SUCCESS) {
    AICPU_REPORT_INNER_ERROR("BuildAicpuNodeDef failed, op[%s].", node->GetName().c_str());
    return ErrorCode::CREATE_NODEDEF_FAILED;
  }

  std::string node_def_str;
  bool ret = node_def.SerializeToString(&node_def_str);
  if (!ret) {
    AICPU_REPORT_INNER_ERROR("The serialization from nodedef probuf to str failed, op[%s].", node->GetName().c_str());
    return ErrorCode::CREATE_NODEDEF_FAILED;
  }

  const uint8_t *node_def_data = reinterpret_cast<const uint8_t *>(node_def_str.data());
  ret = ge::AttrUtils::SetZeroCopyBytes(
      op_desc_ptr, kCustomizedOpDef,
      ge::Buffer::CopyFrom(node_def_data, node_def_str.length()));
  if (!ret) {
    AICPU_REPORT_CALL_ERROR("Failed to call SetZeroCopyBytes to set tf node def, op[%s].", node->GetName().c_str());
    return ErrorCode::CREATE_NODEDEF_FAILED;
  }
  return SUCCESS;
}
}  // namespace aicpu