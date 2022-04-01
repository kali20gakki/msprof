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

#ifndef GE_GRAPH_PASSES_AICPU_CONSTANT_FOLDING_PASS_H_
#define GE_GRAPH_PASSES_AICPU_CONSTANT_FOLDING_PASS_H_

#include <string>
#include <vector>

#include "common/opskernel/ops_kernel_info_store.h"
#include "graph/passes/potential_folding_pass.h"

namespace ge {
class AicpuConstantFoldingPass : public PotentialFoldingPass {
 public:
  Status Init(uint64_t session_id, const ComputeGraphPtr &compute_graph);
  Status Finalize() const;
  bool NeedIgnorePass(const NodePtr &node) override;
  bool NeedFold() const override;
  Status ComputePotentialWeight(NodePtr &node, std::vector<GeTensorPtr> &outputs) override;
  std::string GetPassName() const override;

 private:
  enum AddrType { kData = 0, kSummary = 1, kTypeEnd };

  struct AddrAndType {
    uint64_t input_addr;
    AddrType attr_type;
  } __attribute__((packed));

  struct DataPtrInfo {
    uint64_t release_flag;
    uint64_t data_size;
    uint64_t src_ptr;
    uint64_t dst_ptr;
  } __attribute__((packed));
  Status ComputeWithAICPUKernel(const NodePtr &node, const std::vector<ConstGeTensorPtr> &input_weight_vec,
                                std::vector<GeTensorPtr> &outputs);
  bool IsSkipFold(const ge::NodePtr &node) const;
  Status GetInputAddrs(const std::vector<ConstGeTensorPtr> &weight_vec, std::vector<AddrAndType> &input_addrs) const;
  Status GetOutputAddrs(const OpDescPtr &node_desc, std::vector<uint64_t> &output_addrs) const;
  Status GenerateTaskForLaunch(STR_FWK_OP_KERNEL &aicpu_task, void *&task_buf) const;
  Status GenerateDataPtrInfo(const std::vector<uint64_t> &output_addrs, std::vector<DataPtrInfo> &data_vec,
                             std::vector<uint64_t> &data_infos) const;
  Status GenerateGeTensor(const OpDescPtr &node_desc, const std::vector<DataPtrInfo> &data_vec,
                          std::vector<GeTensorPtr> &outputs) const;
  Status UpdateWorkSpaceAddr(std::string &task_info, STR_FWK_OP_KERNEL &task) const;
  Status UpdateInputAndOutputAddr(const std::vector<uint64_t> &io_addrs, STR_FWK_OP_KERNEL &task) const;
  Status UpdateSingleOpAddr(std::string &task_info, const std::vector<AddrAndType> &input_addrs,
                            const std::vector<uint64_t> &outputs_addr_vec, STR_FWK_OP_KERNEL &task) const;
  Status UpdateMemCopyAddr(std::string &task_info, const std::vector<uint64_t> &data_infos, std::vector<uint64_t> &internal_addrs,
                           STR_FWK_OP_KERNEL &task) const;
  Status LaunchSingleOpRunTask(const NodePtr &node, const std::vector<AddrAndType> &input_addrs,
                               const std::vector<uint64_t> &output_addrs);
  Status LaunchMemCopyTask(const std::vector<uint64_t> &data_infos);
  void ReleaseMemory(const std::vector<AddrAndType> &input_addrs, const std::vector<uint64_t> &output_addrs,
                     const std::vector<DataPtrInfo> &data_vec) const;
  Status KernelLaunch(void *task_buf) const;
  rtError_t rt_get_device_count_err_ = RT_ERROR_NONE;
  ComputeGraphPtr cur_compute_graph_ = nullptr;
  uint64_t cur_session_id_ = 0;
  bool need_fold_ = true;
};
}  // namespace ge

#endif  // GE_GRAPH_PASSES_AICPU_CONSTANT_FOLDING_PASS_H_
