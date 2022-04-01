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
#ifndef AICPU_AICPU_OPS_KERNEL_INFO_STORE_H_
#define AICPU_AICPU_OPS_KERNEL_INFO_STORE_H_

#include <memory>
#include <vector>

#include "aicpu_ops_kernel_info_store/op_struct.h"
#include "common/opskernel/ops_kernel_info_store.h"
#include "ge/ge_api_types.h"

namespace aicpu {
class KernelInfo;
using KernelInfoPtr = std::shared_ptr<KernelInfo>;

class AicpuOpsKernelInfoStore : public ge::OpsKernelInfoStore {
 public:
  /**
   * Contructor
   */
  explicit AicpuOpsKernelInfoStore(const std::string &engine_name)
      : engine_name_(engine_name) {}

  /**
   * Destructor
   */
  virtual ~AicpuOpsKernelInfoStore() = default;

  /**
   * Initialize related resources of the aicpu kernelinfo store
   * @return status whether this operation success
   */
  ge::Status Initialize(
      const std::map<std::string, std::string> &options) override;

  /**
   * Release related resources of the aicpu kernelinfo store
   * @return status whether this operation success
   */
  ge::Status Finalize() override;

  /**
   * Returns all operator information of the AICPU
   * @param infos Reference of a map, return operator's name and detailed
   * information
   * @return void
   */
  void GetAllOpsKernelInfo(
      std::map<std::string, ge::OpInfo> &infos) const override;

  /**
   * Check to see if an operator is fully supported or partially supported.
   * @param op_desc_ptr OpDesc pointer, return op name in opDesc's attr
   * @param unsupported_reason unsupported reason
   * @return bool value indicate whether the operator is fully supported
   */
  bool CheckSupported(const ge::OpDescPtr &op_desc_ptr,
                      std::string &unsupported_reason) const override;

  /**
   * Check to see if an operator is constant folding supported.
   * @param node Node information
   * @param ops_flag ops_flag[0] indicates constant folding
   * @return void
   */
  void opsFlagCheck(const ge::Node &node, std::string &ops_flag) override;

  /**
   * For ops in AIcore, Call CompileOp before Generate task in AICPU
   * @param node_vec Node information
   * @return status whether operation successful
   */
  ge::Status CompileOp(vector<ge::NodePtr> &node_vec) override;

  /**
   * Get all operator full kernel information
   * @param  infos Operator full infos
   * @return void
   */
  void GetAllOpsFullKernelInfo(
      std::map<std::string, aicpu::OpFullInfo> &infos) const;

  // Copy prohibited
  AicpuOpsKernelInfoStore(
      const AicpuOpsKernelInfoStore &aicpu_ops_kernel_info_store) = delete;

  // Move prohibited
  AicpuOpsKernelInfoStore(
      const AicpuOpsKernelInfoStore &&aicpu_ops_kernel_info_store) = delete;

  // Copy prohibited
  AicpuOpsKernelInfoStore &operator=(
      const AicpuOpsKernelInfoStore &aicpu_ops_kernel_info_store) = delete;

  // Move prohibited
  AicpuOpsKernelInfoStore &operator=(
      AicpuOpsKernelInfoStore &&aicpu_ops_kernel_info_store) = delete;

 private:
  // Load Internal KernelLibs instance and initialize
  ge::Status LoadKernelLibs(const map<string, string> &options);

  // Initialize member opKernelLibMap_ and infos_
  void FillKernelInfos();

  /**
   * Check to see if an operator input type is fully supported or partially
   * supported.
   * @param op_desc_ptr OpDesc pointer, return op name in opDesc's attr
   * @param data_types read input supported type from ops config
   * @param in_out_real_name read input name from ops config
   * @param unsupported_reason unsupported reason
   * @return bool value indicate whether the operator is fully supported
   */
  bool CheckInputSupported(
      const ge::OpDescPtr &op_desc_ptr,
      const std::map<std::string, std::string> data_types,
      const std::map<std::string, std::string> in_out_real_name,
      std::string &unsupported_reason) const;

  /**
   * Check to see if an operator output type is fully supported or partially
   * supported.
   * @param op_desc_ptr OpDesc pointer, return op name in opDesc's attr
   * @param data_types read output supported type from ops config
   * @param in_out_real_name read output name from ops config
   * @param unsupported_reason unsupported reason
   * @return bool value indicate whether the operator is fully supported
   */
  bool CheckOutputSupported(
      const ge::OpDescPtr &op_desc_ptr,
      const std::map<std::string, std::string> data_types,
      const std::map<std::string, std::string> in_out_real_name,
      std::string &unsupported_reason) const;

 private:
  std::string engine_name_;

  // store name and KernelInfoPtr
  std::map<std::string, KernelInfoPtr> kernel_libs_;

  // store kernelinfo names sorted by priority
  std::vector<std::string> kernel_lib_names_;

  // store op name and OpInfo key-value pair filtered by priority
  std::map<std::string, ge::OpInfo> op_infos_;

  // store op name and OpFullInfo key-value pair filtered by priority
  std::map<std::string, aicpu::OpFullInfo> op_full_infos_;
};
}  // namespace aicpu

#endif  // AICPU_OPS_KERNEL_INFO_STORE_H_
