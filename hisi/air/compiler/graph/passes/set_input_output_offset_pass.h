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
#ifndef GE_GRAPH_PASSES_SET_INPUT_OUTPUT_OFFSET_PASS_H_
#define GE_GRAPH_PASSES_SET_INPUT_OUTPUT_OFFSET_PASS_H_

#include "inc/graph_pass.h"

namespace ge {
class SetInputOutputOffsetPass : public GraphPass {
 public:
  Status Run(ComputeGraphPtr graph) override;

 private:
  Status SetInputOffset(const NodePtr &node, const std::vector<int32_t> &connect_input) const;
  Status SetOutputOffset(const NodePtr &node, const std::vector<int32_t> &connect_output) const;
  Status SetInputOffsetForFusion(const std::vector<int64_t> &memory_type, const ge::NodePtr &node) const;
  Status SetInputOffsetForHcom(const NodePtr &node, const std::vector<int32_t> &connect_input) const;
  Status SetOutputOffsetForConcat(const NodePtr &node) const;
  Status SetOutputOffsetForHcom(const NodePtr &node, const std::vector<int32_t> &connect_output) const;

  bool CheckBufferFusion(const NodePtr &node) const;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_SET_INPUT_OUTPUT_OFFSET_PASS_H_
