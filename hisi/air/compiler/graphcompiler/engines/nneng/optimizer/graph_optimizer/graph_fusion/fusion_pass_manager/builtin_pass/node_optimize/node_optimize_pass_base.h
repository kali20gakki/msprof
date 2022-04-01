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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_NODE_OPTIMIZE_PASS_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_NODE_OPTIMIZE_PASS_BASE_H_

#include <vector>
#include "common/fe_inner_attr_define.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {
class NodeOptimizePassBase : public PatternFusionBasePass {
 public:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;
  virtual Status DoFusion(ge::ComputeGraph &graph, ge::NodePtr &node_ptr, vector<ge::NodePtr> &fusion_nodes) = 0;
  virtual vector<string> GetNodeTypes() = 0;
  virtual string GetPatternName() = 0;

 protected:
  int64_t GetDimAttrValue(const ge::OpDescPtr &op_desc_ptr, const string &dim_attr, const bool &is_input) const;
  Status InsertNode(const ge::OutDataAnchorPtr &src, const ge::InDataAnchorPtr &dst,
                    ge::NodePtr &new_node, ge::DataType quant_data_type=ge::DT_FLOAT);
  Status CreateStridedRead(ge::NodePtr next_node, std::shared_ptr<ge::OpDesc> &strided_read_opdesc);
  Status CreateStridedWrite(ge::NodePtr prev_node, std::shared_ptr<ge::OpDesc> &strided_write_opdesc);
  void SetGeAttrForConcat(const ge::OpDescPtr &concat_op_desc_ptr, const size_t &dim_index) const;
  void SetGeAttrForSplit(const ge::OpDescPtr &split_op_desc_ptr, const size_t &dim_index) const;
  Status GetNC1HWC0Shape(ge::GeTensorDescPtr tensor_desc, const ge::DataType &quant_data_type) const;
  bool is_single_out_and_ref(const ge::NodePtr &node_ptr) const;
  Status JudgeOp(ge::NodePtr node) const;

  const string CONCATD = "ConcatD";
  const string CONCATV2D = "ConcatV2D";
  const string SPLITD = "SplitD";
  const string SPLITVD = "SplitVD";
  const string QUANT = "AscendQuant";
  const string DEQUANT = "AscendDequant";
  const string REQUANT = "AscendRequant";
  const string CONV2D = "Conv2D";
  const string CONV2D_COMPRESS = "Conv2DCompress";
  const string MAXPOOL = "MaxPool";
  const string MAXPOOLV3 = "MaxPoolV3";
  const string POOLING = "Pooling";
  const string RELU = "Relu";
  const string MISH = "Mish";
  const string LEAKYRELU = "LeakyRelu";
  const string STRIDEDWRITE = "StridedWrite";
  const string STRIDEDREAD = "StridedRead";
  const string STRIDE_ATTR_STRIDE = "stride";
  const string STRIDE_ATTR_AXIS = "axis";
  const string ATTR_SCALE = "scale";
  const string ATTR_OFFSET = "offset";
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_NODE_OPTIMIZE_PASS_BASE_H_
