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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_PARSER_L2_FUSION_PARSER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_PARSER_L2_FUSION_PARSER_H_

#include <vector>
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"

namespace fe {

class L2FusionParser {
  CCE_DECLARE_SINGLETON(L2FusionParser)

 public:
  Status GetDataDependentCountMap(const ge::ComputeGraph &graph,
                                  k_data_dependent_count_map &data_dependent_count_map) const;
  Status GetDataFromGraph(std::vector<ge::NodePtr> &nodes, ge::ComputeGraph &graph, k_l2_task_datas_map_t &datas_map);

 private:
  bool HasAtomicNode(const ge::NodePtr &nodePtr) const;
  Status GetDataFromNode(ge::NodePtr node, ge::ComputeGraph &graph, k_l2_task_datas_t &datas);
  Status GetInputDataFromNode(ge::NodePtr node, ge::ComputeGraph &graph, k_l2_datas_t &datas) const;
  Status GetOutputDataFromNode(ge::NodePtr node, ge::ComputeGraph &graph, k_l2_datas_t &datas);

  int64_t GetDataUnitDataId(ge::NodePtr node, uint32_t data_id, uint32_t data_type, const ge::GeTensorDesc &tensor,
                            const ge::ComputeGraph &graph) const;
  Status ModifyNodeTaskNum(ge::NodePtr node, uint32_t &task_num) const;

  bool IsNotSupportOp(const ge::NodePtr &node) const;
  bool NoNeedAllocInputsAndOutputs(const ge::NodePtr &node) const;
  bool NoNeedAllocOutputs(const ge::NodePtr &node) const;
  bool NoNeedAllocOutput(const ge::GeTensorDesc &tensor_desc) const;

};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_PARSER_L2_FUSION_PARSER_H_
