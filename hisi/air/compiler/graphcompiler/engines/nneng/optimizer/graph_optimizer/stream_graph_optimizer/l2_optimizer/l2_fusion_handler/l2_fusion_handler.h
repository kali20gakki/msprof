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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_HANDLER_L2_FUSION_HANDLER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_HANDLER_L2_FUSION_HANDLER_H_

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"
#include "common/math_util.h"
#include "common/l2fusion_struct.h"

namespace fe {

class L2FusionHandler {
  CCE_DECLARE_SINGLETON(L2FusionHandler)

 public:
  Status GetL2DataAlloc(uint64_t mem_base, ge::ComputeGraph &graph, TaskL2InfoFEMap_t &l2_info) const;

 private:
  Status GetNormalL2DataAlloc(uint64_t mem_base, ge::ComputeGraph &graph, TaskL2InfoFEMap_t &l2_info) const;
  Status GenerateFinalL2Info(uint64_t mem_base, ge::ComputeGraph &graph, k_l2_task_data_allocs_map_t &l2_alloc,
                             TaskL2InfoFEMap_t &l2) const;
  void GenerateL2Data(uint64_t mem_base, const k_l2_data_allocs_t &src_data, L2DataMap_t &dst_data) const;

  Status DisplayL2Info(TaskL2InfoFEMap_t &l2) const;
  Status DisplayL2DataInfo(string title, L2DataMap_t &input, rtL2Ctrl_t &l2ctrl, L2DataMap_t data) const;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_HANDLER_L2_FUSION_HANDLER_H_
