/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#include "graph_optimizer/ub_fusion/fusion_graph_merge/l1_fusion_graph_merge.h"

namespace fe {
L1FusionGraphMerge::L1FusionGraphMerge(const std::string &scope_attr, const GraphCommPtr &graph_comm_ptr)
    : FusionGraphMerge(scope_attr, graph_comm_ptr) {}

L1FusionGraphMerge::~L1FusionGraphMerge() {}
}
