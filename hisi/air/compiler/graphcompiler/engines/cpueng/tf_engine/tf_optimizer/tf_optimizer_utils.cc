/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "tf_optimizer_utils.h"

#include "graph/utils/op_desc_utils.h"
#include "graph/utils/graph_utils.h"
#include "elewise_calculation_ops.h"
#include "array_ops.h"
#include "state_ops.h"
#include "util/util.h"
#include "aicpu_graph_optimizer/graph_optimizer_utils.h"
#include "util/tf_util.h"
#include "util/constant.h"
#include "common/sgt_slice_type.h"

namespace {
const std::string kValidateShapeAttr = "validate_shape";
const std::string kPlaceholderOp = "PlaceHolder";
const std::string kEndOp = "End";
const std::string kUseLocking = "use_locking";
}

namespace aicpu {
ge::Status TfVariableGraph::CreateAssign(ge::GeTensorDesc &first_src_tensor_desc,
                                         ge::GeTensorDesc &second_src_tensor_desc,
                                         ge::GeTensorDesc &dst_tensor_desc,
                                         ge::ComputeGraph &graph,
                                         ge::NodePtr &assign_node) {
  auto assign_op_owner = ge::op::Assign();
  ge::OpDescPtr assign_op = ge::OpDescUtils::GetOpDescFromOperator(assign_op_owner);
  AICPU_CHECK_NOTNULL(assign_op)
  assign_op->SetType("AssignExt");

  AICPU_CHECK_RES_WITH_LOG(assign_op->UpdateInputDesc(0, first_src_tensor_desc),
      "Call ge::op::Assign::UpdateInputDesc function failed, input[0].")
  AICPU_CHECK_RES_WITH_LOG(assign_op->UpdateInputDesc(1, second_src_tensor_desc),
      "Call ge::op::Assign::UpdateInputDesc function failed, input[1].")
  AICPU_CHECK_RES_WITH_LOG(assign_op->UpdateOutputDesc(0, dst_tensor_desc),
      "Call ge::op::Assign::UpdateOutputDesc function failed, output[0].")

  CHECK_RES_BOOL(ge::AttrUtils::SetBool(assign_op, kValidateShapeAttr, false),
      ge::FAILED,
      AICPU_REPORT_CALL_ERROR(
          "Call ge::AttrUtils::SetBool failed to set attr[%s], op[%s]",
          kValidateShapeAttr.c_str(), assign_op->GetName().c_str()))
  CHECK_RES_BOOL(ge::AttrUtils::SetBool(assign_op, kUseLocking, false),
      ge::FAILED,
      AICPU_REPORT_CALL_ERROR(
          "Call ge::AttrUtils::SetBool failed to set attr[%s], op[%s]",
          kUseLocking.c_str(), assign_op->GetName().c_str()))
  assign_node = graph.AddNode(assign_op);
  AICPU_CHECK_NOTNULL(assign_node)
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::CreateIdentity(ge::GeTensorDesc &src_tensor_desc,
                                           ge::GeTensorDesc &dst_tensor_desc,
                                           ge::ComputeGraph &graph,
                                           ge::NodePtr &identity_node) {
  auto identity_op_owner = ge::op::Identity();
  ge::OpDescPtr identity_op = ge::OpDescUtils::GetOpDescFromOperator(identity_op_owner);
  AICPU_CHECK_NOTNULL(identity_op)
  identity_op->SetType("IdentityExt");
  AICPU_CHECK_RES_WITH_LOG(identity_op->UpdateInputDesc(0, src_tensor_desc),
      "Call ge::op::Identity::UpdateInputDesc function failed, input[0].")
  AICPU_CHECK_RES_WITH_LOG(identity_op->UpdateOutputDesc(0, dst_tensor_desc),
      "Call ge::op::Identity::UpdateOutputDesc function failed, output[0].")
  identity_node = graph.AddNode(identity_op);
  AICPU_CHECK_NOTNULL(identity_node)
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::CreateVariable(ge::GeTensorDesc &dst_tensor_desc,
                                           ge::ComputeGraph &graph,
                                           ge::NodePtr &variable_node) {
  auto variable_op_owner = ge::op::TemporaryVariable();
  ge::OpDescPtr variable_op = ge::OpDescUtils::GetOpDescFromOperator(variable_op_owner);
  AICPU_CHECK_NOTNULL(variable_op)
  variable_op->SetType("VariableExt");
  std::vector<int64_t> values;
  for (const auto value : dst_tensor_desc.GetShape().GetDims()) {
    AICPUE_LOGD("Dim value is [%ld].", value);
    values.push_back(value);
  }
  variable_op->SetAttr("shape", ge::GeAttrValue::CreateFrom<std::vector<int64_t>>(values));
  std::string current_time = CurrentTimeInStr();
  static int index = 0;
  std::string shared_name;
  shared_name.append("Variable").append("_").append(Stringcat(index));
  index++;
  variable_op->SetAttr("container", ge::GeAttrValue::CreateFrom<std::string>("Variable"));
  variable_op->SetAttr("shared_name", ge::GeAttrValue::CreateFrom<std::string>(shared_name.c_str()));
  AICPU_CHECK_NOTNULL(variable_op)

  AICPU_CHECK_RES_WITH_LOG(variable_op->UpdateOutputDesc(0, dst_tensor_desc),
      "Call ge::op::TemporaryVariable::UpdateOutputDesc function failed, output[0].")

  variable_node = graph.AddNode(variable_op);
  AICPU_CHECK_NOTNULL(variable_node)
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::InsertVariable(ge::OutDataAnchorPtr &src_anchor,
                                           const ge::InDataAnchorPtr &dst_anchor,
                                           ge::NodePtr &variable_node) {
  AICPUE_LOGD("Enter InsertVariable Func.");
  AICPU_CHECK_NOTNULL(variable_node)
  AICPU_CHECK_NOTNULL(src_anchor)
  AICPU_CHECK_NOTNULL(dst_anchor)
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::RemoveEdge(src_anchor, dst_anchor),
      "Call ge::GraphUtils::RemoveEdge function failed to remove edge between"
      " op[%s] and op[%s]", src_anchor->GetOwnerNode()->GetName().c_str(),
      dst_anchor->GetOwnerNode()->GetName().c_str())
  ge::OutDataAnchorPtr y_anchor_from_variable = variable_node->GetOutDataAnchor(0);
  AICPUE_LOGD("Get Variable outDataAnchor success.");
  AICPU_CHECK_NOTNULL(y_anchor_from_variable)
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::AddEdge(y_anchor_from_variable, dst_anchor),
      "Call ge::GraphUtils::AddEdge function failed to add edge between"
      " op[%s] and [%s]", variable_node->GetName().c_str(),
      dst_anchor->GetOwnerNode()->GetName().c_str())
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::InsertAssign(ge::OutDataAnchorPtr &src_anchor,
                                         const ge::InDataAnchorPtr &dst_anchor,
                                         const ge::NodePtr &node_ptr,
                                         ge::NodePtr &assign_node) {
  AICPUE_LOGD("Enter InsertAssign Func.");
  AICPU_CHECK_NOTNULL(src_anchor)
  AICPU_CHECK_NOTNULL(dst_anchor)
  AICPU_CHECK_NOTNULL(assign_node)
  ge::OutDataAnchorPtr y_anchor_from_variable = node_ptr->GetOutDataAnchor(0);
  AICPUE_LOGD("Get Variable outDataAnchor success.");
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::RemoveEdge(y_anchor_from_variable, dst_anchor),
      "Call ge::GraphUtils::RemoveEdge function failed to remove edge between op[%s] and op[%s]",
      node_ptr->GetName().c_str(), dst_anchor->GetOwnerNode()->GetName().c_str())
  ge::InDataAnchorPtr input1_anchor_from_assign = assign_node->GetInDataAnchor(0);
  AICPUE_LOGD("Get Assign inDataAnchor success.");
  ge::InDataAnchorPtr input2_anchor_from_assign = assign_node->GetInDataAnchor(1);
  AICPUE_LOGD("Get Assign inDataAnchor success.");
  ge::OutDataAnchorPtr output_anchor_from_assign = assign_node->GetOutDataAnchor(0);
  AICPUE_LOGD("Get Assign outDataAnchor success.");
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::AddEdge(src_anchor, input2_anchor_from_assign),
      "Call ge::GraphUtils::AddEdge function failed to add edge between op[%s] and op[%s]",
      src_anchor->GetOwnerNode()->GetName().c_str(), assign_node->GetName().c_str())
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::AddEdge(y_anchor_from_variable, input1_anchor_from_assign),
      "Call ge::GraphUtils::AddEdge function failed to add edge between op[%s] and op[%s]",
      node_ptr->GetName().c_str(), assign_node->GetName().c_str())
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::AddEdge(output_anchor_from_assign, dst_anchor),
      "Call ge::GraphUtils::AddEdge function failed to add edge between op[%s] and op[%s]",
      assign_node->GetName().c_str(), dst_anchor->GetOwnerNode()->GetName().c_str())
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::InsertIdentity(ge::OutDataAnchorPtr &src_anchor,
                                           const ge::InDataAnchorPtr &dst_anchor,
                                           ge::NodePtr &identity_node) {
  AICPUE_LOGD("Enter InsertIdentity Func.");
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::RemoveEdge(src_anchor, dst_anchor),
      "Call ge::GraphUtils::RemoveEdge function failed to remove edge between op[%s] and op[%s]",
      src_anchor->GetOwnerNode()->GetName().c_str(),
      dst_anchor->GetOwnerNode()->GetName().c_str())
  AICPU_CHECK_NOTNULL(identity_node)
  ge::InDataAnchorPtr x_anchor_from_identity = identity_node->GetInDataAnchor(0);
  AICPUE_LOGD("Get Identity inDataAnchor success.");
  ge::OutDataAnchorPtr y_anchor_from_identity = identity_node->GetOutDataAnchor(0);
  AICPUE_LOGD("Get Identity outDataAnchor success.");
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::AddEdge(src_anchor, x_anchor_from_identity),
      "Call ge::GraphUtils::AddEdge function failed to add edge between op[%s] and op[%s]",
      src_anchor->GetOwnerNode()->GetName().c_str(),
      identity_node->GetName().c_str())
  AICPU_CHECK_RES_WITH_LOG(ge::GraphUtils::AddEdge(y_anchor_from_identity, dst_anchor),
      "Call ge::GraphUtils::AddEdge function failed to add edge between op[%s] and op[%s]",
      identity_node->GetName().c_str(),
      dst_anchor->GetOwnerNode()->GetName().c_str())
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::CreateAndInsertVariable(ge::OutDataAnchorPtr &src_anchor,
                                                    const ge::InDataAnchorPtr &dst_anchor,
                                                    ge::NodePtr &variable_node,
                                                    ge::ComputeGraph &graph) {
  AICPUE_LOGD("Enter CreateAndInsertVariable Func.");
  const ge::NodePtr src_node = src_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(src_node)
  const ge::OpDescPtr src_op = src_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(src_op)
  int idx = ge::AnchorUtils::GetIdx(src_anchor);
  // -1 is invalid index
  CHECK_RES_BOOL((idx != -1), ge::FAILED,
      AICPU_REPORT_INNER_ERROR("invalid output anchor index[%d], op[%s].",
          idx, src_op->GetType().c_str()))
  uint32_t src_anchor_idx = static_cast<uint32_t>(idx);

  // dst_anchor already check not null and valid
  const ge::OpDescPtr dst_op = dst_anchor->GetOwnerNode()->GetOpDesc();
  AICPU_CHECK_NOTNULL(dst_op);
  uint32_t dst_anchor_idx = static_cast<uint32_t>(ge::AnchorUtils::GetIdx(dst_anchor));

  ge::GeTensorDesc src_tensor_desc = src_op->GetOutputDesc(src_anchor_idx);
  ge::GeTensorDesc dst_tensor_desc = dst_op->GetInputDesc(dst_anchor_idx);
  AICPUE_LOGD("Enter CreateVariable Func.");
  AICPU_CHECK_RES_WITH_LOG(CreateVariable(dst_tensor_desc, graph, variable_node),
      "Call TfVariableGraph::CreateVariable function failed, op[%s].",
      src_op->GetName().c_str())
  AICPUE_LOGD("Enter InsertVariable Func.");
  AICPU_CHECK_RES_WITH_LOG(InsertVariable(src_anchor, dst_anchor, variable_node),
      "Call TfVariableGraph::InsertVariable function failed, op[%s].",
      src_op->GetName().c_str())
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::CreateAndInsertIdentity(ge::OutDataAnchorPtr &src_anchor,
                                                    const ge::InDataAnchorPtr &dst_anchor,
                                                    ge::ComputeGraph &graph) {
  AICPUE_LOGD("Enter CreateAndInsertIdentity Func.");
  const ge::NodePtr src_node = src_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(src_node)
  const ge::OpDescPtr src_op = src_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(src_op)

  int idx = ge::AnchorUtils::GetIdx(src_anchor);
  CHECK_RES_BOOL((idx != -1), ge::FAILED,
      AICPU_REPORT_INNER_ERROR("invalid output anchor index[%d], op[%s].",
                               idx, src_op->GetType().c_str()))
  uint32_t src_anchor_idx = static_cast<uint32_t>(idx);
  ge::GeTensorDesc src_tensor_desc = src_op->GetOutputDesc(src_anchor_idx);

  // dst_anchor already check not null and valid
  const ge::OpDescPtr dst_op = dst_anchor->GetOwnerNode()->GetOpDesc();
  uint32_t dst_anchor_idx = static_cast<uint32_t>(ge::AnchorUtils::GetIdx(dst_anchor));
  ge::GeTensorDesc dst_tensor_desc = dst_op->GetInputDesc(dst_anchor_idx);

  ge::NodePtr identity_node;
  AICPUE_LOGD("Enter CreateIdentity Func.");
  AICPU_CHECK_RES_WITH_LOG(CreateIdentity(src_tensor_desc, dst_tensor_desc, graph, identity_node),
      "Call TfVariableGraph::CreateIdentity function failed, op[%s]",
      src_op->GetName().c_str())
  AICPUE_LOGD("Enter CreateIdentity Func.");
  AICPU_CHECK_RES_WITH_LOG(InsertIdentity(src_anchor, dst_anchor, identity_node),
      "Call TfVariableGraph::InsertIdentity function failed, op[%s]",
      src_op->GetName().c_str())
  return ge::SUCCESS;
}

ge::Status TfVariableGraph::CreateAndInsertAssign(ge::OutDataAnchorPtr &src_anchor,
                                                  const ge::InDataAnchorPtr &dst_anchor,
                                                  const ge::NodePtr &node_ptr,
                                                  ge::ComputeGraph &graph) {
  AICPUE_LOGD("Enter CreateAndInsertAssign Func.");
  const ge::NodePtr src_node = src_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(src_node)
  const ge::OpDescPtr src_op = src_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(src_op)
  const ge::OpDescPtr variable_op = node_ptr->GetOpDesc();
  AICPU_CHECK_NOTNULL(variable_op)

  int idx = ge::AnchorUtils::GetIdx(src_anchor);
  CHECK_RES_BOOL((idx != -1), ge::FAILED,
      AICPU_REPORT_INNER_ERROR("invalid output anchor index[%d], op[%s].",
                               idx, src_op->GetType().c_str()))
  uint32_t src_anchor_idx = static_cast<uint32_t>(idx);

  ge::GeTensorDesc tensor_desc1 = variable_op->GetOutputDesc(0);
  ge::GeTensorDesc tensor_desc2 = src_op->GetOutputDesc(src_anchor_idx);
  ge::NodePtr assign_node;
  AICPUE_LOGD("Enter CreateAssign Func.");
  AICPU_CHECK_RES_WITH_LOG(CreateAssign(tensor_desc1, tensor_desc2, tensor_desc1, graph, assign_node),
      "Call TfVariableGraph::CreateAssign function failed, op[%s]",
      src_op->GetName().c_str())
  AICPUE_LOGD("Enter InsertAssign Func.");
  AICPU_CHECK_RES_WITH_LOG(InsertAssign(src_anchor, dst_anchor, node_ptr, assign_node),
      "Call TfVariableGraph::InsertAssign function failed, op[%s]",
      src_op->GetName().c_str())
  return ge::SUCCESS;
}

// generate tf graph
ge::Status TfVariableGraph::GenerateTfVariableGraph(ge::ComputeGraph &graph) {
  for (const ge::NodePtr &cur_node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(cur_node)
    ge::OpDescPtr cur_op_desc_ptr = cur_node->GetOpDesc();
    AICPU_CHECK_NOTNULL(cur_op_desc_ptr)
    std::string op_type = cur_op_desc_ptr->GetType();
    // if op type is variable, replace it with assign and variable
    if (op_type == kPlaceholderOp) {
      // get all output node and anchors
      for (auto &anchor : cur_node->GetOutDataNodesAndAnchors()) {
        if (IsRefTensorDesc(anchor.first->GetOpDesc()->GetInputDesc(static_cast<uint32_t>(anchor.second->GetIdx())))) {
          AICPUE_LOGD("Current op type is [%s]. Insert Variable Op and replace it with TF Variable and Assign.",
                      op_type.c_str());
          AICPUE_LOGD("Before get src_anchor from PlaceHolder.");
          // placeholder op just have one output edge
          ge::OutDataAnchorPtr src_anchor = cur_node->GetOutDataAnchor(0);
          AICPU_CHECK_NOTNULL(src_anchor)

          AICPUE_LOGD("After get src_anchor from PlaceHolder.");
          // get idx
          int idx = ge::AnchorUtils::GetIdx(src_anchor);
          AICPUE_LOGD("InDataAnchor Idx value is [%d].", idx);
          // -1 is invalid index
          if (idx == -1) {
            AICPUE_LOGD("Current op's inDataAnchor Idx value -1 is invalid.");
            continue;
          }
          AICPUE_LOGD("Before get dst_anchor from PlaceHolder.");
          bool sgt_flag = false;
          ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
          (void)GraphOptimizerUtils::CheckIsFftsPlus(cur_op_desc_ptr, slice_info_ptr, sgt_flag);
          // see all others anchor
          for (const auto &dst_anchor : src_anchor->GetPeerInDataAnchors()) {
            ge::NodePtr variable_node;
            AICPU_CHECK_RES_WITH_LOG(CreateAndInsertVariable(src_anchor, dst_anchor, variable_node, graph),
                "Call TfVariableGraph::CreateAndInsertVariable function failed, op[%s].",
                cur_node->GetName().c_str())
            // Variable op just have one output edge
            ge::OutDataAnchorPtr out_data_anchor = variable_node->GetOutDataAnchor(0);
            AICPU_CHECK_NOTNULL(out_data_anchor)
            ge::InDataAnchorPtr dst_variable_anchor = (*out_data_anchor->GetPeerInDataAnchors().begin());
            AICPU_CHECK_RES_WITH_LOG(CreateAndInsertAssign(src_anchor, dst_variable_anchor, variable_node, graph),
                "Call TfVariableGraph::CreateAndInsertAssign function failed, op[%s].",
                cur_node->GetName().c_str())
            if ((sgt_flag) && (slice_info_ptr != nullptr)) {
              (void)variable_node->GetOpDesc()->SetExtAttr(kAttrNameSgtStruct, slice_info_ptr);
              (void)ge::AttrUtils::SetInt(variable_node->GetOpDesc(), kAttrNameThreadScopeId,
                                          slice_info_ptr->thread_scope_id);
            }
          }
        }
      }
    }
    if (op_type == kEndOp) {
      AICPUE_LOGD("Current op type is [%s]. Insert Variable Op and replace it with TF Variable and Assign.", op_type.c_str());
      for (auto &node_and_anchor : cur_node->GetInDataNodesAndAnchors()) {
        if (IsRefTensorDesc(node_and_anchor.first->GetOpDesc()->GetOutputDesc(
            static_cast<uint32_t>(node_and_anchor.second->GetIdx())))) {
          ge::NodePtr node = (*cur_node->GetInNodes().begin());
          AICPU_CHECK_NOTNULL(node)
          ge::OutDataAnchorPtr src_anchor = node->GetOutDataAnchor(0);
          AICPU_CHECK_NOTNULL(src_anchor)
          ge::InDataAnchorPtr dst_anchor = cur_node->GetInDataAnchor(0);
          AICPU_CHECK_NOTNULL(dst_anchor)
          CreateAndInsertIdentity(src_anchor, dst_anchor, graph);
        }
      }
    }
  }
  return ge::SUCCESS;
}

bool TfVariableGraph::IsRefTensorDesc(const ge::GeTensorDesc &tensor_desc) {
  return !tensor_desc.GetRefPortIndex().empty();
}
}