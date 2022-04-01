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

#include <sstream>
#include "common/fe_error_code.h"
#include "common/op_info_common.h"
#include "common/unknown_shape_util.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/insert_trans_node_strategy.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "register/graph_optimizer/fusion_common/graph_pass_util.h"

namespace fe {
const std::string kAssign = "Assign";

TransNodeBaseGenerator::TransNodeBaseGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr, TransInfoPtr trans_info_ptr)
    : trans_info_ptr_(trans_info_ptr), fe_ops_store_info_ptr_(fe_ops_store_ptr) {}

TransNodeBaseGenerator::~TransNodeBaseGenerator() {}

ge::OpDescPtr TransNodeBaseGenerator::CreateBasicOpDescForTransNode(const string &op_type) const {
  stringstream op_name_temp;
  // The atomic id of trans nodes must be unique.(start from 0)
  op_name_temp << "trans_" << op_type << "_" << GetTransAtomicId();

  ge::OpDescPtr op_desc_ptr = nullptr;
  FE_MAKE_SHARED(op_desc_ptr = std::make_shared<ge::OpDesc>(op_name_temp.str().c_str(), op_type), return nullptr);
  FE_LOGD("Create op [%s].", op_desc_ptr->GetName().c_str());
  return op_desc_ptr;
}

Status TransNodeBaseGenerator::SetTensorDescInfo(ge::OpDescPtr &op_desc_ptr) const {
  FE_CHECK_NOTNULL(op_desc_ptr);
  for (auto input_tensor : op_desc_ptr->GetAllInputsDescPtr()) {
    input_tensor->SetOriginFormat(trans_info_ptr_->src_out_original_format);
    input_tensor->SetOriginShape(trans_info_ptr_->src_out_original_shape);
  }
  bool is_from_const_op = false;
  (void)ge::AttrUtils::GetBool(trans_info_ptr_->src_op_desc, kIsComeFromConstOp, is_from_const_op);
  bool is_src_from_const_op = is_from_const_op || trans_info_ptr_->src_op_desc->GetType() == CONSTANT ||
          trans_info_ptr_->src_op_desc->GetType() == CONSTANTOP;
  (void)ge::AttrUtils::SetBool(op_desc_ptr, kIsComeFromConstOp, is_src_from_const_op);
  for (auto output_tensor : op_desc_ptr->GetAllOutputsDescPtr()) {
    output_tensor->SetOriginFormat(trans_info_ptr_->src_out_original_format);
    output_tensor->SetOriginShape(trans_info_ptr_->src_out_original_shape);
    if (!is_src_from_const_op) {
      GraphPassUtil::SetOutputDescAttr(trans_info_ptr_->src_out_tensor_desc_ptr,
                                       static_cast<int64_t>(trans_info_ptr_->src_anchor->GetIdx()),
                                       trans_info_ptr_->src_op_desc,
                                       output_tensor);
    }
  }
  return SUCCESS;
}

Status TransNodeBaseGenerator::SetTensorRealDimCountAndNewShape(ge::OpDescPtr &op_desc_ptr,
                                                                std::vector<ge::GeShape> inputs_shape,
                                                                ge::GeShape output_shape) const {
  FE_CHECK_NOTNULL(op_desc_ptr);
  uint32_t index = 0;
  for (auto &input_tensor : op_desc_ptr->GetAllInputsDescPtr()) {
    if (index >= inputs_shape.size()) {
      break;
    }
    if (index < inputs_shape[index].GetDims().size()) {
      ge::TensorUtils::SetRealDimCnt(*input_tensor.get(), static_cast<uint32_t>(inputs_shape[index].GetDims().size()));
      input_tensor->SetShape(inputs_shape[index]);
    }
    index++;
  }
  for (auto &output_tensor : op_desc_ptr->GetAllOutputsDescPtr()) {
    ge::TensorUtils::SetRealDimCnt(*output_tensor.get(), static_cast<uint32_t>(output_shape.GetDims().size()));
    output_tensor->SetShape(output_shape);
  }
  return SUCCESS;
}

Status TransNodeBaseGenerator::SetNewShapeRange(const ge::OpDescPtr &op_desc_ptr,
                                                vector<std::pair<int64_t, int64_t>> &inputs_range,
                                                vector<std::pair<int64_t, int64_t>> &output_range) const {
  FE_CHECK_NOTNULL(op_desc_ptr);
  if (IsFeSupportedDynamicOp(*op_desc_ptr, true)) {
    uint32_t index = 0;
    for (auto &input_tensor : op_desc_ptr->GetAllInputsDescPtr()) {
      if (index < inputs_range.size()) {
        input_tensor->SetShapeRange(inputs_range);
      }
      index++;
    }
    for (auto &output_tensor : op_desc_ptr->GetAllOutputsDescPtr()) {
      output_tensor->SetShapeRange(output_range);
    }
  }
  return SUCCESS;
}

Status TransNodeBaseGenerator::AddNecessaryPeerNodes(ge::ComputeGraph &fused_graph, ge::NodePtr new_node) const {
  return SUCCESS;
}

Status TransNodeBaseGenerator::AddEdgesAndFreshTransInfo(ge::ComputeGraph &fused_graph,
                                                         const ge::OpDescPtr &op_desc_ptr) {
  ge::NodePtr new_node = fused_graph.AddNode(op_desc_ptr);

  FE_CHECK_NOTNULL(new_node);

  if (AddEdgesForNewNode(new_node) != SUCCESS) {
    // new_op,src_op, des_op, src_format,dest_format,graph_name
    std::map<std::string, std::string> error_key_map;
    error_key_map[EM_NEW_OP] = new_node->GetOpDesc()->GetType();

    // get the op desc of source node
    ge::OpDescPtr src_op_desc_ptr = trans_info_ptr_->src_anchor->GetOwnerNode()->GetOpDesc();
    error_key_map[EM_SRC_OP] = src_op_desc_ptr->GetType();

    // get the format of source node
    FE_CHECK_NOTNULL(src_op_desc_ptr->GetOutputDescPtr(0));
    ge::Format src_node_format =
        static_cast<ge::Format>(ge::GetPrimaryFormat(src_op_desc_ptr->GetOutputDescPtr(0)->GetFormat()));
    error_key_map[EM_SRC_FORMAT] = ge::TypeUtils::FormatToSerialString(src_node_format);

    // get the op desc of dest node
    ge::OpDescPtr dest_op_desc_ptr = trans_info_ptr_->dst_anchor->GetOwnerNode()->GetOpDesc();
    error_key_map[EM_DEST_OP] = dest_op_desc_ptr->GetType();

    // get the format of destination node
    ge::Format dest_node_format =
        static_cast<ge::Format>(ge::GetPrimaryFormat(dest_op_desc_ptr->GetInputDesc(0).GetFormat()));
    error_key_map[EM_DEST_FORMAT] = ge::TypeUtils::FormatToSerialString(dest_node_format);
    error_key_map[EM_GRAPH_NAME] = fused_graph.GetName();

    REPORT_FE_ERROR(
        "[GraphOptJdgInst][ShapeTrans][AddEgFreshTransInfo] Failed to add edges for new node. src_node[%s],\
                    node[%s], Failed to add op[%s] between op[%s, format[%s]] and op[%s, format[%s]],\
                    when processing the graph_name[%s].",
        trans_info_ptr_->src_op_desc->GetName().c_str(), new_node->GetName().c_str(), error_key_map[EM_NEW_OP].c_str(),
        error_key_map[EM_SRC_OP].c_str(), error_key_map[EM_SRC_FORMAT].c_str(), error_key_map[EM_DEST_OP].c_str(),
        error_key_map[EM_DEST_FORMAT].c_str(), error_key_map[EM_GRAPH_NAME].c_str());
    return FAILED;
  } else {
    Status ret = AddNecessaryPeerNodes(fused_graph, new_node);
    if (ret != SUCCESS) {
      return ret;
    }
    FE_LOGD("Add edges for new node successfully! src_node[%s], node[%s]",
            trans_info_ptr_->src_op_desc->GetName().c_str(), new_node->GetName().c_str());
    /* After inserting new TransData or Permute op, re-write the
     * shape of det_op_desc. */
    /* After inserting trans node, src will become trans node. */
    RefreshSourceTransInfo(new_node);
    return SUCCESS;
  }
}

Status TransNodeBaseGenerator::AddEdgesForNewNode(ge::NodePtr new_node) const {
  ge::OutDataAnchorPtr src_anchor = trans_info_ptr_->src_anchor;
  ge::InDataAnchorPtr dst_anchor = trans_info_ptr_->dst_anchor;
  Status ret;
  /*
   * Variable ---> Assign ---> X
   *                          / (control edges)
   *     ...AssignAdd -------
   *
   * Because operator Assign has the following characteristic:
   * The variable from the output of Assign will depend on the
   * AssignAdd or AssignSub. Only after calculating AssignAdd and
   * AssignSub we can execute X.
   * If Insert a node A between Assgin and X.
   * Every control edge to the node X should be
   * moved to the node A. So we use InsertNodeBefore. */
  if (trans_info_ptr_->src_op_desc_type == kAssign) {
    ret = ge::GraphUtils::InsertNodeBefore(dst_anchor, new_node);
  } else {
    ret = ge::GraphUtils::InsertNodeAfter(src_anchor, {dst_anchor}, new_node);
  }

  if (ret != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][TransNd] Add Edge failed, node[%s].", new_node->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

void TransNodeBaseGenerator::RefreshSourceTransInfo(ge::NodePtr src_node) const {
  trans_info_ptr_->src_op_desc = src_node->GetOpDesc();
  trans_info_ptr_->src_node_ptr = src_node;
  trans_info_ptr_->src_anchor = src_node->GetOutDataAnchor(0);

  uint32_t src_anchor_index = static_cast<uint32_t>(trans_info_ptr_->src_anchor->GetIdx());
  trans_info_ptr_->src_out_tensor_desc_ptr = trans_info_ptr_->src_op_desc->GetOutputDescPtr(src_anchor_index);
  if (trans_info_ptr_->src_out_tensor_desc_ptr == nullptr) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][RefreshSourceTransInfo] src_out_tensor_desc_ptr is null.");
    return;
  }

  auto src_out_format = trans_info_ptr_->src_out_tensor_desc_ptr->GetFormat();
  trans_info_ptr_->src_out_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(src_out_format));
  trans_info_ptr_->src_out_sub_format = static_cast<ge::Format>(ge::GetSubFormat(src_out_format));
  trans_info_ptr_->src_out_data_type = trans_info_ptr_->src_out_tensor_desc_ptr->GetDataType();
  trans_info_ptr_->src_out_shape = trans_info_ptr_->src_out_tensor_desc_ptr->GetShape();
  trans_info_ptr_->src_out_range = GetShapeRange(*trans_info_ptr_->src_out_tensor_desc_ptr);

  trans_info_ptr_->src_op_desc_type = trans_info_ptr_->src_op_desc->GetType();

  trans_info_ptr_->is_source_weight = CheckOpConstOrVariableInOriGraph(trans_info_ptr_->src_op_desc);

  uint64_t strategy_ext_val = CalcStrategyIdExtraVal(trans_info_ptr_);
  uint64_t strategy_id =
      CalcStrategyId(trans_info_ptr_->src_out_primary_format, trans_info_ptr_->dst_in_primary_format, strategy_ext_val);

  trans_info_ptr_->strategy_id = strategy_id;
}

Status TransNodeBaseGenerator::TransformDimTo4(bool increasing_flag) const {
  std::vector<int64_t> dims;
  ge::GeShape new_shape;
  if (increasing_flag) {
    if (IsShapeContainUnknownDimNum(trans_info_ptr_->src_out_shape)) {
      FE_LOGD("The shape of output [%u] of op (name [%s] type [%s]) is unknown, do not need to pad shape to 4 dims.",
              trans_info_ptr_->src_anchor->GetIdx(), trans_info_ptr_->src_op_desc->GetName().c_str(),
              trans_info_ptr_->src_op_desc->GetType().c_str());
      return SUCCESS;
    }
    dims = trans_info_ptr_->src_out_shape.GetDims();
    auto old_dims_size = dims.size();
    ExpandDimension(dims, trans_info_ptr_->dst_op_desc->GetType(), trans_info_ptr_->src_out_primary_format,
                    trans_info_ptr_->dst_in_primary_format, trans_info_ptr_->dst_anchor->GetIdx(),
                    trans_info_ptr_->dst_reshape_type);
    trans_info_ptr_->src_out_shape = ge::GeShape(dims);
    trans_info_ptr_->src_out_range = GetShapeRange(*trans_info_ptr_->src_out_tensor_desc_ptr);
    FE_LOGD(
        "The size of output [%u] of op (name [%s] type [%s]) is less than 4. Size is [%lu]"
        "Now fill in the dims with value[1] until size reaches 4.",
        trans_info_ptr_->src_anchor->GetIdx(), trans_info_ptr_->src_op_desc->GetName().c_str(),
        trans_info_ptr_->src_op_desc->GetType().c_str(), old_dims_size);
  }

  return SUCCESS;
}

bool TransNodeBaseGenerator::TransNodeCheckAccuracySupported(const ge::OpDescPtr &op_desc_ptr, bool real_query,
                                                             bool not_need_check_support_flag) const {
  FE_CHECK_NOTNULL(fe_ops_store_info_ptr_);
  if (not_need_check_support_flag) {
    // set the fe and ge imply type of the op
    (void)ge::AttrUtils::SetInt(op_desc_ptr, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    (void)ge::AttrUtils::SetInt(op_desc_ptr, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
    (void)ge::AttrUtils::SetInt(op_desc_ptr, IS_CHECK_SUPPORTED, 0);
    return true;
  } else {
    /* Check trans-nodes supported in cache */
    if (fe_ops_store_info_ptr_->CheckAccuracySupportByCache(op_desc_ptr)) {
      return true;
    }
    std::string un_supported_reason;
    bool ret = fe_ops_store_info_ptr_->CheckAccuracySupported(op_desc_ptr, un_supported_reason, real_query);
    /* Store the result of check accuracy support for trans-nodes. */
    fe_ops_store_info_ptr_->StoreCheckSuportResultForTransNodes(op_desc_ptr, ret);
    return ret;
  }
}

uint64_t TransNodeBaseGenerator::GetTransAtomicId() {
  static std::atomic<uint64_t> global_trans_atomic_id(0);
  return global_trans_atomic_id.fetch_add(1, std::memory_order_relaxed);
}
}  // namespace fe