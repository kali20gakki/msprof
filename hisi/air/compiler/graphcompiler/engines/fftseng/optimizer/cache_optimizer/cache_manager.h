/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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
#ifndef FFTS_ENGINE_OPTIMIZER_CACHE_OPTIMIZER_CACHE_MANAGEMENT_H_
#define FFTS_ENGINE_OPTIMIZER_CACHE_OPTIMIZER_CACHE_MANAGEMENT_H_
#include "inc/ffts_error_codes.h"
#include "graph/compute_graph.h"

namespace ffts {
const uint32_t kPersistThreshold = 2;
class CacheManager {
 public:
  CacheManager();

  ~CacheManager();

  static Status SetCacheOperation(ge::ComputeGraph &graph);

  static void HandleCachePersist(ge::NodePtr &node);

  /* Set Cache persist for all nodes.
   * The following scenario is not supported:
   * ConstA -> FunctionOp -> X(X is a arbitary op),
   * and inside FunctionOp the ConstA mapped as a Data:
   * Data ---> C (thread scope n)
   *      \--> D (thread scope n),
   * In this case, ConstA needs persistent. But Currently,
   * we do not support. */
  static void SetPersistWeightForGraph(ge::ComputeGraph& graph);
  /**
  * @ingroup ffts
  * @brief prohibit copy and assign construct
  */
  CacheManager(const CacheManager &) = delete;

  CacheManager &operator=(const CacheManager &) = delete;

 private:
  /* If one weight node has more than one successor which belongs to
   * same thread scope, we consider the weight node needs cache
   * persistent. Set one attribute kCachePersistScopeIds on its OpDesc.
   * kCachePersistScopeIds:
   * A list of thread scope ids which need cache
   * persistent in their SQE. */
  static void SetPersistScopeIds(const ge::NodePtr &node);
};

}
#endif // OPTIMIZER_GRAPH_OPTIMIZER_FFTS_PLUS_CACHE_MANAGEMENT_H_
