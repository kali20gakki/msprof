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
#include "tf_kernel_info.h"

#include "config/ops_json_file.h"
#include "util/constant.h"
#include "util/log.h"
#include "util/tf_util.h"
#include "error_code/error_code.h"
#include "ir2tf/ir2tf_parser_factory.h"

#include "common/util/error_manager/error_manager.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"

using domi::tensorflow::NodeDef;

namespace aicpu {
KernelInfoPtr TfKernelInfo::instance_ = nullptr;

inline KernelInfoPtr TfKernelInfo::Instance() {
  static std::once_flag flag;
  std::call_once(flag, [&]() {
    instance_.reset(new (std::nothrow) TfKernelInfo);
  });
  return instance_;
}

ge::Status TfKernelInfo::Initialize(const std::map<std::string, std::string> &options) {
  ge::Status status = KernelInfo::Initialize(options);
  if (status != ge::SUCCESS) {
    return status;
  }
  std::shared_ptr<Ir2tfBaseParser> parser = Ir2tfBaseParser::Instance();
  if (parser == nullptr) {
    AICPU_REPORT_INNER_ERROR("Create Ir2tfBaseParser object failed.");
    return ErrorCode::GET_IR2TF_PARSER_FAILED;
  }
  return parser->LoadMappingConfig();
}

bool TfKernelInfo::ReadOpInfoFromJsonFile() {
  std::string real_tf_ops_file_path;
  const char* path_env = getenv("ASCEND_OPP_PATH");
  if (path_env != nullptr) {
    real_tf_ops_file_path = std::string(path_env) + kTfOpsFileBasedOnEnvPath;
  } else {
    real_tf_ops_file_path = GetOpsPath(reinterpret_cast<void*>(&TfKernelInfo::Instance)) + kTfOpsFileRelativePath;
  }
  AICPUE_LOGI("TfKernelInfo real_tf_ops_file_path is [%s].", real_tf_ops_file_path.c_str());
  aicpu::State ret = OpsJsonFile::Instance().ParseUnderPath(real_tf_ops_file_path, op_info_json_file_);
  if (ret.state != ge::SUCCESS) {
    std::map<std::string, std::string> err_map;
    err_map["filename"] = real_tf_ops_file_path;
    err_map["reason"] = ret.msg;
    ErrorManager::GetInstance().ReportErrMessage(GetViewErrorCodeStr(ViewErrorCode::LOAD_AICPU_KERNEL_INFO_ERR),
                                                 err_map);
    AICPUE_LOGE("load tf kernel info file[%s] failed, %s",
        real_tf_ops_file_path.c_str(), ret.msg.c_str());
  }
  return ret.state == ge::SUCCESS;
}

/**
 * For ops in AIcore, Call CompileOp before Generate task in AICPU
 * @param node Node information
 * @return status whether operation successful
 */
ge::Status TfKernelInfo::CompileOp(ge::NodePtr &node) {
  AICPU_CHECK_NOTNULL_ERRCODE(node, INPUT_PARAM_NULL)

  map<string, OpFullInfo> all_op_info;
  AICPU_CHECK_RES_WITH_LOG(GetOpInfos(all_op_info), "Get op infos failed, op[%s].",
                           node->GetName().c_str())
  AICPU_CHECK_RES_WITH_LOG(UpdataOpInfo(*node, all_op_info),
                           "Updata function attr failed, op[%s].",
                           node->GetName().c_str())

  // create nodedef
  ge::Status status = CreateNodeDef(*node);
  if (status != ge::SUCCESS) {
    AICPU_REPORT_CALL_ERROR("Call CreateNodeDef failed, op[%s]", node->GetName().c_str());
    return status;
  }

  // calc Op run para
  status = CalcTfOpRunningParam(*node);
  if (status != ge::SUCCESS) {
    AICPU_REPORT_CALL_ERROR("Call CalcOpRunningParam failed, op[%s]", node->GetName().c_str());
    return status;
  }

  // set shape type
  status = CheckAndSetUnknowType(node);
  if (status != ge::SUCCESS) {
    AICPU_REPORT_CALL_ERROR("Call CheckAndSetUnknowType failed, op[%s]", node->GetName().c_str());
    return status;
  }
  return ge::SUCCESS;
}
FACTORY_KERNELINFO_CLASS_KEY(TfKernelInfo, kTfKernelInfoChoice)
} // namespace aicpu
