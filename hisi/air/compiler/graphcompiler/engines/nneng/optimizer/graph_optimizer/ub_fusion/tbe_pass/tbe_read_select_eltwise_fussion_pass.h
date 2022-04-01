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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_READ_SELECT_ELTWISE_FUSSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_READ_SELECT_ELTWISE_FUSSION_PASS_H_

#include <vector>
#include "common/fe_log.h"
#include "graph_optimizer/buffer_fusion/buffer_fusion_pass_base.h"
#include "graph_optimizer/graph_fusion/pass.h"

namespace fe {
class TbeReadSelectEltwiseFusionPass : public BufferFusionPassBase {
 public:
  explicit TbeReadSelectEltwiseFusionPass() {}

  ~TbeReadSelectEltwiseFusionPass() override {}

 protected:
  /*
  * @brief:  define read_select and eltwise ops fusion pattern
  *
  *    ReadSelect-->ElemWise
  *
  * @return TbeFusionPattern: return all valid patterns.
  */
  vector<BufferFusionPattern *> DefinePatterns() override;

  /*
  * @brief: parse nodes matched in mapping and call DoFusion
  * @param [in] graph: original graph
  * @param [out] mapping: nodes matched by pattern
  * @return bool: fusion status ok or not.
  */
  Status GetFusionNodes(const BufferFusionMapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_READ_SELECT_ELTWISE_FUSSION_PASS_H_
