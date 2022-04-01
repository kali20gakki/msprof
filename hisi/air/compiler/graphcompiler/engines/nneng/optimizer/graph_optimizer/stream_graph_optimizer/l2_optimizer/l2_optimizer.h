/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_OPTIMIZER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_OPTIMIZER_H_

#include <map>
#include "common/configuration.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "common/graph/fe_graph_utils.h"
#include "graph/compute_graph.h"
#include "runtime/base.h"

namespace fe {
class L2Optimizer {
 public:
  explicit L2Optimizer(const std::string &engine_name);

  ~L2Optimizer();
  Status GetL2DataAlloc(ge::ComputeGraph &stream_graph, uint64_t mem_base, const rtStream_t &streamId) const;
  Status UpdateInputForL2Fusion(const ge::ComputeGraph &stream_graph) const;
  Status UpdateDDRForL2Fusion(const ge::ComputeGraph &stream_graph, uint64_t mem_base) const;

 private:
  std::string engine_name_;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_OPTIMIZER_H_
