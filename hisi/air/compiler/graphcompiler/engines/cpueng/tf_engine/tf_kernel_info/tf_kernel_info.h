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
#ifndef AICPU_TF_KERNEL_INFO_H_
#define AICPU_TF_KERNEL_INFO_H_

#include "common/aicpu_ops_kernel_info_store/kernel_info.h"
#include "proto/fwk_adapter.pb.h"

namespace aicpu {
using KernelInfoPtr = std::shared_ptr<KernelInfo>;

class TfKernelInfo : public KernelInfo {
 public:
  /**
   * Destructor
   */
  virtual ~TfKernelInfo() = default;

  /**
   * @return kernel info object
   */
  static KernelInfoPtr Instance();

  /**
   * init tf kernel info, get ir to tf parser
   * @param options passed by GE, used to config parameter
   * @return status whether this operation successful
   */
  ge::Status Initialize(const std::map<std::string, std::string> &options) override;

  /**
   * For ops in AIcore, Call CompileOp before Generate task in AICPU
   * @param node Node information
   * @return status whether operation successful
   */
  ge::Status CompileOp(ge::NodePtr &node) override;

  // Copy prohibited
  TfKernelInfo(const TfKernelInfo &tf_kernel_info) = delete;

  // Move prohibited
  TfKernelInfo(const TfKernelInfo &&tf_kernel_info) = delete;

  // Copy prohibited
  TfKernelInfo &operator=(const TfKernelInfo &tf_kernel_info) = delete;

  // Move prohibited
  TfKernelInfo &operator=(TfKernelInfo &&tf_kernel_info) = delete;
 protected:
  /**
   * Read json file in specified path(based on source file's current path) e.g. ops_data/tf_kernel.json
   * @return whether read file successfully
   */
  bool ReadOpInfoFromJsonFile() override;

 private:
  /**
   * Constructor
   */
  TfKernelInfo() = default;
private:
  // singleton instance
  static KernelInfoPtr instance_;
};
} // namespace aicpu
#endif // AICPU_TF_KERNEL_INFO_H_
