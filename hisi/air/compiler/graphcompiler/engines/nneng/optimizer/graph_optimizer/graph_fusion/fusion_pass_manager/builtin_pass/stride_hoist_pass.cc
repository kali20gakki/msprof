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

#include "stride_hoist_pass.h"
#include <memory>

namespace fe {

static const size_t DEFAULT_PATH_NUM = 2;
static const size_t THIRD_INDEX = 2;
static const size_t FOURTH_INDEX = 3;
static const int STRIDE_INDEX = 2;
vector<FusionPattern *> StrideHoistingPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FusionPattern *pattern_elt_quant = new (std::nothrow) FusionPattern("StrideHoist");
  FE_CHECK(pattern_elt_quant == nullptr, REPORT_FE_ERROR("[GraphOpt][StrideHoist][DefPtn] new an object failed."),
           return patterns);

  pattern_elt_quant->AddOpDesc("Eltwise", {"Eltwise"})
      .AddOpDesc("LRelu", {"LeakyRelu", "Relu"})
      .AddOpDesc("Quant", {"AscendQuant"})
      .SetInputs("LRelu", {"Eltwise"})
      .SetInputs("Quant", {"LRelu"})
      .SetOutput("Quant");
  patterns.push_back(pattern_elt_quant);

  FusionPattern *pattern_elt = new (std::nothrow) FusionPattern("StrideHoist");
  FE_CHECK(pattern_elt == nullptr, REPORT_FE_ERROR("[GraphOpt][StrideHoist][DefPtn] new an object failed."),
           return patterns);

  pattern_elt->AddOpDesc("Eltwise", {"Eltwise"})
      .AddOpDesc("LRelu", {"LeakyRelu", "Relu"})
      .SetInputs("LRelu", {"Eltwise"})
      .SetOutput("LRelu");
  patterns.push_back(pattern_elt);

  FusionPattern *pattern_conv_quant = new (std::nothrow) FusionPattern("StrideHoist");
  FE_CHECK(pattern_conv_quant == nullptr, REPORT_FE_ERROR("[GraphOpt][StrideHoist][DefPtn] new an object failed."),
           return patterns);

  pattern_conv_quant->AddOpDesc("Conv3x3", {"Conv2D"})
      .AddOpDesc("Dequant", {"AscendDequant"})  // relu_flag: 1
      .AddOpDesc("Quant", {"AscendQuant"})
      .SetInputs("Dequant", {"Conv3x3"})
      .SetInputs("Quant", {"Dequant"})
      .SetOutput("Quant");
  patterns.push_back(pattern_conv_quant);

  FusionPattern *pattern_conv = new (std::nothrow) FusionPattern("StrideHoist");
  FE_CHECK(pattern_conv == nullptr, REPORT_FE_ERROR("[GraphOpt][StrideHoist][DefPtn] new an object failed."),
           return patterns);

  pattern_conv->AddOpDesc("Conv3x3", {"Conv2D"})
      .AddOpDesc("LRelu", {"LeakyRelu", "Relu"})
      .SetInputs("LRelu", {"Conv3x3"})
      .SetOutput("LRelu");
  patterns.push_back(pattern_conv);

  FusionPattern *pattern_requants16 = new (std::nothrow) FusionPattern("StrideHoist");
  FE_CHECK(pattern_requants16 == nullptr, REPORT_FE_ERROR("[GraphOpt][StrideHoist][DefPtn] new an object failed."),
           return patterns);

  pattern_requants16->AddOpDesc("requants16", {ASCEND_REQUANTS16}).SetOutput("requants16");
  patterns.push_back(pattern_requants16);

  return patterns;
}

ge::NodePtr StrideHoistingPass::GetLowestCommonAncestor(ge::NodePtr node) {
  ge::NodePtr ancestor = nullptr;  // lowest common ancestor of in_nodes
  unordered_set<ge::NodePtr> parent_nodes;
  queue<ge::NodePtr> que;
  que.push(node);
  ge::NodePtr curr_node;
  while (!que.empty() && ancestor == nullptr) {
    curr_node = que.front();
    que.pop();
    ge::Node::Vistor<ge::NodePtr> in_nodes = curr_node->GetInDataNodes();
    for (auto in_node : in_nodes) {
      if (in_node == nullptr || in_node->GetName() == "" ||
          node_types_in_path.find(in_node->GetType()) == node_types_in_path.end()) {
        continue;
      }
      if (parent_nodes.find(in_node) == parent_nodes.end()) {
        parent_nodes.insert(in_node);
        que.push(in_node);
      } else {
        ancestor = in_node;
        FE_LOGI("We find the ancestor: %s", ancestor->GetName().c_str());
        break;
      }
    }
  }

  if (ancestor == nullptr) {
    FE_LOGI("Cannot find ancestor. Maybe a strange structure.");
    return nullptr;
  }
  ge::Node::Vistor<ge::NodePtr> out_nodes = ancestor->GetOutDataNodes();
  /**
   * if node type is AscendRequantS16, it has three inputs and two outputs, not equal
   * if node type is Eltwise, inputs num is equal to outputs num
   */
  if ((node->GetType() == ELTWISE && out_nodes.size() != node->GetInDataNodes().size()) ||
      (node->GetType() == ASCEND_REQUANTS16 && out_nodes.size() != node->GetInDataNodes().size() - 1)) {
    FE_LOGI("The found ancester is not valid. Maybe a strange structure.");
    return nullptr;
  }
  return ancestor;
}

/**
 * @brief       first find the lowest common ancestor of the multiple inputs
 *              of node. return the paths from this ancestor to node.
 * @param[in]   graph, node. node should have multiple inputs.
 * @param[out]  void
 * @return      vector<vector<NodePtr>>: paths from the lowest common ancester
 *              of inputs of node to node
 */
vector<vector<ge::NodePtr>> StrideHoistingPass::GetPathsFromLowestCommonAncestor(ge::NodePtr node) {
  vector<vector<ge::NodePtr>> paths;
  if (node->GetType() != ELTWISE && node->GetType() != ASCEND_REQUANTS16) {
    FE_LOGI("node type should be two-input Eltwise or AscendRequantS16");
    return paths;
  }
  ge::NodePtr ancestor = GetLowestCommonAncestor(node);
  if (ancestor == nullptr) {
    FE_LOGW("Cannot find ancestor. Maybe a strange structure.");
    return paths;
  }
  ge::Node::Vistor<ge::NodePtr> out_nodes = ancestor->GetOutDataNodes();
  // Each path has at least two elements, i.e., ancestor and node.
  FE_LOGI("Get paths from %s to %s.", ancestor->GetName().c_str(), node->GetName().c_str());
  for (auto out_node : out_nodes) {
    uint32_t depth = 0;
    vector<ge::NodePtr> path;
    path.push_back(ancestor);
    depth++;
    ge::NodePtr curr_node = out_node;
    path.push_back(curr_node);
    depth++;
    while (curr_node != node) {
      if (depth > 10) {
        FE_LOGW("Current path is two long, or ancestor is wrong.");
        paths.clear();
        return paths;
      }
      ge::Node::Vistor<ge::NodePtr> tmp_nodes = curr_node->GetOutDataNodes();
      if (tmp_nodes.size() != 1) {
        FE_LOGW("Node %s has zero or multiple outputs", curr_node->GetName().c_str());
        paths.clear();
        return paths;
      }
      curr_node = tmp_nodes.at(0);
      path.push_back(curr_node);
      depth++;
    }
    paths.push_back(path);
  }
  return paths;
}

Status StrideHoistingPass::GetHWDim(const ge::Format &format, vector<size_t> &dims) const {
  size_t h_dim = 0;
  size_t w_dim = 0;
  if (format == ge::FORMAT_NCHW) {
    h_dim = 2;  // ge::NCHW_DIM_H;
    w_dim = 3;  // ge::NCHW_DIM_W;
  } else if (format == ge::FORMAT_NHWC) {
    h_dim = 1;  // ge::NHWC_DIM_H;
    w_dim = 2;  // ge::NHWC_DIM_W;
  } else if (format == ge::FORMAT_HWCN) {
    h_dim = 0;
    w_dim = 1;
  } else if (format == ge::FORMAT_NC1HWC0) {
    h_dim = 2;
    w_dim = 3;
  } else {
    return FAILED;
  }
  dims = {h_dim, w_dim};
  return SUCCESS;
}

/**
 * @ingroup fe
 * @brief       As the function name stated.
 * @param[in]   shape
 * @param[out]  shape, havled
 * @return      void
 */
Status StrideHoistingPass::HalveShape(ge::GeShape &shape, const ge::Format format) const {
  vector<size_t> h_w_dim;
  FE_CHECK(GetHWDim(format, h_w_dim) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][HalveShp] Get H and W dim failed"),
           return FAILED);
  FE_CHECK(h_w_dim.size() != 2, REPORT_FE_ERROR("[GraphOpt][StrideHoist][HalveShp] Get H and W dim failed"),
           return FAILED);
  int64_t h = shape.GetDim(h_w_dim[0]);
  shape.SetDim(h_w_dim[0], (h + 1) / 2);  // 7 -> 4, 8 -> 4
  int64_t w = shape.GetDim(h_w_dim[1]);
  shape.SetDim(h_w_dim[1], (w + 1) / 2);
  return SUCCESS;
}

/**
 * @ingroup fe
 * @brief       As the function name stated.
 * @param[in]   node:ReadSelect op. parentt_tensor: input tensor to node.
 * @param[out]  node
 * @return      void
 */
Status StrideHoistingPass::SetInputOutputShapeOfReadSelectOp(ge::NodePtr node, ge::GeTensorDesc &parent_tensor) {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  ge::GeTensorDesc input_tensor = op_desc->GetInputDesc(0);
  ge::GeTensorDesc output_tensor = op_desc->GetOutputDesc(0);

  ge::GeShape origin_shape = parent_tensor.GetOriginShape();
  input_tensor.SetOriginShape(origin_shape);
  FE_CHECK(HalveShape(origin_shape, parent_tensor.GetOriginFormat()) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][SetInOutShp] Halve shape failed"), return FAILED);
  output_tensor.SetOriginShape(origin_shape);

  ge::GeShape shape = parent_tensor.GetShape();
  input_tensor.SetShape(shape);
  FE_CHECK(HalveShape(shape, static_cast<ge::Format>(ge::GetPrimaryFormat(parent_tensor.GetFormat()))) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][SetInOutShp] Halve shape failed"), return FAILED);
  output_tensor.SetShape(shape);

  input_tensor.SetOriginFormat(parent_tensor.GetOriginFormat());
  input_tensor.SetFormat(parent_tensor.GetFormat());
  input_tensor.SetDataType(parent_tensor.GetDataType());
  input_tensor.SetOriginDataType(parent_tensor.GetOriginDataType());
  output_tensor.SetOriginFormat(parent_tensor.GetOriginFormat());
  output_tensor.SetFormat(parent_tensor.GetFormat());
  output_tensor.SetDataType(parent_tensor.GetDataType());
  output_tensor.SetOriginDataType(parent_tensor.GetOriginDataType());

  if (op_desc->UpdateInputDesc(0, input_tensor) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][SetInOutShp] UpdateInputDesc of read_select op: %s failed!",
                    node->GetName().c_str());
    return FAILED;
  }
  if (op_desc->UpdateOutputDesc(0, output_tensor) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][SetInOutShp] UpdateOutputDesc of read_select op: %s failed!",
                    node->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status StrideHoistingPass::ConnectReadSelectOp(ge::NodePtr src_node, ge::NodePtr dst_node, ge::NodePtr rs_node) {
  int out_idx = -1;
  int in_idx = -1;
  ge::Node::Vistor<ge::OutDataAnchorPtr> out_anchors = src_node->GetAllOutDataAnchors();
  for (size_t i = 0; i < out_anchors.size(); ++i) {
    auto out_anc = out_anchors.at(i);
    ge::OutDataAnchor::Vistor<ge::InDataAnchorPtr> in_anchors = out_anc->GetPeerInDataAnchors();
    for (auto in_anc : in_anchors) {
      if (in_anc == nullptr) {
        continue;
      }
      ge::NodePtr child_node = in_anc->GetOwnerNode();
      if (child_node->GetName() == dst_node->GetName()) {
        out_idx = i;
        in_idx = in_anc->GetIdx();
      }
    }
  }
  FE_LOGD("ReadSelect op is connected to the %d-th output of srcNode and %d-th input of dst_node", out_idx, in_idx);

  if (ge::GraphUtils::RemoveEdge(src_node->GetOutDataAnchor(out_idx), dst_node->GetInDataAnchor(in_idx)) !=
      ge::GRAPH_SUCCESS) {
    FE_LOGW("Remove edge from src to dst op failed.");
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(src_node->GetOutDataAnchor(out_idx), rs_node->GetInDataAnchor(0)) != ge::GRAPH_SUCCESS) {
    FE_LOGW("Add edge from src to rs op failed.");
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(rs_node->GetOutDataAnchor(0), dst_node->GetInDataAnchor(in_idx)) != ge::GRAPH_SUCCESS) {
    FE_LOGW("Add edge from rs to dst op failed.");
    return FAILED;
  }

  ge::GeTensorDesc parent_tensor = src_node->GetOpDesc()->GetOutputDesc(out_idx);
  if (SetInputOutputShapeOfReadSelectOp(rs_node, parent_tensor) != SUCCESS) {
    FE_LOGW("Set Input and Output of rs node failed");
    return FAILED;
  }

  return SUCCESS;
}

ge::NodePtr StrideHoistingPass::InsertReadSelectOp(ge::ComputeGraph &graph, vector<ge::NodePtr> &path) {
  /* path: vector of size 2. We wil insert a ReadSelect op between the two nodes
  idx: index of the ReadSelect op to be inserted in the graph. */
  if (path.size() != 2) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][InsReadSelOp] path should be size 2 to insert read_select op");
    return nullptr;
  }
  ge::NodePtr src_node = path[0];
  ge::NodePtr dst_node = path[1];

  string node_name = dst_node->GetName() + "_read_select";
  FE_LOGD("Insert ReadSelect op: %s between src_node %s and dst_node %s.", node_name.c_str(),
          src_node->GetName().c_str(), dst_node->GetName().c_str());
  ge::GeTensorDescPtr tensor_desc = nullptr;
  FE_MAKE_SHARED(tensor_desc = std::make_shared<ge::GeTensorDesc>(), return nullptr);
  FE_CHECK(tensor_desc == nullptr, REPORT_FE_ERROR("[GraphOpt][StrideHoist][InsReadSelOp] Tensor desc is nullptr."),
           return nullptr);
  ge::OpDescPtr op_desc = nullptr;
  FE_MAKE_SHARED(op_desc = std::make_shared<ge::OpDesc>(node_name, READ_SELECT), return nullptr);
  FE_CHECK(op_desc == nullptr,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][InsReadSelOp] OpDesc of ReadSelect is nullptr."),
           return nullptr);
  if (op_desc->AddInputDesc(tensor_desc->Clone()) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][InsReadSelOp] Add input desc to read_select op failed");
    return nullptr;
  }
  if (op_desc->AddOutputDesc(tensor_desc->Clone()) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][InsReadSelOp] Add output desc to read_select op failed");
    return nullptr;
  }
  graph.AddNode(op_desc);
  ge::NodePtr rs_node = graph.FindNode(node_name);
  FE_CHECK(rs_node == nullptr, REPORT_FE_ERROR("[GraphOpt][StrideHoist][InsReadSelOp] Add read_select node failed"),
           return nullptr);
  op_desc = rs_node->GetOpDesc();
  std::vector<std::string> datadump_origin_op_names;
  ge::AttrUtils::SetListStr(op_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, datadump_origin_op_names);

  if (ConnectReadSelectOp(src_node, dst_node, rs_node) != SUCCESS) {
    FE_LOGW("Connect read_delect op to src and dst failed.");
    return nullptr;
  }

  std::string SELECT_STRIDE_LIST = "stride_list";
  ge::AttrUtils::SetListInt(op_desc, SELECT_STRIDE_LIST, {1, 1, 2, 2, 1});

  return rs_node;
}

Status StrideHoistingPass::ReduceInput(unordered_set<ge::NodePtr> nodes) {
  for (auto node : nodes) {
    if (node != nullptr) {
      FE_LOGD("change the input to half of node: %s ", node->GetName().c_str());
      ge::OpDescPtr node_opdesc = node->GetOpDesc();
      vector<int64_t> input_idx = {0};
      if (node->GetType() == ELTWISE) {
        input_idx.push_back(1);
      } else if (node->GetType() == ASCEND_REQUANTS16) {
        input_idx.push_back(static_cast<int64_t>(THIRD_INDEX));
      }
      for (auto idx : input_idx) {
        ge::GeTensorDesc tensor = node_opdesc->GetInputDesc(idx);

        ge::GeShape origin_shape = tensor.GetOriginShape();
        FE_CHECK(HalveShape(origin_shape, tensor.GetOriginFormat()) != SUCCESS,
                 REPORT_FE_ERROR("[GraphOpt][StrideHoist][ReduceIn] Halve shape failed"), return FAILED);
        tensor.SetOriginShape(origin_shape);

        ge::GeShape shape = tensor.GetShape();
        FE_CHECK(HalveShape(shape, static_cast<ge::Format>(ge::GetPrimaryFormat(tensor.GetFormat()))) != SUCCESS,
                 REPORT_FE_ERROR("[GraphOpt][StrideHoist][ReduceIn] Halve shape failed"), return FAILED);
        tensor.SetShape(shape);

        node_opdesc->UpdateInputDesc(idx, tensor);
      }
    }
  }
  return SUCCESS;
}

Status StrideHoistingPass::ReduceOutput(unordered_set<ge::NodePtr> nodes) const {
  for (auto node : nodes) {
    if (node != nullptr) {
      FE_LOGD("change the output to half of node: %s", node->GetName().c_str());
      ge::OpDescPtr node_opdesc = node->GetOpDesc();
      // !!! hard code here: only one output
      ge::GeTensorDesc tensor = node_opdesc->GetOutputDesc(0);

      ge::GeShape origin_shape = tensor.GetOriginShape();
      FE_CHECK(HalveShape(origin_shape, tensor.GetOriginFormat()) != SUCCESS,
               REPORT_FE_ERROR("[GraphOpt][StrideHoist][ReduceOut] Halve shape failed"), return FAILED);
      tensor.SetOriginShape(origin_shape);

      ge::GeShape shape = tensor.GetShape();
      FE_CHECK(HalveShape(shape, static_cast<ge::Format>(ge::GetPrimaryFormat(tensor.GetFormat()))) != SUCCESS,
               REPORT_FE_ERROR("[GraphOpt][StrideHoist][ReduceOut] Halve shape failed"), return FAILED);
      tensor.SetShape(shape);

      node_opdesc->UpdateOutputDesc(0, tensor);
    }
  }
  return SUCCESS;
}

Status StrideHoistingPass::ReduceInputAndOutput(unordered_set<ge::NodePtr> nodes) {
  FE_CHECK(ReduceInput(nodes) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][ReduceInOut] Reduce input shape failed"),
           return FAILED);
  FE_CHECK(ReduceOutput(nodes) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][ReduceInOut] Reduce output shape failed"),
           return FAILED);
  return SUCCESS;
}

vector<int64_t> StrideHoistingPass::GetConvAttrs(ge::NodePtr node) const {
  vector<int64_t> result;
  if (node->GetType() != CONV2D) {
    FE_LOGW(
        "GetConvAttr only work for CONV2D node, while input node"
        "%s has type %s",
        node->GetName().c_str(), node->GetType().c_str());
    return result;
  }
  ge::OpDescPtr op_desc = node->GetOpDesc();
  ge::Format format = static_cast<ge::Format>(ge::GetPrimaryFormat(op_desc->GetInputDesc(0).GetFormat()));
  vector<size_t> h_w_dim;
  FE_CHECK(GetHWDim(format, h_w_dim) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][GetConvAttr] Get H and W dim failed"),
           return result);
  FE_CHECK(h_w_dim.size() != 2, REPORT_FE_ERROR("[GraphOpt][StrideHoist][GetConvAttr] Get H and W dim failed"),
           return result);
  size_t h_dim = h_w_dim[0];
  size_t w_dim = h_w_dim[1];
  ge::GeTensorDesc filter = op_desc->GetInputDesc(1);
  ge::Format filter_format = static_cast<ge::Format>(ge::GetPrimaryFormat(filter.GetFormat()));
  vector<size_t> filter_h_w_dim;
  FE_CHECK(GetHWDim(filter_format, filter_h_w_dim) != SUCCESS, REPORT_FE_ERROR("[GraphOpt][StrideHoist][GetConvAttr] \
           Get H and W dim failed"), return result);
  FE_CHECK(filter_h_w_dim.size() != 2, REPORT_FE_ERROR("[GraphOpt][StrideHoist][GetConvAttr] Get H and W dim failed"),
           return result);
  size_t filter_h_dim = filter_h_w_dim[0];
  size_t filter_w_dim = filter_h_w_dim[1];
  ge::GeShape filter_shape = filter.GetOriginShape();
  if (filter_shape.GetDimNum() != 4) {
    FE_LOGW("Cannot get kernel size of node %s", node->GetName().c_str());
    return result;
  }

  vector<int64_t> stride;
  ge::AttrUtils::GetListInt(op_desc, CONV_ATTR_NAME_STRIDES, stride);
  if (stride.size() != 4) {
    FE_LOGW("Cannot get stride of node %s", node->GetName().c_str());
    return result;
  }

  vector<int64_t> pad;
  ge::AttrUtils::GetListInt(op_desc, CONV_ATTR_NAME_PADS, pad);
  if (pad.size() != 4) {
    FE_LOGW("Cannot get pad of node %s", node->GetName().c_str());
    return result;
  }

  vector<int64_t> dilation;
  ge::AttrUtils::GetListInt(op_desc, CONV_ATTR_NAME_DILATIONS, dilation);
  if (dilation.size() != 4) {
    FE_LOGW("Cannot get dilation of node %s", node->GetName().c_str());
    return result;
  }

  result = {filter_shape.GetDim(filter_h_dim),
            filter_shape.GetDim(filter_w_dim),
            stride[h_dim],
            stride[w_dim],
            pad[h_dim],
            pad[w_dim],
            dilation[h_dim],
            dilation[w_dim]};

  return result;
}

bool StrideHoistingPass::CheckChildren(ge::NodePtr node) const {
  FE_LOGD("Now chencking child node: %s, type: %s.", node->GetName().c_str(), node->GetType().c_str());
  ge::Node::Vistor<ge::NodePtr> out_nodes = node->GetOutDataNodes();

  for (auto out_node : out_nodes) {
    if (out_node->GetType() != CONV2D) {  // all the children must be conv2d op
      return false;
    }

    vector<int64_t> conv_attr = GetConvAttrs(out_node);
    if (conv_attr.size() != required_child_conv_attr.size()) {
      return false;
    }

    if (conv_attr == required_child_conv_attr) {
      continue;
    } else {
      return false;
    }
  }

  return true;
}

Status StrideHoistingPass::ChangeStride(ge::NodePtr node, int stride) const {
  FE_LOGD("change stride to %d of node %s", stride, node->GetName().c_str());
  if (node->GetType() != CONV2D) {
    FE_LOGW("ChangeStride only support conv2d node, while input node %s has type %s", node->GetName().c_str(),
            node->GetType().c_str());
    return FAILED;
  }
  ge::OpDescPtr op_desc = node->GetOpDesc();
  ge::Format format = static_cast<ge::Format>(ge::GetPrimaryFormat(op_desc->GetInputDesc(0).GetFormat()));
  std::vector<size_t> h_w_dim;
  FE_CHECK(GetHWDim(format, h_w_dim) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][ChgStride] Get H and W dim failed"),
           return FAILED);
  FE_CHECK(h_w_dim.size() != 2, REPORT_FE_ERROR("[GraphOpt][StrideHoist][ChgStride] Get H and W dim failed"),
           return FAILED);
  vector<int64_t> strides;
  ge::AttrUtils::GetListInt(op_desc, CONV_ATTR_NAME_STRIDES, strides);
  if (strides.size() == 4) {
    strides[h_w_dim[0]] = stride;
    strides[h_w_dim[1]] = stride;
  } else {
    FE_LOGW("Unsupported stride dims or get stride failed of node %s", node->GetName().c_str());
    return FAILED;
  }
  ge::AttrUtils::SetListInt(op_desc, CONV_ATTR_NAME_STRIDES, strides);
  return SUCCESS;
}

bool StrideHoistingPass::ConvExists(vector<vector<ge::NodePtr>> &paths) {
  // there exist a path which ends with a conv, to UBFuse read_select op
  bool conv_existing = false;
  for (auto path : paths) {
    size_t sz = path.size();
    if (sz > 2) {  // path[sz-1] is eltwise
      if ((path[sz - THIRD_INDEX]->GetType() == CONV2D) ||
          (path[sz - FOURTH_INDEX]->GetType() == CONV2D && (path[sz - THIRD_INDEX]->GetType() == ASCEND_DEQUANT ||
                                                            path[sz - THIRD_INDEX]->GetType() == ASCEND_DEQUANTS16))) {
        conv_existing = true;
        break;
      }
    }
  }
  return conv_existing;
}

bool StrideHoistingPass::AllPathsAreSimple(vector<vector<ge::NodePtr>> &paths) {
  // check if all paths are simple paths
  bool simple_flag = true;
  for (auto path : paths) {
    if (path.size() > 2) {
      for (size_t j = 1; j < path.size() - 1; ++j) {
        if (path[j]->GetOutDataNodes().size() != 1 || path[j]->GetType() == ELTWISE ||
            path[j]->GetType() == ASCEND_REQUANTS16) {
          FE_LOGW("The structure is too complex to handle. Return.");
          simple_flag = false;
          break;
        }
      }
    }
  }
  return simple_flag;
}

bool StrideHoistingPass::CheckFmapShapeEven(const ge::NodePtr &node) {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  ge::Format format = static_cast<ge::Format>(ge::GetPrimaryFormat(op_desc->GetInputDesc(0).GetFormat()));
  std::vector<size_t> h_w_dim;
  FE_CHECK(GetHWDim(format, h_w_dim) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][ChkFmapShpEven] Get H and W dim failed"),
           return false);
  FE_CHECK(h_w_dim.size() != 2, REPORT_FE_ERROR("[GraphOpt][StrideHoist][ChkFmapShpEven] Get H and W dim failed"),
           return FAILED);
  ge::GeTensorDescPtr tensor = op_desc->MutableInputDesc(0);
  auto shape = tensor->MutableShape();
  return (shape.GetDim(h_w_dim[0]) % 2 == 0 && shape.GetDim(h_w_dim[1]) % 2 == 0);
}

bool StrideHoistingPass::CheckPaths(vector<vector<ge::NodePtr>> &paths) {
  bool total_flag = true;
  bool sized_two_flag = false;  // if there is already a sized 2 path
  for (auto path : paths) {
    bool path_flag = false;
    if (path.size() < 2) {
      FE_LOGW("Not a valid path. At least two nodes on a path.");
      return false;
    } else if (path.size() == 2 && !sized_two_flag) {
      path_flag = true;  // will insert read_select op
      // inserting read_select in two paths is not acceptable.
      sized_two_flag = true;
    } else if (path.size() > 2) {  // there are nodes between ancestor and node.
      for (size_t j = path.size() - 2; j > 0; --j) {
        ge::NodePtr tmp_node = path[j];
        if (tmp_node->GetType() == CONV2D) {
          vector<int64_t> conv_attr = GetConvAttrs(tmp_node);
          if (conv_attr.size() != conv_attrs_size) {
            return false;
          }
          if (std::find(required_conv_attrs.begin(), required_conv_attrs.end(),
                        conv_attr) != required_conv_attrs.end()) {  // find requiared conv
            path_flag = CheckFmapShapeEven(tmp_node);
          }
        }
      }
    }
    total_flag = total_flag && path_flag;
  }

  if (sized_two_flag) {  // need to insert readselect op
    total_flag = total_flag && paths.size() == 2 && ConvExists(paths);
  }

  return total_flag && AllPathsAreSimple(paths);
}

Status StrideHoistingPass::ChangeGraphConvPath(vector<ge::NodePtr> &path) {
  for (size_t j = path.size() - 2; j > 0; --j) {
    ge::NodePtr tmp_node = path[j];
    if (tmp_node->GetType() == CONV2D) {
      vector<int64_t> conv_attr = GetConvAttrs(tmp_node);
      if (conv_attr.size() != conv_attrs_size) {
        return FAILED;
      }
      if (std::find(required_conv_attrs.begin(), required_conv_attrs.end(), conv_attr) != required_conv_attrs.end()) {
        // filter: (kw = kh = 3, stride = 1) ==> (kw = kh = 3, stride = 2)
        if (ChangeStride(tmp_node, 2) == FAILED) {
          FE_LOGW("Change stride failed");
          return FAILED;
        }
        nodes_output_reduced.insert(tmp_node);
        for (size_t k = j + 1; k < path.size(); ++k) {
          nodes_input_and_output_reduced.insert(path[k]);
        }
        break;
      }
    }
  }
  return SUCCESS;
}

Status StrideHoistingPass::ChangeGraph(ge::ComputeGraph &graph, vector<vector<ge::NodePtr>> paths) {
  FE_LOGD("Now change graph");
  ge::Node::Vistor<ge::NodePtr> out_nodes = node_has_children_k1_s2->GetOutDataNodes();
  for (auto out_node : out_nodes) {
    if (ChangeStride(out_node, 1) == FAILED) {
      FE_LOGD("Change stride failed");
      return FAILED;
    }
  }

  for (auto path : paths) {
    if (path.size() == 2) {  // This path is a skip connection.
      // we need to insert a skip-reading op to downsample the feature map
      ge::NodePtr rs_node = InsertReadSelectOp(graph, path);
      FE_CHECK(rs_node == nullptr, FE_LOGD("Insert ReadSelect op Failed"), return FAILED);
      nodes_input_and_output_reduced.insert(path[1]);
    } else if (path.size() > 2) {
      // there are nodes between ancestor and node. There is a 3x3 conv.
      if (ChangeGraphConvPath(path) != SUCCESS) {
        FE_LOGD("Change conv path failed");
        return FAILED;
      }
    }
  }

  return SUCCESS;
}

Status StrideHoistingPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) {
  vector<ge::NodePtr> nodes = GetNodesFromMapping(mapping);
  for (auto node : nodes) {
    if (node->GetType() == ASCEND_REQUANTS16) {
      return FusionRequantS16Case(graph, mapping);
    }
    if (node->GetType() == CONV2D) {
      return FusionConvCase(graph, mapping);
    }
  }

  return FusionEltwiseCase(graph, mapping);
}

Status StrideHoistingPass::FusionRequantS16Case(ge::ComputeGraph &graph, Mapping &mapping) {
  ge::NodePtr requants16 = GetNodeFromMapping("requants16", mapping);
  if (requants16 == nullptr) {
    FE_LOGW("Get requants16 node from mapping failed.");
    return NOT_CHANGED;
  }
  FE_LOGI("Enter StrideHoistingPass with node %s", requants16->GetName().c_str());

  // 1. Find the node which has all children being kw=kh=1 s=2 conv2d op
  if (!CheckChildren(requants16)) {
    return NOT_CHANGED;
  }
  FE_LOGI("Find node: %s has required children.", requants16->GetName().c_str());
  node_has_children_k1_s2 = requants16;
  nodes_input_reduced.clear();
  nodes_output_reduced.clear();
  nodes_input_and_output_reduced.clear();

  ge::Node::Vistor<ge::NodePtr> out_nodes = requants16->GetOutDataNodes();
  for (auto out_node : out_nodes) {
    nodes_input_reduced.insert(out_node);
  }

  // 3. Get the input paths to this requants16 op.
  // These paths should have a common starting node, the ancestor.
  vector<vector<ge::NodePtr>> paths = GetPathsFromLowestCommonAncestor(requants16);
  if (paths.size() < DEFAULT_PATH_NUM) {
    FE_LOGW("The paths should be more than 1");
    return NOT_CHANGED;
  }

  // 4. Check if all the paths meet the requirements.
  if (!CheckPaths(paths)) {
    return NOT_CHANGED;
  }

  // 5. Change the graph, including insert ReadSelect op, modifying strides.
  if (ChangeGraph(graph, paths) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusRequtS16Case] Change graph failed!");
    return FAILED;
  }

  // 6. Change the shapes of the affected nodes.
  FE_CHECK(ReduceInput(nodes_input_reduced) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusRequtS16Case] Reduce input shape failed"),
           return FAILED);
  FE_CHECK(ReduceOutput(nodes_output_reduced) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusRequtS16Case] Reduce output shape failed"),
           return FAILED);
  FE_CHECK(ReduceInputAndOutput(nodes_input_and_output_reduced) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusRequtS16Case] Reduce input and output shape failed"),
           return FAILED);

  if (graph.TopologicalSorting() != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusRequtS16Case] TopologicalSorting failed");
    return FAILED;
  }
  return SUCCESS;
}

Status StrideHoistingPass::FusionEltwiseCase(ge::ComputeGraph &graph, Mapping &mapping) {
  ge::NodePtr elt_node = GetNodeFromMapping("Eltwise", mapping);
  if (elt_node == nullptr) {
    FE_LOGW("Get elt_node from mapping failed.");
    return NOT_CHANGED;
  }
  FE_LOGI("Enter StrideHoistingPass with node %s", elt_node->GetName().c_str());

  std::string out_node_id = (mapping.size() == 3) ? "Quant" : "LRelu";
  ge::NodePtr node = GetNodeFromMapping(out_node_id, mapping);
  if (node == nullptr) {
    FE_LOGW("Get node: %s from mapping failed.", out_node_id.c_str());
    return NOT_CHANGED;
  }

  // 1. Find the node which has all children being kw=kh=1 s=2 conv2d op
  if (!CheckChildren(node)) {
    return NOT_CHANGED;
  }
  FE_LOGI("Find node: %s has required children.", node->GetName().c_str());
  node_has_children_k1_s2 = node;
  nodes_input_reduced.clear();
  nodes_output_reduced.clear();
  nodes_input_and_output_reduced.clear();

  ge::Node::Vistor<ge::NodePtr> out_nodes = node->GetOutDataNodes();
  for (auto out_node : out_nodes) {
    nodes_input_reduced.insert(out_node);
  }

  // 2. Going up to skip the relu op. Get the Conv2D op or ELtwise op.
  ge::NodePtr lrelu_node = GetNodeFromMapping("LRelu", mapping);
  if (lrelu_node == nullptr) {
    FE_LOGW("Get node: LRelu from mapping failed");
    return NOT_CHANGED;
  }
  nodes_input_and_output_reduced.insert(lrelu_node);
  if (mapping.size() == 3) {
    ge::NodePtr quant_node = GetNodeFromMapping("Quant", mapping);
    if (quant_node == nullptr) {
      FE_LOGW("Get node: Quant from mapping failed");
      return NOT_CHANGED;
    }
    nodes_input_and_output_reduced.insert(quant_node);
  }
  return CheckShapesAndTopological(graph, elt_node);
}

Status StrideHoistingPass::CheckShapesAndTopological(ge::ComputeGraph &graph, ge::NodePtr elt_node) {
  // 3. Get the input paths to this eltwise op.
  // These paths should have a common starting node, the ancestor.
  vector<vector<ge::NodePtr>> paths = GetPathsFromLowestCommonAncestor(elt_node);
  if (paths.size() < DEFAULT_PATH_NUM) {
    FE_LOGW("The paths should be more than 1");
    return NOT_CHANGED;
  }

  // 4. Check if all the paths meet the requirements.
  if (!CheckPaths(paths)) {
    return NOT_CHANGED;
  }

  // 5. Change the graph, including insert ReadSelect op, modifying strides.
  if (ChangeGraph(graph, paths) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusEltwiseCase] Change graph failed!");
    return FAILED;
  }
   // 6. Change the shapes of the affected nodes.
  FE_CHECK(ReduceInput(nodes_input_reduced) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusEltwiseCase] Reduce input shape failed"),
           return FAILED);
  FE_CHECK(ReduceOutput(nodes_output_reduced) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusEltwiseCase] Reduce output shape failed"),
           return FAILED);
  FE_CHECK(ReduceInputAndOutput(nodes_input_and_output_reduced) != SUCCESS,
           REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusEltwiseCase] Reduce input and output shape failed"),
           return FAILED);

  if (graph.TopologicalSorting() != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusEltwiseCase] TopologicalSorting failed");
    return FAILED;
  }
  return SUCCESS;
}

Status StrideHoistingPass::ReduceAllImpl(ge::ComputeGraph &graph, ge::NodePtr conv_node,
                                         ge::Node::Vistor<ge::NodePtr> out_nodes) {
  // Change the graph, modifying strides.
  if (ChangeStride(conv_node, STRIDE_INDEX) == FAILED) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusConvCase] Change stride failed");
    return FAILED;
  }
  for (auto out_node : out_nodes) {
    if (ChangeStride(out_node, 1) == FAILED) {
      REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusConvCase] Change stride failed");
      return FAILED;
    }
  }

  // 6. Change the shapes of the affected nodes.
  if (ReduceInput(nodes_input_reduced) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusConvCase] Reduce input shape failed");
    return FAILED;
  }

  if (ReduceOutput(nodes_output_reduced) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusConvCase] Reduce output shape failed");
    return FAILED;
  }

  if (ReduceInputAndOutput(nodes_input_and_output_reduced) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusConvCase] Reduce input and output shape failed");
    return FAILED;
  }

  if (graph.TopologicalSorting() != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][StrideHoist][FusConvCase] TopologicalSorting failed");
    return FAILED;
  }
  return SUCCESS;
}

Status StrideHoistingPass::FusionConvCase(ge::ComputeGraph &graph, Mapping &mapping) {
  ge::NodePtr conv_node = GetNodeFromMapping("Conv3x3", mapping);
  FE_CHECK(conv_node == nullptr, FE_LOGW("Get node: Conv3x3 from mapping failed"), return NOT_CHANGED);
  FE_LOGI("Enter StrideHoistingPass with node %s", conv_node->GetName().c_str());
  vector<int64_t> conv_attr = GetConvAttrs(conv_node);
  if (conv_attr.size() != conv_attrs_size ||
      std::find(required_conv_attrs.begin(), required_conv_attrs.end(), conv_attr) == required_conv_attrs.end()) {
    FE_LOGD("Parent conv node attr not valid");
    return NOT_CHANGED;
  }
  // conv3x3 -> lrelu, conv3x3 -> dequant(relu_falg = 1) -> quant
  std::string out_node_id = (mapping.size() == 3) ? "Quant" : "LRelu";
  ge::NodePtr node = GetNodeFromMapping(out_node_id, mapping);
  FE_CHECK(node == nullptr, FE_LOGW("Get node: %s from mapping failed", out_node_id.c_str()), return NOT_CHANGED);
  // 1. Find the node which has all children being kw=kh=1 s=2 conv2d op
  if (!CheckChildren(node)) {
    FE_LOGD("The children not qualified. Not changed. Return.");
    return NOT_CHANGED;
  }
  FE_LOGI("Find node: %s has required children.", node->GetName().c_str());
  node_has_children_k1_s2 = node;
  nodes_input_reduced.clear();
  nodes_output_reduced.clear();
  nodes_input_and_output_reduced.clear();
  nodes_output_reduced.insert(conv_node);
  ge::Node::Vistor<ge::NodePtr> out_nodes = node->GetOutDataNodes();
  for (auto out_node : out_nodes) {
    nodes_input_reduced.insert(out_node);
  }
  if (mapping.size() == 2) {
    ge::NodePtr lrelu_node = GetNodeFromMapping("LRelu", mapping);
    FE_CHECK(lrelu_node == nullptr, FE_LOGW("Get node: LRelu from mapping failed"), return NOT_CHANGED);
    nodes_input_and_output_reduced.insert(lrelu_node);
  } else if (mapping.size() == 3) {
    ge::NodePtr dequant_node = GetNodeFromMapping("Dequant", mapping);
    FE_CHECK(dequant_node == nullptr, FE_LOGW("Get node: Dequant from mapping failed"), return NOT_CHANGED);
    nodes_input_and_output_reduced.insert(dequant_node);
    ge::NodePtr quant_node = GetNodeFromMapping("Quant", mapping);
    FE_CHECK(quant_node == nullptr, FE_LOGW("Get node: Quant from mapping failed"), return NOT_CHANGED);
    nodes_input_and_output_reduced.insert(quant_node);
  }
  if (ReduceAllImpl(graph, conv_node, out_nodes) != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

}  // namespace fe
