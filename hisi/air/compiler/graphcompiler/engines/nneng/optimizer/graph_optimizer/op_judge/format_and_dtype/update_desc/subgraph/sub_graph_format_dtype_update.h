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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_SUBGRAPH_SUB_GRAPH_FORMAT_DTYPE_UPDATE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_SUBGRAPH_SUB_GRAPH_FORMAT_DTYPE_UPDATE_H_
#include "common/fe_inner_attr_define.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/graph/fe_graph_utils.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

namespace fe {
using RefRelationsPtr = std::shared_ptr<ge::RefRelations>;
class SubGraphFormatDtypeUpdate {
 public:
  explicit SubGraphFormatDtypeUpdate(RefRelationsPtr reflection_builder_ptr)
    : reflection_builder_ptr_(reflection_builder_ptr){};
  virtual ~SubGraphFormatDtypeUpdate(){};

  virtual Status UpdateTensorDesc(ge::NodePtr node_ptr) = 0;
  void UpdateFormat(ge::NodePtr node_ptr, const int &index, const bool &is_input);
  Status UpdateDtypeOfRelatedEdges(const ge::GeTensorDesc &tensor_desc, const ge::NodePtr &node_ptr,
                                   const ge::InOutFlag &in_out_flag, const int &index);

 protected:
  RefRelationsPtr reflection_builder_ptr_;

 private:
  Status UpdateDtypeOfRelatedEdges(const std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections,
                                   const RelationUpdateInfo &relation_update_info) const;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_UPDATE_DESC_SUBGRAPH_SUB_GRAPH_FORMAT_DTYPE_UPDATE_H_
