/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_CONNECTIVITY_MATRIX_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_CONNECTIVITY_MATRIX_H_
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "common/util/constants.h"
#include "common/util/op_info_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "common/large_bitmap.h"

namespace fe {
class ConnectivityMatrix {
 public:
  explicit ConnectivityMatrix(const ge::ComputeGraph &graph);

  ~ConnectivityMatrix();

  bool IsConnected(const ge::NodePtr &a, const ge::NodePtr &b) const;

  // inputs are all input nodes of parameter node.
  // if there is a path between A->B, then B will own A's
  // connectivity. The reason is :
  // If some node can reach A, than it can also reach B.
  void SetConnectivity(const std::vector<ge::NodePtr> &inputs, const ge::NodePtr &node);

  /* Computes the connectivity between two nodes in the
   * computation. The returned ConnectivityMatrix is constructed such that
   * ConnectivityMatrix::IsConnected(a, b) returns true iff there exists a
   * directed path (from producer to consumer) from 'a' to 'b'. Both data
   * connection and control connection are considered for connectivity.
   * A node is connected to itself. */
  static std::shared_ptr<ConnectivityMatrix> Generate(ge::ComputeGraph &graph);

  // update reachablity map for fused nodes.
  void Update(const ge::ComputeGraph &graph, std::vector<ge::NodePtr> &fusion_nodes);

 private:
  int64_t GetIndex(const ge::NodePtr &node) const;

  const LargeBitMap &GetBitMap(const ge::NodePtr &node) const;

  LargeBitMap &GetBitMap(const ge::NodePtr &node);

  const size_t size_;

  std::vector<LargeBitMap> bit_maps;

  std::map<std::string, int64_t> name_to_index_;
};
}
#endif  // AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_CONNECTIVITY_MATRIX_H_