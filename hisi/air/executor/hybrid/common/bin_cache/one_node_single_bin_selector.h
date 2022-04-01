/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#ifndef AIR_CXX_EXECUTOR_HYBRID_COMMON_BIN_CACHE_ONE_NODE_SINGLE_BIN_SELECTOR_H_
#define AIR_CXX_EXECUTOR_HYBRID_COMMON_BIN_CACHE_ONE_NODE_SINGLE_BIN_SELECTOR_H_

#include "graph/bin_cache/node_bin_selector.h"
#include "external/ge/ge_api_error_codes.h"
#include "graph/bin_cache/node_bin_selector_factory.h"
namespace ge {
class OneNodeSingleBinSelector : public NodeBinSelector {
 public:
  OneNodeSingleBinSelector() = default;
  NodeCompileCacheItem *SelectBin(const NodePtr &node, const GEThreadLocalContext *ge_context,
                                  std::vector<domi::TaskDef> &task_defs) override;
  Status Initialize() override;

 private:
  NodeCompileCacheItem cci_;
};
}  // namespace ge

#endif //AIR_CXX_EXECUTOR_HYBRID_COMMON_BIN_CACHE_ONE_NODE_SINGLE_BIN_SELECTOR_H_
