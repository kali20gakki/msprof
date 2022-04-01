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

#include "graph/preprocess/graph_preprocess.h"
#include <map>
#include <set>
#include <string>
#include "common/formats/format_transfers/format_transfer_fractal_nz.h"
#include "common/formats/format_transfers/format_transfer_nchw_nc1hwc0.h"
#include "common/formats/format_transfers/format_transfer_nhwc_nc1hwc0.h"
#include "common/formats/format_transfers/format_transfer_transpose.h"
#include "common/formats/utils/formats_trans_utils.h"
#include "framework/common/helper/model_helper.h"
#include "common/math/math_util.h"
#include "framework/common/op/ge_op_utils.h"
#include "ir_build/option_utils.h"
#include "common/ge_call_wrapper.h"
#include "common/local_context.h"
#include "common/transop_util.h"
#include "graph/common/trans_op_creator.h"
#include "graph/ge_context.h"
#include "graph/shape_refiner.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/util/rt_context_util.h"
#include "graph/optimize/graph_optimize.h"
#include "graph/passes/addn_pass.h"
#include "graph/passes/aicpu_constant_folding_pass.h"
#include "graph/passes/assert_pass.h"
#include "graph/passes/common_subexpression_elimination_pass.h"
#include "graph/passes/cond_pass.h"
#include "graph/passes/cond_remove_pass.h"
#include "graph/passes/constant_folding_pass.h"
#include "graph/passes/dimension_adjust_pass.h"
#include "graph/passes/dimension_compute_pass.h"
#include "graph/passes/split_shape_n_pass.h"
#include "graph/passes/potential_const_taken_effect_pass.h"
#include "graph/passes/dropout_pass.h"
#include "graph/passes/enter_pass.h"
#include "graph/passes/for_pass.h"
#include "graph/passes/guarantee_const_pass.h"
#include "graph/passes/hccl_memcpy_pass.h"
#include "graph/passes/hccl_group_pass.h"
#include "graph/passes/identity_pass.h"
#include "graph/passes/infershape_pass.h"
#include "graph/passes/infer_value_range_pass.h"
#include "graph/passes/merge_pass.h"
#include "graph/passes/net_output_pass.h"
#include "graph/passes/no_use_reshape_remove_pass.h"
#include "graph/passes/parallel_concat_start_op_pass.h"
#include "graph/passes/placeholder_with_default_pass.h"
#include "graph/passes/prevent_gradient_pass.h"
#include "graph/passes/print_op_pass.h"
#include "graph/passes/prune_pass.h"
#include "graph/passes/replace_transshape_pass.h"
#include "graph/passes/replace_with_empty_const_pass.h"
#include "graph/passes/save_pass.h"
#include "graph/passes/shape_operate_op_remove_pass.h"
#include "graph/passes/snapshot_pass.h"
#include "graph/passes/stop_gradient_pass.h"
#include "graph/passes/switch_dead_branch_elimination.h"
#include "graph/passes/unused_const_pass.h"
#include "graph/passes/var_is_initialized_op_pass.h"
#include "graph/passes/variable_prepare_op_pass.h"
#include "graph/passes/mark_force_unknown_for_cond_pass.h"
#include "graph/preprocess/insert_op/util_insert_aipp_op.h"
#include "graph/utils/type_utils.h"
#include "inc/pass_manager.h"
#include "init/gelib.h"
#include "graph/preprocess/multi_batch_copy_graph.h"
#include "graph/passes/data_pass.h"
#include "graph/passes/mark_agnostic_pass.h"
#include "inc/expand_dimension.h"
#include "inc/transfer_shape_according_to_format.h"
#include "inc/transfer_range_according_to_format.h"

namespace ge {
namespace {
static std::map<std::string, ge::DataType> output_type_str_to_datatype = {
    {"FP32", ge::DT_FLOAT},    {"FP16", ge::DT_FLOAT16},  {"INT8", ge::DT_INT8},    {"INT16", ge::DT_INT16},
    {"UINT16", ge::DT_UINT16}, {"UINT8", ge::DT_UINT8},   {"INT32", ge::DT_INT32},  {"INT64", ge::DT_INT64},
    {"UINT32", ge::DT_UINT32}, {"UINT64", ge::DT_UINT64}, {"DOUBLE", ge::DT_DOUBLE}};

const char *const kMbatchSwitchnName = "mbatch-switch-name";

// the size of user defined output datatype or format std::string after split by ":".
const size_t kUserDefinedElementCount = 2;
const int32_t kDataOutIndex = 0;
const int64_t kInvalidDynaimcDimsType = -1;

OpDescPtr CreateTensorShape(const GeTensorDesc &data_tensor) {
  GeTensorPtr tensor = MakeShared<GeTensor>();
  if (tensor == nullptr) {
    REPORT_CALL_ERROR("E19999", "New GeTensor failed");
    GELOGE(INTERNAL_ERROR, "[New][GeTensor] failed");
    return nullptr;
  }
  tensor->MutableTensorDesc().SetDataType(DT_INT32);
  tensor->MutableTensorDesc().SetFormat(FORMAT_ND);
  auto dst_ge_shape = data_tensor.GetShape();
  auto dim_cnt = static_cast<int64_t>(dst_ge_shape.GetDimNum());
  if (dim_cnt == 0) {  // if the dim_cnt is 0, the tensor is a scalar
    tensor->MutableTensorDesc().SetShape(GeShape());
    int32_t dst_shape = 1;
    if (tensor->SetData(reinterpret_cast<const uint8_t *>(&dst_shape), sizeof(int32_t)) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Set data to tensor failed");
      GELOGE(INTERNAL_ERROR, "[Set][Data] to tensor failed");
      return nullptr;
    }
  } else {
    tensor->MutableTensorDesc().SetShape(GeShape(std::vector<int64_t>({dim_cnt})));
    unique_ptr<int32_t[]> dst_shape(new (std::nothrow) int32_t[dim_cnt]());
    if (dst_shape == nullptr) {
      REPORT_CALL_ERROR("E19999", "Malloc buffer failed, size:%zu", dim_cnt);
      GELOGE(INTERNAL_ERROR, "[Malloc][Buffer] failed, size:%zu", dim_cnt);
      return nullptr;
    }
    for (int64_t i = 0; i < dim_cnt; ++i) {
      dst_shape[i] = dst_ge_shape.GetDim(static_cast<size_t>(i));
    }

    GE_IF_BOOL_EXEC(
        tensor->SetData(reinterpret_cast<const uint8_t *>(dst_shape.get()), dim_cnt * sizeof(int32_t)) != GRAPH_SUCCESS,
        REPORT_CALL_ERROR("E19999", "Set data to tensor failed");
        GELOGE(INTERNAL_ERROR, "[Set][Data] to tensor failed");
        return nullptr);
  }

  GELOGD("Create shape input dim [%s]", dst_ge_shape.ToString().c_str());
  return OpDescUtils::CreateConstOp(tensor);
}

OpDescPtr CreateTransOp(const string &op_name, const string &op_type, const GeTensorDesc &input_desc,
                        const GeTensorDesc &output_desc) {
  if (op_type == TRANSDATA) {
    return TransOpCreator::CreateTransDataOp(op_name, input_desc.GetFormat(), output_desc.GetFormat());
  } else if (op_type == TRANSPOSED) {
    return TransOpCreator::CreateTransPoseDOp(op_name, input_desc.GetFormat(), output_desc.GetFormat());
  } else if (op_type == CAST) {
    return TransOpCreator::CreateCastOp(op_name, input_desc.GetDataType(), output_desc.GetDataType());
  }
  // no need to set attrs.
  return MakeShared<OpDesc>(op_name, op_type);
}

NodePtr CreateTransNode(const std::string &name, const std::string &node_type, const GeTensorDesc &input,
                        const GeTensorDesc &output, NodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, trans_name:%s, trans_type:%s, check invalid",
                       name.c_str(), node_type.c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] Param node is nullptr, trans_name:%s, trans_type:%s",
           name.c_str(), node_type.c_str());
    return nullptr;
  }
  auto graph = node->GetOwnerComputeGraph();
  if (graph == nullptr) {
    REPORT_INNER_ERROR("E19999", "Owner graph in node is nullptr, trans_name:%s, trans_type:%s, check invalid",
                       name.c_str(), node_type.c_str());
    GELOGE(PARAM_INVALID, "[Get][OwnerGraph] in node is nullptr, trans_name:%s, trans_type:%s",
           name.c_str(), node_type.c_str());
    return nullptr;
  }

  auto index = TransOpUtil::GetTransOpDataIndex(node_type);
  if (index < 0) {
    REPORT_INNER_ERROR("E19999", "The trans node type %s does not exist, it must be %s",
                       node_type.c_str(), TransOpUtil::TransopMapToString().c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] The trans node type %s does not exist.", node_type.c_str());
    return nullptr;
  }
  OpDescPtr op_desc = CreateTransOp(name, node_type, input, output);
  if (op_desc == nullptr) {
    REPORT_CALL_ERROR("E19999", "New OpDesc failed, trans_name:%s, trans_type:%s",
                      name.c_str(), node_type.c_str());
    GELOGE(INTERNAL_ERROR, "[New][OpDesc] failed, trans_name:%s, trans_type:%s",
           name.c_str(), node_type.c_str());
    return nullptr;
  }

  // for data dump
  GE_IF_BOOL_EXEC(
      !AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::move(std::vector<std::string>())),
      GELOGW("CreateTransNode: SetListStr failed");)

  // Default single input and single output
  auto ret = op_desc->AddInputDesc(input);
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add input desc into op:%s(%s) failed",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Add][InputDesc] into op:%s(%s) failed",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return nullptr;
  }
  ret = op_desc->AddOutputDesc(output);
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add output desc into op:%s(%s) failed",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Add][OutputDesc] into op:%s(%s) failed",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return nullptr;
  }

  NodePtr shape_node = nullptr;
  if (node_type == RESHAPE) {
    auto shape_desc = CreateTensorShape(output);
    if (shape_desc == nullptr) {
      GELOGE(INTERNAL_ERROR, "[Create][TensorShape] Failed to add shape for reshape %s",
             node->GetName().c_str());
      return nullptr;
    }
    ret = op_desc->AddInputDesc(shape_desc->GetOutputDesc(0));
    if (ret != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add input desc into op:%s(%s) failed",
                        op_desc->GetName().c_str(), op_desc->GetType().c_str());
      GELOGE(INTERNAL_ERROR, "[Add][InputDesc] into op:%s(%s) failed",
             op_desc->GetName().c_str(), op_desc->GetType().c_str());
      return nullptr;
    }

    shape_node = graph->AddNode(shape_desc);
    if (shape_node == nullptr) {
      REPORT_CALL_ERROR("E19999", "Add node:%s(%s) to graph:%s failed",
                        shape_desc->GetName().c_str(), shape_desc->GetType().c_str(),
                        graph->GetName().c_str());
      GELOGE(INTERNAL_ERROR, "[Add][Node] %s(%s) to graph:%s failed",
             shape_desc->GetName().c_str(), shape_desc->GetType().c_str(), graph->GetName().c_str());
      return nullptr;
    }
  }

  auto trans_node = graph->AddNode(op_desc);
  if (trans_node == nullptr) {
    REPORT_CALL_ERROR("E19999", "Add node:%s(%s) to graph:%s failed",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                      graph->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[Add][Node] %s(%s) to graph:%s failed",
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), graph->GetName().c_str());
    return nullptr;
  }

  if (node_type == RESHAPE) {
    if (GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0), trans_node->GetInDataAnchor(1)) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(out_index:0) and op:%s(%s)(in_index:1) failed",
                        shape_node->GetName().c_str(), shape_node->GetType().c_str(),
                        trans_node->GetName().c_str(), trans_node->GetType().c_str());
      GELOGE(INTERNAL_ERROR, "[Add][Edge] between op:%s(%s)(out_index:0) and op:%s(%s)(in_index:1) failed",
             shape_node->GetName().c_str(), shape_node->GetType().c_str(),
             trans_node->GetName().c_str(), trans_node->GetType().c_str());
      return nullptr;
    }
  }

  return trans_node;
}

Status RecoverOneTransNodeForVar(const std::string &name, const TransNodeInfo &trans_node_info, NodePtr node,
                                 NodePtr &trans_node) {
  GE_CHECK_NOTNULL(node);
  trans_node = CreateTransNode(name, trans_node_info.node_type, trans_node_info.output, trans_node_info.input, node);
  if (trans_node == nullptr) {
    return INTERNAL_ERROR;
  }

  auto ret = GraphUtils::ReplaceNodeDataAnchors(trans_node, node, {}, {0});
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Replace out anchors of node:%s(%s) by node:%s(%s) failed",
                      node->GetName().c_str(), node->GetType().c_str(),
                      trans_node->GetName().c_str(), trans_node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Replace][OutAnchors] of node:%s(%s) by node:%s(%s) failed",
           node->GetName().c_str(), node->GetType().c_str(),
           trans_node->GetName().c_str(), trans_node->GetType().c_str());
    return INTERNAL_ERROR;
  }

  ret = GraphUtils::AddEdge(node->GetOutDataAnchor(0), trans_node->GetInDataAnchor(0));
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(out_index:0) and op:%s(%s)(in_index:0) failed",
                      node->GetName().c_str(), node->GetType().c_str(),
                      trans_node->GetName().c_str(), trans_node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Add][Edge] between op:%s(%s)(out_index:0) and op:%s(%s)(in_index:0) failed",
           node->GetName().c_str(), node->GetType().c_str(),
           trans_node->GetName().c_str(), trans_node->GetType().c_str());
    return INTERNAL_ERROR;
  }

  ret = GraphUtils::MoveOutCtrlEdges(node, trans_node);
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Move out control edges from node:%s(%s) to node:%s(%s) failed",
                      node->GetName().c_str(), node->GetType().c_str(),
                      trans_node->GetName().c_str(), trans_node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[MoveOut][ControlEdges] from node:%s(%s) to node:%s(%s) failed",
           node->GetName().c_str(), node->GetType().c_str(),
           trans_node->GetName().c_str(), trans_node->GetType().c_str());
    return INTERNAL_ERROR;
  }

  return SUCCESS;
}

Status RecoverOneTransNodeForVarRef(const std::string &name, const TransNodeInfo &trans_node_info, NodePtr node,
                                    NodePtr &trans_node) {
  GE_CHECK_NOTNULL(node);
  trans_node = CreateTransNode(name, trans_node_info.node_type, trans_node_info.input, trans_node_info.output, node);
  if (trans_node == nullptr) {
    return INTERNAL_ERROR;
  }

  auto ret = GraphUtils::ReplaceNodeDataAnchors(trans_node, node, {0}, {});
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Replace out anchors of node:%s(%s) by node:%s(%s) failed",
                      node->GetName().c_str(), node->GetType().c_str(),
                      trans_node->GetName().c_str(), trans_node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Replace][OutAnchors] of node:%s(%s) by node:%s(%s) failed",
           node->GetName().c_str(), node->GetType().c_str(),
           trans_node->GetName().c_str(), trans_node->GetType().c_str());
    return INTERNAL_ERROR;
  }

  ret = GraphUtils::AddEdge(trans_node->GetOutDataAnchor(0), node->GetInDataAnchor(0));
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(out_index:0) and op:%s(%s)(in_index:0) failed",
                      trans_node->GetName().c_str(), trans_node->GetType().c_str(),
                      node->GetName().c_str(), node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Add][Edge] between op:%s(%s)(out_index:0) and op:%s(%s)(in_index:0) failed",
           trans_node->GetName().c_str(), trans_node->GetType().c_str(),
           node->GetName().c_str(), node->GetType().c_str());
    return INTERNAL_ERROR;
  }

  ret = GraphUtils::MoveInCtrlEdges(node, trans_node);
  if (ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Move in control edges from node:%s(%s) to node:%s(%s) failed",
                      node->GetName().c_str(), node->GetType().c_str(),
                      trans_node->GetName().c_str(), trans_node->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[MoveIn][CtrlEdges] from node:%s(%s) to node:%s(%s) failed",
           node->GetName().c_str(), node->GetType().c_str(),
           trans_node->GetName().c_str(), trans_node->GetType().c_str());
    return INTERNAL_ERROR;
  }

  return SUCCESS;
}

Status UpdateVarFormats(const NodePtr &var, const GeTensorDesc &tensor_desc) {
  GE_IF_BOOL_EXEC(var == nullptr, GELOGW("node : var is nullptr"); return INTERNAL_ERROR);
  GE_CHECK_NOTNULL(var->GetOpDesc());
  if (var->GetOpDesc()->GetOutputsSize() > 0) {
    auto output_desc = var->GetOpDesc()->GetOutputDesc(0);
    output_desc.SetFormat(tensor_desc.GetFormat());
    output_desc.SetDataType(tensor_desc.GetDataType());
    output_desc.SetShape(tensor_desc.GetShape());
    output_desc.SetOriginFormat(tensor_desc.GetOriginFormat());
    output_desc.SetOriginDataType(tensor_desc.GetOriginDataType());
    output_desc.SetOriginShape(tensor_desc.GetOriginShape());
    GE_IF_BOOL_EXEC(var->GetOpDesc()->UpdateOutputDesc(0, output_desc) != GRAPH_SUCCESS,
                    REPORT_CALL_ERROR("E19999", "Update output desc of node:%s(%s) failed, index:0,",
                                      var->GetName().c_str(), var->GetType().c_str());
                    GELOGE(INTERNAL_ERROR, "[Update][OutputDesc] of node:%s(%s) failed, index:0,",
                           var->GetName().c_str(), var->GetType().c_str());
                    return INTERNAL_ERROR;);
  }

  if (var->GetOpDesc()->GetInputsSize() > 0) {
    auto desc = var->GetOpDesc()->GetInputDesc(0);
    desc.SetFormat(tensor_desc.GetFormat());
    desc.SetDataType(tensor_desc.GetDataType());
    desc.SetShape(tensor_desc.GetShape());
    desc.SetOriginFormat(tensor_desc.GetOriginFormat());
    desc.SetOriginDataType(tensor_desc.GetOriginDataType());
    desc.SetOriginShape(tensor_desc.GetOriginShape());
    GE_IF_BOOL_EXEC(var->GetOpDesc()->UpdateInputDesc(0, desc) != GRAPH_SUCCESS,
                    REPORT_CALL_ERROR("E19999", "Update input desc of node:%s(%s) failed, index:0,",
                                      var->GetName().c_str(), var->GetType().c_str());
                    GELOGE(INTERNAL_ERROR, "[Update][InputDesc] of node:%s(%s) failed, index:0,",
                           var->GetName().c_str(), var->GetType().c_str());
                    return INTERNAL_ERROR;)
  }
  return SUCCESS;
}

Status RecoverTransRoadForVar(const NodePtr &var, const VarTransRoad &road) {
  GE_CHECK_NOTNULL(var);
  static std::atomic_int index(0);
  NodePtr last_node = var;
  for (auto iter = road.rbegin(); iter != road.rend(); ++iter) {
    auto trans_name = var->GetName() + "_trans_" + std::to_string(index++);
    auto ret = RecoverOneTransNodeForVar(trans_name, *iter, last_node, last_node);
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Failed to recover trans node for variable %s, index %s, type %s",
                        var->GetName().c_str(), std::to_string(index).c_str(), iter->node_type.c_str());
      GELOGE(INTERNAL_ERROR, "[Recover][TransNode] for variable %s, index %s, type %s", var->GetName().c_str(),
             std::to_string(index).c_str(), iter->node_type.c_str());
      return INTERNAL_ERROR;
    }
    // set stream_label
    OpDescPtr var_desc = var->GetOpDesc();
    GE_CHECK_NOTNULL(var_desc);
    std::string stream_label;
    (void)AttrUtils::GetStr(var_desc, ATTR_NAME_STREAM_LABEL, stream_label);
    if (!stream_label.empty()) {
      auto status = SetStreamLabel(last_node, stream_label);
      if (status != ge::SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Set stream_label:%s to op:%s(%s) failed",
                          stream_label.c_str(), last_node->GetName().c_str(), last_node->GetType().c_str());
        GELOGE(status, "[Set][StreamLabel] %s to op:%s(%s) failed.",
               stream_label.c_str(), last_node->GetName().c_str(), last_node->GetType().c_str());
        return status;
      }
    }
    GE_CHK_BOOL_EXEC((ge::AttrUtils::SetBool(last_node->GetOpDesc(), ge::ATTR_INSERTED_BY_GE, true)),
                     REPORT_CALL_ERROR("E19999", "Set Attr:%s to node:%s(%s) failed",
                                       ge::ATTR_INSERTED_BY_GE.c_str(),
                                       last_node->GetName().c_str(), last_node->GetType().c_str());
                     return INTERNAL_ERROR,
                     "[Set][Attr] %s to node:%s(%s) failed", ge::ATTR_INSERTED_BY_GE.c_str(),
                     last_node->GetName().c_str(), last_node->GetType().c_str());
    GELOGD("Recover trans node %s type %s success", trans_name.c_str(), iter->node_type.c_str());
  }
  if (road.empty()) {
    return SUCCESS;
  }
  return UpdateVarFormats(var, road.rbegin()->output);
}

Status RecoverTransRoadForVarRef(const std::set<NodePtr> &nodes, const VarTransRoad &road) {
  for (auto &var : nodes) {
    GE_CHECK_NOTNULL(var);
    static std::atomic_int index(0);
    NodePtr last_node = var;
    GELOGI("Recover trans nodes for variable ref %s", var->GetName().c_str());
    for (auto iter = road.rbegin(); iter != road.rend(); ++iter) {
      auto trans_name = var->GetName() + "_trans_" + std::to_string(index++);
      auto ret = RecoverOneTransNodeForVarRef(trans_name, *iter, last_node, last_node);
      if (ret != SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Failed to recover trans node for variable %s, index %s, type %s",
                          var->GetName().c_str(), std::to_string(index).c_str(), iter->node_type.c_str());
        GELOGE(INTERNAL_ERROR, "[Recover][TransNode] for variable %s failed, index %s, type %s",
               var->GetName().c_str(), std::to_string(index).c_str(), iter->node_type.c_str());
        return INTERNAL_ERROR;
      }
      // set stream_label
      OpDescPtr var_desc = var->GetOpDesc();
      GE_CHECK_NOTNULL(var_desc);
      std::string stream_label;
      (void)AttrUtils::GetStr(var_desc, ATTR_NAME_STREAM_LABEL, stream_label);
      if (!stream_label.empty()) {
        auto status = SetStreamLabel(last_node, stream_label);
        if (status != ge::SUCCESS) {
          REPORT_CALL_ERROR("E19999", "Set stream_label:%s to op:%s(%s) failed",
                            stream_label.c_str(), last_node->GetName().c_str(), last_node->GetType().c_str());
          GELOGE(status, "[Set][StreamLabel] %s to op:%s(%s) failed.",
                 stream_label.c_str(), last_node->GetName().c_str(), last_node->GetType().c_str());
          return status;
        }
      }

      GE_CHK_BOOL_EXEC((ge::AttrUtils::SetBool(last_node->GetOpDesc(), ge::ATTR_INSERTED_BY_GE, true)),
                       REPORT_CALL_ERROR("E19999", "Set Attr:%s of node:%s(%s) failed",
                                         ge::ATTR_INSERTED_BY_GE.c_str(),
                                         last_node->GetName().c_str(), last_node->GetType().c_str());
                       return INTERNAL_ERROR,
                       "[Set][Attr] %s of node:%s(%s) failed", ge::ATTR_INSERTED_BY_GE.c_str(),
                       last_node->GetName().c_str(), last_node->GetType().c_str());
    }
    if (!(road.empty()) && (UpdateVarFormats(var, road.rbegin()->output) != SUCCESS)) {
      return INTERNAL_ERROR;
    }
  }
  return SUCCESS;
}

using VarNamesToRefs = std::map<std::string, std::set<NodePtr>>;

VarNamesToRefs CollectVarNamesToRefs(const ComputeGraphPtr &graph) {
  VarNamesToRefs names_to_refs;
  std::string var_name;
  if (graph == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param graph is nullptr, check invalid");
    GELOGE(PARAM_INVALID, "[Check][Param] graph is nullptr.");
    return names_to_refs;
  }
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() != VARIABLE) {
      continue;
    }
    if (AttrUtils::GetStr(node->GetOpDesc(), REF_VAR_SRC_VAR_NAME, var_name)) {
      (void)names_to_refs[var_name].insert(node);
    }
  }
  return names_to_refs;
}

Status TransferShape2NC1HWC0(Format src_format, const std::vector<int64_t> &src_shape, DataType dt, Format dst_format,
                             std::vector<int64_t> &dst_shape) {
  if (src_format == FORMAT_NCHW) {
    formats::FormatTransferNchwNc1hwc0 transfer;
    if (transfer.TransShape(src_format, src_shape, dt, dst_format, dst_shape) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Trans][Shape] failed");
      return FAILED;
    }
  } else if (src_format == FORMAT_NHWC) {
    formats::FormatTransferNhwcNc1hwc0 transfer;
    if (transfer.TransShape(src_format, src_shape, dt, dst_format, dst_shape) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Trans][Shape] failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status ModifyInputFormatAndShape(const NodePtr &node_ptr) {
  GE_CHECK_NOTNULL(node_ptr);
  auto op_desc = node_ptr->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  const GeTensorDescPtr &input = op_desc->MutableInputDesc(0);
  GE_CHECK_NOTNULL(input);
  ge::Format old_format = input->GetFormat();
  std::vector<int64_t> old_shape = input->GetShape().GetDims();
  ge::DataType dt = input->GetDataType();
  std::vector<int64_t> dst_shape_dims;
  if (TransferShape2NC1HWC0(old_format, old_shape, dt, FORMAT_NC1HWC0, dst_shape_dims) != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Transfer shape to NC1HWC0 failed, op:%s(%s),",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Transfer][Shape] to NC1HWC0 failed, op:%s(%s),",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return FAILED;
  }

  input->SetFormat(FORMAT_NC1HWC0);
  input->SetShape(ge::GeShape(dst_shape_dims));

  auto output = op_desc->MutableOutputDesc(0);
  GE_CHECK_NOTNULL(output);
  output->SetFormat(FORMAT_NC1HWC0);
  output->SetShape(ge::GeShape(dst_shape_dims));

  int64_t size = 0;
  graphStatus graph_status = TensorUtils::GetTensorMemorySizeInBytes(*output, size);
  if (graph_status != ge::GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get output tensor size failed, op:%s(%s), index:0",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(graph_status, "[Get][TensorSize] In Bytes failed, op:%s(%s), index:0",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return FAILED;
  }
  ge::TensorUtils::SetSize(*output, size);
  ge::TensorUtils::SetSize(*input, size);

  return SUCCESS;
}

Status ModifyFormatAndShapeForSingleTensor(const GeTensorDescPtr &input_output) {
  GE_CHECK_NOTNULL(input_output);
  ge::Format old_format = input_output->GetFormat();
  std::vector<int64_t> old_shape = input_output->GetShape().GetDims();
  ge::DataType dt = input_output->GetDataType();
  std::vector<int64_t> dst_shape_dims;
  if (TransferShape2NC1HWC0(old_format, old_shape, dt, FORMAT_NC1HWC0, dst_shape_dims) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Trans][Shape] to NC1HWC0 failed");
    return FAILED;
  }
  input_output->SetFormat(FORMAT_NC1HWC0);
  input_output->SetShape(ge::GeShape(dst_shape_dims));
  return SUCCESS;
}

void GetShapeAndRangeAccordingToFormat(const OpDescPtr &op_desc, uint32_t index, Format storage_format,
                                       std::vector<int64_t> &dst_shape_dims,
                                       std::vector<std::pair<int64_t, int64_t>> &output_range) {
  // This is an internal function, op_desc and input have been verified when the upper layer invokes this function.
  const GeTensorDescPtr &input = op_desc->MutableInputDesc(index);
  std::vector<std::pair<int64_t, int64_t>> origin_range;
  (void)input->GetOriginShapeRange(origin_range);
  if (origin_range.empty() || (!input->GetShape().IsUnknownShape())) {
    GELOGD("Use storage shape and range set before, node:%s, index:%u.", op_desc->GetName().c_str(), index);
    return;
  }
  Format format = input->GetFormat();
  std::vector<int64_t> shape_dims = input->GetShape().GetDims();
  if (format == storage_format) {
    GELOGI("Format is same with storage format, node:%s, index:%u.", op_desc->GetName().c_str(), index);
    dst_shape_dims = shape_dims;
    return;
  }
  DataType data_type = input->GetDataType();

  // transfer shape
  GeShape expand_shape(shape_dims);
  (void)transformer::ExpandDimension(op_desc->GetType(), format, storage_format, index, "", expand_shape);
  transformer::ShapeTransferAccordingToFormat shape_transfer;
  GeShape curr_shape(expand_shape);
  transformer::ShapeAndFormat shape_and_format_info {curr_shape, format, storage_format, data_type};
  (void)shape_transfer.GetShapeAccordingToFormat(op_desc, shape_and_format_info);
  dst_shape_dims = curr_shape.GetDims();
  GELOGI("Transform shape, node:%s, index:%d, shape:%s, expend shape:%s, format:%s, new shape:%s, new format:%s.",
         op_desc->GetName().c_str(), index, formats::ShapeToString(shape_dims).c_str(),
         expand_shape.ToString().c_str(),
         ge::TypeUtils::FormatToSerialString(format).c_str(), curr_shape.ToString().c_str(),
         ge::TypeUtils::FormatToSerialString(storage_format).c_str());

  // transfer range
  std::vector<std::pair<int64_t, int64_t>> expand_range = origin_range;
  transformer::ExpandRangeDimension(op_desc->GetName(), format, storage_format, index, "", expand_range);
  transformer::RangeTransferAccordingToFormat range_transfer;
  transformer::RangeAndFormat range_and_format_info {expand_shape, expand_range, output_range,
                                                     format, storage_format, data_type};
  (void)range_transfer.GetRangeAccordingToFormat(range_and_format_info);
  GELOGI("Transform range, node:%s, index:%d, range:%s, expend range:%s, format:%s, new range:%s, new format:%s.",
         op_desc->GetName().c_str(), index, formats::RangeToString(origin_range).c_str(),
         formats::RangeToString(expand_range).c_str(),
         ge::TypeUtils::FormatToSerialString(format).c_str(), formats::RangeToString(output_range).c_str(),
         ge::TypeUtils::FormatToSerialString(storage_format).c_str());
}

Status ModifyNetOutputFormatAndShape(const OpDescPtr &op_desc, uint32_t index, Format storage_format,
                                     std::vector<int64_t> &storage_dims) {
  GE_CHECK_NOTNULL(op_desc);
  const GeTensorDescPtr &input = op_desc->MutableInputDesc(index);
  GE_CHECK_NOTNULL(input);

  std::vector<std::pair<int64_t, int64_t>> output_range;
  GetShapeAndRangeAccordingToFormat(op_desc, index, storage_format, storage_dims, output_range);

  ge::Format old_format = input->GetFormat();
  std::vector<int64_t> old_shape = input->GetShape().GetDims();

  input->SetShape(ge::GeShape(storage_dims));
  input->SetFormat(storage_format);

  if (!output_range.empty()) {
    (void)input->SetShapeRange(output_range);
  }

  if (!input->MutableShape().IsUnknownShape()) {
    int64_t size = 0;
    graphStatus graph_status = TensorUtils::GetTensorMemorySizeInBytes(*input, size);
    if (graph_status != ge::GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Get output tensor size failed, op:%s(%s), index:%u",
                        op_desc->GetName().c_str(), op_desc->GetType().c_str(), index);
      GELOGE(graph_status, "[Get][TensorSize] In Bytes failed, op:%s(%s), index:%u",
             op_desc->GetName().c_str(), op_desc->GetType().c_str(), index);
      return FAILED;
    }
    ge::TensorUtils::SetSize(*input, size);

    GELOGI("Modify net output node %s input %u %s[%s] -> %s[%s]",
           op_desc->GetName().c_str(), index, ge::TypeUtils::FormatToSerialString(old_format).c_str(),
           formats::JoinToString(old_shape).c_str(), ge::TypeUtils::FormatToSerialString(storage_format).c_str(),
           formats::JoinToString(storage_dims).c_str());
  }

  return SUCCESS;
}

Status ModifyDataFormatAndShape(const OpDescPtr &op_desc, uint32_t index, Format storage_format,
                                std::vector<int64_t> &storage_dims) {
  GE_CHECK_NOTNULL(op_desc);
  const GeTensorDescPtr &input = op_desc->MutableInputDesc(index);
  GE_CHECK_NOTNULL(input);
  auto output = op_desc->MutableOutputDesc(index);
  GE_CHECK_NOTNULL(output);

  std::vector<std::pair<int64_t, int64_t>> output_range;
  GetShapeAndRangeAccordingToFormat(op_desc, index, storage_format, storage_dims, output_range);

  if (!output_range.empty()) {
    (void)input->SetShapeRange(output_range);
    (void)output->SetShapeRange(output_range);
  }

  if (!output->MutableShape().IsUnknownShape()) {
    ge::Format old_format = input->GetFormat();
    std::vector<int64_t> old_shape = input->GetShape().GetDims();
    input->SetShape(ge::GeShape(storage_dims));
    input->SetFormat(storage_format);
    output->SetShape(ge::GeShape(storage_dims));
    output->SetFormat(storage_format);

    int64_t size = 0;
    graphStatus graph_status = TensorUtils::GetTensorMemorySizeInBytes(*output, size);
    if (graph_status != ge::GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Get output tensor size failed, op:%s(%s), index:%u",
                        op_desc->GetName().c_str(), op_desc->GetType().c_str(), index);
      GELOGE(graph_status, "[Get][TensorSize] In Bytes failed, op:%s(%s), index:%u",
             op_desc->GetName().c_str(), op_desc->GetType().c_str(), index);
      return FAILED;
    }
    ge::TensorUtils::SetSize(*input, size);
    ge::TensorUtils::SetSize(*output, size);

    GELOGI("Modify data node %s input&output %u %s[%s] -> %s[%s] success",
           op_desc->GetName().c_str(), index, ge::TypeUtils::FormatToSerialString(old_format).c_str(),
           formats::JoinToString(old_shape).c_str(), ge::TypeUtils::FormatToSerialString(storage_format).c_str(),
           formats::JoinToString(storage_dims).c_str());
  } else {
    input->SetShape(ge::GeShape(storage_dims));
    input->SetFormat(storage_format);
    output->SetShape(ge::GeShape(storage_dims));
    output->SetFormat(storage_format);
  }

  return SUCCESS;
}

Status CheckIfDynamicBatchScene(const NodePtr &data_node, bool &is_dynamic_batch, NodePtr &mbatch_case_node,
                                int32_t &index) {
  is_dynamic_batch = false;
  std::string related_node_name;
  if (AttrUtils::GetStr(data_node->GetOpDesc(), kMbatchSwitchnName, related_node_name)) {
    if (related_node_name.empty()) {
      REPORT_INNER_ERROR("E19999", "The data node %s has switchn node flag, but the value is empty",
                         data_node->GetName().c_str());
      GELOGE(INTERNAL_ERROR, "[Check][Param] The data node %s has switchn node flag, but the value is empty",
             data_node->GetName().c_str());
      return INTERNAL_ERROR;
    }

    auto out_data_nodes_anchors = data_node->GetOutDataNodesAndAnchors();
    for (const auto &out_data_node_anchor : out_data_nodes_anchors) {
      if (out_data_node_anchor.first->GetName() == related_node_name) {
        mbatch_case_node = out_data_node_anchor.first;
        index = out_data_node_anchor.second->GetIdx();
        break;
      }
    }

    if (mbatch_case_node == nullptr) {
      REPORT_INNER_ERROR("E19999", "The data node %s has switchn node %s, but can not find it on the graph",
                         data_node->GetName().c_str(), related_node_name.c_str());
      GELOGE(INTERNAL_ERROR, "[Check][Param] The data node %s has switchn node %s, but can not find it on the graph",
             data_node->GetName().c_str(), related_node_name.c_str());
      return INTERNAL_ERROR;
    }
    is_dynamic_batch = true;
  }
  return SUCCESS;
}

bool CheckOpType(const NodePtr &node, const std::string &type) {
  return (node->GetType() == type);
}

Status CheckIfNeedSetNdFormat(const NodePtr &node_ptr) {
  auto op = node_ptr->GetOpDesc();
  GE_CHECK_NOTNULL(op);
  auto inputDescsPtr = op->GetAllInputsDescPtr();
  auto outputDescsPtr = op->GetAllOutputsDescPtr();
  ge::Format format = ge::FORMAT_ND;
  // if user set shape larger than 4, inferformat may set NCHW or NHWC, GE should set ND before FE
  // process, otherwise fe will insert transdata.
  for (auto &inputDescPtr : inputDescsPtr) {
    GE_CHECK_NOTNULL(inputDescPtr);
    if ((inputDescPtr->GetShape().GetDims().size() > ge::DIM_DEFAULT_SIZE) &&
        ((inputDescPtr->GetFormat() == ge::FORMAT_NCHW) || (inputDescPtr->GetFormat() == ge::FORMAT_NHWC))) {
      GELOGI("The node inputdesc [%s] format need to be set ND", op->GetName().c_str());
      inputDescPtr->SetFormat(format);
      inputDescPtr->SetOriginFormat(format);
    }
  }
  for (auto &outputDescPtr : outputDescsPtr) {
    GE_CHECK_NOTNULL(outputDescPtr);
    if ((outputDescPtr->GetShape().GetDims().size() > ge::DIM_DEFAULT_SIZE) &&
        ((outputDescPtr->GetFormat() == ge::FORMAT_NCHW) || (outputDescPtr->GetFormat() == ge::FORMAT_NHWC))) {
      GELOGI("The node outputdesc [%s] format need to be set ND", op->GetName().c_str());
      outputDescPtr->SetFormat(format);
      outputDescPtr->SetOriginFormat(format);
    }
  }
  return SUCCESS;
}

// A new function ending in 'DynShape' has been added for the dynamic shape processing.
// In the dynamic shape process, transnode insertion by FE is advanced to the stage of whole
// graph optimization, GE only sets the final data_type/format/shape information for variable,
// data and netoutput, and no longer inserts the transnode.
Status ProcessInputDtDynShape(const NodePtr &node_ptr, const DataType &dt_set) {
  GE_CHECK_NOTNULL(node_ptr);
  auto op_desc = node_ptr->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  const GeTensorDescPtr &input = op_desc->MutableInputDesc(0);
  GE_CHECK_NOTNULL(input);
  ge::DataType src_dtype = input->GetDataType();
  if (src_dtype == dt_set) {
    GELOGI("The node name, %s dtype is fp16", node_ptr->GetName().c_str());
    return SUCCESS;
  }
  input->SetDataType(dt_set);
  const GeTensorDescPtr &output = op_desc->MutableOutputDesc(0);
  GE_CHECK_NOTNULL(output);
  output->SetDataType(dt_set);

  GeShape shape = input->GetShape();
  if (!shape.IsUnknownShape()) {
    int64_t input_shape_size = 0;
    int64_t output_shape_size = 0;
    ge::graphStatus input_graph_status = ge::TensorUtils::GetTensorSizeInBytes(*input, input_shape_size);
    ge::graphStatus output_graph_status = ge::TensorUtils::GetTensorMemorySizeInBytes(*input, output_shape_size);
    if (input_graph_status != ge::GRAPH_SUCCESS && output_graph_status != ge::GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Get input tensor size failed, op:%s(%s), index:0",
                        op_desc->GetName().c_str(), op_desc->GetType().c_str());
      GELOGE(GRAPH_FAILED, "[Process][InputOp] Get tensor size of op [%s] failed!", node_ptr->GetName().c_str());
      return FAILED;
    }
    ge::TensorUtils::SetSize(*input, input_shape_size);
    ge::TensorUtils::SetSize(*output, output_shape_size);
    GELOGI("[Process][InputDynShape] Set input and output size of node [%s] success.", node_ptr->GetName().c_str());
  }

  return SUCCESS;
}

Status UpdateInputOutputDataType(const NodePtr &mbatch_node, const DataType &dt_set, int32_t index) {
  auto mbatch_desc = mbatch_node->GetOpDesc();
  GE_CHECK_NOTNULL(mbatch_desc);
  auto mbatch_input = mbatch_desc->MutableInputDesc(index);
  GE_CHECK_NOTNULL(mbatch_input);
  mbatch_input->SetDataType(dt_set);

  GELOGD("Update input and output data type of node[name: %s, type: %s, input index: %d] to %s.",
         mbatch_node->GetName().c_str(), mbatch_node->GetType().c_str(), index,
         TypeUtils::DataTypeToSerialString(dt_set).c_str());

  return SUCCESS;
}

Status UpdateSubgraphDataOfCase(const NodePtr &mbatch_node, const DataType &dt_set, int32_t index) {
  if (mbatch_node->GetType() != CASE) {
    return SUCCESS;
  }

  std::vector<ComputeGraphPtr> subgraphs;
  if (NodeUtils::GetDirectSubgraphs(mbatch_node, subgraphs) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get subgraphs of node %s failed", mbatch_node->GetName().c_str());
    GELOGE(FAILED, "[Check][Param] Get subgraphs of node %s failed", mbatch_node->GetName().c_str());
    return FAILED;
  }
  for (const auto &subgraph : subgraphs) {
    GE_CHECK_NOTNULL(subgraph);
    for (auto &sub_node : subgraph->GetDirectNode()) {
      GE_CHECK_NOTNULL(sub_node);
      if (sub_node->GetType() != DATA) {
        continue;
      }

      auto data_desc = sub_node->GetOpDesc();
      GE_CHECK_NOTNULL(data_desc);
      int32_t parent_node_index = 0;
      if (!AttrUtils::GetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, parent_node_index) ||
          (parent_node_index != index)) {
        continue;
      }

      auto data_input = data_desc->MutableInputDesc(0);
      GE_CHECK_NOTNULL(data_input);
      data_input->SetDataType(dt_set);
      auto data_output = data_desc->MutableOutputDesc(0);
      GE_CHECK_NOTNULL(data_output);
      data_output->SetDataType(dt_set);
      GELOGD("Update input and output data type of node[name: %s, type: %s, parent_node_index: %d] in subgraph %s "
             "to %s.", data_desc->GetName().c_str(), data_desc->GetType().c_str(), parent_node_index,
             subgraph->GetName().c_str(), TypeUtils::DataTypeToSerialString(dt_set).c_str());
    }
  }

  return SUCCESS;
}

Status ProcessMbatchScene(const NodePtr &mbatch_node, const DataType &dt_set, int32_t index) {
  GELOGI("The node [%s] dtype set fp16.", mbatch_node->GetName().c_str());
  if (UpdateInputOutputDataType(mbatch_node, dt_set, index) != SUCCESS) {
    GELOGE(FAILED, "[Update][InputOutputDataType] of node[name: %s, type: %s] to %s failed.",
           mbatch_node->GetName().c_str(), mbatch_node->GetType().c_str(),
           TypeUtils::DataTypeToSerialString(dt_set).c_str());
    return FAILED;
  }

  if (UpdateSubgraphDataOfCase(mbatch_node, dt_set, index) != SUCCESS) {
    GELOGE(FAILED, "[Update][SubgraphDataOfCase] node[parent_node_index:%d] in subgraphs of "
           "node[name:%s, type:%s] to %s failed.", index, mbatch_node->GetName().c_str(),
           mbatch_node->GetType().c_str(), TypeUtils::DataTypeToSerialString(dt_set).c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status ProcessInputNC1HWC0DynShape(const NodePtr &node_ptr, const bool &is_dynamic_batch, const NodePtr &switchn_node) {
  GE_CHECK_NOTNULL(node_ptr);
  auto op_desc = node_ptr->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  const GeTensorDescPtr &input = op_desc->MutableInputDesc(0);
  GE_CHECK_NOTNULL(input);
  ge::Format old_format = input->GetFormat();
  ge::GeShape old_shape = input->GetShape();
  bool support = ((old_format == FORMAT_NC1HWC0) || (old_format == FORMAT_NCHW) || (old_format == FORMAT_NHWC));
  if (!support) {
    REPORT_INNER_ERROR("E19999",
                       "The format:%s of op:%s(%s) is unsupported, only support FORMAT_NC1HWC0,FORMAT_NCHW,FORMAT_NHWC",
                       TypeUtils::FormatToSerialString(old_format).c_str(),
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Check][Param] The format [%s] is unsupported, op:%s",
           TypeUtils::FormatToSerialString(old_format).c_str(), op_desc->GetName().c_str());
    return FAILED;
  }
  if (ModifyInputFormatAndShape(node_ptr) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Modify][InputFormatAndShape] failed, op:%s(%s)",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return FAILED;
  }
  if (is_dynamic_batch) {
    auto switchn_op_desc = switchn_node->GetOpDesc();
    GE_CHECK_NOTNULL(switchn_op_desc);
    const GeTensorDescPtr &switchn_input = switchn_op_desc->MutableInputDesc(0);
    if (ModifyFormatAndShapeForSingleTensor(switchn_input) != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Modify format and shape of input:0 in op:%s(%s) failed",
                        switchn_op_desc->GetName().c_str(), switchn_op_desc->GetType().c_str());
      GELOGE(INTERNAL_ERROR, "[Modify][FormatAndShape] of input:0 in op:%s(%s) failed",
             switchn_op_desc->GetName().c_str(), switchn_op_desc->GetType().c_str());
      return FAILED;
    }
    for (uint32_t i = 0; i < switchn_node->GetAllOutDataAnchorsSize(); ++i) {
      auto switchn_output = switchn_op_desc->MutableOutputDesc(i);
      GE_CHECK_NOTNULL(switchn_output);
      old_format = switchn_output->GetFormat();
      old_shape = switchn_output->GetShape();
      if (ModifyFormatAndShapeForSingleTensor(switchn_output) != SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Modify format and shape of output:%u in op:%s(%s) failed", i,
                          switchn_op_desc->GetName().c_str(), switchn_op_desc->GetType().c_str());
        GELOGE(INTERNAL_ERROR, "[Modify][FormatAndShape] of output:%u in op:%s(%s) failed", i,
               switchn_op_desc->GetName().c_str(), switchn_op_desc->GetType().c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status ProcessDataNodeDynShape(const NodePtr &data_node) {
  auto op_desc = data_node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  std::string set_dt_str;
  if (!ge::AttrUtils::GetStr(data_node->GetOpDesc(), ATTR_ATC_USER_DEFINE_DATATYPE, set_dt_str)) {
    return SUCCESS;
  }
  const DataType &dt_set = TypeUtils::SerialStringToDataType(set_dt_str);
  GELOGI("input_fp16 is found, the node name is %s.", data_node->GetName().c_str());
  bool is_dynamic_batch = false;
  NodePtr mbatch_case_node = nullptr;
  int32_t index = 0;
  if (CheckIfDynamicBatchScene(data_node, is_dynamic_batch, mbatch_case_node, index)) {
    GELOGE(INTERNAL_ERROR, "[Call][CheckIfDynamicBatchScene] failed, op:%s", op_desc->GetName().c_str());
    return FAILED;
  }
  if (ProcessInputDtDynShape(data_node, dt_set) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Process][InputDtDynShape] ProcessInputFP16 failed, op:%s", op_desc->GetName().c_str());
    return FAILED;
  }
  if (is_dynamic_batch && ProcessMbatchScene(mbatch_case_node, dt_set, index) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Process][MbatchScene] failed");
    return FAILED;
  }

  // check if need to set format
  std::string set_format;
  bool ret = ge::AttrUtils::GetStr(data_node->GetOpDesc(), ATTR_ATC_USER_DEFINE_FORMAT, set_format);
  if (ret && (!set_format.empty()) && TypeUtils::SerialStringToFormat(set_format) == FORMAT_NC1HWC0) {
    GELOGI("The format of node [%s] should be set NC1HWC0.", data_node->GetName().c_str());
    if (ProcessInputNC1HWC0DynShape(data_node, is_dynamic_batch, mbatch_case_node) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Process][InputNC1HWC0] failed, op:%s", data_node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status GetStorageFormatAndShape(const OpDescPtr &op_desc, const GeTensorDescPtr &tensor_desc_ptr,
                                Format &storage_format, std::vector<int64_t> &dst_shape_dims) {
  GE_CHECK_NOTNULL(op_desc);
  GE_CHECK_NOTNULL(tensor_desc_ptr);

  storage_format = FORMAT_RESERVED;
  int64_t format = FORMAT_RESERVED;
  dst_shape_dims.clear();
  if (ge::AttrUtils::GetInt(*tensor_desc_ptr, ATTR_NAME_STORAGE_FORMAT, format)) {
    storage_format = static_cast<Format>(format);
    std::vector<int32_t> storage_shape;
    if (ge::AttrUtils::GetListInt(*tensor_desc_ptr, ATTR_NAME_STORAGE_SHAPE, storage_shape)) {
      for (auto dim : storage_shape) {
        dst_shape_dims.push_back(static_cast<int64_t>(dim));
      }
      GELOGI("Update node by storage format, node: [%s], storage_format: [%s], storage_shape:[%s]",
             op_desc->GetName().c_str(), TypeUtils::FormatToSerialString(storage_format).c_str(),
             formats::JoinToString(storage_shape).c_str());
    } else {
      ErrorManager::GetInstance().ATCReportErrMessage(
          "15003", {"opname", "format"},
          {op_desc->GetName(), TypeUtils::FormatToSerialString(storage_format)});
      GELOGE(PARAM_INVALID, "[Check][Param] Update node by storage format failed, storage_shape not set. "
             "node:[%s], storage_format [%s]",
             op_desc->GetName().c_str(), TypeUtils::FormatToSerialString(storage_format).c_str());
      return FAILED;
    }

    ge::Format old_format = tensor_desc_ptr->GetFormat();
    auto old_shape = tensor_desc_ptr->GetShape().GetDims();
    if (old_format == storage_format && old_shape == dst_shape_dims) {
      GELOGI("Update node by storage format, not changed.");
      storage_format = FORMAT_RESERVED;
      return SUCCESS;
    }
  }
  return SUCCESS;
}
Status ProcessNetoutputNodeFp16Nc1hwc0DynShape(GeTensorDesc &src_desc, GeTensorDescPtr &net_output_input_desc,
                                               const NodePtr &node) {
  bool is_dynamic = CheckOpType(node, MERGE);
  auto src_op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(src_op_desc);
  ge::GeShape src_shape = src_desc.GetShape();
  ge::Format src_format = src_desc.GetFormat();

  net_output_input_desc->SetDataType(DT_FLOAT16);
  if (is_dynamic) {
    auto merge_output = src_op_desc->MutableOutputDesc(0);
    GE_CHECK_NOTNULL(merge_output);
    merge_output->SetDataType(DT_FLOAT16);
    for (uint32_t i = 0; i < node->GetAllInDataAnchorsSize(); ++i) {
      auto merge_input = src_op_desc->MutableInputDesc(i);
      GE_CHECK_NOTNULL(merge_input);
      merge_input->SetDataType(DT_FLOAT16);
    }
  }
  std::vector<int64_t> dst_shape_dims;
  std::vector<int64_t> src_shape_dims = src_shape.GetDims();
  if (TransferShape2NC1HWC0(src_format, src_shape_dims, DT_FLOAT16, FORMAT_NC1HWC0, dst_shape_dims) != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Transfer output:0 shape of op:%s(%s) to NC1HWC0 format failed, shape:%s, format:%s",
                      src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str(),
                      src_shape.ToString().c_str(), TypeUtils::FormatToSerialString(src_format).c_str());
    GELOGE(INTERNAL_ERROR, "[Trans][Shape] of op:%s(%s) to NC1HWC0 format failed, shape:%s, format:%s",
           src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str(),
           src_shape.ToString().c_str(), TypeUtils::FormatToSerialString(src_format).c_str());
    return FAILED;
  }
  ge::GeShape dst_shape(dst_shape_dims);
  net_output_input_desc->SetFormat(FORMAT_NC1HWC0);
  net_output_input_desc->SetShape(dst_shape);
  if (is_dynamic) {
    auto merge_out = src_op_desc->MutableOutputDesc(0);
    GE_CHECK_NOTNULL(merge_out);
    if (ModifyFormatAndShapeForSingleTensor(merge_out) != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Modify format and shape of output:0 in op:%s(%s) failed",
                        src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str());
      GELOGE(INTERNAL_ERROR, "[Modify][FormatAndShape] of output:0 in op:%s(%s) failed",
             src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str());
      return FAILED;
    }
    for (uint32_t i = 0; i < node->GetAllInDataAnchorsSize(); ++i) {
      auto merge_in = src_op_desc->MutableInputDesc(i);
      GE_CHECK_NOTNULL(merge_in);
      if (ModifyFormatAndShapeForSingleTensor(merge_in) != SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Modify format and shape of input:%u in op:%s(%s) failed", i,
                          src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str());
        GELOGE(INTERNAL_ERROR, "[Modify][FormatAndShape] of input:%u in op:%s(%s) failed", i,
               src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

bool NeedUpdateDtByOutputTypeParm(const OpDescPtr &netout_desc, const uint32_t &index, ge::DataType &dt) {
  GE_CHECK_NOTNULL(netout_desc);
  std::vector<std::string> output_dt_str;
  if (ge::AttrUtils::GetListStr(netout_desc, ATTR_ATC_USER_DEFINE_DATATYPE, output_dt_str)) {
    for (auto dt_str : output_dt_str) {
      std::vector<std::string> dt_str_split = StringUtils::Split(dt_str, ':');
      if (dt_str_split.size() == kUserDefinedElementCount) {
        if (dt_str_split[0] == to_string(index)) {
          dt = TypeUtils::SerialStringToDataType(dt_str_split[1]);
          GELOGI("Find netoutput node output %u datatype should be set %s .", index,
                 TypeUtils::DataTypeToSerialString(dt).c_str());
          return true;
        }
      }
    }
  }
  return false;
}

bool NeedUpdateFormatByOutputTypeParm(const OpDescPtr &netout_desc, const uint32_t &index) {
  GE_CHECK_NOTNULL(netout_desc);
  std::vector<std::string> output_format_str;
  if (ge::AttrUtils::GetListStr(netout_desc, ATTR_ATC_USER_DEFINE_FORMAT, output_format_str)) {
    for (auto format_str : output_format_str) {
      std::vector<std::string> format_str_split = StringUtils::Split(format_str, ':');
      if (format_str_split.size() == kUserDefinedElementCount) {
        if (format_str_split[0] == to_string(index)) {
          GELOGI("Find netoutput node output %u format should be set NC1HWC0.", index);
          return true;
        }
      }
    }
  }
  return false;
}

Status ProcessNetoutputNodeDynShape(const NodePtr &node) {
  auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  ge::DataType output_data_type = ge::DT_FLOAT;

  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    auto index = static_cast<uint32_t>(in_anchor->GetIdx());
    auto peer_out = in_anchor->GetPeerOutAnchor();
    GE_CHECK_NOTNULL(peer_out);
    auto src_node = peer_out->GetOwnerNode();
    GE_CHECK_NOTNULL(src_node);
    bool is_dynamic = CheckOpType(src_node, MERGE);

    OpDescPtr src_op_desc = src_node->GetOpDesc();
    GE_CHECK_NOTNULL(src_op_desc);
    auto net_output_input_desc = op_desc->MutableInputDesc(index);
    GE_CHECK_NOTNULL(net_output_input_desc);

    ge::GeShape old_shape = net_output_input_desc->GetShape();
    ge::Format old_format = net_output_input_desc->GetFormat();
    ge::DataType old_dtype = net_output_input_desc->GetDataType();
    // Update datatype
    if (NeedUpdateDtByOutputTypeParm(op_desc, index, output_data_type)) {
      GELOGI("Enter into process output_type schedule");
      net_output_input_desc->SetDataType(output_data_type);
      if (is_dynamic) {
        auto merge_output = src_op_desc->MutableOutputDesc(0);
        GE_CHECK_NOTNULL(merge_output);
        merge_output->SetDataType(output_data_type);
        for (uint32_t i = 0; i < src_node->GetAllInDataAnchorsSize(); ++i) {
          auto merge_input = src_op_desc->MutableInputDesc(i);
          GE_CHECK_NOTNULL(merge_input);
          merge_input->SetDataType(output_data_type);
        }
      }
    }
    // check if is_output_adjust_hw_layout is set
    if (NeedUpdateFormatByOutputTypeParm(op_desc, index)) {
      if ((old_format != FORMAT_NCHW) && (old_format != FORMAT_NHWC) && (old_format != FORMAT_NC1HWC0)) {
        REPORT_INNER_ERROR("E19999", "Format:%s of op:%s(%s) is not one of NCHW, NHWC, NC1HWC0.",
                           TypeUtils::FormatToSerialString(old_format).c_str(),
                           op_desc->GetName().c_str(), op_desc->GetType().c_str());
        GELOGE(INTERNAL_ERROR, "[Check][Param] Format is not one of NCHW, NHWC, NC1HWC0.");
        return FAILED;
      }

      GeTensorDesc old_desc(old_shape, old_format, old_dtype);
      if (ProcessNetoutputNodeFp16Nc1hwc0DynShape(old_desc, net_output_input_desc, src_node) != SUCCESS) {
        GELOGE(INTERNAL_ERROR, "[Process][NetOutput] fp16 nc1hwc0 failed.");
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status GetDynamicInputShapeRange(const std::vector<GeTensor> &user_input, const std::map<std::string,
                                 std::string> &graph_option,
                                 std::vector<std::vector<std::pair<int64_t, int64_t>>> &range_vec) {
  // check both mode and shape_range option are all enabled
  auto mode_iter = graph_option.find(OPTION_EXEC_DYNAMIC_EXECUTE_MODE);
  bool enable_dynamic_execute_mode = (mode_iter != graph_option.end()) && (mode_iter->second == "dynamic_execute");
  if (!enable_dynamic_execute_mode) {
    GELOGD("Graph Option: Can not find %s option in graph options or option value is empty",
           OPTION_EXEC_DYNAMIC_EXECUTE_MODE);
  }
  auto iter = graph_option.find(OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE);
  bool enable_input_shape_range = (iter != graph_option.end()) && (!iter->second.empty());
  if (!enable_input_shape_range) {
    GELOGD("Graph Option: Can not find %s option in graph options or option value is empty",
           OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE);
  }
  if (enable_dynamic_execute_mode && enable_input_shape_range) {
    GELOGD("GraphOption: %s value is dynamic_execute, %s value is %s.", OPTION_EXEC_DYNAMIC_EXECUTE_MODE,
           OPTION_EXEC_DATA_INPUTS_SHAPE_RANGE, iter->second.c_str());
  } else if (enable_dynamic_execute_mode && !enable_input_shape_range) {
    GELOGD("GraphOption: %s value is dynamic_execute, without input shape range, lauch fuzz compile mode.",
           OPTION_EXEC_DYNAMIC_EXECUTE_MODE);
    // currently ge.shape_generalized_build_mode has been set by tfa
    // here set bin_mode to one_node_multi_bin
    auto graph_options = GetThreadLocalContext().GetAllGraphOptions();
    graph_options.emplace("RUNTIME_NODE_BIN_MODE", "1"); //1 means kOneNodeMultipleBinsMode
    GetThreadLocalContext().SetGraphOption(graph_options);
    return SUCCESS;
  } else {
    return SUCCESS;
  }
  if (ParseInputShapeRange(iter->second, range_vec) != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Parse][ShapeRange] Parse dynamic input shape range failed.");
    return PARAM_INVALID;
  }
  if (range_vec.size() != user_input.size()) {
    GELOGE(PARAM_INVALID, "[Check][Param] Dynamic input shape range size is %zu, inputs size is %zu. Not match.",
           range_vec.size(), user_input.size());
    return PARAM_INVALID;
  }
  return SUCCESS;
}

Status UpdateDynamicInputShapeRange(const int64_t index,
                                    const std::vector<std::vector<std::pair<int64_t, int64_t>>> &range_vec,
                                    OpDescPtr &op,
                                    GeTensorDesc &desc) {
  auto origin_shape = desc.GetShape();
  auto current_shape_range_vec = range_vec.at(index);
  if (origin_shape.IsScalar()) {
    GELOGI("Cur input %ld is scalar, no need set shape range.", index);
    return SUCCESS;
  }
  if (current_shape_range_vec.size() != origin_shape.GetDimNum()) {
    REPORT_INNER_ERROR("E19999", "Given shape_range dim num is %zu, current dim:%s num is %zu, not match, "
                       "check invalid", current_shape_range_vec.size(), origin_shape.ToString().c_str(),
                       origin_shape.GetDimNum());
    GELOGE(PARAM_INVALID, "[Check][Param] Given shape_range dim num is %zu, current dim num is %zu, "
           "not match.Pleace Check.", current_shape_range_vec.size(), origin_shape.GetDimNum());
    return PARAM_INVALID;
  }
  for (size_t i = 0; i < origin_shape.GetDimNum(); ++i) {
    auto curr_dim = origin_shape.GetDim(i);
    auto left_range = current_shape_range_vec.at(i).first;
    auto right_range = current_shape_range_vec.at(i).second;
    if (left_range == right_range) {
      // given shape_range is known dim, check is same as origin or not
      if (curr_dim != left_range) {
        REPORT_INNER_ERROR("E19999", "Given shape range is %ld, current dim shape is %ld, not match, dim_index:%zu, "
                          "check invalid", left_range, curr_dim, i);
        GELOGE(PARAM_INVALID, "[Check][Param] Given shape range is %ld, current dim shape is %ld, "
               "not match.Pleace Check.", left_range, curr_dim);
        return PARAM_INVALID;
      }
      origin_shape.SetDim(i, left_range);
    } else {
      // given shape_range is fix range, check input_shape is in this range or not
      if (right_range != UNKNOWN_DIM) {
        if ((curr_dim < left_range) || (curr_dim > right_range)) {
          REPORT_INNER_ERROR("E19999", "Given shape range is [%ld~%ld], current dim shape is %ld, out of range, "
                             "dim_index:%zu, check invalid",
                             left_range, right_range, curr_dim, i);
          GELOGE(PARAM_INVALID, "[Check][Param] Given shape range is [%ld~%ld], current dim shape is %ld, "
                 "out of range.Pleace Check.", left_range, right_range, curr_dim);
          return PARAM_INVALID;
        }
      }
      origin_shape.SetDim(i, UNKNOWN_DIM);
    }
  }
  desc.SetShape(origin_shape);
  desc.SetOriginShape(origin_shape);
  desc.SetShapeRange(current_shape_range_vec);

  graphStatus graph_ret = op->UpdateInputDesc(0, desc);
  GE_CHK_GRAPH_STATUS_RET(graph_ret, "[Update][InputDesc] fail, graph ret: %u", graph_ret);
  graph_ret = op->UpdateOutputDesc(0, desc);
  GE_CHK_GRAPH_STATUS_RET(graph_ret, "[Update][OutputDesc] fail, graph ret: %u", graph_ret);
  return SUCCESS;
}

bool IsInEdgeZeroNode(const NodePtr &node) {
  if (node == nullptr) {
    return false;
  }
  for (const auto &anchor : node->GetAllInDataAnchors()) {
    if (anchor->GetPeerAnchorsSize() > 0) {
      return false;
    }
  }
  return true;
}

void UpdateInputOutputFormat(const NodePtr &node) {
  // Inner function, input param has been checked in caller.
  // Update input
  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    auto input_desc = node->GetOpDesc()->MutableInputDesc(in_anchor->GetIdx());
    auto peer_anchor = in_anchor->GetPeerOutAnchor();
    if (peer_anchor == nullptr) {
      continue;
    }
    auto peer_op_desc = peer_anchor->GetOwnerNode()->GetOpDesc();
    if (peer_op_desc == nullptr) {
      continue;
    }
    auto peer_output_desc = peer_op_desc->MutableOutputDesc(peer_anchor->GetIdx());
    if (peer_output_desc == nullptr) {
      continue;
    }
    peer_output_desc->SetFormat(input_desc->GetFormat());
    peer_output_desc->SetOriginFormat((input_desc->GetOriginFormat()));
    if (peer_op_desc->GetType() == DATA) {
      auto data_in_desc = peer_op_desc->MutableInputDesc(0);
      if (data_in_desc != nullptr) {
        data_in_desc->SetFormat(input_desc->GetFormat());
        data_in_desc->SetOriginFormat((input_desc->GetOriginFormat()));
      }
    }
    GELOGD("Update %s:%d from %s:%d, format:%s, origin format:%s.", peer_op_desc->GetName().c_str(),
           peer_anchor->GetIdx(), node->GetName().c_str(), in_anchor->GetIdx(),
           TypeUtils::FormatToSerialString(peer_output_desc->GetFormat()).c_str(),
           TypeUtils::FormatToSerialString(peer_output_desc->GetOriginFormat()).c_str());
  }

  // Update output
  for (const auto &out_anchor : node->GetAllOutDataAnchors()) {
    auto output_desc = node->GetOpDesc()->MutableOutputDesc(out_anchor->GetIdx());
    for (const auto &peer_anchor : out_anchor->GetPeerInDataAnchors()) {
      if (peer_anchor == nullptr) {
        continue;
      }
      auto peer_op_desc = peer_anchor->GetOwnerNode()->GetOpDesc();
      if (peer_op_desc == nullptr) {
        continue;
      }
      auto peer_input_desc = peer_op_desc->MutableInputDesc(peer_anchor->GetIdx());
      if (peer_input_desc != nullptr) {
        peer_input_desc->SetFormat(output_desc->GetFormat());
        peer_input_desc->SetOriginFormat((output_desc->GetOriginFormat()));
        GELOGD("Update %s:%d from %s:%d, format:%s, origin format:%s.", peer_op_desc->GetName().c_str(),
               peer_anchor->GetIdx(), node->GetName().c_str(), out_anchor->GetIdx(),
               TypeUtils::FormatToSerialString(peer_input_desc->GetFormat()).c_str(),
               TypeUtils::FormatToSerialString(peer_input_desc->GetOriginFormat()).c_str());
      }
    }
  }
}

// Format of compute-operator doesn't propagate after InferFormatForSingleOp in single operator scenario,
// so we propagate it here after CheckAndUpdateInput.
// This is a special processing, waiting for compilation of operator to understand what the input means.
Status PropagateFormatIfNeed(ComputeGraphPtr graph) {
  GE_CHECK_NOTNULL(graph);
  bool is_single_op_scene = false;
  (void)AttrUtils::GetBool(graph, ATTR_SINGLE_OP_SCENE, is_single_op_scene);
  if (is_single_op_scene) {
    for (ge::NodePtr &node : graph->GetAllNodes()) {
      GE_CHECK_NOTNULL(node);
      GE_CHECK_NOTNULL(node->GetOpDesc());
      bool single_op = false;
      bool ret = AttrUtils::GetBool(node->GetOpDesc(), ATTR_SINGLE_OP_SCENE, single_op);
      if (ret && is_single_op_scene) {
        UpdateInputOutputFormat(node);
      }
    }
  }
  return SUCCESS;
}

void InitDummyShapeOnControlFlow(const ComputeGraphPtr &graph) {
  for (const auto &node : graph->GetAllNodes()) {
    std::string type;
    (void)GetOriginalType(node, type);
    if (type == MERGE || type == REFMERGE) {
      for (size_t i = 0; i < node->GetAllInDataAnchorsSize(); ++i) {
        GELOGD("Prepare for infershape: update %s input_shape as dummy.", node->GetName().c_str());
        (void)NodeUtils::UpdateInputOriginalShapeAndShape(*node, i, GeShape(DUMMY_SHAPE));
      }
    } else if (type == WHILE) {
      for (size_t i = 0; i < node->GetAllInDataAnchorsSize(); ++i) {
        GELOGD("Prepare for infershape: update %s output_shape as dummy.", node->GetName().c_str());
        (void)NodeUtils::UpdateOutputOriginalShapeAndShape(*node, i, GeShape(DUMMY_SHAPE));
      }
    }
  }
}
}  // namespace

GraphPrepare::GraphPrepare() : compute_graph_(nullptr) {}

GraphPrepare::~GraphPrepare() {}

/**
 * @param graph
 * @return
 */
Status GraphPrepare::UpdateVariableFormats(const ComputeGraphPtr &graph) const {
  GE_CHECK_NOTNULL(graph);
  auto var_names_to_refs = CollectVarNamesToRefs(graph);
  for (auto &node : graph->GetAllNodes()) {
    if (node == nullptr) {
      continue;
    }
    if (node->GetType() != VARIABLE) {
      continue;
    }
    auto trans_road = VarManager::Instance(graph->GetSessionID())->GetTransRoad(node->GetName());
    if (trans_road == nullptr) {
      GELOGD("The variable %s does not have any trans road", node->GetName().c_str());
      continue;
    }

    GELOGI("Recover the trans road for var %s reversely", node->GetName().c_str());

    auto ret = RecoverTransRoadForVar(node, *trans_road);
    if (ret != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Recover][TransRoad] for var %s failed", node->GetName().c_str());
      return INTERNAL_ERROR;
    }
    const auto iter = var_names_to_refs.find(node->GetName());
    if (iter != var_names_to_refs.end()) {
      ret = RecoverTransRoadForVarRef(iter->second, *trans_road);
      if (ret != SUCCESS) {
        GELOGE(INTERNAL_ERROR, "[Recover][TransRoad] for var ref %s failed", node->GetName().c_str());
        return INTERNAL_ERROR;
      }
    }
  }

  return SUCCESS;
}

void GraphPrepare::SetOptions(const ge::GraphManagerOptions &options) { options_ = options; }

Status GraphPrepare::Init(const ge::Graph &graph, uint64_t session_id, GraphRebuildStateCtrl *graph_rebuild_state_ctrl,
                          ResourceContextMgr *resource_context_mgr) {
  compute_graph_ = GraphUtils::GetComputeGraph(graph);
  if (compute_graph_ != nullptr) {
    compute_graph_->SetSessionID(session_id);
  }
  session_id_ = session_id;

  Status ret = CheckGraphAndUpdateOriginShape();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Check][Graph] fail, ret:%u", ret);
    return ret;
  }
  (void)compute_graph_->TopologicalSorting();
  ret = CheckRefOp();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Check][RefOp] fail, ret:%u", ret);
    return ret;
  }
  graph_rebuild_state_ctrl_ = graph_rebuild_state_ctrl;
  resource_context_mgr_ = resource_context_mgr;
  return ret;
}

Status GraphPrepare::CheckGraphAndUpdateOriginShape() const {
  if (compute_graph_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "compute_graph_ is nullptr, check invalid");
    GELOGE(GE_GRAPH_INIT_FAILED, "[Check][Param] compute_graph_ is nullptr");
    return GE_GRAPH_INIT_FAILED;
  }
  auto nodes = compute_graph_->GetAllNodes();
  if (nodes.empty()) {
    REPORT_INNER_ERROR("E19999", "nodes in graph is empty, check invalid");
    GELOGE(GE_GRAPH_INIT_FAILED, "[Check][Param] Invalid graph, no nodes in this graph.");
    return GE_GRAPH_INIT_FAILED;
  }
  for (const NodePtr &node : compute_graph_->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    if (node->GetOpDesc() == nullptr) {
      REPORT_INNER_ERROR("E19999", "node without opdesc exist in graph, check invalid");
      GELOGE(GE_GRAPH_INIT_FAILED, "[Get][OpDesc] failed, Check Graph node opdesc is NULL");
      return GE_GRAPH_INIT_FAILED;
    }
    if (IsInEdgeZeroNode(node)) {
      auto ret = UpdateUninitializedOriginShape(node);
      if (ret != SUCCESS) {
        REPORT_INNER_ERROR("E19999", "update original shape for node:%s failed", node->GetName().c_str());
        GELOGE(GE_GRAPH_INIT_FAILED, "[Update][OriginalShape]update original shape for node:%s failed.",
               node->GetName().c_str());
        return ret;
      }
    }
  }
  return SUCCESS;
}

Status GraphPrepare::CheckRefInputNode(const NodePtr &node, const std::string &input_name,
                                       const std::set<NodePtr> &ref_nodes) const {
  static std::set<std::string> block_list = {ge::CONSTANTOP, ge::CONSTANT};
  GE_CHECK_NOTNULL(node);
  const auto &op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  const auto input_index = op_desc->GetInputIndexByName(input_name);
  const auto &in_anchor = node->GetInDataAnchor(input_index);
  GE_CHECK_NOTNULL(in_anchor);
  const auto &peer_out_anchor = in_anchor->GetPeerOutAnchor();
  GE_CHECK_NOTNULL(peer_out_anchor);
  const auto &input_node = peer_out_anchor->GetOwnerNode();
  GE_CHECK_NOTNULL(input_node);
  const auto &input_op_desc = input_node->GetOpDesc();
  GE_CHECK_NOTNULL(input_op_desc);

  bool is_ref = (ref_nodes.find(input_node) != ref_nodes.cend());
  if (is_ref) {
    return SUCCESS;
  }
  auto input_type = input_op_desc->GetType();
  if (input_type == ge::FRAMEWORKOP) {
    if (!ge::AttrUtils::GetStr(input_op_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, input_type)) {
      REPORT_INNER_ERROR("E19999", "Get Attr:%s of op:%s(%s) failed",
                         ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE.c_str(),
                         input_op_desc->GetName().c_str(), input_op_desc->GetType().c_str());
      GELOGE(PARAM_INVALID, "[Get][Attr] %s of op:%s(%s) failed", ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE.c_str(),
             input_op_desc->GetName().c_str(), input_op_desc->GetType().c_str());
      return PARAM_INVALID;
    }
  }
  bool is_block = (block_list.find(input_type) != block_list.cend());
  if (is_block) {
    REPORT_INNER_ERROR("E19999", "The ref input memory of ref node %s[%s] cannot be referenced from const node,"
                                 "but node %s[%s] is const", node->GetName().c_str(), node->GetType().c_str(),
                       node->GetOpDesc()->GetName().c_str(),
                       node->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] The ref input memory of ref node %s[%s] cannot be referenced from const"
                          " node, but node %s[%s] is const", node->GetName().c_str(), node->GetType().c_str(),
           node->GetOpDesc()->GetName().c_str(),
           node->GetType().c_str());
    return PARAM_INVALID;
  }

  return SUCCESS;
}

Status GraphPrepare::CheckRefOp() {
  GE_CHECK_NOTNULL(compute_graph_);
  std::set<NodePtr> ref_nodes;
  for (const NodePtr &node : compute_graph_->GetDirectNode()) {
    if (node == nullptr) {
      REPORT_INNER_ERROR("E19999", "nullptr node exist in graph, check invalid");
      GELOGE(PARAM_INVALID, "[Check][Param] param [node] must not be null.");
      return PARAM_INVALID;
    }
    auto op_desc = node->GetOpDesc();
    if (op_desc == nullptr) {
      REPORT_INNER_ERROR("E19999", "node without opdesc exist in graph, check invalid");
      GELOGE(PARAM_INVALID, "[Check][Param] OpDesc of param [node] must not be null.");
      return PARAM_INVALID;
    }

    auto input_name_index = op_desc->GetAllInputName();
    auto outputs = op_desc->GetAllOutputName();
    for (const auto &name_index : input_name_index) {
      if (op_desc->GetOutputIndexByName(name_index.first) != -1) {
        if (CheckRefInputNode(node, name_index.first, ref_nodes) != SUCCESS) {
          GELOGE(PARAM_INVALID, "[Check][RefInputNode] failed, node:%s.", op_desc->GetName().c_str());
          return PARAM_INVALID;
        }
        (void)ref_nodes.insert(node); // no need to check value
      }
    }
  }
  return SUCCESS;
};

Status GraphPrepare::AdjustDataOpOutput(const NodePtr &node) const {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, check invalid");
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Check][Param] Input node is nullptr");
    return GE_GRAPH_GRAPH_NODE_NULL;
  }
  OpDescPtr op_desc_ptr = node->GetOpDesc();
  if (op_desc_ptr == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node's op_desc is nullptr, check invalid");
    GELOGE(GE_GRAPH_GRAPH_NODE_NULL, "[Get][OpDesc] Input node opdesc is NULL");
    return GE_GRAPH_GRAPH_NODE_NULL;
  }
  GeTensorDesc output = op_desc_ptr->GetOutputDesc(0);
  GeShape output_shape = output.GetShape();
  if (output_shape.IsUnknownShape()) {
    GELOGD("[Adjust][DataOpOutput] Shape of op [%s] output is unknown.", node->GetName().c_str());
    return SUCCESS;
  }

  int64_t tensor_size = 0;
  graphStatus graph_status = TensorUtils::GetTensorMemorySizeInBytes(output, tensor_size);
  if (graph_status != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "GetTensorMemorySize by ouput index:0 of op:%s(%s) failed",
                      op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
    GELOGE(graph_status, "[Call][GetTensorMemorySizeInBytes] failed, op:%s", node->GetName().c_str());
    return FAILED;
  }
  TensorUtils::SetSize(output, tensor_size);
  graphStatus graph_ret = op_desc_ptr->UpdateOutputDesc(0, output);
  if (graph_ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Update output desc of op:%s(%s) failed, index:0",
                      op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
    GELOGE(graph_ret, "[Update][OutputDesc] of op:%s(%s) failed, index:0",
           op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str());
    return graph_ret;
  }
  return SUCCESS;
}

Status GraphPrepare::CheckInternalFormat(const NodePtr &input_node, const GeTensorDesc &desc) const {
  auto format = desc.GetFormat();
  auto origin_format = desc.GetOriginFormat();
  auto tune_flag = (options_.build_mode == BUILD_MODE_TUNING);
  bool need_check_internal_format = (!IsTansDataOpData(input_node)) && (!options_.is_single_op) && (!tune_flag);
  if (need_check_internal_format) {
    bool is_internal = TypeUtils::IsInternalFormat(format) || TypeUtils::IsInternalFormat(origin_format);
    if (is_internal) {
      std::string reason = "Input format[" + TypeUtils::FormatToSerialString(format) + "] or origin_format[" +
                           TypeUtils::FormatToSerialString(origin_format) + "] of op:" + input_node->GetName() +
                           " is not support";
      REPORT_INPUT_ERROR("E19025", std::vector<std::string>({"reason"}), std::vector<std::string>({reason}));
      GELOGE(PARAM_INVALID, "[Check][Param] Input format %s or origin_format %s is not support.",
             TypeUtils::FormatToSerialString(format).c_str(), TypeUtils::FormatToSerialString(origin_format).c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status GraphPrepare::UpdateDataInputOutputDesc(int64_t index, const OpDescPtr &op, GeTensorDesc &desc) const {
  auto data_type = desc.GetDataType();
  uint32_t length = 1;
  bool type_ret = TypeUtils::GetDataTypeLength(data_type, length);
  if (!type_ret) {
    std::string reason = "Input datatype[" + TypeUtils::DataTypeToSerialString(data_type) + "] of index:" +
                         std::to_string(index) + " input tensor is not support";
    REPORT_INPUT_ERROR("E19025", std::vector<std::string>({"reason"}), std::vector<std::string>({reason}));
    GELOGE(PARAM_INVALID, "[Check][Param] Input datatype %s is not support.",
           TypeUtils::DataTypeToSerialString(data_type).c_str());
    return FAILED;
  }
  int64_t desc_shape = desc.GetShape().GetShapeSize();
  FMK_INT64_UINT32_MULCHECK(desc_shape, length);
  int64_t shape_size = desc_shape * length;
  GE_IF_BOOL_EXEC(shape_size == 0 && desc.GetShape().GetDimNum() == 0, shape_size = static_cast<int64_t>(length));
  int64_t size = 0;
  GE_IF_BOOL_EXEC(ge::TensorUtils::GetSize(desc, size) != GRAPH_SUCCESS,
                  REPORT_CALL_ERROR("E19999", "Get size of user input tensor failed, index:%ld", index);
                  GELOGE(INTERNAL_ERROR, "[Get][Size] of user input tensor failed, index:%ld", index); return FAILED);
  bool size_check = (size != 0 && shape_size != size);
  if (size_check) {
    std::string reason = "input tensor[index:" + std::to_string(index) + "]'s data size[" + std::to_string(size) +
                         "] != shape_size[" + std::to_string(size) + "], check invalid";
    REPORT_INPUT_ERROR("E19025", std::vector<std::string>({"reason"}), std::vector<std::string>({reason}));
    GELOGE(PARAM_INVALID, "[Check][Param] input data size = %ld, shape_size = %ld.", size, shape_size);
    return FAILED;
  }
  ge::TensorUtils::SetSize(desc, shape_size);

  // this attr set by tune moudle, because in tune mode ge can not decide when to refresh data format
  // by user input.
  bool skip_refresh_data_format = false;
  (void) AttrUtils::GetBool(op, "_skip_refresh_data_format", skip_refresh_data_format);
  (void) op->DelAttr("_skip_refresh_data_format");
  if (skip_refresh_data_format) {
    GELOGI("data %s skip update info in tune mode", op->GetName().c_str());
    return SUCCESS;
  }

  graphStatus graph_ret = op->UpdateInputDesc(0, desc);
  if (graph_ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Update input desc of op:%s(%s) failed, index:0", op->GetName().c_str(),
                      op->GetType().c_str());
    GELOGE(graph_ret, "[Update][InputDesc] of op:%s(%s) failed, index:0", op->GetName().c_str(), op->GetType().c_str());
    return graph_ret;
  }
  // Size will be recalculated in the build stage
  ge::TensorUtils::SetSize(desc, 0);
  graph_ret = op->UpdateOutputDesc(0, desc);
  if (graph_ret != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Update output desc of op:%s(%s) failed, index:0", op->GetName().c_str(),
                      op->GetType().c_str());
    GELOGE(graph_ret, "[Update][OutputDesc] of op:%s(%s) failed, index:0", op->GetName().c_str(),
           op->GetType().c_str());
    return graph_ret;
  }
  return SUCCESS;
}

Status GraphPrepare::UpdateUninitializedOriginShape(const NodePtr &input_node) const {
  GE_CHECK_NOTNULL(input_node);
  auto op_desc = input_node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  GELOGD("Start to update original shape for node:%s, type:%s",
         op_desc->GetName().c_str(), op_desc->GetType().c_str());
  for (size_t i = 0; i < op_desc->GetInputsSize(); ++i) {
    auto input_desc = op_desc->MutableInputDesc(i);
    GE_CHECK_NOTNULL(input_desc);
    if (input_desc->IsOriginShapeInitialized()) {
      GELOGD("The input[%zu] original shape of node:%s has already initialized, no need to change",
             i, op_desc->GetName().c_str());
      continue;
    }
    input_desc->SetOriginShape(GeShape(input_desc->GetShape().GetDims()));
    GELOGI("The input[%zu] original shape of node:%s has been changed to %s",
           i, op_desc->GetName().c_str(), input_desc->GetOriginShape().ToString().c_str());
  }

  if (op_desc->GetType() == FILECONSTANT) {
    auto output_desc = op_desc->MutableOutputDesc(0U);
    GE_CHECK_NOTNULL(output_desc);
    std::vector<int64_t> attr_shape;
    (void)AttrUtils::GetListInt(op_desc, "shape", attr_shape);
    if (!(output_desc->IsOriginShapeInitialized())) {
      output_desc->SetShape(GeShape(attr_shape));
      output_desc->SetOriginShape(GeShape(attr_shape));
      GELOGI("The output original shape of node:%s has been changed to %s", op_desc->GetName().c_str(),
             output_desc->GetOriginShape().ToString().c_str());
    }
  }
  for (size_t i = 0; i < op_desc->GetOutputsSize(); ++i) {
    auto output_desc = op_desc->MutableOutputDesc(i);
    GE_CHECK_NOTNULL(output_desc);
    if (output_desc->IsOriginShapeInitialized()) {
      GELOGD("The output[%zu] original shape of node:%s has already initialized, no need to change",
             i, op_desc->GetName().c_str());
      continue;
    }
    output_desc->SetOriginShape(GeShape(output_desc->GetShape().GetDims()));
    GELOGI("The output[%zu] original shape of node:%s has been changed to %s",
           i, op_desc->GetName().c_str(), output_desc->GetOriginShape().ToString().c_str());
  }

  if (op_desc->GetType() == CONSTANTOP || op_desc->GetType() == CONSTANT) {
    GeTensorPtr weight = nullptr;
    (void)AttrUtils::MutableTensor(op_desc, ATTR_NAME_WEIGHTS, weight);
    if (weight != nullptr && !(weight->GetTensorDesc().IsOriginShapeInitialized())) {
      weight->MutableTensorDesc().SetOriginShape(GeShape(weight->MutableTensorDesc().GetShape().GetDims()));
      GELOGI("The weight original shape of node:%s has been changed to %s",
             op_desc->GetName().c_str(), weight->GetTensorDesc().GetOriginShape().ToString().c_str());
      auto output_desc = op_desc->MutableOutputDesc(0);
      GE_CHECK_NOTNULL(output_desc);
      output_desc->SetShape(weight->MutableTensorDesc().GetShape());
      output_desc->SetOriginShape((weight->MutableTensorDesc().GetOriginShape()));
    }
  }
  return SUCCESS;
}

Status GraphPrepare::UpdateInput(const std::vector<GeTensor> &user_input,
                                 const std::map<std::string, std::string> &graph_option) {
  // Get shape range of input in dynamic_execute mode
  std::vector<std::vector<std::pair<int64_t, int64_t>>> dynamic_shape_range_vec;
  auto ret = GetDynamicInputShapeRange(user_input, graph_option, dynamic_shape_range_vec);
  GE_CHK_STATUS_RET(ret, "[Get][DynamicInputShapeRange] failed, Graph option is not right on Dynamic execute mode.");
  compute_graph_->SaveDataFormat(ge::TypeUtils::DomiFormatToFormat(GetLocalOmgContext().format));
  for (NodePtr &input_node : compute_graph_->GetDirectNode()) {
    GE_CHECK_NOTNULL(input_node);
    OpDescPtr op = input_node->GetOpDesc();
    GE_CHECK_NOTNULL(op);
    if (op->GetType() == DATA) {
      int64_t index = 0;
      if ((!(AttrUtils::GetInt(op, ATTR_NAME_INDEX, index))) || (GetLocalOmgContext().is_dynamic_input)) {
        GELOGW("Get index from data attr failed");
        continue;
      }

      if ((index < 0) || (static_cast<size_t>(index) >= user_input.size())) {
        std::string reason = "exist data op:" + input_node->GetName() + " index " + std::to_string(index) +
                             " bigger than input tensor size[" + std::to_string(user_input.size()) + "], check invalid";
        REPORT_INPUT_ERROR("E19025", std::vector<std::string>({"reason"}), std::vector<std::string>({reason}));
        GELOGE(PARAM_INVALID, "[Check][Param] user_input size = %zu, graph data op index = %ld.",
               user_input.size(), index);
        return FAILED;
      }

      if (IsDynamicDims(input_node)) {
        continue;
      }
      GeTensorDesc desc(user_input[index].GetTensorDesc());
      if (!desc.IsOriginShapeInitialized()) {
        desc.SetOriginShape(GeShape(desc.GetShape().GetDims()));
        GELOGI("user_input set original shape with null, should update with shape:%s",
               desc.GetOriginShape().ToString().c_str());
      }
      // data maybe internal format [FRACTAL_NZ] at singleop process such as GEMM.
      ret = CheckInternalFormat(input_node, desc);
      if (ret != SUCCESS) {
        GELOGE(INTERNAL_ERROR, "[Check][InternalFormat] on %s failed", op->GetName().c_str());
        return ret;
      }

      ret = UpdateDataInputOutputDesc(index, op, desc);
      if (ret != SUCCESS) {
        GELOGE(FAILED, "[Update][DataInputOutputDesc] on %s failed", op->GetName().c_str());
        return ret;
      }

      if (!dynamic_shape_range_vec.empty()) {
        ret = UpdateDynamicInputShapeRange(index, dynamic_shape_range_vec, op, desc);
        GE_CHK_STATUS_RET(ret, "[Update][DynamicInputShapeRange] on %s failed.", op->GetName().c_str());
        continue;
      }

      if (!options_.train_graph_flag) {
        ret = AdjustDataOpOutput(input_node);
        GE_IF_BOOL_EXEC(ret != SUCCESS, GELOGE(ret, "[Adjust][DataOpOutput] fail, ret:%u", ret); return ret);
      }
    }
  }

  return SUCCESS;
}

Status GraphPrepare::TryDoAipp() {
  // infer and with aipp configure file, then call aipp insert
  if ((!options_.train_graph_flag) && (!options_.insert_op_file.empty())) {
    GE_DUMP(compute_graph_, "Before_insert_aipp");
    Status ret = ge::InsertNewOpUtil::Instance().Init();
    if (ret != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Init][InsertNewOpUtil] failed.");
      return INTERNAL_ERROR;
    }
    ret = ge::InsertNewOpUtil::Instance().Parse(options_.insert_op_file.c_str());
    if (ret != SUCCESS) {
      GELOGE(GE_GRAPH_OPTIMIZE_INSERT_OP_PARSE_FAILED, "[Parse][ConfigFile] %s failed",
             options_.insert_op_file.c_str());
      return GE_GRAPH_OPTIMIZE_INSERT_OP_PARSE_FAILED;
    }
    ret = ge::InsertNewOpUtil::Instance().InsertAippOps(compute_graph_, options_.insert_op_file);
    if (ret != SUCCESS) {
      GELOGE(GE_GRAPH_OPTIMIZE_INSERT_DYN_OP_FAILED, "[Insert][AippOps] failed, ret:%u", ret);
      return GE_GRAPH_OPTIMIZE_INSERT_DYN_OP_FAILED;
    }
  }
  return SUCCESS;
}

Status GraphPrepare::FormatAndShapeProcess() {
  GE_TIMESTAMP_START(InferOriginFormat1);
  Status ret = compute_graph_->InferOriginFormat();
  GE_TIMESTAMP_END(InferOriginFormat1, "GraphPrepare::InferOriginFormat1");
  GE_DUMP(compute_graph_, "after_first_inferformat");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][InferOriginFormat] Prepare Graph first inferformat failed");
    return ret;
  }

  GE_TIMESTAMP_START(InferShapeForPreprocess);
  ret = InferShapeForPreprocess(compute_graph_, graph_rebuild_state_ctrl_, resource_context_mgr_);
  GE_DUMP(compute_graph_, "after_infershape");
  if (ret != SUCCESS) {
    GELOGE(GE_GRAPH_INFERSHAPE_FAILED, "[Call][InferShapeForPreprocess] Prepare Graph infershape failed");
    return GE_GRAPH_INFERSHAPE_FAILED;
  }
  GE_TIMESTAMP_END(InferShapeForPreprocess, "GraphPrepare::InferShapeForPreprocess");

  GE_TIMESTAMP_START(InferOriginFormat2);
  ret = compute_graph_->InferOriginFormat();
  GE_TIMESTAMP_END(InferOriginFormat2, "GraphPrepare::InferOriginFormat2");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Call][InferOriginFormat] Prepare Graph inferformat failed");
    return ret;
  }
  return ret;
}

Status GraphPrepare::UpdateDataNetOutputByStorageFormat() const {
  for (auto &node_ptr : compute_graph_->GetAllNodes()) {
    GE_CHECK_NOTNULL(node_ptr);
    if (node_ptr->GetType() == DATA) {
      uint32_t index = 0;
      auto op_desc = node_ptr->GetOpDesc();
      GE_CHECK_NOTNULL(op_desc);
      const GeTensorDescPtr input = op_desc->MutableInputDesc(index);
      Format storage_format = FORMAT_RESERVED;
      std::vector<int64_t> dst_shape_dims;
      if (GetStorageFormatAndShape(op_desc, input, storage_format, dst_shape_dims) != SUCCESS) {
        GELOGE(INTERNAL_ERROR, "[Get][StorageFormatAndShape] for input failed, op:%s, index:0",
               op_desc->GetName().c_str());
        return FAILED;
      }

      if (storage_format == FORMAT_RESERVED) {
        continue;
      }

      if (ModifyDataFormatAndShape(op_desc, index, storage_format, dst_shape_dims) != SUCCESS) {
        GELOGE(INTERNAL_ERROR, "[Modify][DataNetOutputFormatAndShape] for input failed, op:%s, index:0",
               op_desc->GetName().c_str());
        return FAILED;
      }
    }

    if (node_ptr->GetType() == ge::NETOUTPUT) {
      auto op_desc = node_ptr->GetOpDesc();
      GE_CHECK_NOTNULL(op_desc);
      for (uint32_t index = 0; index < op_desc->GetInputsSize(); index++) {
        const GeTensorDescPtr output = op_desc->MutableInputDesc(index);
        Format storage_format = FORMAT_RESERVED;
        std::vector<int64_t> dst_shape_dims;
        if (GetStorageFormatAndShape(op_desc, output, storage_format, dst_shape_dims) != SUCCESS) {
          GELOGE(INTERNAL_ERROR, "[Get][StorageFormatAndShape] from output failed, op:%s, index:%u",
                 op_desc->GetName().c_str(), index);
          return FAILED;
        }
        if (storage_format == FORMAT_RESERVED) {
          continue;
        }
        if (ModifyNetOutputFormatAndShape(op_desc, index, storage_format, dst_shape_dims) != SUCCESS) {
          GELOGE(INTERNAL_ERROR, "[Modify][DataNetOutputFormatAndShape] for output failed, op:%s, index:%u",
                 op_desc->GetName().c_str(), index);
          return FAILED;
        }
      }
    }
  }
  return SUCCESS;
}

Status GraphPrepare::SaveOriginalGraphToOmModel() const {
  if (options_.save_original_model == "true") {
    ModelHelper model_helper;
    Status ret = model_helper.SaveOriginalGraphToOmModel(ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph_),
                                                         options_.original_model_file);
    if (ret != SUCCESS) {
      // If save original model fail, process continue
      GELOGW("SaveOriginalGraphToOmModel fail");
    }
  }
  return SUCCESS;
}

#define PP_RUN_AND_DUMP(name, func, ...)                                               \
  do {                                                                                 \
    GE_RUN(Prepare, func, __VA_ARGS__);                                                \
    GE_DUMP(compute_graph, "PrepareAfter" name);                                       \
    GELOGI("Prepare %s on graph %s success.", name, compute_graph->GetName().c_str()); \
  } while (0)

#define PP_RUN(name, func, ...)                                                        \
  do {                                                                                 \
    GE_RUN(Prepare, func, __VA_ARGS__);                                                \
    GELOGI("Prepare %s on graph %s success.", name, compute_graph->GetName().c_str()); \
  } while (0)

Status GraphPrepare::PrepareInitAndUpdateInput(const GraphNodePtr &graph_node,
                                               const std::vector<GeTensor> &user_input,
                                               uint64_t session_id,
                                               GraphRebuildStateCtrl *graph_rebuild_state_ctrl,
                                               ResourceContextMgr *resource_context_mgr) {
  GE_CHECK_NOTNULL(graph_node->GetGraph());

  GetLocalOmgContext().type = static_cast<domi::FrameworkType>(options_.framework_type);
  const Graph &const_graph = *graph_node->GetGraph();
  auto compute_graph = GraphUtils::GetComputeGraph(const_graph);
  GE_CHECK_NOTNULL(compute_graph);

  PP_RUN("Init", Init, const_graph, session_id, graph_rebuild_state_ctrl, resource_context_mgr);
  PP_RUN("SetRtContext", RtContextUtil::GetInstance().SetRtContext, session_id, compute_graph->GetGraphID(),
                         GetContext().DeviceId(), RT_CTX_GEN_MODE, rtContext_t());
  PP_RUN_AND_DUMP("CheckAndUpdateInput", CheckAndUpdateInput, user_input, graph_node->GetOptions());
  PP_RUN_AND_DUMP("PropagateFormatIfNeed", PropagateFormatIfNeed, compute_graph_);
  return SUCCESS;
}
Status GraphPrepare::PrepareDynShape() {
  auto compute_graph = compute_graph_;
  PP_RUN_AND_DUMP("GraphEquivalentTransformation", GraphEquivalentTransformation);
  PP_RUN_AND_DUMP("ProcessOutput", ProcessNetOutput);
  PP_RUN_AND_DUMP("ProcessMultiBatch", multibatch::ProcessMultiBatch, compute_graph_, session_id_);
  PP_RUN_AND_DUMP("InsertAipp", TryDoAipp);
  PP_RUN_AND_DUMP("ProcessBeforeInfershape", ProcessBeforeInfershape);
  PP_RUN_AND_DUMP("InferFormatAndShape", FormatAndShapeProcess);
  PP_RUN_AND_DUMP("CtrlFlowPreProcess", CtrlFlowPreProcess);
  PP_RUN_AND_DUMP("GetDynamicOutputShape", multibatch::GetDynamicOutputShape, compute_graph_);
  PP_RUN_AND_DUMP("ProcessAippStage2", InsertNewOpUtil::Instance().UpdateDataNodeByAipp, compute_graph_);
  PP_RUN("SaveOriginalGraphToOmModel", SaveOriginalGraphToOmModel);
  PP_RUN_AND_DUMP("PrepareOptimize", PrepareOptimize);

  return SUCCESS;
}

Status GraphPrepare::CtrlFlowPreProcess() const {
  PassManager graph_pass;

  // After InferShape Mark v1 control flow for unknown shape.
  GE_CHK_STATUS_RET(graph_pass.AddPass("PreRun::MarkForceUnknownForCondPass",
                                       new (std::nothrow) MarkForceUnknownForCondPass));

  GE_CHK_STATUS_RET(graph_pass.Run(compute_graph_));
  return SUCCESS;
}

Status GraphPrepare::RecordAIPPInfo(const ge::ComputeGraphPtr &compute_graph) const {
  PP_RUN("RecordAIPPInfo", InsertNewOpUtil::Instance().RecordAIPPInfoToData, compute_graph_);
  return SUCCESS;
}

Status GraphPrepare::PrepareRunningFormatRefiner() {
  auto compute_graph = compute_graph_;
  PassManager pass_manager;
  GE_CHK_STATUS_RET(pass_manager.AddPass("PrepareRunningFormatRefiner::VariablePrepareOpPass",
                                         new (std::nothrow) VariablePrepareOpPass));
  GE_TIMESTAMP_START(pass_manager);
  auto ret = pass_manager.Run(compute_graph);
  GE_TIMESTAMP_END(pass_manager, "GraphPrepare::PrepareRunningFormatRefiner");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][Passes] for running format refiner failed, ret:%u.", ret);
    return ret;
  }
  PP_RUN_AND_DUMP("UpdateInputOutputByUserOptions", UpdateInputOutputByOptions);
  PP_RUN_AND_DUMP("UpdateVariableFormats", UpdateVariableFormats, compute_graph_);
  return SUCCESS;
}

Status GraphPrepare::SwitchOpOptimize(ComputeGraphPtr &compute_graph) const {
  if (compute_graph == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param compute_graph is nullptr, check invalid");
    GELOGE(GE_GRAPH_NULL_INPUT, "[Check][Param] Input Graph is NULL");
    return GE_GRAPH_NULL_INPUT;
  }
  GEPass ge_passes(compute_graph);
  NamesToPass hccl_group;
  HcclGroupPass hccl_group_pass;
  GELOGD("Add hccl group pass success");
  hccl_group.emplace_back("HcclGroupPass", &hccl_group_pass);
  auto ret = ge_passes.Run(hccl_group);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][HcclGroupPass] pass for preprocess failed, ret:%u.", ret);
    return ret;
  }
  ret = compute_graph->TopologicalSorting();
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Topological sorting failed");
    GELOGE(ret, "[Call][TopologicalSorting] Graph topological sort failed, ret:%u.", ret);
    return ret;
  }
  return SUCCESS;
}
#undef PP_RUN_AND_DUMP
#undef PP_RUN

Status GraphPrepare::GenerateInfershapeGraph(ConstGraphPtr graph) {
  if (graph == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param graph is nullptr, check invalid");
    GELOGE(GE_GRAPH_NULL_INPUT, "[Check][Param] Input Graph is NULL");
    return GE_GRAPH_NULL_INPUT;
  }
  const Graph &const_graph = *graph;
  Status ret = Init(const_graph, 0);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Init][GraphPrepare] fail, ret:%u", ret);
    return ret;
  }
  GE_DUMP(compute_graph_, "after_parser");
  GELOGI("Start infershape for dump json process.");
  ret = compute_graph_->InferOriginFormat();
  GE_DUMP(compute_graph_, "after_inferformat");
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Infer OriginFormat failed");
    GELOGE(ret, "[Infer][OriginFormat] failed");
    return ret;
  }
  InferShapePass infer_shape_pass;
  NamesToPass names_to_passes;
  names_to_passes.emplace_back("InferShapePass", &infer_shape_pass);
  GEPass ge_passes(compute_graph_);
  ret = ge_passes.Run(names_to_passes);
  GE_DUMP(compute_graph_, "after_infershape");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][GePasses] infershape for preprocess failed, ret:%u.", ret);
    return ret;
  }
  ShapeRefiner::ClearContextMap();
  return SUCCESS;
}

Status GraphPrepare::CheckConstOp() const {
  for (auto &node_ptr : compute_graph_->GetAllNodes()) {
    GE_CHECK_NOTNULL(node_ptr);
    if (node_ptr->GetType() == CONSTANT) {
      Status ret = VerifyConstOp(node_ptr);
      GE_CHK_BOOL_RET_STATUS(ret == SUCCESS, ret, "Const Op Check failed");
    } else if (node_ptr->GetType() == FRAMEWORKOP) {
      auto op_desc = node_ptr->GetOpDesc();
      if (op_desc == nullptr) {
        REPORT_INNER_ERROR("E19999", "op_desc is nullptr, check invalid");
        GELOGE(PARAM_INVALID, "[Get][OpDesc] of node failed, op_desc is nullptr, node type:FRAMEWORKOP.");
        return PARAM_INVALID;
      }
      std::string original_type;
      GE_IF_BOOL_EXEC(ge::AttrUtils::GetStr(op_desc, ATTR_NAME_FRAMEWORK_ORIGINAL_TYPE, original_type),
                      GELOGI("Get FrameWorkOp original type [%s]", original_type.c_str()));
      GELOGI("original type is %s", original_type.c_str());
      if (original_type == CONSTANT) {
        Status ret = VerifyConstOp(node_ptr);
        GE_CHK_BOOL_RET_STATUS(ret == SUCCESS, ret, "Const Op Check failed");
      }
    }
  }
  return SUCCESS;
}
Status GraphPrepare::VerifyConstOp(const NodePtr &node) const {
  GE_CHECK_NOTNULL(node);
  auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  ConstGeTensorPtr ge_tensor_ptr;
  if (!(AttrUtils::GetTensor(op_desc, ATTR_NAME_WEIGHTS, ge_tensor_ptr))) {
    REPORT_INNER_ERROR("E19999", "Get Attr:%s of op:%s(%s) failed", ATTR_NAME_WEIGHTS.c_str(),
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Get][Attr] %s of op:%s(%s) failed", ATTR_NAME_WEIGHTS.c_str(),
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return PARAM_INVALID;
  }
  GE_CHECK_NOTNULL(ge_tensor_ptr);
  auto data_size = ge_tensor_ptr->GetData().GetSize();
  auto ge_tensor_desc = ge_tensor_ptr->GetTensorDesc();
  int64_t shape_size = ge_tensor_desc.GetShape().GetShapeSize();
  auto data_type = ge_tensor_desc.GetDataType();
  if (data_type == DT_STRING) {
    return SUCCESS;
  }
  uint32_t length = 1;
  bool type_ret = TypeUtils::GetDataTypeLength(data_type, length);
  if (!type_ret) {
    REPORT_INNER_ERROR("E19999", "const node:%s's input datatype:%s it is not support",
                       node->GetName().c_str(), TypeUtils::DataTypeToSerialString(data_type).c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] Input datatype %s is not support.",
           TypeUtils::DataTypeToSerialString(data_type).c_str());
    return FAILED;
  }
  FMK_INT64_UINT32_MULCHECK(shape_size, length);
  GELOGI("Const real value Size:%zu, op_desc Shape Size:%ld, data_type:%s.", data_size, shape_size * length,
         TypeUtils::DataTypeToSerialString(data_type).c_str());
  if (shape_size == 0) {
    if (ge_tensor_desc.GetShape().GetDims().size() == 0) {
      // shape = [], means it's a sclar tensor.
      GE_CHK_BOOL_EXEC(data_size / length == 1,
          REPORT_INNER_ERROR("E19999", "Const Node:%s is invalid, data size:%zu not equal to tensor size:%u",
                             node->GetName().c_str(), data_size, length);
          return PARAM_INVALID, "[Check][Param] Const is invalid scalar tensor.");
    } else {
      // shape = [x, y, 0,...], means it's a vector tensor that value is [].
      GE_CHK_BOOL_EXEC(data_size == 0,
          REPORT_INNER_ERROR("E19999", "Const Node:%s is invalid, data size:%zu not equal to tensor size:0",
                             node->GetName().c_str(), data_size);
          return PARAM_INVALID, "[Check][Param] Const is invalid vector scalar.");
    }
  } else {
    GE_CHK_BOOL_EXEC(
        data_size == static_cast<size_t>(shape_size * length) && data_size != 0,
        REPORT_INNER_ERROR("E19999", "Const Node:%s is invalid, data size:%zu not equal to tensor size:%ld",
                           node->GetName().c_str(), data_size, shape_size * length);
        return PARAM_INVALID, "[Check][Param] Const input data size is not equal with tensor desc shape");
  }
  return SUCCESS;
}

bool GraphPrepare::IsDynamicDims(const NodePtr &input_node) const {
  auto data_shape = NodeUtils::GetOutputDesc(*input_node, kDataOutIndex).GetShape();
  const auto &dims = data_shape.GetDims();
  bool all_is_positive = false;
  if (std::all_of(dims.begin(), dims.end(), [](int64_t val) { return val >= 0; })) {
    all_is_positive = true;
  }
  if (!all_is_positive && !options_.input_shape.empty() && !options_.dynamic_dims.empty() &&
      options_.dynamic_node_type != kInvalidDynaimcDimsType) {
    GELOGI("No need to check and update desc info, the dims of %s is %s.", input_node->GetName().c_str(),
           formats::JoinToString(dims).c_str());
    return true;
  }
  return false;
}

Status GraphPrepare::CheckUserInput(const std::vector<GeTensor> &user_input) {
  if (GetLocalOmgContext().is_dynamic_input) {
    return SUCCESS;
  }
  uint32_t node_num = 0U;
  uint32_t data_num = 0U;
  for (NodePtr &input_node : compute_graph_->GetDirectNode()) {
    GE_CHECK_NOTNULL(input_node);
    OpDescPtr op = input_node->GetOpDesc();
    GE_CHECK_NOTNULL(op);
    node_num++;
    if (op->GetType() == DATA || op->GetType() == AIPPDATA) {
      data_num++;
      int64_t index = 0;
      if (!(AttrUtils::GetInt(op, ATTR_NAME_INDEX, index))) {
        REPORT_INNER_ERROR("E19999", "Get Attr:%s of op:%s(%s) failed", ATTR_NAME_WEIGHTS.c_str(),
                           op->GetName().c_str(), op->GetType().c_str());
        GELOGE(GE_GRAPH_INIT_FAILED, "[Get][Attr] %s of op:%s(%s) failed", ATTR_NAME_WEIGHTS.c_str(),
               op->GetName().c_str(), op->GetType().c_str());
        return GE_GRAPH_INIT_FAILED;
      }
      if ((index < 0) || (static_cast<size_t>(index) >= user_input.size())) {
        std::string reason = "exist data op:" + input_node->GetName() + " index " + std::to_string(index) +
                             " bigger than input tensor size[" + std::to_string(user_input.size()) + "], check invalid";
        REPORT_INPUT_ERROR("E19025", std::vector<std::string>({"reason"}), std::vector<std::string>({reason}));
        GELOGE(GE_GRAPH_INIT_FAILED, "[Check][Param] user_input size:%zu must larger than data op index:%ld.",
               user_input.size(), index);
        return GE_GRAPH_INIT_FAILED;
      }
      if (IsDynamicDims(input_node)) {
        continue;
      }
      GeTensorDesc desc(user_input[index].GetTensorDesc());

      for (size_t i = 0; i < desc.GetShape().GetDimNum(); ++i) {
        int64_t dim = desc.GetShape().GetDim(i);
        if (dim < UNKNOWN_DIM_NUM) {
          std::string reason = "data dim[" + std::to_string(i) + "][" + std::to_string(dim) + "] of index:" +
                               std::to_string(index) + " input tensor it need >= -2";
          REPORT_INPUT_ERROR(
              "E19025", std::vector<std::string>({"reason"}), std::vector<std::string>({reason}));
          GELOGE(GE_GRAPH_INIT_FAILED, "[Check][InputDim]data dim %zu is not supported, need >= -2, real:%ld.", i, dim);
          return GE_GRAPH_INIT_FAILED;
        }
      }
    }
  }
  if (node_num <= data_num) {
    GELOGW("Prepare check user input, data_num = %u, node_num = %u", data_num, node_num);
  }
  return SUCCESS;
}

Status GraphPrepare::InferShapeForPreprocess(ComputeGraphPtr &compute_graph, GraphRebuildStateCtrl *rebuild_ctrl,
                                             ResourceContextMgr *resource_mgr) {
  GELOGI("Start infershape for preprocess.");
  // Prepare dummy_shape for v1 control_flow op before infershape
  InitDummyShapeOnControlFlow(compute_graph);

  GEPass ge_passes(compute_graph);
  NamesToPass after_infer_passes;
  PotentialConstTakenEffectPass convert_potential_const_to_official_pass;
  after_infer_passes.emplace_back("ConvertPotentialConstantToOfficialPass", &convert_potential_const_to_official_pass);
  ge_passes.AddPassAfterGraphOptimized(after_infer_passes);

  NamesToPass names_to_passes;
  AssertPass assert_pass;
  names_to_passes.emplace_back("AssertPass", &assert_pass);
  SwitchDeadBranchElimination switch_dead_branch_elimination;
  names_to_passes.emplace_back("SwitchDeadBranchElimination", &switch_dead_branch_elimination);
  MergePass merge_pass;
  names_to_passes.emplace_back("MergePass", &merge_pass);
  InferShapePass infer_shape_pass(rebuild_ctrl, resource_mgr);
  names_to_passes.emplace_back("InferShapePass", &infer_shape_pass);
  bool need_fold = false;
  ReplaceWithEmptyConstPass replace_with_empty_const_pass(need_fold);
  names_to_passes.emplace_back("ReplaceWithEmptyConstPass", &replace_with_empty_const_pass);
  SplitShapeNPass split_shape_n_pass;
  names_to_passes.emplace_back("SplitShapeNPass", &split_shape_n_pass);
  DimensionComputePass dimension_compute_pass(need_fold);
  names_to_passes.emplace_back("DimensionComputePass", &dimension_compute_pass);
  ConstantFoldingPass constant_folding_pass;
  names_to_passes.emplace_back("ConstantFoldingPass", &constant_folding_pass);
  InferValueRangePass infer_value_pass;
  names_to_passes.emplace_back("InferValuePass", &infer_value_pass);

  AicpuConstantFoldingPass aicpu_constant_folding_pass;
  if (aicpu_constant_folding_pass.Init(compute_graph->GetSessionID(), compute_graph) == SUCCESS) {
    names_to_passes.emplace_back("AicpuConstantFoldingPass", &aicpu_constant_folding_pass);
  }

  Status ret = ge_passes.Run(names_to_passes);
  auto finalize_ret = aicpu_constant_folding_pass.Finalize();
  if (finalize_ret != SUCCESS) {
    return finalize_ret;
  }
  ShapeRefiner::ClearContextMap();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][GePasses] infershape for preprocess failed, ret:%u.", ret);
    return ret;
  }
  return SUCCESS;
}
Status GraphPrepare::PrepareOptimize() {
  GELOGI("Start optimize for preprocess.");
  // check rw type
  GraphOptimize graph_optimize;
  bool has_conflict = false;
  graph_optimize.CheckRWConflict(compute_graph_, has_conflict);
  if (has_conflict) {
    GELOGE(GRAPH_PARAM_INVALID, "[Check][RWConflict] There has rw conflict.Stop optimize.");
    return FAILED;
  }
  PassManager original_graph_passes;
  // Graph pass
  try {
    (void)original_graph_passes.AddPass("PrepareOptimize::ShapeOperateOpRemovePass", new ShapeOperateOpRemovePass);
    (void)original_graph_passes.AddPass("PrepareOptimize::ReplaceTransShapePass", new ReplaceTransShapePass);
    (void)original_graph_passes.AddPass("PrepareOptimize::MarkAgnosticPass", new MarkAgnosticPass);
  } catch (std::bad_alloc &e) {
    REPORT_INNER_ERROR("E19999", "bad memory allocation occur when add Pass");
    GELOGE(INTERNAL_ERROR, "[Add][Pass] failed, bad memory allocation occurs.");
    return INTERNAL_ERROR;
  }

  GE_TIMESTAMP_START(original_graph_passes);
  Status ret = original_graph_passes.Run(compute_graph_);
  GE_TIMESTAMP_END(original_graph_passes, "GraphPrepare::OriginalGraphPasses");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][GraphPasses] optimize for preprocess failed, ret:%u.", ret);
    return ret;
  }
  // New pass
  GEPass ge_passes(compute_graph_);
  NamesToPass names_to_passes;
  EnterPass enter_pass;
  names_to_passes.emplace_back("EnterPass", &enter_pass);
  CondPass cond_pass;
  names_to_passes.emplace_back("CondPass", &cond_pass);
  PrintOpPass print_pass;
  if (options_.enable_print_op_pass) {
    names_to_passes.emplace_back("PrintOpPass", &print_pass);
  }
  NoUseReshapeRemovePass no_use_reshape_remove_pass;
  names_to_passes.emplace_back("NoUseReshapeRemovePass", &no_use_reshape_remove_pass);

  DropOutPass dropout_pass;
  AssertPass assert_pass;
  UnusedConstPass unused_const_pass;
  StopGradientPass stop_gradient_pass;
  PreventGradientPass prevent_gradient_pass;
  PlaceholderWithDefaultPass placeholder_with_default_pass;
  GuaranteeConstPass guarantee_const_pass;
  VarIsInitializedOpPass var_is_initialized_pass;
  ParallelConcatStartOpPass parallel_concat_start_op_pass;
  IdentityPass identity_pass(false);
  SnapshotPass snapshot_pass;
  if (!options_.train_graph_flag) {
    names_to_passes.emplace_back("DropOutPass", &dropout_pass);
    names_to_passes.emplace_back("AssertPass", &assert_pass);
  }
  names_to_passes.emplace_back("UnusedConstPass", &unused_const_pass);
  names_to_passes.emplace_back("StopGradientPass", &stop_gradient_pass);
  names_to_passes.emplace_back("PreventGradientPass", &prevent_gradient_pass);
  names_to_passes.emplace_back("PlaceholderWithDefaultPass", &placeholder_with_default_pass);
  names_to_passes.emplace_back("SnapshotPass", &snapshot_pass);
  names_to_passes.emplace_back("GuaranteeConstPass", &guarantee_const_pass);
  names_to_passes.emplace_back("VarIsInitializedOpPass", &var_is_initialized_pass);
  names_to_passes.emplace_back("ParallelConcatStartOpPass", &parallel_concat_start_op_pass);
  names_to_passes.emplace_back("IdentityPass", &identity_pass);
  GE_TIMESTAMP_START(names_to_passes);
  ret = ge_passes.Run(names_to_passes);
  GE_TIMESTAMP_END(names_to_passes, "GraphPrepare::NamesToPasses");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][GePasses] optimize for preprocess failed, ret:%u.", ret);
    return ret;
  }

  PassManager graph_pass;
  try {
    (void)graph_pass.AddPass("PrepareOptimize::PrunePass", new PrunePass);
    // can't move to optimize1/2 directly, may cause more identity insert, cause CI fail
    (void)graph_pass.AddPass("PrepareOptimize::HcclMemcpyPass", new HcclMemcpyPass);
  } catch (std::bad_alloc &e) {
    REPORT_INNER_ERROR("E19999", "bad memory allocation occur when add Pass");
    GELOGE(INTERNAL_ERROR, "[Add][Pass] failed, bad memory allocation occurs.");
    return INTERNAL_ERROR;
  }

  GE_TIMESTAMP_START(graph_passes);
  ret = graph_pass.Run(compute_graph_);
  GE_TIMESTAMP_END(graph_passes, "GraphPrepare::GraphPasses");
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    GELOGE(ret, "[Run][GraphPasses] optimize for preprocess failed, ret:%u.", ret);
    return ret;
  }
  // The constant for train is CONSTANTOP, and is CONSTANT for inference. They will be unified in future.
  TypeConversionOfConstant();

  ret = compute_graph_->TopologicalSorting();
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Topological sorting failed");
    GELOGE(ret, "[Call][TopologicalSorting] Graph topological sort failed, ret:%u.", ret);
    return ret;
  }

  GELOGI("End optimize for preprocess.");

  return SUCCESS;
}

void GraphPrepare::TypeConversionOfConstant() const {
  bool is_acl_compile = false;
  for (ge::NodePtr &n : compute_graph_->GetAllNodes()) {
    // This can ensure that n is not a null pointer
    // No Conversion when called by aclOpCompile
    (void)AttrUtils::GetBool(n->GetOpDesc(), ATTR_SINGLE_OP_SCENE, is_acl_compile);
    if (is_acl_compile) {
      return;
    }
  }

  if (options_.train_graph_flag) {
    GELOGD("trans CONSTANT to CONSTANTOP in train.");
    for (ge::NodePtr &n : compute_graph_->GetAllNodes()) {
      // This can ensure that n is not a null pointer
      if (n->GetOpDesc()->GetType() == CONSTANT) {
        n->GetOpDesc()->SetType(CONSTANTOP);
      }
    }
  } else {
    GELOGD("trans CONSTANTOP to CONSTANT in inferrence.");
    for (ge::NodePtr &n : compute_graph_->GetAllNodes()) {
      // This can ensure that n is not a null pointer
      if (n->GetOpDesc()->GetType() == CONSTANTOP) {
        n->GetOpDesc()->SetType(CONSTANT);
      }
    }
  }
}

Status GraphPrepare::GraphEquivalentTransformation() {
  NamesToPass names_to_pass;
  ForPass for_pass;
  names_to_pass.emplace_back("ForToWhilePass", &for_pass);
  return GEPass(compute_graph_).Run(names_to_pass);
}

Status GraphPrepare::ProcessBeforeInfershape() {
  NamesToPass names_to_passes;
  CondRemovePass condition_remove_pass;
  names_to_passes.emplace_back("CondRemovePass", &condition_remove_pass);
  GE_TIMESTAMP_START(ProcessCondRemove);
  auto ret = GEPass(compute_graph_).Run(names_to_passes);
  GE_TIMESTAMP_END(ProcessCondRemove, "GraphManager::ProcessCondRemove");
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][GEPass] optimize for OptimizeAfterMergeSubGraph failed, ret:%d.", ret);
    return ret;
  }
  return SUCCESS;
}

Status GraphPrepare::ProcessNetOutput() const {
  PassManager graph_passes_before_infershape;
  try {
    graph_passes_before_infershape.AddPass("ProcessNetOutput::SavePass", new (std::nothrow) SavePass);
    graph_passes_before_infershape.AddPass("ProcessNetOutput::NetOutputPass", new (std::nothrow) NetOutputPass);
    graph_passes_before_infershape.AddPass("ProcessNetOutput::DataPass",
                                           new (std::nothrow) DataPass);  // Add NetOutput first.
  } catch (std::bad_alloc &) {
    REPORT_INNER_ERROR("E19999", "bad memory allocation occur when add Pass");
    GELOGE(INTERNAL_ERROR, "[Add][Pass] failed, bad memory allocation occurs.");
    return INTERNAL_ERROR;
  }

  auto ret = graph_passes_before_infershape.Run(compute_graph_);
  if ((ret != SUCCESS) && (ret != NOT_CHANGED)) {
    GELOGE(ret, "[Run][GraphPasses] before Infershape failed, ret:%d.", ret);
    return ret;
  }
  return SUCCESS;
}

Status GraphPrepare::CheckAndUpdateInput(const std::vector<GeTensor> &user_input,
                                         const std::map<std::string, std::string> &graph_option) {
  compute_graph_->SetInputSize(user_input.size());
  if (user_input.empty()) {
    return SUCCESS;
  }

  auto ret = CheckUserInput(user_input);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Check][UserInput] failed, ret:%u", ret);
    return ret;
  }

  ret = UpdateInput(user_input, graph_option);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Update][Input] fail, ret:%u", ret);
    return ret;
  }
  if (user_input.size() != 0) {
    ret = CheckConstOp();
    if (ret != SUCCESS) {
      GELOGE(ret, "[Check][ConstOp] fail, ret:%u", ret);
      return ret;
    }
  } else {
    ret = compute_graph_->TopologicalSorting();
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Topological sorting failed");
      GELOGE(ret, "[Call][TopologicalSorting] failed.");
      return FAILED;
    }
  }
  return SUCCESS;
}
Status GraphPrepare::UpdateInputOutputByOptions() {
  auto ret = UpdateDataNetOutputByStorageFormat();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Update][DataNetOutputByStorageFormat] failed.");
    return ret;
  }

  if (options_.train_graph_flag) {
    GELOGI("This is train mode, no need to do this schedule.");
    return SUCCESS;
  }
  for (auto &node_ptr : compute_graph_->GetDirectNode()) {
    GE_CHECK_NOTNULL(node_ptr);
    if (CheckIfNeedSetNdFormat(node_ptr) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Set][NdFormat] for node:%s failed", node_ptr->GetName().c_str());
      return FAILED;
    }

    if (node_ptr->GetType() == DATA) {
      if (ProcessDataNodeDynShape(node_ptr) != SUCCESS) {
        GELOGE(INTERNAL_ERROR, "[Call][ProcessDataNodeDynShape] for node:%s failed", node_ptr->GetName().c_str());
        return FAILED;
      }
    }

    if (node_ptr->GetType() == ge::NETOUTPUT) {
      if (ProcessNetoutputNodeDynShape(node_ptr) != SUCCESS) {
        GELOGE(INTERNAL_ERROR, "[Call][ProcessNetoutputNodeDynShape] for node:%s failed", node_ptr->GetName().c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

bool GraphPrepare::IsTansDataOpData(const ge::NodePtr &var_node) const {
  for (auto &out_anchor : var_node->GetAllOutDataAnchors()) {
    GE_RT_FALSE_CHECK_NOTNULL(out_anchor);
    for (auto &in_anchor : out_anchor->GetPeerInDataAnchors()) {
      GE_RT_FALSE_CHECK_NOTNULL(in_anchor);
      ge::NodePtr dst_node = in_anchor->GetOwnerNode();
      GE_RT_FALSE_CHECK_NOTNULL(dst_node);
      if (dst_node->GetType() == TRANSDATA) {
        return true;
      }
    }
  }
  return false;
}
}  // namespace ge
