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

#ifndef DAVINCI_CPU_OPTIMIZER_H_
#define DAVINCI_CPU_OPTIMIZER_H_

#include <unordered_map>
#include <vector>
#include "common/aicpu_graph_optimizer/optimizer.h"
#include "graph/compute_graph.h"
#include "graph/detail/attributes_holder.h"
#include "graph/op_desc.h"
#include "proto/cpu_attr.pb.h"
#include "proto/cpu_node_def.pb.h"
#include "proto/cpu_tensor.pb.h"
#include "proto/cpu_tensor_shape.pb.h"
#include "util/constant.h"

namespace aicpu {

class CpuOptimizer : public Optimizer {
 public:
  /**
   * constructor
   * @param void
   */
  CpuOptimizer() = default;

  /**
   * Destructor
   */
  virtual ~CpuOptimizer() = default;

  /**
   * optimizer fused Graph, find aicpu ops which can be fused and fuse them
   * @param graph, Compute graph
   * @param all_op_info, map stored all aicpu ops information
   * @return result is success or not
   */
  ge::Status OptimizeFusedGraph(
      ge::ComputeGraph &graph,
      const std::map<std::string, OpFullInfo> &all_op_info) const override;

  /**
   * init optimizer
   * @return status whether this operation success
   */
  ge::Status Initialize() override;

 private:
  /**
   * Read bin file
   * @param op_desc_ptr Op desc pointer
   * @param bin_folder_path bin file path
   * @param op_full_info op full info
   * @param graph_id graph id
   * @param exist_graph_id exist graph id
   * @return bool flag
   */
  bool PackageBinFile(ge::OpDescPtr op_desc_ptr,
                      const std::string &bin_folder_path,
                      const OpFullInfo &op_full_info, uint32_t graph_id,
                      bool exist_graph_id) const;

  /**
   * Get custom aicpu kernel so path
   * @param null
   * @return string path
   */
  const std::string GetCustKernelSoPath() const;

  /**
   * Get cpu kernel so path (libcpu_kernels.so)
   * @param null
   * @return string path
   */
  const std::string GetCpuKernelSoPath() const;

  /**
   * Init load cpu kernels type
   */
  void InitLoadCpuKernelsType();

  /**
   * Check so whether need load in model
   * @param op_full_info op full info
   * @param file_path file path
   * @return true or false
   */
  bool CheckSoNeedLoadInModel(const OpFullInfo &op_full_info,
                              std::string &file_path) const;

  /**
   * Get bin file name
   * @param op_full_info op full info
   * @param bin_folder_path bin folder path
   * @param bin_file_name bin file name
   * @param graph_id graph id
   * @return result is success or not
   */
  ge::Status GetBinFileName(const OpFullInfo &op_full_info,
                            const std::string &bin_folder_path,
                            std::string &bin_file_name) const;

  ge::Status SetCustKernelBinFile(
      ge::OpDescPtr op_desc_ptr,
      const std::map<std::string, OpFullInfo> &all_op_info, uint32_t graph_id,
      bool exist_graph_id) const;

  void CheckAndSetSocVersion(const std::string &soc_version_from_ge) const;

 private:
  // load type for libcpu_kernels.so
  uint64_t load_type_for_cpu_kernels_ = kDefaultLoadTypeForCpuKernels;
};
}  // namespace aicpu

#endif  // DAVINCI_AICPU_OPTIMIZER_H
