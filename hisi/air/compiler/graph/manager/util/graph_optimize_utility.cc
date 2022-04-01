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

#include "graph/manager/util/graph_optimize_utility.h"
#include <memory>
#include "graph/preprocess/graph_preprocess.h"

namespace ge {
GraphOptimizeUtility::GraphOptimizeUtility() {}
GraphOptimizeUtility::~GraphOptimizeUtility() {}

Status GraphOptimizeUtility::InferShape(ComputeGraph &compute_graph) {
  auto compute_graph_ptr = std::make_shared<ComputeGraph>(compute_graph);
  return GraphPrepare::InferShapeForPreprocess(compute_graph_ptr, nullptr, nullptr);
}

Status GraphOptimizeUtility::InferShape(const ComputeGraphPtr &compute_graph) {
  auto compute_graph_ptr = compute_graph;
  return GraphPrepare::InferShapeForPreprocess(compute_graph_ptr, nullptr, nullptr);
}
}  // namespace ge
