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

#include "graph/passes/print_op_pass.h"
#include <string>

namespace ge {
Status PrintOpPass::Run(ge::NodePtr &node) {
  GELOGD("PrintOpPass is running");
  GE_CHECK_NOTNULL(node);
  std::string ori_type;
  Status ret = GetOriginalType(node, ori_type);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Get][OriginalType] of node:%s failed", node->GetName().c_str());
    return ret;
  }
  if (ori_type == "Print") {
    GELOGI("Delete node: %s, type: Print.", node->GetName().c_str());
    return IsolateAndDeleteNode(node, {});
  }
  return SUCCESS;
}
}  // namespace ge
