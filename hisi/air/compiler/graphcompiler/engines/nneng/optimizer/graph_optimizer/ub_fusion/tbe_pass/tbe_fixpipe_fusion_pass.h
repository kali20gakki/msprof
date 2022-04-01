/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#ifndef AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_FIXPIPE_FUSION_PASS_H_
#define AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_FIXPIPE_FUSION_PASS_H_

#include <vector>
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "graph_optimizer/buffer_fusion/buffer_fusion_pass_base.h"

namespace fe {
class TbeFixPipeFusionPass : public BufferFusionPassBase {
 public:
  explicit TbeFixPipeFusionPass() {}

  ~TbeFixPipeFusionPass() {}

 protected:
  /*
   * @brief:  define common rules0 ops fusion pattern
   *
   *
   * @return BufferFusionPattern: return all valid patterns.
   */
  vector<BufferFusionPattern *> DefinePatterns() override;

  /*
   * @brief: parse nodes matched in mapping and call DoFusion
   * @param [in] graph: original graph
   * @param [out] mapping: nodes matched by pattern
   * @return bool: fusion status ok or not.
   */
  Status GetFusionNodes(const BufferFusionMapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  void CheckFixPipeNode(const ge::NodePtr &cube_node, const ge::NodePtr &fixpipe_node,
                        vector<ge::NodePtr> &fusion_nodes) const;
};
}  // namespace fe

#endif  // AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_FIXPIPE_FUSION_PASS_H_
