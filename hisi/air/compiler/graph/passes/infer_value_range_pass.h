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

#ifndef GE_GRAPH_PASSES_INFER_VALUE_RANGE_PASS_H_
#define GE_GRAPH_PASSES_INFER_VALUE_RANGE_PASS_H_

#include "graph/passes/infer_base_pass.h"

namespace ge {
class InferValueRangePass : public InferBasePass {
 public:
  graphStatus Infer(NodePtr &node) override;

 private:
  std::string SerialTensorInfo(const GeTensorDescPtr &tensor_desc) const override;
  graphStatus UpdateTensorDesc(const GeTensorDescPtr &src, GeTensorDescPtr &dst, bool &changed) override;
  graphStatus UpdateOutputFromSubgraphs(const std::vector<GeTensorDescPtr> &src, const GeTensorDescPtr &dst) override;
  graphStatus UpdateOutputFromSubgraphsForMultiDims(const std::vector<GeTensorDescPtr> &src,
                                                    const GeTensorDescPtr &dst) override;
  graphStatus UpdateOutputFromSubgraphsForSubgraphMultiDims(const std::vector<GeTensorDescPtr> &src,
                                                            const GeTensorDescPtr &dst) override;
  bool NeedInfer(const NodePtr &node) const override;

  bool InputIsDynamic(const NodePtr &node) const;
  bool InputIsConstOrHasValueRange(const NodePtr &node) const;
  void CheckInputValueRange(const NodePtr &node, bool &has_unknown_value_range, bool &has_zero_in_value_range) const;
  graphStatus GenerateWorstValueRange(const NodePtr &node) const;
  template <typename T>
  graphStatus ConstructData(const GeTensorDesc &tensor_desc, bool use_floor_value,
                            const GeTensorPtr &output_ptr) const;
  graphStatus ConstructDataByType(const GeTensorDesc &tensor_desc, bool use_floor_value,
                                  const GeTensorPtr &output_ptr) const;
  std::vector<ConstGeTensorPtr> ConstructInputTensors(const NodePtr &node, bool use_floor_value) const;
  template <typename T>
  void ConstructValueRange(const GeTensorPtr &left_tensor, const GeTensorPtr &right_tensor,
                           std::vector<std::pair<int64_t, int64_t>> &value_range) const;
  graphStatus ConstructInputAndInferValueRange(const NodePtr &node) const;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_INFER_VALUE_RANGE_PASS_H_
