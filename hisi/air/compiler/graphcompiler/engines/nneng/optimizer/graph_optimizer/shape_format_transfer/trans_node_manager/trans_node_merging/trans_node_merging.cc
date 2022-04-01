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

#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_merging/trans_node_merging.h"
#include <memory>
#include <stack>
#include <string>
#include "common/fe_utils.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {

static const std::map<ge::DataType, ge::DataType> precision_loss_map = {{ge::DT_FLOAT, ge::DT_BOOL},
                                                                        {ge::DT_INT64, ge::DT_BOOL}};

TransNodeMerging::TransNodeMerging() {}

TransNodeMerging::~TransNodeMerging() {}

bool IsTransOpType(const string &op_type) {
  if (op_type == TRANSDATA || op_type == CAST || op_type == TRANSPOSE || op_type == RESHAPE || op_type == REFORMAT) {
    return true;
  }
  return false;
}

bool CheckAxisC(ge::DataType original_data_type, int64_t multiply_result1, int64_t multiply_result2, int64_t dims1,
                int64_t dims2) {
  int64_t c0 = GetC0(original_data_type);

  /* If size_of_dims1 == axis_c_unsigned, this branch will not be reached.
   * This means if thr origianl_format is not valid, GetAxisIndexByFormat
   * will return -1, and we just check the product of all elements. */
  if (multiply_result1 != multiply_result2) {
    return false;
  } else {
    if (dims1 != dims2 && ((dims1 % c0) || (dims2 % c0))) {
      /* If axis C is not equal, if any of the dims cannot be divided by
       * c0, return false; */
      return false;
    }
  }
  return true;
}

/* Equivalent means:
 * 1. Two dims are completely the same.
 * 2. Two dims are same in memory. One may be the reshape version
 * of the other. For example:
 * dims1  8, 9, 28, 28, 32
 * dims2  72, 1, 28, 28, 32
 * Same in memory means:
 * 1. Dims size must be the same.
 * 2. Product of two dims are the same.
 * 3. Each dimension of dims and dims can be divided by each other, either
 * dims1[i] mod dims2[i] is equal to 0 or
 * dims2[i] mod dims1[i] is equal to 0.
 *
 *
 * For NCHW, NHWC and HWCN need to ADDTIONAL check the product of all axis at
 * the left of C and C itself is the same and the axis c can be divided by
 * C0. If the axis C is the same for both dims, we will not check this.
 * For example:
 * For NCHW, we need to check whether the product the dim N and dim C is the
 * same.
 * For NHWC, we just check all.
 * For HWCN, we need to addtionally check the product of H * W * C. */
bool CheckTwoDimsEquivalent(const std::vector<int64_t> &dims1, const std::vector<int64_t> &dims2,
                            ge::Format original_format, ge::DataType original_data_type, string op_type) {
  if (dims1 == dims2) {
    return true;
  } else if (op_type == TRANSDATA) {
    FE_LOGD("Check two Transdata equivalent. Dims1 = %s, dims2 = %s", StringUtils::IntegerVecToString(dims1).c_str(),
            StringUtils::IntegerVecToString(dims2).c_str());
    FE_LOGD("Original format = %u and original dtype = %u", original_format, original_data_type);
    size_t size_of_dims1 = dims1.size();
    size_t size_of_dims2 = dims2.size();
    size_t axis_c_unsigned;
    int64_t axis_c = GetAxisIndexByFormat(original_format, C_AXIS_NAME);
    if (axis_c >= static_cast<int64_t>(size_of_dims1) || axis_c < 0) {
      axis_c_unsigned = size_of_dims1;
    } else {
      axis_c_unsigned = static_cast<size_t>(axis_c);
    }
    if (size_of_dims1 == size_of_dims2) {
      int64_t multiply_result1 = 1;
      int64_t multiply_result2 = 1;
      for (size_t i = 0; i < size_of_dims1; i++) {
        if (dims1[i] == 0 || dims2[i] == 0) {
          /* if one tensor contains a dim 0, and two tensor's shape are not the
           * same, for simplicity, we just consider they are not the same tensor.
           * But they might be the same, for example:
           * {2,0,3,4,6} and {0,7,8,9,5} is the same. */
          return false;
        }

        multiply_result1 *= dims1[i];
        multiply_result2 *= dims2[i];
        if (multiply_result1 % multiply_result2 != 0 && multiply_result2 % multiply_result1 != 0) {
          return false;
        }
        if ((i == axis_c_unsigned) &&
            !CheckAxisC(original_data_type, multiply_result1, multiply_result2, dims1[i], dims2[i])) {
          return false;
        }
      }
      FE_LOGD("%ld %ld", multiply_result1, multiply_result2);
      return multiply_result1 == multiply_result2;
    }
  }
  return false;
}

bool CheckTwoTransOpDescMergable(const ge::NodePtr &src_node, const ge::OpDescPtr &dst_op_desc) {
  // dst_op_desc -> src_node
  FE_CHECK(src_node == nullptr || dst_op_desc == nullptr, FE_LOGD("Src node or dst op desc is null."), return false);

  ge::OpDescPtr src_op_desc = src_node->GetOpDesc();
  FE_CHECK(src_op_desc == nullptr, FE_LOGD("Source op desc is null."), return false);

  string src_op_desc_type = src_op_desc->GetType();
  string dst_op_desc_type = dst_op_desc->GetType();
  int flag =
      (src_op_desc_type != dst_op_desc_type || !IsTransOpType(src_op_desc_type) || !IsTransOpType(dst_op_desc_type));
  if (flag) {
    return false;
  }

  ge::ConstGeTensorDescPtr src_out_tensor_desc_ptr = src_op_desc->GetOutputDescPtr(0);
  ge::ConstGeTensorDescPtr src_in_tensor_desc_ptr = src_op_desc->GetInputDescPtr(0);
  ge::ConstGeTensorDescPtr dst_in_tensor_desc_ptr = dst_op_desc->GetInputDescPtr(0);
  ge::ConstGeTensorDescPtr dst_out_tensor_desc_ptr = dst_op_desc->GetOutputDescPtr(0);
  FE_CHECK(src_out_tensor_desc_ptr == nullptr || src_in_tensor_desc_ptr == nullptr ||
               dst_in_tensor_desc_ptr == nullptr || dst_out_tensor_desc_ptr == nullptr,
           FE_LOGD("Tensor_desc_ptr is null."), return false);

  if (dst_op_desc_type == CAST) {
    auto iter = precision_loss_map.find(dst_in_tensor_desc_ptr->GetDataType());
    if (iter != precision_loss_map.end() && iter->second == dst_out_tensor_desc_ptr->GetDataType()) {
      return false;
    }
  }

  ge::GeShape src_out_shape = src_out_tensor_desc_ptr->GetShape();
  ge::GeShape src_in_shape = src_in_tensor_desc_ptr->GetShape();

  ge::GeShape dst_in_shape = dst_in_tensor_desc_ptr->GetShape();
  ge::GeShape dst_out_shape = dst_out_tensor_desc_ptr->GetShape();

  auto src_in_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(src_in_tensor_desc_ptr->GetFormat()));
  auto src_out_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(src_out_tensor_desc_ptr->GetFormat()));
  auto dst_in_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(dst_in_tensor_desc_ptr->GetFormat()));
  auto dst_out_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(dst_out_tensor_desc_ptr->GetFormat()));
  if (src_in_primary_format == dst_out_primary_format &&
      src_in_tensor_desc_ptr->GetDataType() == dst_out_tensor_desc_ptr->GetDataType() &&
      CheckTwoDimsEquivalent(src_in_shape.GetDims(), dst_out_shape.GetDims(), dst_out_primary_format,
                             src_in_tensor_desc_ptr->GetDataType(), src_op_desc_type) &&
      src_out_primary_format == dst_in_primary_format &&
      src_out_tensor_desc_ptr->GetDataType() == dst_in_tensor_desc_ptr->GetDataType() &&
      CheckTwoDimsEquivalent(src_out_shape.GetDims(), dst_in_shape.GetDims(), dst_in_primary_format,
                             src_out_tensor_desc_ptr->GetDataType(), src_op_desc_type)) {
    return true;
  } else {
    return false;
  }
}

void UpdateAttrNames(ge::OpDescPtr dst_op_desc, ge::OpDescPtr old_src_op_desc, ge::OpDescPtr new_src_op_desc) {
  auto src_name_of_dst_node = dst_op_desc->GetSrcName();
  auto input_name_of_dst_node = dst_op_desc->GetInputName();

  vector<string> new_src_name;
  vector<string> new_input_name;

  auto old_src_name = old_src_op_desc->GetName();
  auto new_name = new_src_op_desc->GetName();
  if (src_name_of_dst_node.size() == 0) {
    new_src_name.push_back(new_name);
  } else {
    for (auto iter = src_name_of_dst_node.begin(); iter != src_name_of_dst_node.end(); iter++) {
      if (*iter != old_src_name) {
        new_src_name.push_back(*iter);
      } else {
        new_src_name.push_back(new_name);
      }
    }
  }

  if (input_name_of_dst_node.size() == 0) {
    new_input_name.push_back(new_name);
  } else {
    for (auto iter = input_name_of_dst_node.begin(); iter != input_name_of_dst_node.end(); iter++) {
      if (*iter != old_src_name) {
        new_input_name.push_back(*iter);
      } else {
        new_input_name.push_back(new_name);
      }
    }
  }

  dst_op_desc->SetSrcName(new_src_name);
  dst_op_desc->SetInputName(new_input_name);
}

Status TransNodeMerging::MergeAllTransOps(ge::ComputeGraph &fused_graph) {
  for (ge::NodePtr &node : fused_graph.GetDirectNode()) {
    if (node == nullptr || node->GetOpDesc() == nullptr) {
      continue;
    }

    ge::OpDescPtr op_desc = node->GetOpDesc();
    string op_desc_type = op_desc->GetType();

    if (!IsTransOpType(op_desc_type)) {
      FE_LOGD("Merge trans op from normal op(name [%s] type [%s]) backwards", op_desc->GetName().c_str(),
              op_desc->GetType().c_str());
      for (auto &in_anchor : node->GetAllInDataAnchors()) {
        if (in_anchor == nullptr || in_anchor->GetPeerOutAnchor() == nullptr ||
            in_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr) {
          continue;
        }
        ge::OutDataAnchorPtr out_anchor = in_anchor->GetPeerOutAnchor();
        ge::NodePtr src_node = out_anchor->GetOwnerNode();

        /* Every Time we merge all trans nodes between two non-trans nodes. */
        Status ret = MergeTransOpBetweenTwoNormalOp(fused_graph, src_node, op_desc_type, op_desc->GetName(), in_anchor);
        if (ret == FAILED) {
          FE_LOGD("Merging not success. After merging, the graph [%s] is as below:", fused_graph.GetName().c_str());
          for (auto &node_tmp : fused_graph.GetDirectNode()) {
            FE_LOGD("Node named [%s]", node_tmp->GetName().c_str());
          }
          return FAILED;
        }
      }
    }
  }
  FE_LOGD("Merging successfully. After merging, the graph [%s] is as below:", fused_graph.GetName().c_str());
  return SUCCESS;
}

Status RemoveNodeAndEdges(const BasicInfoForRemovingNode &info, bool father_node_nullflag,
                          std::stack<uint32_t> &in_anchor_index_stack, ge::ComputeGraph &fused_graph) {
  string node_name = info.node->GetName();
  /* Third: Remove node itself from graph. */
  /* If we encounter multiple peer in anchors, we cannot remove the node. So
   * we remove the anchor relation between node and it's father, and add a
   * edge between node and it's father's father. */
  if (info.dst_in_anchor_size_of_node == 1) {
    if (ge::GraphUtils::IsolateNode(info.node, {0}) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][RmNdEg] [3]:Failed to remove edge from [%s] to [%s].",
                      info.src_op_desc->GetName().c_str(), node_name.c_str());
      return FAILED;
    }
    FE_LOGD("[Trans][Merge] Isolate Node %s.", node_name.c_str());

    if (ge::GraphUtils::RemoveNodeWithoutRelink(fused_graph.shared_from_this(), info.node) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][ShapeTrans][RmNdEg] Failed to remove node (name[%s] type[%s]) from \
                      graph (name[%s]).",
          node_name.c_str(), info.node->GetType().c_str(), fused_graph.GetName().c_str());
      return FAILED;
    }
    uint32_t new_in_anchor_index = info.src_anchor->GetPeerInDataAnchors().size() - 1;
    in_anchor_index_stack.push(new_in_anchor_index);
    FE_LOGD("[Trans][Merge] Remove node (name [%s] type [%s])!", node_name.c_str(), info.node->GetType().c_str());
  } else {
    if (ge::GraphUtils::RemoveEdge(info.out_anchor_of_node, info.dst_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][RmNdEg] [1]:Failed to remove edge from [%s] to [%s].",
                      node_name.c_str(), info.dst_node->GetName().c_str());
      return FAILED;
    }
    FE_LOGD("[Trans][Merge] Remove edge from [%s] to [%s].", node_name.c_str(), info.dst_node->GetName().c_str());

    bool result =
        info.src_anchor != nullptr && ge::GraphUtils::AddEdge(info.src_anchor, info.dst_anchor) != ge::GRAPH_SUCCESS;
    if (result) {
      REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][RmNdEg] [2]:Failed to add edge from [%s] to [%s].",
                      info.src_op_desc->GetName().c_str(), info.dst_node->GetName().c_str());
      return FAILED;
    }
    if (info.src_anchor->GetPeerInDataAnchors().empty()) {
      REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][RmNdEg] Anchor %d's peer in data anchor is empty.",
                      info.src_anchor->GetIdx());
      return FAILED;
    }
    FE_LOGD("[Trans][Merge] Add edge from [%s] to [%s].",
            info.src_op_desc->GetName().c_str(),
            info.dst_node->GetName().c_str());

    FE_LOGI("We keep node (name [%s] type [%s]) in graph because its output has [%zu] peer input anchors.",
            node_name.c_str(), info.node->GetType().c_str(), info.dst_in_anchor_size_of_node);
    uint32_t new_in_anchor_index = info.src_anchor->GetPeerInDataAnchors().size() - 1;
    in_anchor_index_stack.push(new_in_anchor_index);
  }
  UpdateAttrNames(info.dst_op_desc, info.node->GetOpDesc(), info.src_op_desc);
  return SUCCESS;
}

Status TransNodeMerging::MergeOneNode(ge::ComputeGraph &fused_graph, ge::NodePtr node,
                                      const uint32_t &current_in_anchor_index,
                                      std::stack<uint32_t> &in_anchor_index_stack) {
  /* First: Remove edge from source to node, source is predecessor of node.
   * Here TransOp(TransData, Cast, Transpose) will only have one input, so we
   * get in data anchor 0. */
  auto node_name = node->GetName();
  FE_LOGD("Try to remove node: [%s]", node_name.c_str());
  ge::InDataAnchorPtr in_anchor_of_node = node->GetInDataAnchor(0);
  ge::OutDataAnchorPtr src_anchor = nullptr;
  ge::NodePtr src_node = nullptr;
  ge::OpDescPtr src_op_desc = nullptr;

  bool father_node_nullflag = (in_anchor_of_node == nullptr || in_anchor_of_node->GetPeerOutAnchor() == nullptr ||
                               in_anchor_of_node->GetPeerOutAnchor()->GetOwnerNode() == nullptr ||
                               in_anchor_of_node->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc() == nullptr);

  if (father_node_nullflag) {
    FE_LOGW("InAnchor or its predecessor of Node (name [%s] in subGraph (name [%s]) is null!", node_name.c_str(),
            fused_graph.GetName().c_str());
  } else {
    src_anchor = in_anchor_of_node->GetPeerOutAnchor();
    src_node = src_anchor->GetOwnerNode();
    src_op_desc = src_node->GetOpDesc();
  }
  /* Second: Remove edges from node to all its successors. */
  ge::OutDataAnchorPtr out_anchor_of_node = node->GetOutDataAnchor(0);
  bool out_anchor_null_flag = (out_anchor_of_node == nullptr || out_anchor_of_node->GetPeerInDataAnchors().empty());
  if (out_anchor_null_flag) {
    FE_LOGW("outAnchorOfNode or its successor node is null! Failed to remove node [%s] from graph %s!",
            node_name.c_str(), fused_graph.GetName().c_str());
  } else {
    auto dst_in_anchors = out_anchor_of_node->GetPeerInDataAnchors();
    /* Default dst_anchor is anchor 0, we use it to get the op type */
    if (current_in_anchor_index > dst_in_anchors.size()) {
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][ShapeTrans][MrgOneNd] inAnchor index %u is Invalid. It is larger than the \
                      size of dst_in_anchors %zu",
          current_in_anchor_index, dst_in_anchors.size());
      return FAILED;
    }

    ge::InDataAnchorPtr dst_anchor = dst_in_anchors.at(current_in_anchor_index);
    bool dst_anchor_null_flag = (dst_anchor == nullptr || dst_anchor->GetOwnerNode() == nullptr);
    if (dst_anchor_null_flag) {
      REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][MrgOneNd] dstAnchor or its successor node is null!");
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][ShapeTrans][MrgOneNd] Failed to remove node (name [%s] optype [%s]) from \
                      sub_graph (name [%s])!",
          node_name.c_str(), node->GetType().c_str(), fused_graph.GetName().c_str());
      return FAILED;
    } else {
      auto dst_node = dst_anchor->GetOwnerNode();
      auto dst_op_desc = dst_node->GetOpDesc();
      auto dst_in_anchor_size_of_node = dst_in_anchors.size();
      if (current_in_anchor_index >= dst_in_anchor_size_of_node) {
        REPORT_FE_ERROR(
            "[GraphOptJdgInst][ShapeTrans][MrgOneNd] Index [%u] is larger than size of dst_in_anchors \
                        [%zu]. dst op is [%s] and current op is [%s].",
            current_in_anchor_index, dst_in_anchor_size_of_node, dst_node->GetName().c_str(), node->GetName().c_str());
        return FAILED;
      }
      BasicInfoForRemovingNode info = {
          src_node,   src_op_desc, dst_node,          dst_op_desc,        node,
          dst_anchor, src_anchor,  in_anchor_of_node, out_anchor_of_node, dst_in_anchor_size_of_node};
      if (RemoveNodeAndEdges(info, father_node_nullflag, in_anchor_index_stack, fused_graph) != SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

uint32_t TransNodeMerging::FindIndexOfCurrentNode(const Vistor<ge::InDataAnchorPtr> &in_data_anchor_ptr_vec,
                                                  const ge::InDataAnchorPtr &in_anchor) const {
  for (uint32_t i = 0; i < in_data_anchor_ptr_vec.size(); i++) {
    if (in_data_anchor_ptr_vec.at(i) == in_anchor) {
      return i;
    }
  }
  return 0xffffffff;
}

Status TransNodeMerging::MergeTransOpBetweenTwoNormalOp(ge::ComputeGraph &fused_graph, ge::NodePtr src_node,
                                                        const string &normal_op_type, const string &normal_op_name,
                                                        const ge::InDataAnchorPtr &in_anchor) {
  uint32_t loop_count = 0;
  /* We use stack to store all TransNodes between two normal Nodes. */
  std::stack<ge::NodePtr> trans_node_stack;
  std::stack<uint32_t> in_anchor_index_stack;
  in_anchor_index_stack.push(FindIndexOfCurrentNode(src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors(), in_anchor));

  while (IsTransOpType(src_node->GetType()) && loop_count < 100) {
    loop_count++; /* Use loop count to prevent infinite loops */
                  /* Check stack empty first and then use its top() function.
                   * Otherwise, the program will get a seg-fault. */
    if (!trans_node_stack.empty() && CheckTwoTransOpDescMergable(trans_node_stack.top(), src_node->GetOpDesc())) {
      ge::NodePtr node_after_src_node = trans_node_stack.top();
      ge::InDataAnchorPtr in_anchor_of_src_node = src_node->GetInDataAnchor(0);
      ge::NodePtr node_before_src_node = nullptr;
      if (in_anchor_of_src_node == nullptr || in_anchor_of_src_node->GetPeerOutAnchor() == nullptr) {
        node_before_src_node = nullptr;
      } else {
        node_before_src_node = in_anchor_of_src_node->GetPeerOutAnchor()->GetOwnerNode();
      }
      /* Remove two nodes and correct relationships of edges. */
      in_anchor_index_stack.pop();

      uint32_t in_anchor_index_of_node_after_src_node = in_anchor_index_stack.top();

      in_anchor_index_stack.pop();

      if (MergeOneNode(fused_graph, node_after_src_node, in_anchor_index_of_node_after_src_node,
                       in_anchor_index_stack) != SUCCESS) {
        return FAILED;
      }
      in_anchor_index_of_node_after_src_node = in_anchor_index_stack.top();

      in_anchor_index_stack.pop();

      if (MergeOneNode(fused_graph, src_node, in_anchor_index_of_node_after_src_node, in_anchor_index_stack) !=
          SUCCESS) {
        return FAILED;
      }
      trans_node_stack.pop();

      if (node_before_src_node == nullptr) {
        FE_LOGD("We merged two nodes ([%s] and [%s]) between source [nullptr] and dst node (type [%s] name [%s]).",
                src_node->GetName().c_str(), node_after_src_node->GetName().c_str(), normal_op_type.c_str(),
                normal_op_name.c_str());
        return MERGE_TRANS_OP_NO_MORE_PREDECESSOR;
      } else {
        FE_LOGD("We merged two nodes ([%s] and [%s]) between source [%s] and dst node (type [%s] name [%s]).",
                src_node->GetName().c_str(), node_after_src_node->GetName().c_str(),
                node_before_src_node->GetName().c_str(), normal_op_type.c_str(), normal_op_name.c_str());
        src_node = node_before_src_node;
      }
    } else {
      trans_node_stack.push(src_node);
      /* TransOp only have one input. */
      ge::InDataAnchorPtr in_anchor_of_src_node = src_node->GetInDataAnchor(0);
      int flag = (in_anchor_of_src_node == nullptr || in_anchor_of_src_node->GetPeerOutAnchor() == nullptr ||
                  in_anchor_of_src_node->GetPeerOutAnchor()->GetOwnerNode() == nullptr);
      if (flag) {
        return MERGE_TRANS_OP_NO_MORE_PREDECESSOR;
      }

      src_node = in_anchor_of_src_node->GetPeerOutAnchor()->GetOwnerNode();
      uint32_t in_anchor_index =
          FindIndexOfCurrentNode(src_node->GetOutDataAnchor(0)->GetPeerInDataAnchors(), in_anchor_of_src_node);
      in_anchor_index_stack.push(in_anchor_index);
    }
  }
  return SUCCESS;
}

}  // namespace fe