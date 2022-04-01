/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "graph/passes/set_ffts_plus_attr_pass.h"
#include "graph/utils/node_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace {
// temporary attr.FE formal solotion will determine the attribute name.
const std::string ATTR_NAME_FFTS_PLUS_ENGINE = "_ffts_plus";
}

namespace ge {
Status SetFftsPlusAttrPass::Run(ComputeGraphPtr graph) {
  GE_CHECK_NOTNULL(graph);

  if (graph->GetParentNode() == nullptr) {
    return SUCCESS;
  }

  const OpDescPtr &op_desc = graph->GetParentNode()->GetOpDesc();
  if (!op_desc->HasAttr(ATTR_NAME_FFTS_PLUS_SUB_GRAPH)) {
    return SUCCESS;
  }

  for (const NodePtr &node : graph->GetAllNodes()) {
    const auto tmp_op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(tmp_op_desc);
    (void)ge::AttrUtils::SetBool(tmp_op_desc, ATTR_NAME_FFTS_PLUS_ENGINE, true);

    if (tmp_op_desc->GetType() == NOOP) {
      (void)ge::AttrUtils::SetBool(tmp_op_desc, ATTR_NAME_NOTASK, true);
    }
  }

  return SUCCESS;
}
}
