/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_CMO_GENERATE_CMO_TYPE_PREFETCH_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_CMO_GENERATE_CMO_TYPE_PREFETCH_H_

#include "generate_cmo_type_base.h"
namespace fe {
class GenerateCMOTypePrefetch : public GenerateCMOTypeBase {
public:
  GenerateCMOTypePrefetch();

  ~GenerateCMOTypePrefetch() override {};

  void GenerateType(const ge::NodePtr &node) override;

private:
  void LabeledPrefetch(const ge::NodePtr &src_node, const ge::NodePtr &dst_node);

  ge::NodePtr GetLastPreNode(const ge::NodePtr &node) const;

  bool CheckSizeIsAvailable(const ge::NodePtr &src_node, const ge::NodePtr &dst_node) const;

  bool CheckNeedPretch(const ge::NodePtr &src_node, const ge::NodePtr &dst_node) const;

  std::unordered_map<ge::NodePtr, ge::NodePtr> prefetch_cache_map_;
};
} // namespace fe
#endif
