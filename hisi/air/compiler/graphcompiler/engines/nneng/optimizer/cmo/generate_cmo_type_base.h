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


#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_CMO_GENERATE_CMO_TYPE_BASE_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_CMO_GENERATE_CMO_TYPE_BASE_H_

#include "graph/node.h"
#include "utils/graph_utils.h"
#include "common/aicore_util_types.h"

namespace fe {
class GenerateCMOTypeBase {
public:
  GenerateCMOTypeBase();

  virtual ~GenerateCMOTypeBase(){};

  virtual void GenerateType(const ge::NodePtr &node) {};
protected:
  void AddToNodeCmoAttr(const ge::OpDescPtr &op_desc, const std::string &cmo_type,
                        const std::vector<CmoAttr> &attr_vec) const;

  uint64_t GetCacheSize() const;

  int64_t GetInputTensorSize(const ge::OpDescPtr &op_desc) const;

  int64_t GetOutputTensorSize(const ge::OpDescPtr &op_desc) const;

  int64_t GetWorkSpaceSize(const ge::OpDescPtr &op_desc) const;

  int64_t GetWeightSize(const ge::NodePtr &node) const;

  bool CheckParentOpIsAiCore(const ge::InDataAnchorPtr &in_anchor) const;

  bool ReadIsLifeCycleEnd(const ge::NodePtr &node, const ge::InDataAnchorPtr &in_anchor) const;
private:
  void CalcTotalTensorSize(const ge::GeTensorDescPtr &tensor_desc, int64_t &total_tensor_size) const;
}; // class GenerateCMOTypeBase
}  // namespace fe

#endif
