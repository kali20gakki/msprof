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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_STRIDE_HOIST_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_STRIDE_HOIST_PASS_H_

#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#include "common/fe_log.h"
#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/range_vistor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {
using std::set;
using std::unordered_set;
using std::queue;

class StrideHoistingPass : public PatternFusionBasePass {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) override;

 private:
  ge::NodePtr GetLowestCommonAncestor(ge::NodePtr node);
  vector<vector<ge::NodePtr>> GetPathsFromLowestCommonAncestor(ge::NodePtr node);
  Status SetInputOutputShapeOfReadSelectOp(ge::NodePtr node, ge::GeTensorDesc &parent_tensor);
  Status ConnectReadSelectOp(ge::NodePtr src_node, ge::NodePtr dst_node, ge::NodePtr rs_node);
  ge::NodePtr InsertReadSelectOp(ge::ComputeGraph &graph, vector<ge::NodePtr> &path);
  Status GetHWDim(const ge::Format &format, vector<size_t> &dims) const;
  Status HalveShape(ge::GeShape &shape, const ge::Format format) const;
  Status ReduceInput(unordered_set<ge::NodePtr> nodes);
  Status ReduceOutput(unordered_set<ge::NodePtr> nodes) const;
  Status ReduceInputAndOutput(unordered_set<ge::NodePtr> nodes);
  bool CheckFmapShapeEven(const ge::NodePtr &node);
  vector<int64_t> GetConvAttrs(ge::NodePtr node) const;
  bool CheckChildren(ge::NodePtr node) const;
  Status ChangeStride(ge::NodePtr node, int stride) const;
  bool ConvExists(vector<vector<ge::NodePtr>> &paths);
  bool AllPathsAreSimple(vector<vector<ge::NodePtr>> &paths);
  bool CheckPaths(vector<vector<ge::NodePtr>> &paths);
  Status ChangeGraphConvPath(vector<ge::NodePtr> &path);
  Status ChangeGraph(ge::ComputeGraph &graph, vector<vector<ge::NodePtr>> paths);
  Status CheckShapesAndTopological(ge::ComputeGraph &graph, ge::NodePtr elt_node);
  Status FusionRequantS16Case(ge::ComputeGraph &graph, Mapping &mapping);
  Status FusionConvCase(ge::ComputeGraph &graph, Mapping &mapping);
  Status FusionEltwiseCase(ge::ComputeGraph &graph, Mapping &mapping);
  Status ReduceAllImpl(ge::ComputeGraph &graph, ge::NodePtr conv_node, ge::Node::Vistor<ge::NodePtr> out_nodes);
  ge::NodePtr node_has_children_k1_s2;
  std::string CONV2D = "Conv2D";
  std::string CONV2D_COMPRESS = "Conv2DCompress";
  std::string ELTWISE = "Eltwise";
  std::string RELU = "Relu";
  std::string LEAKY_RELU = "LeakyRelu";
  std::string READ_SELECT = "ReadSelect";
  std::string ASCEND_QUANT = "AscendQuant";
  std::string ASCEND_DEQUANT = "AscendDequant";
  std::string ASCEND_REQUANT = "AscendRequant";
  std::string ASCEND_REQUANTS16 = "AscendRequantS16";
  std::string ASCEND_DEQUANTS16 = "AscendDequantS16";
  std::string CONV_ATTR_NAME_STRIDES = "strides";
  std::string CONV_ATTR_NAME_PADS = "pads";
  std::string CONV_ATTR_NAME_DILATIONS = "dilations";
  std::set<std::string> node_types_in_path = {CONV2D, ELTWISE, RELU, LEAKY_RELU,
                                              ASCEND_QUANT, ASCEND_DEQUANT, ASCEND_REQUANT,
                                              ASCEND_REQUANTS16, ASCEND_DEQUANTS16};

  // unordered sets maintain the nodes whose shapes should be changed.
  // the input should be change from (2H,2W) to (H,W).
  unordered_set<ge::NodePtr> nodes_input_reduced;
  // the output should be change from (2H,2W) to (H,W).
  unordered_set<ge::NodePtr> nodes_output_reduced;
  // both input and output should be changed.
  unordered_set<ge::NodePtr> nodes_input_and_output_reduced;

  size_t conv_attrs_size = 8;
  // k, stride, pad, dilation
  vector<vector<int64_t>> required_conv_attrs = {{3, 3, 1, 1, 1, 1, 1, 1},
                                                 {5, 5, 1, 1, 2, 2, 1, 1}};

  vector<int64_t> required_child_conv_attr = {1, 1, 2, 2, 0, 0, 1, 1};
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_STRIDE_HOIST_PASS_H_
