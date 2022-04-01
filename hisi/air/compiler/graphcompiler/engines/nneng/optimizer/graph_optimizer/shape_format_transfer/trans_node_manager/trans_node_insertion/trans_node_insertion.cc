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

#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"
#include <common/unknown_shape_util.h>
#include <external/graph/types.h>
#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "ops_store/ops_kernel_manager.h"
#include "graph/debug/ge_attr_define.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_cast_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_reformat_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_reshape_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdata_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdatarnn_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transpose_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_squeeze_v2_generator.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_unsqueeze_v2_generator.h"

namespace fe {

Status TransNodeInsertion::AddTransNodeType(TransNodeBaseGenerator *trans_node_type) {
  FE_CHECK(
      trans_node_type == nullptr,
      REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][AddTransNdType] transNodeType is null, AddTransNodeType failed"),
      return PARAM_INVALID);
  whole_trans_nodes_vector_.push_back(trans_node_type);
  return SUCCESS;
}

Status TransNodeInsertion::initialize() {
  FE_MAKE_SHARED(global_trans_info_ptr_ = std::make_shared<TransInfo>(), return FAILED);
  TransInfoPtr trans_info_front;
  TransInfoPtr trans_info_end;
  FE_MAKE_SHARED(trans_info_front = std::make_shared<TransInfo>(), return FAILED);
  FE_MAKE_SHARED(trans_info_end = std::make_shared<TransInfo>(), return FAILED);
  trans_info_front_and_end_.push_back(trans_info_front);
  trans_info_front_and_end_.push_back(trans_info_end);
  AddTransNodeType(new (std::nothrow) TransNodeReshapeGenerator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  AddTransNodeType(new (std::nothrow) TransNodeTransposeGenerator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  AddTransNodeType(new (std::nothrow) TransNodeCastGenerator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  AddTransNodeType(new (std::nothrow) TransNodeTransdataGenerator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  AddTransNodeType(new (std::nothrow) TransNodeTransdataRNNGenerator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  AddTransNodeType(new (std::nothrow) TransNodeReformatGenerator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  AddTransNodeType(new (std::nothrow) TransNodeSqueezeV2Generator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  AddTransNodeType(new (std::nothrow) TransNodeUnsqueezeV2Generator(fe_ops_store_info_ptr_, global_trans_info_ptr_));
  trans_nodes_insertion_strategy_ = STRATEGY_CONSECUTIVE_PRINCIPLE;
  return SUCCESS;
}

TransNodeInsertion::TransNodeInsertion(FEOpsKernelInfoStorePtr fe_ops_store_ptr)
    : fe_ops_store_info_ptr_(fe_ops_store_ptr), global_trans_info_ptr_(nullptr) {}

TransNodeInsertion::~TransNodeInsertion() {
  for (auto trans_node : whole_trans_nodes_vector_) {
    delete trans_node;
  }
}

/* Check whether the parent Op Type is equal to one of the
 * element of input param op_type_list */
bool CheckParentSpecificOp(ge::OpDescPtr parent_op_desc, std::vector<string> op_type_list) {
  if (parent_op_desc->GetType() == OP_TYPE_PLACE_HOLDER || parent_op_desc->GetType() == OP_TYPE_END) {
    string parent_op_type;
    if (!ge::AttrUtils::GetStr(parent_op_desc, PARENT_OP_TYPE, parent_op_type)) {
      return false;
    }
    for (auto &expected_op_type : op_type_list) {
      if (parent_op_type == expected_op_type) {
        return true;
      }
    }
  }
  return false;
}

void SubstituteNDWithOriginalFormat(TransInfoPtr &trans_info_ptr) {
  if (trans_info_ptr->src_out_primary_format == ge::FORMAT_ND) {
    auto dst_in_origin_format = trans_info_ptr->dst_in_tensor_desc_ptr->GetOriginFormat();
    trans_info_ptr->src_out_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(dst_in_origin_format));
    trans_info_ptr->init_src_out_primary_format = trans_info_ptr->src_out_primary_format;
    trans_info_ptr->src_out_sub_format = ge::GetSubFormat(dst_in_origin_format);

    if (!IsShapeContainUnknownDimNum(trans_info_ptr->src_out_shape)) {
      trans_info_ptr->src_out_shape = trans_info_ptr->dst_in_tensor_desc_ptr->GetOriginShape();
    }
    trans_info_ptr->src_out_range = GetShapeRange(*trans_info_ptr->dst_in_tensor_desc_ptr);
  }
  if (trans_info_ptr->dst_in_primary_format == ge::FORMAT_ND) {
    auto src_out_origin_format = trans_info_ptr->src_out_tensor_desc_ptr->GetOriginFormat();
    trans_info_ptr->dst_in_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(src_out_origin_format));
    trans_info_ptr->init_dst_in_primary_format = trans_info_ptr->dst_in_primary_format;
    trans_info_ptr->dst_in_sub_format = ge::GetSubFormat(src_out_origin_format);

    if (!IsShapeContainUnknownDimNum(trans_info_ptr->src_out_shape)) {
      trans_info_ptr->dst_in_shape = trans_info_ptr->src_out_tensor_desc_ptr->GetOriginShape();
    }
    trans_info_ptr->dst_in_range = GetShapeRange(*trans_info_ptr->src_out_tensor_desc_ptr);
  }
  if (trans_info_ptr->src_out_original_format == ge::FORMAT_ND) {
    trans_info_ptr->src_out_original_format = trans_info_ptr->dst_in_tensor_desc_ptr->GetOriginFormat();
  }
}

void TransNodeInsertion::SetTransInfoForInsertionModeFront() {
  *trans_info_front_and_end_[0] = *global_trans_info_ptr_;
  auto dst_in_origin_format = global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetOriginFormat();
  trans_info_front_and_end_[0]->src_out_primary_format =
      static_cast<ge::Format>(ge::GetPrimaryFormat(dst_in_origin_format));
  trans_info_front_and_end_[0]->init_src_out_primary_format = trans_info_front_and_end_[0]->src_out_primary_format;
  trans_info_front_and_end_[0]->src_out_sub_format = ge::GetSubFormat(dst_in_origin_format);
  trans_info_front_and_end_[0]->src_out_shape = global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetOriginShape();
  trans_info_front_and_end_[0]->src_out_range = GetShapeRange(*global_trans_info_ptr_->dst_in_tensor_desc_ptr);

  trans_info_front_and_end_[0]->src_out_original_format =
      global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetOriginFormat();
  trans_info_front_and_end_[0]->src_out_original_shape =
      global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetOriginShape();
  trans_info_front_and_end_[0]->insertion_mode = INSERT_TRANS_NODE_BY_ORIGINAL_FORMAT_FRONT;
}

void TransNodeInsertion::SetTransInfoForInsertionModeEnd() {
  *trans_info_front_and_end_[1] = *global_trans_info_ptr_;
  auto src_out_origin_format = global_trans_info_ptr_->src_out_tensor_desc_ptr->GetOriginFormat();
  trans_info_front_and_end_[1]->dst_in_primary_format =
      static_cast<ge::Format>(ge::GetPrimaryFormat(src_out_origin_format));
  trans_info_front_and_end_[1]->init_dst_in_primary_format = trans_info_front_and_end_[1]->dst_in_primary_format;
  trans_info_front_and_end_[1]->dst_in_sub_format = ge::GetSubFormat(src_out_origin_format);
  trans_info_front_and_end_[1]->dst_in_shape = global_trans_info_ptr_->src_out_tensor_desc_ptr->GetOriginShape();
  trans_info_front_and_end_[1]->dst_in_range = GetShapeRange(*global_trans_info_ptr_->src_out_tensor_desc_ptr);
  trans_info_front_and_end_[1]->dst_in_original_format =
      global_trans_info_ptr_->src_out_tensor_desc_ptr->GetOriginFormat();
  trans_info_front_and_end_[1]->dst_in_original_shape =
      global_trans_info_ptr_->src_out_tensor_desc_ptr->GetOriginShape();
  trans_info_front_and_end_[1]->insertion_mode = INSERT_TRANS_NODE_BY_ORIGINAL_FORMAT_END;
}

void TransNodeInsertion::FillTransInfo(const ge::InDataAnchorPtr &dst_anchor, const ge::OutDataAnchorPtr &src_anchor,
                                       const ge::NodePtr &src_node, const ge::NodePtr &dst_node,
                                       bool &use_concecutive_principle) {
  global_trans_info_ptr_->dst_anchor = dst_anchor;
  global_trans_info_ptr_->dst_op_desc = dst_node->GetOpDesc();
  global_trans_info_ptr_->dst_imply_type = static_cast<OpImplType>(0);
  global_trans_info_ptr_->src_anchor = src_anchor;
  global_trans_info_ptr_->src_op_desc = src_node->GetOpDesc();
  global_trans_info_ptr_->src_imply_type = static_cast<OpImplType>(0);
  global_trans_info_ptr_->src_node_ptr = src_node;
  global_trans_info_ptr_->dst_node_ptr = dst_node;
  uint32_t src_anchor_index = static_cast<uint32_t>(global_trans_info_ptr_->src_anchor->GetIdx());
  uint32_t dst_anchor_index = static_cast<uint32_t>(global_trans_info_ptr_->dst_anchor->GetIdx());

  global_trans_info_ptr_->src_out_tensor_desc_ptr =
      global_trans_info_ptr_->src_op_desc->MutableOutputDesc(src_anchor_index);
  if (global_trans_info_ptr_->src_out_tensor_desc_ptr == nullptr) {
    return;
  }

  auto src_out_format = global_trans_info_ptr_->src_out_tensor_desc_ptr->GetFormat();
  global_trans_info_ptr_->src_out_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(src_out_format));
  global_trans_info_ptr_->init_src_out_primary_format = global_trans_info_ptr_->src_out_primary_format;
  global_trans_info_ptr_->src_out_sub_format = static_cast<ge::Format>(ge::GetSubFormat(src_out_format));
  global_trans_info_ptr_->src_out_data_type = global_trans_info_ptr_->src_out_tensor_desc_ptr->GetDataType();
  global_trans_info_ptr_->src_out_shape = global_trans_info_ptr_->src_out_tensor_desc_ptr->GetShape();
  global_trans_info_ptr_->src_out_range = GetShapeRange(*global_trans_info_ptr_->src_out_tensor_desc_ptr);

  /* Here we want to make sure the input of trans node is correct.
   * Shape of const variable may be not correct, so we use their successor's
   * shape. When input is place_holder we
   * need to check it's father's op type is const or variable.
   * And if its father's original shape is empty, we just use it's father's
   * current shape.
   * */
  global_trans_info_ptr_->dst_in_tensor_desc_ptr =
      global_trans_info_ptr_->dst_op_desc->MutableInputDesc(dst_anchor_index);
  if (global_trans_info_ptr_->dst_in_tensor_desc_ptr == nullptr) {
    return;
  }
  global_trans_info_ptr_->is_source_weight = CheckOpConstOrVariableInOriGraph(global_trans_info_ptr_->src_op_desc);
  global_trans_info_ptr_->is_dst_weight = CheckOpConstOrVariableInOriGraph(global_trans_info_ptr_->dst_op_desc);

  auto dst_in_format = global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetFormat();
  global_trans_info_ptr_->dst_in_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(dst_in_format));
  global_trans_info_ptr_->init_dst_in_primary_format = global_trans_info_ptr_->dst_in_primary_format;
  global_trans_info_ptr_->dst_in_sub_format = static_cast<ge::Format>(ge::GetSubFormat(dst_in_format));

  global_trans_info_ptr_->dst_in_data_type = global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetDataType();
  global_trans_info_ptr_->dst_in_shape = global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetShape();
  global_trans_info_ptr_->dst_in_range = GetShapeRange(*global_trans_info_ptr_->dst_in_tensor_desc_ptr);

  FE_LOGD("Dst node name %s type %s, Dtype1 = %s dst_anchor_index %u", dst_node->GetName().c_str(),
          dst_node->GetType().c_str(), DTypeToStr(global_trans_info_ptr_->dst_in_data_type).c_str(), dst_anchor_index);
  FE_LOGD("Src node name %s type %s, Dtype1 = %s dst_anchor_index %u", src_node->GetName().c_str(),
          src_node->GetType().c_str(), DTypeToStr(global_trans_info_ptr_->src_out_data_type).c_str(), src_anchor_index);

  global_trans_info_ptr_->src_op_desc_type = global_trans_info_ptr_->src_op_desc->GetType();
  global_trans_info_ptr_->dst_op_desc_type = global_trans_info_ptr_->dst_op_desc->GetType();

  global_trans_info_ptr_->src_out_original_shape = global_trans_info_ptr_->src_out_tensor_desc_ptr->GetOriginShape();
  global_trans_info_ptr_->src_out_original_format = global_trans_info_ptr_->src_out_tensor_desc_ptr->GetOriginFormat();

  global_trans_info_ptr_->dst_in_original_shape = global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetOriginShape();
  global_trans_info_ptr_->dst_in_original_format = global_trans_info_ptr_->dst_in_tensor_desc_ptr->GetOriginFormat();
  global_trans_info_ptr_->insertion_mode = INSERT_TRANS_NODE_BY_CONSECUTIVE_PRINCIPLE;

  int64_t hidden_size = RNN_HIDDEN_SIZE_DEFAULT_VALUE;
  int64_t input_size = RNN_INPUT_SIZE_DEFAULT_VALUE;
  int64_t state_size = RNN_STATE_SIZE_DEFAULT_VALUE;
  if (global_trans_info_ptr_->dst_in_primary_format == ge::FORMAT_FRACTAL_ZN_RNN ||
      global_trans_info_ptr_->dst_in_primary_format == ge::FORMAT_ND_RNN_BIAS) {
    (void)ge::AttrUtils::GetInt(dst_node->GetOpDesc(), "hidden_size", hidden_size);
    (void)ge::AttrUtils::GetInt(dst_node->GetOpDesc(), "input_size", input_size);
    (void)ge::AttrUtils::GetInt(dst_node->GetOpDesc(), "state_size", state_size);
  } else {
    (void)ge::AttrUtils::GetInt(src_node->GetOpDesc(), "hidden_size", hidden_size);
    (void)ge::AttrUtils::GetInt(src_node->GetOpDesc(), "input_size", input_size);
    (void)ge::AttrUtils::GetInt(src_node->GetOpDesc(), "state_size", state_size);
  }
  global_trans_info_ptr_->hidden_size = hidden_size;
  global_trans_info_ptr_->input_size = input_size;
  global_trans_info_ptr_->state_size = state_size;
  GetReshapeType();
  SubstituteNDWithOriginalFormat(global_trans_info_ptr_);
  use_concecutive_principle = IsAbleToUseConcecutivePrinciple();
  if (!use_concecutive_principle) {
    SetTransInfoForInsertionModeFront();
    SetTransInfoForInsertionModeEnd();
  }

  FE_LOGD(
      "Format of dst node is %s, format of source node is %s, src out sub format is %d, dst in sub format is %d, "
      "dst in original format is %s, src out original format is %s, use_concecutive_principle is %d.",
      FormatToStr(global_trans_info_ptr_->dst_in_primary_format).c_str(),
      FormatToStr(global_trans_info_ptr_->src_out_primary_format).c_str(), global_trans_info_ptr_->src_out_sub_format,
      global_trans_info_ptr_->dst_in_sub_format, FormatToStr(global_trans_info_ptr_->dst_in_original_format).c_str(),
      FormatToStr(global_trans_info_ptr_->src_out_original_format).c_str(), use_concecutive_principle);
}

bool TransNodeInsertion::IsAbleToUseConcecutivePrinciple() {
  /* If two consecutive nodes' original format is not the same,
   * we will insert trans-nodes by their own format and original dtype. */
  bool nz_format = global_trans_info_ptr_->src_out_primary_format == ge::FORMAT_FRACTAL_NZ ||
                   global_trans_info_ptr_->dst_in_primary_format == ge::FORMAT_FRACTAL_NZ;
  if (global_trans_info_ptr_->src_out_original_format != global_trans_info_ptr_->dst_in_original_format && !nz_format) {
    return false;
  }

  /* If the original shape of two closed tensor is not equal, we
   * use original format to insert trans-nodes */
  if (global_trans_info_ptr_->src_out_original_shape.GetDims() !=
      global_trans_info_ptr_->dst_in_original_shape.GetDims()) {
    return false;
  }

  bool dims_equal = global_trans_info_ptr_->src_out_shape.GetDims() == global_trans_info_ptr_->dst_in_shape.GetDims();

  if (nz_format && !dims_equal) {
    return false;
  }

  if (std::find(FE_GROUP_RELA_FORMAT_VECTOR.begin(), FE_GROUP_RELA_FORMAT_VECTOR.end(),
                global_trans_info_ptr_->src_out_primary_format) != FE_GROUP_RELA_FORMAT_VECTOR.end() &&
      global_trans_info_ptr_->src_out_primary_format == global_trans_info_ptr_->dst_in_primary_format && !dims_equal) {
    return false;
  }

  if (global_trans_info_ptr_->src_reshape_type != global_trans_info_ptr_->dst_reshape_type && !nz_format) {
    return false;
  }

  bool conflicting_format = (global_trans_info_ptr_->src_out_primary_format == ge::FORMAT_FRACTAL_Z &&
                             global_trans_info_ptr_->dst_in_primary_format == ge::FORMAT_FRACTAL_ZN_LSTM) ||
                            (global_trans_info_ptr_->src_out_primary_format == ge::FORMAT_FRACTAL_ZN_LSTM &&
                             global_trans_info_ptr_->dst_in_primary_format == ge::FORMAT_FRACTAL_Z);
  if (conflicting_format) {
    return false;
  }

  return true;
}

Status TransNodeInsertion::InsertTransNodes(ge::ComputeGraph &fused_graph, ge::NodePtr dst_node) {
  ge::OpDescPtr dst_op_desc = dst_node->GetOpDesc();

  for (auto &dst_anchor : dst_node->GetAllInDataAnchors()) {
    if (dst_anchor == nullptr) {
      continue;
    }
    ge::OutDataAnchorPtr src_anchor = dst_anchor->GetPeerOutAnchor();
    if (src_anchor == nullptr) {
      continue;
    }
    ge::NodePtr src_node = src_anchor->GetOwnerNode();
    if (src_node == nullptr) {
      continue;
    }
    ge::OpDescPtr src_op_desc = src_node->GetOpDesc();
    if (src_op_desc == nullptr) {
      continue;
    }

    FE_LOGD("Edge: src:%s, src_index:%d, dst:%s, dst_index: %d", src_op_desc->GetName().c_str(), src_anchor->GetIdx(),
            dst_op_desc->GetName().c_str(), dst_anchor->GetIdx());

    bool use_concecutive_principle = true;
    FillTransInfo(dst_anchor, src_anchor, src_node, dst_node, use_concecutive_principle);

    Status ret;
    if (use_concecutive_principle) {
      ret = InsertTransOpByConcecutiveStrategy(fused_graph);
      if (ret != SUCCESS) {
        return ret;
      }
    } else {
      ret = InsertTransOpByOriginalFormat(fused_graph);
      if (ret != SUCCESS) {
        return ret;
      }
    }
  }
  return SUCCESS;
}

void TransNodeInsertion::GetDstReshapeType(const OpKernelInfoPtr &op_kernel) {
  IndexNameMap input_map;
  if (GetInputIndexNameMap(*global_trans_info_ptr_->dst_op_desc, *op_kernel, input_map) == SUCCESS) {
    uint32_t in_anchor_index = static_cast<uint32_t>(global_trans_info_ptr_->dst_anchor->GetIdx());
    InputOrOutputInfoPtr intput_info_ptr = nullptr;
    if (op_kernel->GetInputInfoByName(input_map[in_anchor_index], intput_info_ptr) == SUCCESS) {
      std::string reshape_type = intput_info_ptr->GetReshapeType();
      std::string prop_reshape_type;
      (void)ge::AttrUtils::GetStr(global_trans_info_ptr_->dst_in_tensor_desc_ptr, INFER_RESHAPE_TYPE,
                                  prop_reshape_type);

      if (intput_info_ptr->GetReshapeType().empty() && !prop_reshape_type.empty()) {
        reshape_type = prop_reshape_type;
      }

      global_trans_info_ptr_->dst_reshape_type = reshape_type;
      /* Original A -> A, the reshape_type is the same for both ends */
      trans_info_front_and_end_[1]->src_reshape_type = reshape_type;
      trans_info_front_and_end_[1]->dst_reshape_type = reshape_type;
    }
  }
  global_trans_info_ptr_->dst_op_pattern = op_kernel->GetOpPattern();
  trans_info_front_and_end_[1]->src_op_pattern = op_kernel->GetOpPattern();
  trans_info_front_and_end_[1]->dst_op_pattern = op_kernel->GetOpPattern();
}

void TransNodeInsertion::GetSrcReshapeType(const OpKernelInfoPtr &op_kernel) {
  IndexNameMap output_map;
  if (GetOutputIndexNameMap(*global_trans_info_ptr_->src_op_desc, *op_kernel, output_map) == SUCCESS) {
    uint32_t out_anchor_index = static_cast<uint32_t>(global_trans_info_ptr_->src_anchor->GetIdx());
    InputOrOutputInfoPtr output_info_ptr = nullptr;
    if (op_kernel->GetOutputInfoByName(output_map[out_anchor_index], output_info_ptr) == SUCCESS) {
      std::string reshape_type = output_info_ptr->GetReshapeType();
      std::string prop_reshape_type;
      (void)ge::AttrUtils::GetStr(global_trans_info_ptr_->src_out_tensor_desc_ptr, INFER_RESHAPE_TYPE,
                                  prop_reshape_type);
      if (output_info_ptr->GetReshapeType().empty() && !prop_reshape_type.empty()) {
        reshape_type = prop_reshape_type;
      }
      global_trans_info_ptr_->src_reshape_type = reshape_type;
      /* A->Original A, the reshape_type is the same for both ends */
      trans_info_front_and_end_[0]->src_reshape_type = reshape_type;
      trans_info_front_and_end_[0]->dst_reshape_type = reshape_type;
    }
  }
  global_trans_info_ptr_->src_op_pattern = op_kernel->GetOpPattern();
  trans_info_front_and_end_[0]->src_op_pattern = op_kernel->GetOpPattern();
  trans_info_front_and_end_[0]->dst_op_pattern = op_kernel->GetOpPattern();
}

void TransNodeInsertion::InitReshapeType() {
  global_trans_info_ptr_->src_reshape_type = "";
  trans_info_front_and_end_[0]->src_reshape_type = "";
  trans_info_front_and_end_[1]->src_reshape_type = "";

  global_trans_info_ptr_->dst_reshape_type = "";
  trans_info_front_and_end_[0]->dst_reshape_type = "";
  trans_info_front_and_end_[1]->dst_reshape_type = "";

  global_trans_info_ptr_->src_op_pattern = OP_PATTERN_OP_KERNEL;
  trans_info_front_and_end_[0]->src_op_pattern = OP_PATTERN_OP_KERNEL;
  trans_info_front_and_end_[0]->dst_op_pattern = OP_PATTERN_OP_KERNEL;

  global_trans_info_ptr_->dst_op_pattern = OP_PATTERN_OP_KERNEL;
  trans_info_front_and_end_[1]->src_op_pattern = OP_PATTERN_OP_KERNEL;
  trans_info_front_and_end_[1]->dst_op_pattern = OP_PATTERN_OP_KERNEL;
}

Status TransNodeInsertion::GetReshapeType() {
  InitReshapeType();

  OpKernelInfoPtr src_op_kernel = OpsKernelManager::Instance(fe_ops_store_info_ptr_->GetFEOpsKernelInfoStoreName())
                                      .GetHighPrioOpKernelInfo(global_trans_info_ptr_->src_op_desc_type);
  if (src_op_kernel != nullptr) {
    GetSrcReshapeType(src_op_kernel);
  }

  OpKernelInfoPtr dst_op_kernel = OpsKernelManager::Instance(fe_ops_store_info_ptr_->GetFEOpsKernelInfoStoreName())
                                      .GetHighPrioOpKernelInfo(global_trans_info_ptr_->dst_op_desc_type);
  if (dst_op_kernel != nullptr) {
    GetDstReshapeType(dst_op_kernel);
  }
  return SUCCESS;
}

/* For normal node if the source data type is fp16 we insert cast at the
 * end, other wise we insert cast first */
Status InsertCastGeneralCase(TransInfoPtr trans_info_ptr,
                             std::vector<std::vector<uint32_t>> &strategy_vector_combination,
                             uint32_t front_strategy_vector_index, uint32_t end_strategy_vector_index) {
  if (strategy_vector_combination.empty()) {
    FE_LOGW("Combination of strayegy is empty.");
    return FAILED;
  }

  if (end_strategy_vector_index >= strategy_vector_combination.size() ||
      front_strategy_vector_index >= strategy_vector_combination.size()) {
    FE_LOGW("End index %u or front index %u is invalid. Size is %zu", end_strategy_vector_index,
            front_strategy_vector_index, strategy_vector_combination.size());
    return FAILED;
  }

  if (trans_info_ptr->src_out_tensor_desc_ptr->GetDataType() != ge::DT_FLOAT16) {
    FE_LOGD("Insert Cast at the beginning.");
    strategy_vector_combination[end_strategy_vector_index].insert(
        strategy_vector_combination[end_strategy_vector_index].begin(), CAST_INDEX);
  } else {
    FE_LOGD("Insert Cast at the end.");
    strategy_vector_combination[front_strategy_vector_index].push_back(CAST_INDEX);
  }
  return SUCCESS;
}

Status TransNodeInsertion::CombineAllStrategy(TransInfoPtr trans_info_ptr, uint64_t global_strategy_id,
                                              std::vector<std::vector<uint32_t>> &strategy_vector_combination) {
  (void) global_strategy_id;
  if (strategy_vector_combination.empty()) {
    FE_LOGW("Combination of strayegy is empty.");
    return FAILED;
  }

  if (trans_info_ptr->src_out_data_type != trans_info_ptr->dst_in_data_type) {
    uint32_t front_strategy_vector_index = 0;
    uint32_t end_strategy_vector_index = strategy_vector_combination.size() - 1;

    return InsertCastGeneralCase(trans_info_ptr, strategy_vector_combination, front_strategy_vector_index,
                                 end_strategy_vector_index);
  }

  return SUCCESS;
}

Status TransNodeInsertion::GenerateStrategy(std::vector<std::vector<uint32_t>> &strategy_vector_combination) {
  uint64_t strategy_ext_val = CalcStrategyIdExtraVal(global_trans_info_ptr_);
  uint64_t strategy_id = CalcStrategyId(global_trans_info_ptr_->src_out_primary_format,
                                        global_trans_info_ptr_->dst_in_primary_format, strategy_ext_val);

  global_trans_info_ptr_->strategy_id = strategy_id;

  auto strategy = trans_nodes_insertion_strategy_.find(strategy_id);
  if (strategy == trans_nodes_insertion_strategy_.end()) {
    FE_LOGW("Trans situation is not supported, src format %u, dst format %u.",
            global_trans_info_ptr_->src_out_primary_format, global_trans_info_ptr_->dst_in_primary_format);
    return FAILED;
  }
  if (strategy_vector_combination.empty()) {
    FE_LOGW("Stategy Vector is empty!");
    return FAILED;
  }
  strategy_vector_combination[0] = strategy->second;
  return CombineAllStrategy(global_trans_info_ptr_, strategy_id, strategy_vector_combination);
}

Status TransNodeInsertion::InsertTransOpByConcecutiveStrategy(ge::ComputeGraph &fused_graph) {
  if (global_trans_info_ptr_->src_out_primary_format == global_trans_info_ptr_->dst_in_primary_format &&
      global_trans_info_ptr_->src_out_data_type == global_trans_info_ptr_->dst_in_data_type) {
    FE_LOGD("Format and data type of source node %s and dst node %s is same.",
            global_trans_info_ptr_->src_op_desc_type.c_str(), global_trans_info_ptr_->dst_op_desc_type.c_str());
    return SUCCESS;
  }

  /* Lower dimensional op to higher dimension op:
   * Reshape -> Permute -> Cast -> TransData -> TransDataRNN;
   * higher dimensional op to lower(or equal) dimension op:
   * TransDataRNN -> TransData -> Cast -> Permute -> Reshape. */
  std::vector<std::vector<uint32_t>> strategy_vector_combination;
  strategy_vector_combination.emplace_back(std::vector<uint32_t>());
  GenerateStrategy(strategy_vector_combination);
  Status ret_value;
  for (auto transnode_idx : strategy_vector_combination[0]) {
    if (transnode_idx >= FORBIDDEN_INDEX) {
      REPORT_FE_ERROR("[GraphOpt][Trans][InsertTransByConcec] We do not support trans from %s to %s between %s and %s.",
                      FormatToStr(global_trans_info_ptr_->src_out_primary_format).c_str(),
                      FormatToStr(global_trans_info_ptr_->dst_in_primary_format).c_str(),
                      global_trans_info_ptr_->src_op_desc->GetName().c_str(),
                      global_trans_info_ptr_->dst_op_desc->GetName().c_str());
      return FAILED;
    }
    ret_value = whole_trans_nodes_vector_[transnode_idx]->AddTransNode(fused_graph, global_trans_info_ptr_);
    if (ret_value != SUCCESS) {
      return ret_value;
    }
  }
  return SUCCESS;
}

Status TransNodeInsertion::UpdateNextTransInfo(uint32_t end_strategy_index) {
  if (end_strategy_index < 1) {
    return FAILED;
  }
  auto front_strategy_index = end_strategy_index - 1;
  trans_info_front_and_end_[front_strategy_index]->src_op_desc =
      trans_info_front_and_end_[end_strategy_index]->src_op_desc;
  trans_info_front_and_end_[front_strategy_index]->src_node_ptr =
      trans_info_front_and_end_[end_strategy_index]->src_node_ptr;
  trans_info_front_and_end_[front_strategy_index]->src_anchor =
      trans_info_front_and_end_[end_strategy_index]->src_anchor;
  trans_info_front_and_end_[front_strategy_index]->src_op_desc_type =
      trans_info_front_and_end_[end_strategy_index]->src_op_desc_type;

  trans_info_front_and_end_[front_strategy_index]->src_out_data_type =
      trans_info_front_and_end_[end_strategy_index]->src_out_data_type;

  trans_info_front_and_end_[front_strategy_index]->is_source_weight =
      trans_info_front_and_end_[end_strategy_index]->is_source_weight;

  trans_info_front_and_end_[front_strategy_index]->src_out_tensor_desc_ptr =
      trans_info_front_and_end_[end_strategy_index]->src_out_tensor_desc_ptr;
  return SUCCESS;
}

Status TransNodeInsertion::InsertTransOpByOriginalFormat(ge::ComputeGraph &fused_graph) {
  /* 1st strategy is the front strategy, which inserts trans-nodes
   * from input's original format to the current format.
   * 2nd strategy is the end strategy, which inserts trans-nodes
   * from output's current format to its original format. */
  vector<vector<uint32_t>> strategy_vector_combination;
  strategy_vector_combination.emplace_back(vector<uint32_t>());
  strategy_vector_combination.emplace_back(vector<uint32_t>());

  GenerateStrategyByOrignalFormat(strategy_vector_combination);
  if (strategy_vector_combination.empty()) {
    FE_LOGW("The strategy vector is empty!");
    return FAILED;
  }
  Status ret_value;
  auto combination_size = strategy_vector_combination.size();
  /* We need to insert trans-nodes by END strategy first then
   * by FRONT strategy. So we insert trans-nodes reversely from index size-1,
   * size-2 ... 0 through vector strategy_vector_combination */
  for (uint32_t i = 0; i < combination_size; i++) {
    auto strategy_index = (combination_size - 1) - i;
    auto strategy_vector = strategy_vector_combination[strategy_index];
    FE_LOGD("Step %zu from format %s to format %s", strategy_index,
            FormatToStr(trans_info_front_and_end_[strategy_index]->src_out_primary_format).c_str(),
            FormatToStr(trans_info_front_and_end_[strategy_index]->dst_in_primary_format).c_str());
    for (auto transnode_idx : strategy_vector) {
      if (transnode_idx >= FORBIDDEN_INDEX) {
        REPORT_FE_ERROR("[GraphOpt][Trans][InsertTransByOri] We do not support trans from %s to %s between %s and %s.",
                        FormatToStr(global_trans_info_ptr_->src_out_primary_format).c_str(),
                        FormatToStr(global_trans_info_ptr_->dst_in_primary_format).c_str(),
                        global_trans_info_ptr_->src_op_desc->GetName().c_str(),
                        global_trans_info_ptr_->dst_op_desc->GetName().c_str());
        return FAILED;
      }
      ret_value = whole_trans_nodes_vector_[transnode_idx]->AddTransNode(fused_graph,
                                                                         trans_info_front_and_end_[strategy_index]);
      if (ret_value != SUCCESS) {
        return ret_value;
      }
    }
    /* After insert trans-nodes from src current to src original,
     * we need to update the src-node for insertion from dst original to dst
     * current. */
    if (i == 0) {
      UpdateNextTransInfo(strategy_index);
    }
  }

  return SUCCESS;
}

Status TransNodeInsertion::GenerateStrategyByOrignalFormat(
    std::vector<std::vector<uint32_t>> &strategy_vector_combination) {
  for (size_t i = 0; i < strategy_vector_combination.size(); i++) {
    if (i >= trans_info_front_and_end_.size()) {
      FE_LOGW("Index %zu is larger than the size of trans-info %zu.", i, trans_info_front_and_end_.size());
      return FAILED;
    }
    uint64_t strategy_ext_val = CalcStrategyIdExtraVal(trans_info_front_and_end_[i]);
    uint64_t strategy_id = CalcStrategyId(trans_info_front_and_end_[i]->src_out_primary_format,
                                          trans_info_front_and_end_[i]->dst_in_primary_format, strategy_ext_val);

    trans_info_front_and_end_[i]->strategy_id = strategy_id;

    auto strategy = trans_nodes_insertion_strategy_.find(strategy_id);
    if (strategy == trans_nodes_insertion_strategy_.end()) {
      FE_LOGW("Trans situation is not supported, src format %u, dst format %u.",
              global_trans_info_ptr_->src_out_primary_format, global_trans_info_ptr_->dst_in_primary_format);
      return FAILED;
    }
    strategy_vector_combination[i] = strategy->second;
  }
  /* This ID is created by the source nodes' and dst nodes. */
  uint64_t global_strategy_ext_val = CalcStrategyIdExtraVal(global_trans_info_ptr_);
  uint64_t global_strategy_id = CalcStrategyId(global_trans_info_ptr_->src_out_primary_format,
                                               global_trans_info_ptr_->dst_in_primary_format, global_strategy_ext_val);

  return CombineAllStrategy(global_trans_info_ptr_, global_strategy_id, strategy_vector_combination);
}
}  // namespace fe