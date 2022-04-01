/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#ifndef MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_BASE_MDS_KERNEL_H_
#define MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_BASE_MDS_KERNEL_H_

#include <vector>
#include "common/op/ge_op_utils.h"
#include "graph/compute_graph.h"
#include "graph/graph.h"
#include "graph/op_desc.h"
#include "framework/common/types.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/shape_refiner.h"
#include "graph/passes/mds_kernels/mds_utils.h"
#include "graph/passes/mds_kernels/mds_kernel_factory.h"

namespace ge {
class DeploySchedulerKernel {
 public:
  DeploySchedulerKernel() = default;
  virtual ~DeploySchedulerKernel() = default;

  /// CutN process
  /// @param [in] node: node to cut
  /// @param [in/out] input_node: graph's input node added in cut process
  virtual Status CutN(const NodePtr &node, NodePtr &input_node);

  /// CutH process
  /// @param [in] node: node to cut
  /// @param [in/out] input_node: graph's input node added in cut process
  virtual Status CutH(const NodePtr &node, NodePtr &input_node);

  /// DynamicCutN process
  /// @param [in] node: node to cut
  virtual Status DynamicCutN(const NodePtr &node);

  /// DynamicCutH process
  /// @param [in] node: node to cut
  virtual Status DynamicCutH(const NodePtr &node);

  // halo exchange process
  Status HaloExchangeProcess(NodePtr node, int64_t index, bool local_slice = false) const;
};

namespace mds_cut_pass {
shared_ptr<DeploySchedulerKernel> GetKernelByType(const NodePtr &node);
}
}  // namespace ge
#endif  // MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_BASE_MDS_KERNEL_H_