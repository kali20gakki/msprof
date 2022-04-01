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
#ifndef AIR_CXX_GRAPH_STAGE_CACHE_H
#define AIR_CXX_GRAPH_STAGE_CACHE_H

#include <map>
#include <mutex>
#include <vector>
#include "ge/ge_api_error_codes.h"
#include "node_shape_propagator.h"

namespace ge {
namespace hybrid {
struct CacheTensorDescObserver;
struct NodeItem;

struct GraphStageCache {
  GraphStageCache() = default;
  Status CreatePropagator(NodeItem &cur_node, const int32_t output_index, NodeItem &next_node,
                          const int32_t input_index);
  Status DoPropagate(const int32_t stage);

private:
  struct StageCache {
    StageCache() = default;
    ~StageCache();
    TensorDescObserver CreateTensorDescObserver(const std::shared_ptr<CacheTensorDescObserver> &observer);
    Status DoPropagate();

  private:
    std::vector<std::shared_ptr<CacheTensorDescObserver>> tensor_desc_observers_;
    std::mutex sync_;
  };

private:
  StageCache &GetOrCreate(const int32_t stage);

private:
  std::map<int32_t, StageCache> stage_caches_;
};
} // namespace hybrid
} // namespace ge
#endif
