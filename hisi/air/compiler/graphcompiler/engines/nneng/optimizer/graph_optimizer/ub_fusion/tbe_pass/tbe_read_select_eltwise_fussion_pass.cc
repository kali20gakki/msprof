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

#include "tbe_read_select_eltwise_fussion_pass.h"
#include <algorithm>
#include <string>
#include <vector>

namespace fe {
using std::vector;

namespace {
static const char PATTERN_ELTWISE[] = "ElemWise";

static const char OP_TYPE_READ_SELECT[] = "ReadSelect";
}

/*
* @brief:  define read_select and eltwise op fusion pattern
*
*   ReadSelect-->ElemWise

* @return TbeFusionPattern: return all valid patterns.
*/
vector<BufferFusionPattern *> TbeReadSelectEltwiseFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;

  string pass_name = "TbeReadSelectEltwiseFusionPass";
  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(pass_name);
  FE_CHECK((pattern == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeRdSelcElemFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());
  pattern->AddOpDesc(PATTERN_ELTWISE, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_ELTWISE});

  patterns.push_back(pattern);
  FE_LOGD("End to define %s pass pattern.", pass_name.c_str());

  return patterns;
}

/*
* @brief: parse nodes matched in mapping and call DoFusion
* @param [in] graph: original graph
* @param [out] mapping: nodes matched by pattern
* @return bool: fusion status ok or not.
*/
Status TbeReadSelectEltwiseFusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                      vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeReadSelectEltwiseFusionPass!");
  vector<ge::NodePtr> elemwise_nodes = GetMatchedNodesByDescName(PATTERN_ELTWISE, mapping);

  if (elemwise_nodes.empty()) {
    FE_LOGI("Eltwise UB-fusion Pattern not found.");
    return SUCCESS;
  }

  fusion_nodes = GetMatchedNodes(mapping);
  for (auto elemwise_node : elemwise_nodes) {
    for (auto node_ptr : elemwise_node->GetInAllNodes()) {
      if (node_ptr->GetType() == OP_TYPE_READ_SELECT) {
        fusion_nodes.push_back(node_ptr);
      }
    }
  }
  if (fusion_nodes.size() == 1) {
    fusion_nodes.clear();
  }

  FE_LOGD("End to do TbeReadSelectEltwiseFusionPass!");
  return SUCCESS;
}

}  // namespace fe
