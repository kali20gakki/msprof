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

#include "graph/passes/constant_folding_pass.h"

#include <vector>
#include "graph/utils/node_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/constant_utils.h"
#include "local_engine/engine/host_cpu_engine.h"
#include "init/gelib.h"
#include "register/op_kernel_registry.h"

namespace ge {
namespace {
const int64_t kStartCallNum = 1;
const char *const kKernelLibName = "aicpu_ascend_kernel";
const char *const kOpsFlagClose = "0";
const char *const kPassName = "ConstantFoldingPass";
}

bool ConstantFoldingPass::NeedIgnorePass(const NodePtr &node) {
  if (folding_pass::IsNoNeedConstantFolding(node)) {
    return true;
  }
  if (AreAllOutputsEmptyShape(node->GetOpDesc())) {
    GELOGI("Cur node %s is potential empty const, ignore pass.", node->GetName().c_str());
    return true;
  }
  return false;
}

bool ConstantFoldingPass::NeedFold() const {
  return need_fold_;
}

Status ConstantFoldingPass::ComputePotentialWeight(NodePtr &node, std::vector<GeTensorPtr> &outputs) {
  need_fold_ = true;
  GELOGD("Begin to run constant folding compute on node %s", node->GetName().c_str());
  OpDescPtr node_desc = node->GetOpDesc();
  auto input_nodes_2_out_anchors = OpDescUtils::GetConstInputNodeAndAnchor(*node);
  if (input_nodes_2_out_anchors.empty() || input_nodes_2_out_anchors.size() != node_desc->GetInputsSize()) {
    GELOGD("Node:%s, const input nodes size is %zu, and nodeDesc inputsSize is %zu.", node->GetName().c_str(),
           input_nodes_2_out_anchors.size(), node_desc->GetInputsSize());
    if (ConstantUtils::IsPotentialConst(node_desc)) {
      need_fold_ = false;
    }
    return NOT_CHANGED;
  }
  auto inputs = OpDescUtils::GetWeightsFromNodes(input_nodes_2_out_anchors);
  if (inputs.size() != input_nodes_2_out_anchors.size()) {
    GELOGW("Get weights from const_inputs size %zu, not match with inputs size %zu. Ignore pass.", inputs.size(),
           input_nodes_2_out_anchors.size());
    return NOT_CHANGED;
  }
  // check input nodes has potential const
  for (const auto &node_2_anchor : input_nodes_2_out_anchors) {
    if (ConstantUtils::IsPotentialConst(node_2_anchor.first->GetOpDesc())) {
      need_fold_ = false;
      break;
    }
  }
  // Try to run kernel on host cpu
  uint64_t start_time = GetCurrentTimestamp();
  Status compute_ret = ComputeWithHostCpuKernel(node, inputs, outputs);
  if (compute_ret == SUCCESS) {
    CollectCostTimeOfOpConstantFolding(node, start_time);
  } else {
    // if can't compute on aicpu, try run host kernel inside ge
    compute_ret = ComputeWithBuiltInKernel(node, inputs, outputs);
  }
  GELOGD("Node %s type %s, constant folding compute finish, compute ret is %u.", node->GetName().c_str(),
         node->GetType().c_str(), compute_ret);
  return compute_ret;
}

Status ConstantFoldingPass::ComputeWithBuiltInKernel(NodePtr &node, const vector<ConstGeTensorPtr> &inputs,
                                                     std::vector<GeTensorPtr> &outputs) {
  auto op_kernel = folding_pass::GetKernelByType(node);
  if (op_kernel == nullptr) {
    GELOGD("No op kernel for node %s type %s, skip the constant folding", node->GetName().c_str(),
           node->GetType().c_str());
    return NOT_CHANGED;
  }

  // Statistic of ge constant folding kernel
  uint64_t start_time = GetCurrentTimestamp();
  auto ret = op_kernel->Compute(node->GetOpDesc(), inputs, outputs);
  CollectCostTimeOfGeConstantFolding(node, start_time);
  return ret;
}

Status ConstantFoldingPass::ComputeWithHostCpuKernel(const NodePtr &node, const vector<ConstGeTensorPtr> &inputs,
                                                     std::vector<GeTensorPtr> &outputs) {
  std::shared_ptr<GELib> instance_ptr = ge::GELib::GetInstance();
  if ((instance_ptr == nullptr) || (!instance_ptr->InitFlag())) {
    GELOGE(GE_CLI_GE_NOT_INITIALIZED, "[Check][Param] GE is not initialized or is finalized.");
    return UNSUPPORTED;
  }
  OpsKernelInfoStorePtr kernel_info = instance_ptr->OpsKernelManagerObj().GetOpsKernelInfoStore(kKernelLibName);
  if (kernel_info == nullptr) {
    GELOGE(FAILED, "[Get][OpsKernelInfoStore] %s failed", kKernelLibName);
    return UNSUPPORTED;
  }

  std::string ops_flag;
  kernel_info->opsFlagCheck(*node, ops_flag);
  if (ops_flag == kOpsFlagClose) {
    return UNSUPPORTED;
  }
  return RunOpKernel(node, inputs, outputs);
}

Status ConstantFoldingPass::RunOpKernel(const NodePtr &node,
                                        const std::vector<ConstGeTensorPtr> &inputs,
                                        std::vector<GeTensorPtr> &outputs) {
  const std::string op_type = NodeUtils::GetNodeType(node);
  auto kernel = OpKernelRegistry::GetInstance().CreateHostCpuOp(op_type);
  if (kernel == nullptr) {
    GELOGD("Op of type %s is not supported by host cpu engine", op_type.c_str());
    return UNSUPPORTED;
  }

  GELOGD("Successfully created op kernel. op type = %s", op_type.c_str());
  return HostCpuEngine::GetInstance().Run(node, *kernel, inputs, outputs);
}

const std::map<std::string, std::pair<uint64_t, uint64_t>> &ConstantFoldingPass::GetGeConstantFoldingPerfStatistic()
    const {
  return statistic_of_ge_constant_folding_;
}

const std::map<std::string, std::pair<uint64_t, uint64_t>> &ConstantFoldingPass::GetOpConstantFoldingPerfStatistic()
    const {
  return statistic_of_op_constant_folding_;
}

void ConstantFoldingPass::CollectCostTimeOfGeConstantFolding(const NodePtr &node, uint64_t start_time) {
  uint64_t cost_time = GetCurrentTimestamp() - start_time;
  if (statistic_of_ge_constant_folding_.find(node->GetType()) != statistic_of_ge_constant_folding_.end()) {
    uint64_t &cnt = statistic_of_ge_constant_folding_[node->GetType()].first;
    uint64_t &cur_cost_time = statistic_of_ge_constant_folding_[node->GetType()].second;
    cnt++;
    cur_cost_time += cost_time;
  } else {
    statistic_of_ge_constant_folding_[node->GetType()] = std::pair<uint64_t, uint64_t>(kStartCallNum, cost_time);
  }
}

void ConstantFoldingPass::CollectCostTimeOfOpConstantFolding(const NodePtr &node, uint64_t start_time) {
  if (statistic_of_op_constant_folding_.find(node->GetType()) != statistic_of_op_constant_folding_.end()) {
    uint64_t &cnt = statistic_of_op_constant_folding_[node->GetType()].first;
    uint64_t &cost_time = statistic_of_op_constant_folding_[node->GetType()].second;
    cnt++;
    cost_time += GetCurrentTimestamp() - start_time;
  } else {
    statistic_of_op_constant_folding_[node->GetType()] =
        std::pair<uint64_t, uint64_t>(kStartCallNum, GetCurrentTimestamp() - start_time);
  }
}

string ConstantFoldingPass::GetPassName() const {
  return kPassName;
}
}  // namespace ge
