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
#include "aicpu_cust_kernel_info.h"

#include <fstream>
#include "config/ops_json_file.h"
#include "error_code/error_code.h"
#include "util/constant.h"
#include "util/log.h"
#include "util/util.h"

using namespace std;
namespace aicpu {
KernelInfoPtr AicpuCustKernelInfo::instance_ = nullptr;

KernelInfoPtr AicpuCustKernelInfo::Instance() {
  static once_flag flag;
  call_once(flag,
      [&]() { instance_.reset(new (std::nothrow) AicpuCustKernelInfo); });
  return instance_;
}

bool AicpuCustKernelInfo::ReadOpInfoFromJsonFile() {
  std::string real_cust_aicpu_ops_file_path;
  const char* path_env = std::getenv("ASCEND_OPP_PATH");
  if (path_env != nullptr) {
    std::string path = path_env;
    real_cust_aicpu_ops_file_path = path + kAicpuCustOpsFileBasedOnEnvPath;
  } else {
    real_cust_aicpu_ops_file_path =
        GetOpsPath(reinterpret_cast<void*>(&AicpuCustKernelInfo::Instance)) +
        kAicpuCustOpsFileRelativePath;
  }

  AICPUE_LOGI("AicpuCustKernelInfo real_cust_aicpu_ops_file_path is %s",
              real_cust_aicpu_ops_file_path.c_str());
  std::ifstream ifs(real_cust_aicpu_ops_file_path);
  AICPU_IF_BOOL_EXEC(
      !ifs.is_open(),
      AICPUE_LOGW("Open %s failed, please check whether this file exist",
                  real_cust_aicpu_ops_file_path.c_str());
      op_info_json_file_ = {}; return true);
  AICPU_CHECK_FALSE_EXEC(
      OpsJsonFile::Instance().ParseUnderPath(
          real_cust_aicpu_ops_file_path, op_info_json_file_).state == ge::SUCCESS,
      AICPU_REPORT_INNER_ERROR("Parse json file[%s] failed.",
          real_cust_aicpu_ops_file_path.c_str());
      return false)
  return true;
}

FACTORY_KERNELINFO_CLASS_KEY(AicpuCustKernelInfo, kCustAicpuKernelInfoChoice)
}  // namespace aicpu
