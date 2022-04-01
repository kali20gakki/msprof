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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_SUBGRAPH_SUB_NETOUTPUT_FORMAT_DTYPE_UPDATE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_SUBGRAPH_SUB_NETOUTPUT_FORMAT_DTYPE_UPDATE_H_

#include "graph_optimizer/op_judge/format_and_dtype/update_desc/subgraph/sub_graph_format_dtype_update.h"
namespace fe {

/** @brief update the input and the output descs of the netoutput node in the
* subgraph */
class SubNetOutputFormatDtypeUpdate : public SubGraphFormatDtypeUpdate {
 public:
  explicit SubNetOutputFormatDtypeUpdate(RefRelationsPtr reflection_builder_ptr)
      : SubGraphFormatDtypeUpdate(reflection_builder_ptr){};
  ~SubNetOutputFormatDtypeUpdate() override {};

  Status UpdateTensorDesc(ge::NodePtr node_ptr) override;

 private:
  Status UpdateDtype(ge::NodePtr node_ptr, const ge::InDataAnchorPtr &in_data_anchor_ptr);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_SUBGRAPH_SUB_NETOUTPUT_FORMAT_DTYPE_UPDATE_H_
