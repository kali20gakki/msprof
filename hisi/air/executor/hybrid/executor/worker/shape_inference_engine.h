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

#ifndef GE_HYBRID_EXECUTOR_INFERSHAPE_SHAPE_INFERENCE_ENGINE_H_
#define GE_HYBRID_EXECUTOR_INFERSHAPE_SHAPE_INFERENCE_ENGINE_H_

#include "hybrid/executor/hybrid_execution_context.h"
#include "hybrid/executor/subgraph_context.h"

namespace ge {
namespace hybrid {
class ShapeInferenceEngine {
 public:
  ShapeInferenceEngine(GraphExecutionContext *const execution_context, const bool force_infer_shape);
  ~ShapeInferenceEngine() = default;
  void Config(SubgraphContext *const subgraph_context);
  Status InferShape(const NodeState &node_state) const;
  Status InitInferShapes(const GraphItem *const graph_item,
                         const std::vector<TensorValue> &inputs,
                         const std::vector<ConstGeTensorDescPtr> &input_desc) const;
  bool IsForceInferShape() const { return force_infer_shape_; }

private:
  bool force_infer_shape_;

private:
  Status PropagateOutputs(const NodeItem *const node_item, const TensorValue *const tensor,
                          const GeTensorDesc *const tensor_desc) const;
  Status UpdateShapeAndValue(const NodeItem *const node_item, const TensorValue *const tensor,
                             const GeTensorDesc *const tensor_desc) const;
  Status DoInferShape(const NodeState &node_state) const;
  Status PropagateOutputShapes(const NodeState &node_state) const;
  Status InferShapeForSubgraph(const NodeItem &node_item, const FusedSubgraph &fused_subgraph) const;
  Status UpdatePeerNodeShape(const Node &node) const;
  Status SetDependingTensor(const FusedSubgraph &fused_subgraph) const;

  GraphExecutionContext *execution_context_;
  SubgraphContext *subgraph_context_{nullptr};
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_EXECUTOR_INFERSHAPE_SHAPE_INFERENCE_ENGINE_H_
