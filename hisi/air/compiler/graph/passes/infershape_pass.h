/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#ifndef GE_GRAPH_PASSES_INFERSHAPE_PASS_H_
#define GE_GRAPH_PASSES_INFERSHAPE_PASS_H_

#include <stack>
#include "graph/passes/infer_base_pass.h"
#include "graph/manager/util/graph_rebuild_state_ctrl.h"
#include "graph/resource_context_mgr.h"
#include "external/graph/inference_context.h"

namespace ge {
class InferShapePass : public InferBasePass {
 public:
  InferShapePass() {}
  InferShapePass(GraphRebuildStateCtrl *ctrl, ResourceContextMgr *resource_mgr)
      : resource_op_access_ctrl_(ctrl), resource_context_mgr_(resource_mgr) {}
  std::string SerialTensorInfo(const GeTensorDescPtr &tensor_desc) const override;
  graphStatus Infer(NodePtr &node) override;

  graphStatus UpdateTensorDesc(const GeTensorDescPtr &src, GeTensorDescPtr &dst, bool &changed) override;
  graphStatus UpdateOutputFromSubgraphs(const std::vector<GeTensorDescPtr> &src, const GeTensorDescPtr &dst) override;
  graphStatus UpdateOutputFromSubgraphsForMultiDims(const std::vector<GeTensorDescPtr> &src,
                                                    const GeTensorDescPtr &dst) override;
  graphStatus UpdateOutputFromSubgraphsForSubgraphMultiDims(const std::vector<GeTensorDescPtr> &src,
                                                            const GeTensorDescPtr &dst) override;

  Status OnSuspendNodesLeaked() override;

 private:
  graphStatus InferShapeAndType(NodePtr &node);
  graphStatus CallInferShapeFunc(NodePtr &node, Operator &op) const;
  bool SameTensorDesc(const GeTensorDescPtr &src, const GeTensorDescPtr &dst) const;
  void UpdateCurNodeOutputDesc(const NodePtr &node) const;
  Status RepassReliedNodeIfResourceChanged(const InferenceContextPtr &inference_context, const NodePtr &cur_node);
  Status RegisterNodesReliedOnResource(const InferenceContextPtr &inference_context, NodePtr &node) const;
  Status SuspendV1LoopExitNodes(const NodePtr &node);
  struct SuspendNodes {
    std::stack<NodePtr> nodes;
    std::unordered_set<NodePtr> nodes_set;

    NodePtr PopSuspendedNode() {
      auto top_node = nodes.top();
      nodes.pop();
      nodes_set.erase(top_node);
      return top_node;
    }
  };
  std::map<std::string, SuspendNodes> graphs_2_suspend_nodes_;
  // if resource shape changed, use this ctrl to mark graph relied on that resource rebuild
  GraphRebuildStateCtrl *resource_op_access_ctrl_ = nullptr;
  // This mgr belongs to session, give it when create inference context, so that resource op can
  // get/set resource_shapes to mgr
  ResourceContextMgr *resource_context_mgr_ = nullptr;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_INFERSHAPE_PASS_H_
