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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_eltwise_quant_fusion_pass.h"
#include <string>
#include <vector>

namespace fe {

static const string PATTERN_ELTWISE1 = "eltwise1";
static const string PATTERN_ELTWISE2 = "eltwise2";
static const string PATTERN_ELTWISE3 = "eltwise3";
static const string PATTERN_QUANT = "quant";
static const string PATTERN_OTHER_INPUT = "inputData";
static const string PATTERN_OTHER_INPUT1 = "inputData1";
/*
 * @brief:  define multi elementwise quant fusion pattern
 *
 * pattern configuration limit:
 * 1. total min value must be 1 for all head candidated desc.
 * 2. any head candidated desc max value must be 1.
 * 3. output desc can not be itself.
 *
 * fusion node: ElemWise_1, ElemWise_2,...,Quant
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeEltwiseQuantFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;

  const string pass_name1 = "TbeEltwiseQuantFusion1";
  BufferFusionPattern *pattern1 = new (std::nothrow) BufferFusionPattern(pass_name1);
  FE_CHECK((pattern1 == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeElemQntFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name1.c_str());
  /*
   * bn-->sacle-->relu-->Quant
   *      ^   ^
   *      |   |
   *      | input_data
   *   input_data1
   */
  pattern1->AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OTHER_INPUT, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OTHER_INPUT1, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE3, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_QUANT, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_ELTWISE1})
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_ELTWISE2})
      .SetOutputs(PATTERN_ELTWISE2, {PATTERN_ELTWISE3})
      .SetOutputs(PATTERN_ELTWISE3, {PATTERN_QUANT})
      .SetOutputs(PATTERN_OTHER_INPUT, {PATTERN_ELTWISE2})
      .SetOutputs(PATTERN_OTHER_INPUT1, {PATTERN_ELTWISE2});
  patterns.push_back(pattern1);
  FE_LOGD("End to define %s pass pattern.", pass_name1.c_str());

  const string pass_name2 = "TbeEltwiseQuantFusion2";
  BufferFusionPattern *pattern2 = new (std::nothrow) BufferFusionPattern(pass_name2);
  FE_CHECK((pattern2 == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeElemQntFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name2.c_str());
  /*
   *    ElemWise_1-->ElemWise_2-->ElemWise_3 ...(Max is 5) -->Quant
   */
  pattern2->AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_QUANT, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_ELTWISE1})
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_ELTWISE2})
      .SetOutputs(PATTERN_ELTWISE2, {PATTERN_QUANT}, TBE_OUTPUT_BRANCH_SINGLE, true, false);
  patterns.push_back(pattern2);
  FE_LOGD("End to define %s pass pattern.", pass_name2.c_str());

  const string pass_name3 = "TbeEltwiseQuantFusion3";
  BufferFusionPattern *pattern3 = new (std::nothrow) BufferFusionPattern(pass_name3);
  FE_CHECK((pattern3 == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeElemQntFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name3.c_str());
  /*
   *    ElemWise_1-->Quant
   */
  pattern3->AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_QUANT, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_ELTWISE1})
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_QUANT});
  patterns.push_back(pattern3);
  FE_LOGD("End to define %s pass pattern.", pass_name3.c_str());
  return patterns;
}

Status TbeEltwiseQuantFusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                 vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeEltwiseQuantFusionPass!");
  vector<ge::NodePtr> elem1_node = GetMatchedNodesByDescName(PATTERN_ELTWISE1, mapping);
  vector<ge::NodePtr> quant_node = GetMatchedNodesByDescName(PATTERN_QUANT, mapping);
  if (elem1_node.empty()) {
    FE_LOGW("Elemwise node not matched.");
    return SUCCESS;
  }
  if (quant_node.empty()) {
    FE_LOGW("quant node not matched.");
    return SUCCESS;
  }

  FE_LOGD("Node[%s]:type[%s] Elemwise info", elem1_node[0]->GetName().c_str(), elem1_node[0]->GetType().c_str());
  FE_LOGD("Node[%s]:type[%s] AscendQuant info", quant_node[0]->GetName().c_str(), quant_node[0]->GetType().c_str());

  auto elem1_format = ge::GetPrimaryFormat(elem1_node[0]->GetOpDesc()->GetInputDesc(0).GetFormat());
  auto quant_format = ge::GetPrimaryFormat(quant_node[0]->GetOpDesc()->GetInputDesc(0).GetFormat());

  if (elem1_format != ge::FORMAT_NC1HWC0) {
    fusion_nodes.clear();
    FE_LOGW("Elemwise is not vaild, only support FORMAT_NC1HWC0");
    return SUCCESS;
  }

  if (quant_format != ge::FORMAT_NC1HWC0) {
    fusion_nodes.clear();
    FE_LOGW("quant is not vaild, only support FORMAT_NC1HWC0");
    return SUCCESS;
  }

  if (quant_node[0]->GetType() != "AscendQuant") {
    fusion_nodes.clear();
    FE_LOGW(
        "we only support op type : AscendQuant, "
        "not support %s.",
        quant_node[0]->GetType().c_str());
    return SUCCESS;
  }

  fusion_nodes = GetMatchedNodes(mapping);
  FE_LOGD("End to TbeEltwiseQuantFusionPass!");
  return SUCCESS;
}
}  // namespace fe
