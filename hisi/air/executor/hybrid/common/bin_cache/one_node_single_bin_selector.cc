/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#include "one_node_single_bin_selector.h"

namespace ge {
// for non-fuzz-compile
NodeCompileCacheItem *OneNodeSingleBinSelector::SelectBin(const NodePtr &node, const GEThreadLocalContext *ge_context,
                                                          std::vector<domi::TaskDef> &task_defs) {
  (void)node;
  (void)ge_context;
  (void)task_defs;
  cci_.SetCacheItemId(0);
  return &cci_;
}

Status OneNodeSingleBinSelector::Initialize() {
  return SUCCESS;
}

REGISTER_BIN_SELECTOR(kOneNodeSingleBinMode, OneNodeSingleBinSelector);
}  // namespace ge