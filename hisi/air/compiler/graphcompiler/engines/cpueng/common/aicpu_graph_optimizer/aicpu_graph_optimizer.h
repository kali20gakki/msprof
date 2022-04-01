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

#ifndef AICPU_AICPU_GRAPH_OPTIMIZER_H_
#define AICPU_AICPU_GRAPH_OPTIMIZER_H_

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include "aicpu_ops_kernel_info_store/op_struct.h"
#include "common/optimizer/graph_optimizer.h"
#include "common/optimizer/optimize_utility.h"
#include "config/ops_parallel_json_file.h"

namespace {
constexpr uint64_t kAutoCastModeOff = 0;
constexpr uint64_t kAutoCastModeOn = 1;
constexpr uint64_t kMaxOpsParallelNum = 20;
}

namespace aicpu {
class Optimizer;
using OptimizerPtr = std::shared_ptr<Optimizer>;

class AicpuGraphOptimizer : public ge::GraphOptimizer {
 public:
  /**
   * Contructor
   */
  explicit AicpuGraphOptimizer(const std::string &engine_name)
      : engine_name_(engine_name), auto_cast_mode_(kAutoCastModeOff) {}

  /**
   * Destructor
   */
  virtual ~AicpuGraphOptimizer() = default;

  /**
   * Initialize graph optimizer
   * @param options Initial options
   * @return status whether this operation success
   */
  ge::Status Initialize(
      const std::map<std::string, std::string> &options,
      ge::OptimizeUtility *const optimize_utility) override;

  /**
   * Close graph optimizer
   * @param void
   * @return status whether this operation success
   */
  ge::Status Finalize() override;

  /**
   * Optimize original graph, using in graph preparation stage
   * @param graph Computation graph
   * @return status whether this operation success
   */
  ge::Status OptimizeOriginalGraphJudgeInsert(ge::ComputeGraph &graph) override;

  /**
   * Optimize original graph, using in graph preparation stage
   * @param graph Computation graph
   * @return status whether this operation success
   */
  ge::Status OptimizeOriginalGraph(ge::ComputeGraph &graph) override;

  /**
   * Optimize fused graph
   * @param graph Computation graph
   * @return status whether this operation success
   */
  ge::Status OptimizeFusedGraph(ge::ComputeGraph &graph) override;

  /**
   * Optimize whole graph
   * @param graph Computation graph
   * @return status whether this operation success
   */
  ge::Status OptimizeWholeGraph(ge::ComputeGraph &graph) override {
    return ge::SUCCESS;
  }

  /**
   * Get attribute of graph optimizer
   * @param attrs Graph optimizer attributes
   * @return status whether this operation success
   */
  ge::Status GetAttributes(ge::GraphOptimizerAttribute &attrs) const override;
  
  ge::Status GetOpsParallelRule();

  protected:
  // Read json file in specified path
  bool ReadOpsParallelRuleFromJsonFile();
  // ops parallel rule json serialized object
  nlohmann::json ops_parallel_rule_json_file_;
  // store ops name support paralle
  std::vector<string> ops_parallel_rule_infos_;
 private:
  // Copy prohibit
  AicpuGraphOptimizer(const AicpuGraphOptimizer &) = delete;
  // Copy prohibit
  AicpuGraphOptimizer &operator=(const AicpuGraphOptimizer &) = delete;
  // Move prohibit
  AicpuGraphOptimizer(AicpuGraphOptimizer &&) = delete;
  // Move prohibit
  AicpuGraphOptimizer &operator=(AicpuGraphOptimizer &&) = delete;

  /**
   * Check if graph is empty
   * @param graph Computation graph
   * @return bool if graph is empty
   */
  bool IsEmptyGraph(const ge::ComputeGraph &graph) const;

  /**
   * Get op info
   * @param all_op_info store op information
   * @return status whether this operation success
   */
  ge::Status GetOpsInfo(
      std::map<std::string, aicpu::OpFullInfo> &all_op_info) const;

  ge::Status AddTilingModeAttr(ge::ComputeGraph &graph) const;

  ge::Status FillOpsParallelRuleInfos(RuleInfoDesc &ops_parallel_info);

  ge::Status GetOpsParallelInfo(std::unordered_set<string> &ops_parallel_info) const;

  ge::Status SetStreamLabelForOpsParallel(ge::ComputeGraph &graph) const;

  ge::Status SetStreamLabel(const ge::NodePtr &node, const std::string &label) const;

 private:
  std::string engine_name_;
  // optimizer map
  std::map<std::string, OptimizerPtr> optimizer_map_;
  std::string soc_version_;
  int auto_cast_mode_; 
};
}  // namespace aicpu

#endif  // AICPU_AICPU_GRAPH_OPTIMIZER_H_
