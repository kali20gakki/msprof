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
#ifndef AICPU_KERNEL_INFO_H_
#define AICPU_KERNEL_INFO_H_

#include <nlohmann/json.hpp>

#include "aicpu_ops_kernel_info_store/op_struct.h"
#include "aicpu_ops_kernel_info_store/aicpu_ops_kernel_info_store.h"
#include "error_code/error_code.h"
#include "factory/factory.h"

namespace aicpu {

class KernelInfo {
 public:
  /**
   * Destructor
   */
  virtual ~KernelInfo() = default;

  /**
   * Initialize related kernelinfo store
   * @param options configuration information
   * @return status whether this operation success
   */
  virtual ge::Status Initialize(
      const std::map<std::string, std::string> &options);

  /**
   * Release related resources of the aicpu kernelinfo store
   * @return status whether this operation success
   */
  ge::Status Finalize();

  /**
   * Member access method.
   * @param a map<string, OpInfo> stores operator's name and OpInfo
   * @return status whether this operation success
   */
  ge::Status GetOpInfos(
      std::map<std::string, aicpu::OpFullInfo> &op_infos) const;

  ge::Status GetOpInfo(std::string &op_type, OpFullInfo &op_info) const;

  /**
   * For ops in AIcore, Call CompileOp before Generate task in AICPU
   * @param node Node information
   * @return status whether operation successful
   */
  virtual ge::Status CompileOp(ge::NodePtr &node);

 protected:
  /**
   * Read json file
   * @return whether read file successfully
   */
  virtual bool ReadOpInfoFromJsonFile() { return true; }

  const std::string GetOpsPath(const void *instance) const;

 protected:
  // kernelinfo json serialized object
  nlohmann::json op_info_json_file_;

  // store operator's name and detailed information
  std::map<std::string, aicpu::OpFullInfo> infos_;

 private:
  ge::Status FillOpInfos(OpInfoDescs &info_desc);
};

#define FACTORY_KERNELINFO Factory<KernelInfo>

#define FACTORY_KERNELINFO_CLASS_KEY(CLASS, KEY) \
  FACTORY_KERNELINFO::Register<CLASS> __##CLASS(KEY);

}  // namespace aicpu

#endif  // AICPU_KERNEL_INFO_H_
