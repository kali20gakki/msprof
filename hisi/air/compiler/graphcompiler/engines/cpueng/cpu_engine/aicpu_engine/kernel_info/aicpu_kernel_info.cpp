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
#include "aicpu_kernel_info.h"

#include "config/ops_json_file.h"
#include "error_code/error_code.h"
#include "util/constant.h"
#include "util/log.h"
#include "util/util.h"

using namespace std;
namespace aicpu {
KernelInfoPtr AicpuKernelInfo::instance_ = nullptr;

inline KernelInfoPtr AicpuKernelInfo::Instance() {
  static once_flag flag;
  call_once(flag, [&]() { instance_.reset(new (nothrow) AicpuKernelInfo); });
  return instance_;
}

bool AicpuKernelInfo::ReadOpInfoFromJsonFile() {
  std::string real_aicpu_ops_file_path;
  const char* path_env = getenv("ASCEND_OPP_PATH");
  AICPU_IF_BOOL_EXEC(
      path_env != nullptr, std::string path = path_env;
      real_aicpu_ops_file_path = path + kAicpuOpsFileBasedOnEnvPath;
      AICPUE_LOGI("AicpuKernelInfo real_aicpu_ops_file_path is %s",
                  real_aicpu_ops_file_path.c_str());
      return OpsJsonFile::Instance().ParseUnderPath(real_aicpu_ops_file_path,
                 op_info_json_file_).state == ge::SUCCESS);
  real_aicpu_ops_file_path =
      GetOpsPath(reinterpret_cast<void*>(&AicpuKernelInfo::Instance)) +
      kAicpuOpsFileRelativePath;
  AICPUE_LOGI("AicpuKernelInfo real_aicpu_ops_file_path is %s.",
              real_aicpu_ops_file_path.c_str());

  return OpsJsonFile::Instance().ParseUnderPath(real_aicpu_ops_file_path,
      op_info_json_file_).state == ge::SUCCESS;
}

FACTORY_KERNELINFO_CLASS_KEY(AicpuKernelInfo, kAicpuKernelInfoChoice)
}  // namespace aicpu
