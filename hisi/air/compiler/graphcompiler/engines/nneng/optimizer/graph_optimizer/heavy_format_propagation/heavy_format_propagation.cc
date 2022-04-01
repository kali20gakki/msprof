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

#include "heavy_format_propagation.h"
#include "common/configuration.h"
#include "common/format/axis_name_util.h"
#include "common/graph/fe_graph_utils.h"
#include "common/op_info_common.h"
#include "common/unknown_shape_util.h"
#include "graph/utils/node_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"
#include "ops_store/ops_kernel_manager.h"

namespace fe {
HeavyFormatPropagation::HeavyFormatPropagation(const std::string &engine_name,
                                               OpStoreAdapterManagerPtr op_store_adapter_manager_ptr,
                                               RefRelationsPtr reflection_builder_ptr)
    : engine_name_(engine_name),
      op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr),
      reflection_builder_ptr_(reflection_builder_ptr),
      format_dtype_querier_ptr_(nullptr) {
}

HeavyFormatPropagation::~HeavyFormatPropagation() {}

Status HeavyFormatPropagation::Initialize() {
  for (uint32_t i = 0; i < EN_RESERVED; ++i) {
    imply_type_string_vec_.push_back("");
  }

  FE_MAKE_SHARED(format_dtype_querier_ptr_ = std::make_shared<FormatDtypeQuerier>(op_store_adapter_manager_ptr_),
                 return FAILED);
  FE_MAKE_SHARED(
      format_dtype_setter_ptr_ = std::make_shared<FormatDtypeSetter>(engine_name_, op_store_adapter_manager_ptr_),
      return FAILED);
  FE_MAKE_SHARED(supportformats_updater_ptr_ = std::make_shared<HeavyFormatSupportFormatsUpdater>(
                     format_dtype_querier_ptr_, format_dtype_setter_ptr_),
                 return FAILED);

  FE_MAKE_SHARED(heavy_format_selector_ptr_ = std::make_shared<HeavyFormatSelector>(format_dtype_querier_ptr_),
                 return FAILED);
  if (heavy_format_selector_ptr_ == nullptr) {
    return FAILED;
  }
  return heavy_format_selector_ptr_->Initalize();
}

bool HeavyFormatPropagation::IsHeavyOp(const OpKernelInfoPtr &op_kernel_info_ptr) {
  if (op_kernel_info_ptr == nullptr) {
    return false;
  }
  return op_kernel_info_ptr->GetHeavyOpFlag();
}

bool IsWeightOrData(const ge::OpDescPtr &op_desc_ptr) {
  // sub graph DATA is not DATA op
  bool is_subgraph_data = FeGraphUtils::IsSubGraphData(op_desc_ptr);
  if (!is_subgraph_data) {
    return IsWeight(op_desc_ptr) || op_desc_ptr->GetType() == DATA;
  }

  return false;
}

bool HeavyFormatPropagation::IsConstNodeInSubGraph(const ge::NodePtr &node_ptr) const {
  if (node_ptr == nullptr) {
    return false;
  }
  FE_LOGI("IsConstNodeInSubGraph begin, %s", node_ptr->GetName().c_str());
  if (node_ptr->GetType() != DATA) {
    return false;
  }

  ge::NodePtr parent_node_ptr = ge::NodeUtils::GetParentInput(node_ptr);
  if (parent_node_ptr == nullptr) {
    FE_LOGI("parent node is nullptr.");
    return false;
  }

  FE_LOGI("IsConstNodeInSubGraph parent node, [%s, %s]", parent_node_ptr->GetName().c_str(),
          parent_node_ptr->GetType().c_str());
  return parent_node_ptr->GetType() == CONSTANTOP || parent_node_ptr->GetType() == CONSTANT;
}

bool HeavyFormatPropagation::IsHeavyFormat(const ge::Format &format) const {
  return std::find(FE_HEAVY_FORMAT_VECTOR.begin(), FE_HEAVY_FORMAT_VECTOR.end(), format) !=
         FE_HEAVY_FORMAT_VECTOR.end();
}

Status CheckScalarOrResult(ge::ConstGeTensorDescPtr current_tensor, const TensorInfoPtr &tensor_info_ptr) {
  auto shape = current_tensor->GetShape();
  if (shape.IsScalar() && tensor_info_ptr->format_selection_type == FormatSelectionType::FORMAT_PENETRATION) {
    FE_LOGW("Stop traversing from this scalar weight %s!", tensor_info_ptr->op_desc_ptr->GetName().c_str());
    return GRAPH_OPTIMIZER_STOP_TRAVERSING_SCALAR_WEIGHT;
  }
  if (shape.IsScalar() || (shape.GetDimNum() == 1 && shape.GetDims().at(0) == 1)) {
    FE_LOGW("Stop traversing from this Scalar tensor %s!", tensor_info_ptr->op_desc_ptr->GetName().c_str());
    return GRAPH_OPTIMIZER_STOP_TRAVERSING_SCALAR_TENSOR;
  }
  return SUCCESS;
}

/* If the next op supports heavy format NDC1HWC0, but the original shape of
 * the next op does not match to the correct size of its original format
 * NDHWC(which requires five dimensions), we stop propagating. */
Status CheckOriginalShapeSizeValid(ge::ConstGeTensorDescPtr current_tensor, const TensorInfoPtr &tensor_info_ptr) {
  ge::Format primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(current_tensor->GetFormat()));
  auto dim_num = current_tensor->GetShape().GetDimNum();
  if (tensor_info_ptr->heavy_format == ge::FORMAT_NDC1HWC0 &&
      (primary_format == ge::FORMAT_NDHWC || primary_format == ge::FORMAT_NCDHW) && dim_num != DIMENSION_NUM_FIVE) {
    FE_LOGD("Dim of current tensor %d of op %s is %zu. Current format is %s", tensor_info_ptr->anchor_index,
            tensor_info_ptr->op_desc_ptr->GetName().c_str(), dim_num, FormatToStr(primary_format).c_str());
    return FAILED;
  }
  return SUCCESS;
}

/* In current stage: HWCN and NC1HWC0, NHWC and FRACTAL_Z are two
 * inconsistent cases. */
bool IsHeavyFormatConsistentWithOriFormat(const ge::GeTensorDescPtr &current_tensor,
                                          const TensorInfoPtr &tensor_info_ptr) {
  return !(
      (current_tensor->GetOriginFormat() == ge::FORMAT_HWCN && tensor_info_ptr->heavy_format == ge::FORMAT_NC1HWC0) ||
      (current_tensor->GetOriginFormat() == ge::FORMAT_ND && tensor_info_ptr->heavy_format == ge::FORMAT_NC1HWC0) ||
      (current_tensor->GetOriginFormat() == ge::FORMAT_ND && tensor_info_ptr->heavy_format == ge::FORMAT_NDC1HWC0) ||
      (current_tensor->GetOriginFormat() == ge::FORMAT_NHWC && tensor_info_ptr->heavy_format == ge::FORMAT_FRACTAL_Z) ||
      (tensor_info_ptr->heavy_format == ge::FORMAT_FRACTAL_NZ &&
       current_tensor->GetOriginShape().GetDimNum() < MINIMUM_NZ_SHAPE_DIM_NUM) ||
      (current_tensor->GetDataType() == ge::DT_INT64) ||
      (current_tensor->GetOriginFormat() == ge::FORMAT_ND && tensor_info_ptr->heavy_format == ge::FORMAT_FRACTAL_Z &&
       current_tensor->GetOriginShape().GetDimNum() > MINIMUM_NZ_SHAPE_DIM_NUM));
}

Status HeavyFormatPropagation::SetInferFormat(const TensorInfoPtr &tensor_info_ptr) {
  ge::GeTensorDescPtr current_tensor;
  if (tensor_info_ptr->is_input) {
    current_tensor = tensor_info_ptr->op_desc_ptr->MutableInputDesc(tensor_info_ptr->anchor_index);
  } else {
    current_tensor = tensor_info_ptr->op_desc_ptr->MutableOutputDesc(tensor_info_ptr->anchor_index);
  }
  FE_CHECK_NOTNULL(current_tensor);
  int64_t infer_format = -1;
  /* If the output tensor has been visited, we will still return success.
   * Because one output could give to multiple input. */
  if (ge::AttrUtils::GetInt(current_tensor, INFER_FORMAT, infer_format)) {
    FE_LOGD("%s %d of Op %s has been propagated! Its infer format is %ld.",
            IS_INPUT_TO_STRING(tensor_info_ptr->is_input), tensor_info_ptr->anchor_index,
            tensor_info_ptr->op_desc_ptr->GetName().c_str(), infer_format);
    FE_LOGD("Current heavy_format is %s, sub_format is %d", FormatToStr(tensor_info_ptr->heavy_format).c_str(),
            tensor_info_ptr->sub_format);
    return GRAPH_OPTIMIZER_STOP_TRAVERSING_OTHER_ANCHORS;
  }
  /* Here a abnormal case will show up due to we stop propagation through all
   * inconsistent edges. The case is :
   *       input0(NCHW)     input1(ND)
   *              \        /
   *               \      /
   *                \    /
   *                 \  /
   *                  op
   *                  |
   *                 Conv2D
   * The first input will be inferred as 5HD and the second will still be ND.
   * We consider if op is a normal op if the original format is NCHW and ND,
   * it supports 5HD and ND as two inputs. If op is function op, it is also support two
   * inputs as 5HD and ND. */
  if (!IsHeavyFormatConsistentWithOriFormat(current_tensor, tensor_info_ptr)) {
    FE_LOGD("Original format %u is not consistent with heavy_format %s, sub_format %d.",
            current_tensor->GetOriginFormat(), FormatToStr(tensor_info_ptr->heavy_format).c_str(),
            tensor_info_ptr->sub_format);
    return FAILED;
  }

  /* For normal nodes we will always set the INFER_FORMAT attribute, because each node can only be visited once.
   * For sub-graph net-output, we will not set this attribute, because we allow this node being propagated
   * repeatedly. And sub-graph data will not go into this function. */
  if (!tensor_info_ptr->is_sub_graph_data_or_nt_opt || !tensor_info_ptr->is_forward) {
    (void)ge::AttrUtils::SetInt(current_tensor, INFER_FORMAT, tensor_info_ptr->heavy_format);
  }

  if (!IsHeavyFormat(tensor_info_ptr->heavy_format)) {
    FE_LOGD("Format %s is not heavy format, stop from %s[%u] of node[%s].",
            FormatToStr(tensor_info_ptr->heavy_format).c_str(), IS_INPUT_TO_STRING(tensor_info_ptr->is_input),
            tensor_info_ptr->anchor_index, tensor_info_ptr->op_desc_ptr->GetName().c_str());
    return GRAPH_OPTIMIZER_NOT_HEAVY_FORMAT;
  }
  FE_LOGD("format %s is heavy format, sub_format is %d, we set infer format for %s %u of node: %s",
          FormatToStr(tensor_info_ptr->heavy_format).c_str(), tensor_info_ptr->sub_format,
          IS_INPUT_TO_STRING(tensor_info_ptr->is_input), tensor_info_ptr->anchor_index,
          tensor_info_ptr->op_desc_ptr->GetName().c_str());
  Status ret = CheckScalarOrResult(current_tensor, tensor_info_ptr);
  if (ret != SUCCESS) {
    return ret;
  } else {
    return CheckOriginalShapeSizeValid(current_tensor, tensor_info_ptr);
  }
}

Status GetCurrentTensor(const ge::NodePtr &current_node, const TensorInfoPtr &tensor_info_ptr,
                        ge::ConstGeTensorDescPtr &current_tensor) {
  if (tensor_info_ptr->is_input) {
    if (static_cast<size_t>(tensor_info_ptr->anchor_index) >= current_node->GetOpDesc()->GetAllInputsDescPtr().size()) {
      FE_LOGW("AnchorIndex %u is larger than %zu of op desc %s | %s", tensor_info_ptr->anchor_index,
              current_node->GetOpDesc()->GetAllInputsDescPtr().size(), current_node->GetName().c_str(),
              current_node->GetType().c_str());
      return FAILED;
    }
    current_tensor = current_node->GetOpDesc()->GetInputDescPtr(tensor_info_ptr->anchor_index);
  } else {
    if (static_cast<size_t>(tensor_info_ptr->anchor_index) >=
        current_node->GetOpDesc()->GetAllOutputsDescPtr().size()) {
      FE_LOGW("AnchorIndex %u is larger than %zu of op desc %s | %s", tensor_info_ptr->anchor_index,
              current_node->GetOpDesc()->GetAllOutputsDescPtr().size(), current_node->GetName().c_str(),
              current_node->GetType().c_str());
      return FAILED;
    }
    current_tensor = current_node->GetOpDesc()->GetOutputDescPtr(tensor_info_ptr->anchor_index);
  }

  if (current_tensor == nullptr) {
    return FAILED;
  }
  return SUCCESS;
}
Status HeavyFormatPropagation::GetFormatAndDtypeFromOpKernel(const ge::NodePtr &current_node,
                                                             const TensorInfoPtr &tensor_info_ptr,
                                                             const OpKernelInfoPtr &op_kernel_info_ptr) const {
  if (op_kernel_info_ptr == nullptr) {
    FE_LOGW("opKernelInfoPtr of node %s is nullptr!", current_node->GetName().c_str());
    return FAILED;
  }

  InputOrOutputInfoPtr input_or_output_info;
  if (GetTensorKernelInfo(current_node, tensor_info_ptr, op_kernel_info_ptr, input_or_output_info) != SUCCESS) {
    FE_LOGW("Failed to get tensor kernel info for node %s.", current_node->GetName().c_str());
    return FAILED;
  }

  ge::ConstGeTensorDescPtr current_tensor;
  if (GetCurrentTensor(current_node, tensor_info_ptr, current_tensor) != SUCCESS) {
    return FAILED;
  }

  vector<ge::Format> format_vec;
  if (format_dtype_querier_ptr_->GetSupportFormats(op_kernel_info_ptr, input_or_output_info,
                                                   *(current_node->GetOpDesc().get()), format_vec) != SUCCESS) {
    FE_LOGW("Fail to get the support formats, node:[%s].", current_node->GetName().c_str());
    return FAILED;
  }
  if (static_cast<size_t>(tensor_info_ptr->format_index) >= format_vec.size()) {
    FE_LOGW("formatIndex %d is larger than %zu of format kernel %s | %s", tensor_info_ptr->anchor_index,
            current_node->GetOpDesc()->GetAllOutputsDescPtr().size(), current_node->GetName().c_str(),
            current_node->GetType().c_str());
    return FAILED;
  }

  tensor_info_ptr->heavy_format = format_vec.at(tensor_info_ptr->format_index);
  if (tensor_info_ptr->heavy_format == ge::FORMAT_ND) {
    tensor_info_ptr->heavy_format = static_cast<ge::Format>(ge::GetPrimaryFormat(current_tensor->GetFormat()));
  }
  vector<ge::DataType> data_type_vec;
  if (format_dtype_querier_ptr_->GetSupportDataTypes(op_kernel_info_ptr, input_or_output_info,
                                                     *(current_node->GetOpDesc().get()), data_type_vec) != SUCCESS) {
    FE_LOGW("Fail to get the support data_types, return FAILED.");
    return FAILED;
  }
  if (static_cast<size_t>(tensor_info_ptr->format_index) >= data_type_vec.size()) {
    FE_LOGW("FormatIndex %d is larger than data type size %zu %s | %s", tensor_info_ptr->format_index,
            data_type_vec.size(), current_node->GetName().c_str(),
            current_node->GetType().c_str());
    return FAILED;
  }
  tensor_info_ptr->data_type = data_type_vec.at(tensor_info_ptr->format_index);
  tensor_info_ptr->op_kernel_tensor_info = input_or_output_info;
  FE_LOGD("Heavy format of %s[%u] of node[%s] is %s. sub_format is %d, data_type is %s.",
          IS_INPUT_TO_STRING(tensor_info_ptr->is_input), tensor_info_ptr->anchor_index, current_node->GetName().c_str(),
          ge::TypeUtils::FormatToSerialString(tensor_info_ptr->heavy_format).c_str(), tensor_info_ptr->sub_format,
          ge::TypeUtils::DataTypeToSerialString(tensor_info_ptr->data_type).c_str());
  return SUCCESS;
}

Status GetCurrentTensor(const TensorInfoPtr &tensor_info_ptr, ge::GeTensorDescPtr &current_tensor) {
  if (tensor_info_ptr->is_input) {
    if (static_cast<size_t>(tensor_info_ptr->anchor_index) >=
        tensor_info_ptr->op_desc_ptr->GetAllInputsDescPtr().size()) {
      FE_LOGW("AnchorIndex %u is larger than %zu of op desc %s", tensor_info_ptr->anchor_index,
              tensor_info_ptr->op_desc_ptr->GetAllInputsDescPtr().size(),
              tensor_info_ptr->op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    current_tensor = tensor_info_ptr->op_desc_ptr->MutableInputDesc(tensor_info_ptr->anchor_index);
  } else {
    if (static_cast<size_t>(tensor_info_ptr->anchor_index) >=
        tensor_info_ptr->op_desc_ptr->GetAllOutputsDescPtr().size()) {
      FE_LOGW("AnchorIndex %u is larger than %zu of op desc %s", tensor_info_ptr->anchor_index,
              tensor_info_ptr->op_desc_ptr->GetAllOutputsDescPtr().size(),
              tensor_info_ptr->op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    current_tensor = tensor_info_ptr->op_desc_ptr->MutableOutputDesc(tensor_info_ptr->anchor_index);
  }
  FE_CHECK_NOTNULL(current_tensor);
  return SUCCESS;
}

Status HeavyFormatPropagation::CheckSpecificSubGraphDataOrNetoutput(
    const TensorInfoPtr &tensor_info_ptr, bool &is_sub_graph_data_or_net_output,
    std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections) {
  ge::RefCell key(tensor_info_ptr->node_ptr->GetName(), tensor_info_ptr->node_ptr,
                  tensor_info_ptr->is_input ? ge::NODE_IN : ge::NODE_OUT, tensor_info_ptr->anchor_index);
  is_sub_graph_data_or_net_output = FeGraphUtils::IsSubGraphDataOrNetOutput(tensor_info_ptr->op_desc_ptr);
  if (is_sub_graph_data_or_net_output) {
    FE_LOGD("Begin to get relations, node:%s, is input:%d, anchor_idx:%d", tensor_info_ptr->node_ptr->GetName().c_str(),
            tensor_info_ptr->is_input, tensor_info_ptr->anchor_index);

    auto status = reflection_builder_ptr_->LookUpRefRelations(key, reflections);
    if (status != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][ChkSpfSubgphData] Node[%s]: LookUpRefRelations failed, \
                      the %d input edge", tensor_info_ptr->node_ptr->GetName().c_str(), key.in_out_idx);
      return FAILED;
    }

    FE_LOGD("Get relations result, node:%s, relations size:%zu", tensor_info_ptr->node_ptr->GetName().c_str(),
            reflections.size());

    if (!FeGraphUtils::CheckRelatedEdgesOriginShape(reflections)) {
      return SUCCESS;
    }
  }
  return CONTINUE_TO_SET_FORMAT;
}

Status HeavyFormatPropagation::FindSameNameVariableNodes(const ge::NodePtr &node_ptr,
                                                         std::vector<ge::NodePtr> &var_nodes) {
  FE_CHECK_NOTNULL(node_ptr);
  if (node_ptr->GetType() != VARIABLE) {
    return SUCCESS;
  }
  ge::RefCell key(node_ptr->GetName(), node_ptr, ge::NODE_OUT, 0);
  std::unordered_set<ge::RefCell, ge::RefCellHash> reflections;
  ge::graphStatus status = reflection_builder_ptr_->LookUpRefRelations(key, reflections);
  if (status != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][FindSameNmVarNd] Fail to look up the relation of the same name \
                    variable node[%s].", node_ptr->GetName().c_str());
    return FAILED;
  }

  if (reflections.empty()) {
    return SUCCESS;
  }

  for (auto &cell : reflections) {
    if (node_ptr != cell.node) {
      var_nodes.push_back(cell.node);
    }
  }

  return SUCCESS;
}

Status HeavyFormatPropagation::SetNewShape(const TensorInfoPtr &tensor_info_ptr,
                                           const OpKernelInfoPtr &op_kernel_info_ptr,
                                           const ge::GeTensorDescPtr &current_tensor, ge::GeShape &original_shape,
                                           ge::GeShape &new_shape) {
  std::vector<int64_t> old_dims = original_shape.GetDims();
  auto op_desc = tensor_info_ptr->op_desc_ptr;
  auto op_name = op_desc->GetName();
  auto op_type = op_desc->GetType();
  std::string reshape_type =
      GetPropagationReshapeType(tensor_info_ptr, op_kernel_info_ptr, current_tensor);
  FE_LOGD("Op[name:%s] reshape type is %s.", op_name.c_str(), reshape_type.c_str());
  Status result = ExpandDimension(old_dims, op_type, current_tensor->GetOriginFormat(),
                                  tensor_info_ptr->heavy_format, tensor_info_ptr->anchor_index, reshape_type);
  /* Update the propagated reshape type. Because when reshape type pass through a node, it may change:
   * 1. If the node itself has a reshape type, we should use that reshape type instead of the propagated reshape
   * type from the last node. But when the node does not need dimension padding, we just clear the reshape type.
   *
   * 2. If the node itself does not have a reshape type and it is not a format agnostic node, we
   * should clear the reshape type.
   */
  if (result == NOT_CHANGED) {
    tensor_info_ptr->propagation_info.reshape_type = "";
  } else {
    tensor_info_ptr->propagation_info.reshape_type = reshape_type;
    (void)ge::AttrUtils::SetStr(current_tensor, INFER_RESHAPE_TYPE, reshape_type);
  }

  new_shape = ge::GeShape(old_dims);
  original_shape = ge::GeShape(old_dims);
  ge::Format original_format = current_tensor->GetOriginFormat();
  int64_t op_impl_type = static_cast<int64_t>(EN_IMPL_HW_TBE);
  int64_t group = GROUPS_DEFAULT_VALUE;
  ge::Format new_heavy_format = tensor_info_ptr->heavy_format;
  // if the heavy_format is fraz_g, try to set _fe_group attribute on tensor
  if (std::find(FE_GROUP_RELA_FORMAT_VECTOR.begin(), FE_GROUP_RELA_FORMAT_VECTOR.end(),
                tensor_info_ptr->heavy_format) != FE_GROUP_RELA_FORMAT_VECTOR.end()) {
    new_heavy_format =
        static_cast<ge::Format>(ge::GetFormatFromSub(tensor_info_ptr->heavy_format, tensor_info_ptr->sub_format));
    group = tensor_info_ptr->sub_format;
    if (group < GROUPS_DEFAULT_VALUE) {
      group = GROUPS_DEFAULT_VALUE;
    }
    FE_LOGD("Op[name=%s,type=%s]: new_heavy_format is %s, the group is %ld for the %s [%d].",
            op_name.c_str(), op_type.c_str(), FormatToStr(new_heavy_format).c_str(),
            group, IS_INPUT_TO_STRING(tensor_info_ptr->is_input),
            tensor_info_ptr->anchor_index);
  }

  ShapeAndFormat shape_and_format_info = {
      original_shape, new_shape, original_format, tensor_info_ptr->heavy_format, tensor_info_ptr->data_type,
      op_impl_type,   group};

  (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(shape_and_format_info);
  current_tensor->SetShape(new_shape);
  current_tensor->SetFormat(new_heavy_format);
  (void)ge::AttrUtils::SetStr(current_tensor, INFER_RESHAPE_TYPE, reshape_type);

  /* update shape range for unknown op */
  vector<std::pair<int64_t, int64_t>> new_range_shape;
  vector<std::pair<int64_t, int64_t>> ori_shape_range = GetShapeRange(*current_tensor);
  vector<std::pair<int64_t, int64_t>> old_shape_range = GetAlignShapeRange(ori_shape_range, original_shape);

  RangeAndFormat range_and_format_info = {original_shape,
                                          old_shape_range,
                                          new_range_shape,
                                          original_format,
                                          tensor_info_ptr->heavy_format,
                                          tensor_info_ptr->data_type,
                                          op_impl_type};

  if (SetShapeRange(*op_desc, range_and_format_info, *current_tensor) != SUCCESS) {
    FE_LOGI("Set shape range of op[name:%s,type:%s] failed.", op_name.c_str(),
            op_type.c_str());
    return FAILED;
  }

  string shape = StringUtils::IntegerVecToString<int64_t>(new_shape.GetDims());
  FE_LOGW("Set format %s datatype %u and shape %s to the %u %s of node %s.", FormatToStr(new_heavy_format).c_str(),
          tensor_info_ptr->data_type, shape.c_str(), tensor_info_ptr->anchor_index,
          IS_INPUT_TO_STRING(tensor_info_ptr->is_input), op_name.c_str());
  return SUCCESS;
}

Status HeavyFormatPropagation::SetFormatAndDTypeToOpdesc(const TensorInfoPtr &tensor_info_ptr,
                                                         const OpKernelInfoPtr &op_kernel_info_ptr,
                                                         Status set_format_result) {
  /* We will update the shape and format and datatype according
   * to the AnchorIndex */
  FE_LOGD("Begin to set format and dtype, node:%s, anchor idx:%d",
          tensor_info_ptr->node_ptr->GetName().c_str(), tensor_info_ptr->anchor_index);
  ge::GeTensorDescPtr current_tensor;
  if (GetCurrentTensor(tensor_info_ptr, current_tensor) != SUCCESS) {
    return FAILED;
  }
  std::unordered_set<ge::RefCell, ge::RefCellHash> reflections;
  bool is_sub_graph_data_or_net_output = false;
  Status ret = CheckSpecificSubGraphDataOrNetoutput(tensor_info_ptr, is_sub_graph_data_or_net_output, reflections);
  if (ret != CONTINUE_TO_SET_FORMAT) {
    return ret;
  }

  ge::Format old_format = static_cast<ge::Format>(ge::GetPrimaryFormat(current_tensor->GetFormat()));
  FE_LOGD("Old format of %s %u of node %s is %u, heavy_format=%s, sub_format=%d",
          IS_INPUT_TO_STRING(tensor_info_ptr->is_input), tensor_info_ptr->anchor_index,
          tensor_info_ptr->op_desc_ptr->GetName().c_str(), old_format,
          FormatToStr(tensor_info_ptr->heavy_format).c_str(), tensor_info_ptr->sub_format);
  if (old_format != tensor_info_ptr->heavy_format) {
    ge::GeShape original_shape = current_tensor->GetOriginShape();
    /* We won't change shape of scalar */
    if (set_format_result != GRAPH_OPTIMIZER_STOP_TRAVERSING_SCALAR_TENSOR && original_shape.GetDimNum() != 0) {
      ge::GeShape new_shape;
      Status ret = SetNewShape(tensor_info_ptr, op_kernel_info_ptr, current_tensor, original_shape, new_shape);
      if (ret != SUCCESS) {
        return ret;
      }

      // just update even relative op case, eg. while, optimize later if needed
      // set relative GeTensorDesc, including Function op
      if (is_sub_graph_data_or_net_output) {
        FE_LOGD("Begin to update format, node:%s, is input:%d, anchor_idx:%d",
                tensor_info_ptr->node_ptr->GetName().c_str(), tensor_info_ptr->is_input, tensor_info_ptr->anchor_index);

        RelationUpdateInfo relation_update_info = {tensor_info_ptr->heavy_format, tensor_info_ptr->sub_format,
                                                   new_shape, INFER_FORMAT, 1};

        (void)FeGraphUtils::UpdateFormatOfRelatedEdges(reflections, relation_update_info);
      }
    } else {
      FE_LOGW("Dim of to %s the %u of node %s is 0, stop propagating", IS_INPUT_TO_STRING(tensor_info_ptr->is_input),
              tensor_info_ptr->anchor_index, tensor_info_ptr->op_desc_ptr->GetName().c_str());
      return FAILED;
    }
  }
  FE_LOGD("Set nod %s's tensor format and shape success.", tensor_info_ptr->op_desc_ptr->GetName().c_str());
  return SUCCESS;
}

/* Only the reshape inserted by FE will be penetrable. */
bool IsFEInsertedReshape(const ge::OpDescPtr &op_desc_ptr) {
  bool is_inserted_by_ge = false;
  if (ge::AttrUtils::GetBool(op_desc_ptr, ge::ATTR_INSERTED_BY_GE, is_inserted_by_ge)) {
    /* If the dst trans node is inserted by ge, we won't insert any
     * other trans nodes before it. */
    if (is_inserted_by_ge) {
      return (op_desc_ptr->GetType() == RESHAPE || op_desc_ptr->GetType() == UNSQUEEZE_V2 ||
              op_desc_ptr->GetType() == SQUEEZE_V2) && is_inserted_by_ge;
    }
  }
  return false;
}

/* Our system do not support the TransData of data type int64.
 * If the cast is set as format agnostic, in some case, we must
 * use the int64 TransData which is not supported. */
void CorrectFmtAgnosticType(const ge::OpDescPtr &op_desc_ptr) {
  if (IsDtypeSensitiveOp(op_desc_ptr->GetType()) &&
      (op_desc_ptr->GetInputDescPtr(0)->GetDataType() == ge::DT_INT64 ||
       op_desc_ptr->GetOutputDescPtr(0)->GetDataType() == ge::DT_INT64)) {
    FE_LOGI("Cast %s contains int64 input or output, we do not allow it"
            "as format agnostic op.", op_desc_ptr->GetName().c_str());
    (void)ge::AttrUtils::SetInt(op_desc_ptr, FORMAT_AGNOSTIC, static_cast<int64_t>(
      FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO));
  }
}

void HeavyFormatPropagation::SetFormatAgnosticType(const ge::OpDescPtr &op_desc_ptr, const NodeInfoPtr &node_info) {
  bool weight_or_data = IsWeightOrData(op_desc_ptr);
  /* Weight, Data, TransData, FE-inserted Reshape and other virtual op
   * will be penetrable. */
  bool penetrable = weight_or_data || op_desc_ptr->GetType() == TRANSDATA || IsFEInsertedReshape(op_desc_ptr) ||
                    CheckVirtualOp(op_desc_ptr);

  CorrectFmtAgnosticType(op_desc_ptr);

  node_info->is_sub_graph_data_or_nt_opt = false;
  if (penetrable) {
    node_info->format_selection = FormatSelectionType::FORMAT_PENETRATION;
  } else {
    int64_t format_agnostic = 0;
    int64_t impl_type = 0;
    /* If current op is tvm op, use ops kernel info to select format. */
    if (ge::AttrUtils::GetInt(op_desc_ptr, ge::ATTR_NAME_IMPLY_TYPE, impl_type) &&
        impl_type == static_cast<int64_t>(domi::ImplyType::TVM)) {
      node_info->format_selection = FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO;
      return;
    }
    if (ge::AttrUtils::GetInt(op_desc_ptr, FORMAT_AGNOSTIC, format_agnostic)) {
      if (format_agnostic >= static_cast<int64_t>(FormatSelectionType::FORMAT_AGNOSTIC_BOTTOM) ||
          format_agnostic < 0) {
        node_info->format_selection = FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO;
      } else {
        FE_LOGD("Put this %s Op's format_agnostic(%ld) into vector",
                node_info->current_node->GetOpDesc()->GetName().c_str(), format_agnostic);
        format_agnostic_nodes_info_.push_back(node_info);
        node_info->format_selection = static_cast<FormatSelectionType>(format_agnostic);
      }
    } else {
      if (FeGraphUtils::IsSubGraphDataOrNetOutput(op_desc_ptr)) {
        node_info->is_sub_graph_data_or_nt_opt = true;
        node_info->format_selection = FormatSelectionType::FORMAT_AGNOSTIC_FUNCTION_OP;
        return;
      }
      node_info->format_selection = FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO;
    }
  }
}

Status HeavyFormatPropagation::SetOpKernelAndTensorMap(const NodeInfoPtr &node_info) {
  auto node_name = node_info->current_node->GetName();

  GetOpKernelInfo(node_info->current_node, node_info->current_node_op_kernel_ptr);
  if (node_info->current_node_op_kernel_ptr == nullptr) {
    FE_LOGW("Can not find op kernel for current node %s.", node_name.c_str());
    FE_LOGW("Heavy format is %s for %s[%u] of node[%s], sub format is %d, reshape type is %s.",
            ge::TypeUtils::FormatToSerialString(node_info->propagation_info.heavy_format).c_str(),
            IS_INPUT_TO_STRING(node_info->is_input_of_curr_node), node_info->anchor_index_of_curr_node,
            node_name.c_str(), node_info->propagation_info.sub_format,
            node_info->propagation_info.reshape_type.c_str());
    return FAILED;
  }

  IndexNameMap input_tensor_map, output_tensor_map;
  node_info->tensor_map = {input_tensor_map, output_tensor_map};
  if (GetInputOutputNameMap(*(node_info->current_node->GetOpDesc().get()), node_info->current_node_op_kernel_ptr,
                            node_info->tensor_map[INPUT_INDEX], node_info->tensor_map[OUTPUT_INDEX]) != SUCCESS) {
    return FAILED;
  }

  return SUCCESS;
}

void HeavyFormatPropagation::AddNodeInfoToQueue(NodeInfoPtr &node_info, std::deque<NodeInfoPtr> &next_node_queue) {
  if (node_info == nullptr) {
    return;
  }
  SetFormatAgnosticType(node_info->current_node->GetOpDesc(), node_info);
  if (node_info->format_selection == FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO) {
    if (SetOpKernelAndTensorMap(node_info) != SUCCESS) {
      FE_LOGW("Cannot propagate througth next node %s. It index is %d, heavy_format is %s, sub_format is %d.",
              node_info->current_node->GetName().c_str(), node_info->anchor_index_of_curr_node,
              FormatToStr(node_info->propagation_info.heavy_format).c_str(), node_info->propagation_info.sub_format);
    } else {
      bool is_heavy_op = IsHeavyOp(node_info->current_node_op_kernel_ptr);
      if (!is_heavy_op) {
        FE_LOGD("Next node %s: %d, heavy_format is %s, sub_format is %d.", node_info->current_node->GetName().c_str(),
                node_info->anchor_index_of_curr_node, FormatToStr(node_info->propagation_info.heavy_format).c_str(),
                node_info->propagation_info.sub_format);

        HeavyFormatInfo heavy_format_info(node_info->propagation_info.heavy_format,
                                          node_info->propagation_info.sub_format, node_info->anchor_index_of_curr_node,
                                          node_info->is_input_of_curr_node);
        if (supportformats_updater_ptr_->UpdateSupportFormats(node_info->current_node,
                                                              node_info->current_node_op_kernel_ptr,
                                                              node_info->tensor_map, heavy_format_info) != SUCCESS) {
          FE_LOGW("UpdateSupportFormats failed: next node %s. It index is %d, heavy_format is %s, sub_format is %d.",
                  node_info->current_node->GetName().c_str(), node_info->anchor_index_of_curr_node,
                  FormatToStr(node_info->propagation_info.heavy_format).c_str(),
                  node_info->propagation_info.sub_format);
        }
      }
    }
  }
  next_node_queue.emplace_back(node_info);
}

void HeavyFormatPropagation::CreateNextNodeInfo(const ge::NodePtr &next_node, const NodeInfoPtr &last_node_info,
                                                ge::Format heavy_format, int32_t sub_format,
                                                PropagationInfo &propagation_info, int32_t anchor_index, bool is_input,
                                                NodeInfoPtr &next_node_info,
                                                std::deque<NodeInfoPtr> &next_node_queue) {
  if (IsStaticZeroShapeOp(next_node->GetOpDesc())) {
    FE_LOGD("Skip %s because %s is an zero shape op.", next_node->GetName().c_str(), next_node->GetName().c_str());
    return;
  }

  FE_MAKE_SHARED(next_node_info = std::make_shared<NodeInfo>(next_node, last_node_info, heavy_format, sub_format,
                                                             propagation_info, anchor_index, is_input),
                 return);
  AddNodeInfoToQueue(next_node_info, next_node_queue);

  std::vector<ge::NodePtr> var_nodes;
  FindSameNameVariableNodes(next_node, var_nodes);
  for (ge::NodePtr &var_node : var_nodes) {
    NodeInfoPtr var_node_info = nullptr;
    FE_MAKE_SHARED(var_node_info = std::make_shared<NodeInfo>(var_node, last_node_info, heavy_format, sub_format,
                                                              propagation_info, anchor_index, is_input), return);
    if (var_node_info == nullptr) {
      continue;
    }
    AddNodeInfoToQueue(var_node_info, next_node_queue);
  }
}

bool IsNextNodeVisitedOrNull(const ge::NodePtr &last_node, const ge::NodePtr &next_node) {
  if (next_node == nullptr) {
    return true;
  }
  if (last_node != nullptr && next_node != nullptr &&
      (next_node->GetOwnerComputeGraph()->GetName() == last_node->GetOwnerComputeGraph()->GetName()) &&
      (next_node->GetOpDesc()->GetId() == last_node->GetOpDesc()->GetId())) {
    /* This means we are travering from this last_node, and we won't
     * traverse forwards to this last_node again. */
    FE_LOGD("This node %s has been visited, last node is %s", next_node->GetName().c_str(),
            last_node->GetName().c_str());
    return true;
  }
  return false;
}

Status HeavyFormatPropagation::SetFormats(const TensorInfoPtr &tensor_info_ptr,
                                          const OpKernelInfoPtr &op_kernel_info_ptr) {
  /* Set infer format to indicate that we have already traversed this
   * node */
  FE_LOGD("SetFormats, format_index:%d, heavy_format=%s, sub_format=%d.", tensor_info_ptr->format_index,
          FormatToStr(tensor_info_ptr->heavy_format).c_str(), tensor_info_ptr->sub_format);
  Status ret = SetInferFormat(tensor_info_ptr);
  if (ret != SUCCESS && ret != GRAPH_OPTIMIZER_NOT_HEAVY_FORMAT &&
      ret != GRAPH_OPTIMIZER_STOP_TRAVERSING_SCALAR_TENSOR) {
    return ret;
  }
  /* 1. format_index == -1 means it's the heavy op itself and we don't need to
   * set the format and shape for heavy op.
   * 2. If the next node is weight, we don't need to set the format and
   * shape format it. */
  if (tensor_info_ptr->format_index != -1 &&
      tensor_info_ptr->format_selection_type != FormatSelectionType::FORMAT_PENETRATION) {
    if (SetFormatAndDTypeToOpdesc(tensor_info_ptr, op_kernel_info_ptr, ret) != SUCCESS) {
      return FAILED;
    }
  }
  return ret;
}

Status HeavyFormatPropagation::PropagateForwards(const NodeInfoPtr &node_info, int32_t format_index,
                                                 FormatSelectionType format_selection_type,
                                                 std::deque<NodeInfoPtr> &next_node_queue) {
  auto current_node_name = node_info->current_node->GetName();
  FE_LOGD("Propagate forwards from %s by format index %d, reshape type %s, sub_format=%d.", current_node_name.c_str(),
          format_index, node_info->propagation_info.reshape_type.c_str(), node_info->propagation_info.sub_format);
  ge::OpDescPtr op_desc_ptr = node_info->current_node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);

  TensorInfoPtr tensor_info_ptr;
  /* Three bool: is_input, is_forward */
  FE_MAKE_SHARED(tensor_info_ptr = std::make_shared<FormatAndDataTypeInfo>(
                     node_info->current_node, op_desc_ptr, 0, format_index, false, true, format_selection_type,
                     node_info->propagation_info, node_info->tensor_map[OUTPUT_INDEX]),
                 return FAILED);

  // if current_node is sub graph NETOUTPUT, get next node by relative function node
  bool is_subgraph_netoutput = false;

  if (FeGraphUtils::IsSubGraphNetOutput(op_desc_ptr)) {
    is_subgraph_netoutput = true;
    tensor_info_ptr->is_input = true;
  }

  tensor_info_ptr->is_sub_graph_data_or_nt_opt = node_info->is_sub_graph_data_or_nt_opt;
  // sub graph NETOUTPUT, get Function successor op as NextNode
  if (is_subgraph_netoutput) {
    return PropagateSubNetoutputForwards(node_info, format_index, tensor_info_ptr, next_node_queue);
  } else {
    return PropagateNormalNodeForwards(node_info, format_index, tensor_info_ptr, next_node_queue);
  }
}

Status HeavyFormatPropagation::PropagateFarther(const NodeInfoPtr &curr_node_info, const NodeInfoPtr &next_node_info,
                                                std::deque<NodeInfoPtr> &next_node_queue) {
  if (curr_node_info == nullptr) {
    (void)PropagateForwards(next_node_info, -1,
                            FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO, next_node_queue);
    (void)PropagateBackwards(next_node_info, -1,
                             FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO, next_node_queue);
  } else {
    auto next_node = next_node_info->current_node;
    FE_LOGD("Propagate farther to node: %s by heavy format: %s, sub format: %d.", next_node->GetName().c_str(),
            FormatToStr(next_node_info->propagation_info.heavy_format).c_str(),
            next_node_info->propagation_info.sub_format);
    if (IsNextNodeVisitedOrNull(curr_node_info->last_node, next_node)) {
      return FAILED;
    }

    if (IsSpecialCast(next_node)) {
      FE_LOGI("Node %s is special cast, skip propagating", next_node->GetName().c_str());
      return FAILED;
    }

    (void)RunPropagation(next_node_info, next_node_queue);
  }
  return SUCCESS;
}

Status HeavyFormatPropagation::PropagateBackwards(const NodeInfoPtr &node_info, int32_t format_index,
                                                  FormatSelectionType format_selection_type,
                                                  std::deque<NodeInfoPtr> &next_node_queue) {
  FE_CHECK_NOTNULL(node_info->current_node);
  ge::OpDescPtr op_desc_ptr = node_info->current_node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  if (IsWeightOrData(op_desc_ptr)) {
    FE_LOGD("Weight %s Does not need to traverse back wards.", op_desc_ptr->GetName().c_str());
    return SUCCESS;
  }
  string last_node_name = (node_info->last_node == nullptr) ? "Null" : node_info->last_node->GetName();
  FE_LOGD("Propagate back wards from %s by format index %d", node_info->current_node->GetName().c_str(), format_index);
  TensorInfoPtr tensor_info_ptr;
  FE_MAKE_SHARED(tensor_info_ptr = std::make_shared<FormatAndDataTypeInfo>(
                     node_info->current_node, op_desc_ptr, 0, format_index, true, false, format_selection_type,
                     node_info->propagation_info, node_info->tensor_map[INPUT_INDEX]),
                 return FAILED);

  // if current_node is sub graph DATA, get next node by relative function node
  bool is_subgraph_data = false;
  if (FeGraphUtils::IsSubGraphData(op_desc_ptr)) {
    is_subgraph_data = true;
    // if sub graph DATA, should set is_input = false
    tensor_info_ptr->is_input = false;
  }
  tensor_info_ptr->is_sub_graph_data_or_nt_opt = node_info->is_sub_graph_data_or_nt_opt;
  // sub graph NETOUTPUT, get Function successor op as NextNode
  if (is_subgraph_data) {
    return PropagateSubDataBackwards(node_info, tensor_info_ptr, next_node_queue);
  } else {
    return PropagateNormalNodeBackwards(node_info, format_index, tensor_info_ptr, next_node_queue);
  }
}

Status HeavyFormatPropagation::RunPropagation(const NodeInfoPtr &node_info, std::deque<NodeInfoPtr> &next_node_queue) {
  FE_CHECK_NOTNULL(node_info->current_node);
  auto op_desc_ptr = node_info->current_node->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  auto curr_node_name = node_info->current_node->GetName();
  FE_LOGD("Run propagation from op %s to %s[%u] of op %s by heavy format %s, sub format %d, reshape type %s.",
          node_info->last_node->GetName().c_str(), IS_INPUT_TO_STRING(node_info->is_input_of_curr_node),
          node_info->anchor_index_of_curr_node, curr_node_name.c_str(),
          FormatToStr(node_info->propagation_info.heavy_format).c_str(), node_info->propagation_info.sub_format,
          node_info->propagation_info.reshape_type.c_str());

  if (IsOpTranspose(op_desc_ptr->GetType())) {
    /* We won't propagate trans-nodes' format */
    FE_LOGI("We won't propagate format for transpose node %s %s", op_desc_ptr->GetType().c_str(),
            curr_node_name.c_str());
    return FAILED;
  }

  FE_LOGD("Format selection type of %s is %u", op_desc_ptr->GetName().c_str(),
          static_cast<uint32_t>(node_info->format_selection));
  if (node_info->format_selection != FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO) {
    Status ret = PropagateForwards(node_info, INVALID_FORMAT_INDEX, node_info->format_selection, next_node_queue);
    if (ret == STOP_PROPAGATION_FROM_WEIGHT) {
      return SUCCESS;
    }

    PropagateBackwards(node_info, INVALID_FORMAT_INDEX, node_info->format_selection, next_node_queue);
  } else {
    FE_CHECK(node_info->current_node_op_kernel_ptr == nullptr,
             FE_LOGW("Can not find op kernel for current node %s.", curr_node_name.c_str()), return SUCCESS);

    /* Stop propagating at the next heavy op. */
    if (node_info->current_node_op_kernel_ptr->GetHeavyOpFlag()) {
      FE_LOGD("Stop propagating from heavy op[%s].", curr_node_name.c_str());
      return FAILED;
    }

    int32_t heavy_format_index = -1;
    HeavyFormatInfo heavy_format_info(node_info->propagation_info.heavy_format, node_info->propagation_info.sub_format,
                                      node_info->anchor_index_of_curr_node, node_info->is_input_of_curr_node);
    /* Get the most suitable heavy format index in current node's ops
     * kernel. */
    Status ret = GetHeavyFormatIndex(node_info, heavy_format_info, heavy_format_index);

    if (heavy_format_index == -1 || ret != SUCCESS) {
      return SUCCESS;
    } else {
      PropagateForwards(node_info, heavy_format_index, node_info->format_selection, next_node_queue);
      PropagateBackwards(node_info, heavy_format_index, node_info->format_selection, next_node_queue);
    }
  }
  return SUCCESS;
}

void HeavyFormatPropagation::UpdateOutputFormatAndShape(const ge::NodePtr &current_node,
    const vector<int64_t> &input_non_format_agnostic_index, const vector<int64_t> &output_non_format_agnostic_index,
    const ge::Format &format, const int32_t &sub_format, const ge::GeShape &shape) const {
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  // update this node's all output format
  for (auto &out_data_anchor : current_node->GetAllOutDataAnchors()) {
    int32_t out_anchor_index = out_data_anchor->GetIdx();
    if (std::find(output_non_format_agnostic_index.begin(), output_non_format_agnostic_index.end(), out_anchor_index) !=
        output_non_format_agnostic_index.end()) {
      FE_LOGD("When update output anchor, this anchor %u of %s is not format agnostic!", out_anchor_index,
              current_node->GetName().c_str());
      continue;
    }
    if (format != ge::FORMAT_RESERVED) {
      FE_LOGD("node %s output index %u format %s.", op_desc_ptr->GetName().c_str(), out_anchor_index,
              ge::TypeUtils::FormatToSerialString(format).c_str());
      auto output_desc_ptr = op_desc_ptr->MutableOutputDesc(out_anchor_index);
      if (output_desc_ptr == nullptr) {
        continue;
      }
      ge::Format new_format = static_cast<ge::Format>(ge::GetFormatFromSub(format, sub_format));
      output_desc_ptr->SetFormat(new_format);
      output_desc_ptr->SetShape(shape);
    }
  }
  // update this node's all input format
  for (auto &in_data_anchor : current_node->GetAllInDataAnchors()) {
    int32_t in_anchor_index = in_data_anchor->GetIdx();
    if (std::find(input_non_format_agnostic_index.begin(), input_non_format_agnostic_index.end(), in_anchor_index) !=
        input_non_format_agnostic_index.end()) {
      FE_LOGD("When update input anchor, this anchor %u of %s is not format agnostic!", in_anchor_index,
              current_node->GetName().c_str());
      continue;
    }

    auto input_format = op_desc_ptr->GetInputDesc(in_anchor_index).GetFormat();
    auto input_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_format));
    if (format != ge::FORMAT_RESERVED && input_primary_format != format) {
      FE_LOGD("node %s input index %u format %s.", op_desc_ptr->GetName().c_str(), in_anchor_index,
              ge::TypeUtils::FormatToSerialString(format).c_str());
      ge::GeShape new_shape;
      int64_t hidden_size = 1;
      int64_t input_size = 1;
      int64_t state_size = -1;
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "hidden_size", hidden_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "input_size", input_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "state_size", state_size);
      CalcShapeExtraAttr extra_attr = {hidden_size, input_size, state_size};
      ShapeAndFormat input_and_output_info = {op_desc_ptr->GetInputDesc(in_anchor_index).GetOriginShape(),
                                              new_shape,
                                              op_desc_ptr->GetInputDesc(in_anchor_index).GetOriginFormat(),
                                              format,
                                              op_desc_ptr->GetInputDesc(in_anchor_index).GetDataType(),
                                              static_cast<int64_t>(EN_IMPL_HW_TBE),
                                              sub_format,
                                              extra_attr};
      (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(input_and_output_info);
      ge::Format new_format = static_cast<ge::Format>(ge::GetFormatFromSub(format, sub_format));
      auto input_desc_ptr = op_desc_ptr->MutableInputDesc(in_anchor_index);
      if (input_desc_ptr == nullptr) {
        continue;
      }
      input_desc_ptr->SetFormat(new_format);
      input_desc_ptr->SetShape(new_shape);
    }
  }
}

void HeavyFormatPropagation::UpdateInputFormatAndShape(ge::NodePtr &current_node,
    const vector<int64_t> &input_non_format_agnostic_index, const vector<int64_t> &output_non_format_agnostic_index,
    const ge::Format &format, const int32_t &sub_format, const ge::GeShape &shape) const {
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  // update this node's all input format
  for (auto &in_data_anchor : current_node->GetAllInDataAnchors()) {
    int32_t in_anchor_index = in_data_anchor->GetIdx();
    if (std::find(input_non_format_agnostic_index.begin(), input_non_format_agnostic_index.end(), in_anchor_index) !=
        input_non_format_agnostic_index.end()) {
      FE_LOGD("When update input anchor, this anchor %u of %s is not format agnostic!", in_anchor_index,
              current_node->GetName().c_str());
      continue;
    }
    if (format != ge::FORMAT_RESERVED) {
      FE_LOGD("node %s: input index %u, format %s, sub_format %d.", op_desc_ptr->GetName().c_str(), in_anchor_index,
              ge::TypeUtils::FormatToSerialString(format).c_str(), sub_format);
      auto new_format = static_cast<ge::Format>(ge::GetFormatFromSub(format, sub_format));
      auto input_desc_ptr = op_desc_ptr->MutableInputDesc(in_anchor_index);
      if (input_desc_ptr == nullptr) {
        continue;
      }
      input_desc_ptr->SetFormat(new_format);
      input_desc_ptr->SetShape(shape);
    }
  }
  // update this node's all output format
  for (auto &out_data_anchor : current_node->GetAllOutDataAnchors()) {
    int32_t out_anchor_index = out_data_anchor->GetIdx();
    if (std::find(output_non_format_agnostic_index.begin(), output_non_format_agnostic_index.end(), out_anchor_index) !=
        output_non_format_agnostic_index.end()) {
      FE_LOGD("When update output anchor, this anchor %u of %s is not format agnostic!", out_anchor_index,
              current_node->GetName().c_str());
      continue;
    }

    auto output_format = op_desc_ptr->GetOutputDesc(out_anchor_index).GetFormat();
    auto output_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(output_format));
    if (format != ge::FORMAT_RESERVED && output_primary_format != format) {
      FE_LOGD("node %s output index %u format %s.", op_desc_ptr->GetName().c_str(), out_anchor_index,
              ge::TypeUtils::FormatToSerialString(format).c_str());
      ge::GeShape new_shape;
      int64_t hidden_size = 1;
      int64_t input_size = 1;
      int64_t state_size = -1;
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "hidden_size", hidden_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "input_size", input_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "state_size", state_size);
      CalcShapeExtraAttr extra_attr = {hidden_size, input_size, state_size};
      ShapeAndFormat input_and_output_info = {op_desc_ptr->GetOutputDesc(out_anchor_index).GetOriginShape(),
                                              new_shape,
                                              op_desc_ptr->GetOutputDesc(out_anchor_index).GetOriginFormat(),
                                              format,
                                              op_desc_ptr->GetOutputDesc(out_anchor_index).GetDataType(),
                                              static_cast<int64_t>(EN_IMPL_HW_TBE),
                                              sub_format,
                                              extra_attr};
      (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(input_and_output_info);
      auto output_desc_ptr = op_desc_ptr->MutableOutputDesc(out_anchor_index);
      if (output_desc_ptr == nullptr) {
        continue;
      }
      auto new_format = static_cast<ge::Format>(ge::GetFormatFromSub(format, sub_format));
      output_desc_ptr->SetFormat(new_format);
      output_desc_ptr->SetShape(new_shape);
    }
  }
}

bool HeavyFormatPropagation::CheckFormatCompatible(const ge::Format &primary_format,
                                                   const ge::Format &origin_format) const {
  if (primary_format == ge::FORMAT_NC1HWC0 && origin_format == ge::FORMAT_ND) {
    FE_LOGD("Primary format[%s] and origin format[%s] is not compatible.",
            FormatToStr(primary_format).c_str(), FormatToStr(origin_format).c_str());
    return false;
  }
  return true;
}

Status HeavyFormatPropagation::UpdateOutputForAllType(const ge::NodePtr &current_node,
                                                      const vector<int64_t> &input_non_format_agnostic_index,
                                                      const vector<int64_t> &output_non_format_agnostic_index) const {
  auto node_name = current_node->GetName();
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  // for this node 's all output data
  ge::Format peer_in_primary_format = ge::FORMAT_RESERVED;
  int32_t peer_in_sub_format = 0;
  ge::GeShape peer_in_shape;
  for (ge::OutDataAnchorPtr &out_data_anchor : current_node->GetAllOutDataAnchors()) {
    int64_t output_index = static_cast<int64_t>(out_data_anchor->GetIdx());
    if (std::find(output_non_format_agnostic_index.begin(), output_non_format_agnostic_index.end(), output_index)
        != output_non_format_agnostic_index.end()) {
      FE_LOGD("Output anchor %ld of %s is not format agnostic!", output_index, node_name.c_str());
      continue;
    }
    uint32_t out_anchor_index = static_cast<uint32_t>(output_index);
    ge::ConstGeTensorDescPtr output_desc_ptr = op_desc_ptr->GetOutputDescPtr(out_anchor_index);
    if (output_desc_ptr == nullptr) {
      continue;
    }
    int64_t tensor_format_continuous = 0;
    if (!ge::AttrUtils::GetInt(output_desc_ptr, FORMAT_CONTINUOUS, tensor_format_continuous) ||
        (tensor_format_continuous == 0)) {
      FE_LOGD("Output tensor %u of %s don't have _format_continuous or value equal 0!", out_anchor_index,
              node_name.c_str());
      continue;
    }
    for (auto &peer_in_data_anchor : out_data_anchor->GetPeerInDataAnchors()) {
      int32_t in_anchor_index = peer_in_data_anchor->GetIdx();
      auto input_desc = peer_in_data_anchor->GetOwnerNode()->GetOpDesc()->MutableInputDesc(in_anchor_index);
      if (input_desc == nullptr) {
        FE_LOGD("Input_desc is null.");
        continue;
      }
      ge::Format curr_peer_in_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_desc->GetFormat()));
      int32_t curr_peer_in_sub_format = static_cast<ge::Format>(ge::GetSubFormat(input_desc->GetFormat()));

      auto curr_peer_in_shape = input_desc->MutableShape();
      peer_in_shape = std::move(curr_peer_in_shape);
      if (peer_in_primary_format == ge::FORMAT_RESERVED) {
        peer_in_primary_format = curr_peer_in_primary_format;
        peer_in_sub_format = curr_peer_in_sub_format;
      } else if (peer_in_primary_format != curr_peer_in_primary_format) {
        FE_LOGW("Peer in anchor's format(%s != %s) of %s is not same!",
                ge::TypeUtils::FormatToSerialString(peer_in_primary_format).c_str(),
                ge::TypeUtils::FormatToSerialString(curr_peer_in_primary_format).c_str(), node_name.c_str());
        return NOT_CHANGED;
      }
    }
    if (!CheckFormatCompatible(peer_in_primary_format, output_desc_ptr->GetOriginFormat())) {
      FE_LOGD("Origin format[%s] of output[%u] node[%s, %s] is not compatible with peer in format[%s].",
              FormatToStr(output_desc_ptr->GetOriginFormat()).c_str(), out_anchor_index, op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str(), FormatToStr(peer_in_primary_format).c_str());
      return NOT_CHANGED;
    }
  }
  UpdateOutputFormatAndShape(current_node, input_non_format_agnostic_index, output_non_format_agnostic_index,
                             peer_in_primary_format, peer_in_sub_format, peer_in_shape);
  return SUCCESS;
}

Status HeavyFormatPropagation::UpdateInputForAllType(ge::NodePtr &current_node,
                                                     const vector<int64_t> &input_non_format_agnostic_index,
                                                     const vector<int64_t> &output_non_format_agnostic_index) const {
  auto node_name = current_node->GetName();
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  // for this node 's all input data
  ge::Format peer_out_primary_format = ge::FORMAT_RESERVED;
  int32_t peer_out_sub_format = 0;
  ge::GeShape peer_out_shape;
  for (ge::InDataAnchorPtr &in_data_anchor : current_node->GetAllInDataAnchors()) {
    int64_t input_index = static_cast<int64_t>(in_data_anchor->GetIdx());
    if (std::find(input_non_format_agnostic_index.begin(), input_non_format_agnostic_index.end(), input_index)
        != input_non_format_agnostic_index.end()) {
      FE_LOGD("Input anchor %ld of %s is not format agnostic!", input_index, node_name.c_str());
      continue;
    }
    uint32_t in_anchor_index = static_cast<uint32_t>(input_index);
    ge::ConstGeTensorDescPtr input_desc_ptr = op_desc_ptr->GetInputDescPtr(in_anchor_index);
    if (input_desc_ptr == nullptr) {
      continue;
    }
    int64_t tensor_format_continuous = 0;
    if (!ge::AttrUtils::GetInt(input_desc_ptr, FORMAT_CONTINUOUS, tensor_format_continuous) ||
        (tensor_format_continuous == 0)) {
      FE_LOGD("Input tensor %u of %s don't have _format_continuous or value equal 0!", in_anchor_index,
              node_name.c_str());
      continue;
    }

    if (in_data_anchor->GetPeerOutAnchor() == nullptr) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][UpdAllTypeIn] Input anchor %u of %s 's out anchor is nullptr!",
                      in_anchor_index, node_name.c_str());
      return FAILED;
    }

    auto peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
    /* If the peer out node is variable or constant, we do not need to use this attribute.
     * Becuase weight will always be the original format and if we use continuous attribute, switch
     * will never be inferred as heavy format(5HD or Fz). */
    auto owner_node = peer_out_anchor->GetOwnerNode();
    if (owner_node == nullptr) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][UpdAllTypeIn] Input anchor %u of %s 's owner_node is nullptr!",
                      in_anchor_index, node_name.c_str());
      return FAILED;
    }
    if (IsWeight(owner_node->GetOpDesc())) {
      FE_LOGD("node %s is connected to weight node, so we do not use its _format_continuous attr.",
              current_node->GetName().c_str());
      continue;
    }

    int peer_out_idx = peer_out_anchor->GetIdx();
    // get peer out anchor format & shape of other way op
    auto ouput_desc = owner_node->GetOpDesc()->MutableOutputDesc(peer_out_idx);
    if (ouput_desc == nullptr) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][UpdAllTypeIn] Ouput_desc %u of %s 's out anchor is nullptr!",
                      in_anchor_index, node_name.c_str());
      continue;
    }
    ge::Format curr_peer_out_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(ouput_desc->GetFormat()));
    ge::Format curr_peer_out_sub_format = static_cast<ge::Format>(ge::GetSubFormat(ouput_desc->GetFormat()));
    if (!CheckFormatCompatible(curr_peer_out_primary_format, input_desc_ptr->GetOriginFormat())) {
      FE_LOGD("Origin format[%s] of input[%u] node[%s, %s] is not compatible with peer out format[%s].",
              FormatToStr(input_desc_ptr->GetOriginFormat()).c_str(), in_anchor_index, op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str(), FormatToStr(curr_peer_out_primary_format).c_str());
      return NOT_CHANGED;
    }
    ge::GeShape curr_peer_out_shape = ouput_desc->MutableShape();
    peer_out_shape = std::move(curr_peer_out_shape);
    if (peer_out_primary_format == ge::FORMAT_RESERVED) {
      peer_out_primary_format = curr_peer_out_primary_format;
      peer_out_sub_format = curr_peer_out_sub_format;
    } else if (peer_out_primary_format != curr_peer_out_primary_format) {
      FE_LOGW("Peer out anchor's format(%s != %s) of %s is not same!",
              ge::TypeUtils::FormatToSerialString(peer_out_primary_format).c_str(),
              ge::TypeUtils::FormatToSerialString(curr_peer_out_primary_format).c_str(), node_name.c_str());
      return NOT_CHANGED;
    }
  }
  UpdateInputFormatAndShape(current_node, input_non_format_agnostic_index, output_non_format_agnostic_index,
                            peer_out_primary_format, peer_out_sub_format, peer_out_shape);
  return SUCCESS;
}

Status HeavyFormatPropagation::UpdateInputForPairType(const ge::NodePtr &current_node,
                                                      const vector<int64_t> &input_non_format_agnostic_index,
                                                      const vector<int64_t> &output_non_format_agnostic_index) const {
  // for this node 's all input data
  auto node_name = current_node->GetName();
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  for (ge::InDataAnchorPtr &in_data_anchor : current_node->GetAllInDataAnchors()) {
    int64_t input_index = static_cast<int64_t>(in_data_anchor->GetIdx());
    if (std::find(input_non_format_agnostic_index.begin(), input_non_format_agnostic_index.end(), input_index)
        != input_non_format_agnostic_index.end()) {
      FE_LOGD("Input anchor %ld of %s is not format agnostic!", input_index, node_name.c_str());
      continue;
    }
    uint32_t in_anchor_index = static_cast<uint32_t>(input_index);
    ge::GeTensorDescPtr input_desc_ptr = op_desc_ptr->MutableInputDesc(in_anchor_index);
    if (input_desc_ptr == nullptr) {
      continue;
    }
    int64_t tensor_format_continuous = 0;
    if (!ge::AttrUtils::GetInt(input_desc_ptr, FORMAT_CONTINUOUS, tensor_format_continuous) ||
        (tensor_format_continuous == 0)) {
      FE_LOGD("Input tensor %u of %s don't have _format_continuous or value equal 0!", in_anchor_index,
              node_name.c_str());
      continue;
    }
    if (in_data_anchor->GetPeerOutAnchor() == nullptr) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][UpdPairTypeIn] Input anchor %u of %s 's out anchor is nullptr!",
                      in_anchor_index, node_name.c_str());
      return FAILED;
    }

    int peer_out_idx = in_data_anchor->GetPeerOutAnchor()->GetIdx();
    // get peer out anchor format & shape of other way op
    auto peer_out_output_desc =
        in_data_anchor->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc()->GetOutputDescPtr(peer_out_idx);
    if (peer_out_output_desc == nullptr) {
      continue;
    }
    auto curr_peer_out_format = peer_out_output_desc->GetFormat();
    input_desc_ptr->SetFormat(curr_peer_out_format);
    ge::GeShape curr_peer_out_shape = peer_out_output_desc->GetShape();
    input_desc_ptr->SetShape(curr_peer_out_shape);

    ge::Format curr_peer_out_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(curr_peer_out_format));
    ge::Format curr_peer_out_sub_format = static_cast<ge::Format>(ge::GetSubFormat(curr_peer_out_format));
    if (!CheckFormatCompatible(curr_peer_out_primary_format, input_desc_ptr->GetOriginFormat())) {
      FE_LOGD("Origin format[%s] of input[%u] node[%s, %s] is not compatible with peer out format[%s].",
              FormatToStr(input_desc_ptr->GetOriginFormat()).c_str(), in_anchor_index, op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str(), FormatToStr(curr_peer_out_primary_format).c_str());
      continue;
    }

    auto output_desc = op_desc_ptr->GetOutputDesc(in_anchor_index);
    auto output_format = static_cast<ge::Format>(ge::GetPrimaryFormat(output_desc.GetFormat()));
    if (std::find(output_non_format_agnostic_index.begin(), output_non_format_agnostic_index.end(), in_anchor_index) ==
            output_non_format_agnostic_index.end() &&
        output_format != curr_peer_out_primary_format) {
      FE_LOGD("Output anchor %u of %s is format agnostic, so update this format %s.", in_anchor_index,
              node_name.c_str(), ge::TypeUtils::FormatToSerialString(curr_peer_out_format).c_str());
      ge::GeShape new_shape;
      int64_t hidden_size = 1;
      int64_t input_size = 1;
      int64_t state_size = -1;
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "hidden_size", hidden_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "input_size", input_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "state_size", state_size);
      CalcShapeExtraAttr extra_attr = {hidden_size, input_size, state_size};
      ShapeAndFormat input_and_output_info = {output_desc.GetOriginShape(),  new_shape,
                                              output_desc.GetOriginFormat(), curr_peer_out_primary_format,
                                              output_desc.GetDataType(),     static_cast<int64_t>(EN_IMPL_HW_TBE),
                                              curr_peer_out_sub_format,      extra_attr};
      (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(input_and_output_info);
      auto output_desc_ptr = op_desc_ptr->MutableOutputDesc(in_anchor_index);
      if (output_desc_ptr == nullptr) {
        continue;
      }
      output_desc_ptr->SetFormat(curr_peer_out_format);
      output_desc_ptr->SetShape(new_shape);
    }
  }
  return SUCCESS;
}

Status HeavyFormatPropagation::UpdateOutputForPairType(const ge::NodePtr &current_node,
                                                       const vector<int64_t> &input_non_format_agnostic_index,
                                                       const vector<int64_t> &output_non_format_agnostic_index) const {
  // for this node 's all input data
  auto node_name = current_node->GetName();
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  for (ge::OutDataAnchorPtr &out_data_anchor : current_node->GetAllOutDataAnchors()) {
    int64_t output_index = static_cast<int64_t>(out_data_anchor->GetIdx());
    if (std::find(output_non_format_agnostic_index.begin(), output_non_format_agnostic_index.end(), output_index)
        != output_non_format_agnostic_index.end()) {
      FE_LOGD("Output anchor %ld of %s is not format agnostic!", output_index, node_name.c_str());
      continue;
    }
    uint32_t out_anchor_index = static_cast<uint32_t>(output_index);
    ge::GeTensorDescPtr output_desc_ptr = op_desc_ptr->MutableOutputDesc(out_anchor_index);
    if (output_desc_ptr == nullptr) {
      continue;
    }
    int64_t tensor_format_continuous = 0;
    if (!ge::AttrUtils::GetInt(output_desc_ptr, FORMAT_CONTINUOUS, tensor_format_continuous) ||
        (tensor_format_continuous == 0)) {
      FE_LOGD("Output tensor %u of %s don't have _format_continuous or value equal 0!", out_anchor_index,
              node_name.c_str());
      continue;
    }
    ge::Format peer_in_primary_format = ge::FORMAT_RESERVED;
    int32_t peer_in_sub_format = 0;
    ge::GeShape peer_in_shape;
    for (auto &peer_in_data_anchor : out_data_anchor->GetPeerInDataAnchors()) {
      int32_t in_anchor_index = peer_in_data_anchor->GetIdx();
      auto peer_in_input_desc = peer_in_data_anchor->GetOwnerNode()->GetOpDesc()->GetInputDescPtr(in_anchor_index);
      if (peer_in_input_desc == nullptr) {
        FE_LOGD("Peer_in_input_desc is null.");
        continue;
      }
      ge::Format curr_peer_in_primary_format =
          static_cast<ge::Format>(ge::GetPrimaryFormat(peer_in_input_desc->GetFormat()));
      ge::Format curr_peer_in_sub_format = static_cast<ge::Format>(ge::GetSubFormat(peer_in_input_desc->GetFormat()));

      ge::GeShape curr_peer_in_shape = peer_in_input_desc->GetShape();
      peer_in_shape = curr_peer_in_shape;
      if (peer_in_primary_format == ge::FORMAT_RESERVED) {
        peer_in_primary_format = curr_peer_in_primary_format;
        peer_in_sub_format = curr_peer_in_sub_format;
      } else if (peer_in_primary_format != curr_peer_in_primary_format) {
        FE_LOGW("Peer in anchor's format(%s != %s) (one input-mul output) of %s is not same!",
                ge::TypeUtils::FormatToSerialString(peer_in_primary_format).c_str(),
                ge::TypeUtils::FormatToSerialString(curr_peer_in_primary_format).c_str(), node_name.c_str());
        return NOT_CHANGED;
      }
    }
    if (!CheckFormatCompatible(peer_in_primary_format, output_desc_ptr->GetOriginFormat())) {
      FE_LOGD("Origin format[%s] of output[%u] node[%s, %s] is not compatible with peer in format[%s].",
              FormatToStr(output_desc_ptr->GetOriginFormat()).c_str(), out_anchor_index, op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str(), FormatToStr(peer_in_primary_format).c_str());
      continue;
    }
    auto new_format = static_cast<ge::Format>(ge::GetFormatFromSub(peer_in_primary_format, peer_in_sub_format));
    output_desc_ptr->SetFormat(new_format);
    output_desc_ptr->SetShape(peer_in_shape);
    auto input_desc = op_desc_ptr->GetInputDesc(out_anchor_index);
    auto input_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_desc.GetFormat()));
    if (std::find(input_non_format_agnostic_index.begin(), input_non_format_agnostic_index.end(), out_anchor_index) ==
            input_non_format_agnostic_index.end() &&
        input_primary_format != peer_in_primary_format) {
      FE_LOGD("Input anchor %u of %s is format agnostic, so update this format %s.", out_anchor_index,
              node_name.c_str(), ge::TypeUtils::FormatToSerialString(new_format).c_str());
      ge::GeShape new_shape;
      int64_t hidden_size = 1;
      int64_t input_size = 1;
      int64_t state_size = -1;
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "hidden_size", hidden_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "input_size", input_size);
      (void)ge::AttrUtils::GetInt(op_desc_ptr, "state_size", state_size);
      CalcShapeExtraAttr extra_attr = {hidden_size, input_size, state_size};
      ShapeAndFormat input_and_output_info = {input_desc.GetOriginShape(),
                                              new_shape,
                                              input_desc.GetOriginFormat(),
                                              peer_in_primary_format,
                                              input_desc.GetDataType(),
                                              static_cast<int64_t>(EN_IMPL_HW_TBE),
                                              peer_in_sub_format,
                                              extra_attr};
      (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(input_and_output_info);
      auto input_desc_ptr = op_desc_ptr->MutableInputDesc(out_anchor_index);
      if (input_desc_ptr == nullptr) {
        continue;
      }
      input_desc_ptr->SetFormat(new_format);
      input_desc_ptr->SetShape(new_shape);
    }
  }  // end for
  return SUCCESS;
}

Status HeavyFormatPropagation::UpdateForOneNode(const NodeInfoPtr &node_info) const {
  auto current_node = node_info->current_node;
  auto node_name = current_node->GetName();
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  // get format_agnostic
  int64_t format_agnostic = 0;
  (void)ge::AttrUtils::GetInt(op_desc_ptr, FORMAT_AGNOSTIC, format_agnostic);
  // except input non-format agnostic op
  vector<int64_t> input_non_format_agnostic_index;
  (void)ge::AttrUtils::GetListInt(op_desc_ptr, INPUT_FORMAT_AGNOSTIC_EXCEPTION, input_non_format_agnostic_index);
  // except output non-format agnostic op
  vector<int64_t> output_non_format_agnostic_index;
  (void)ge::AttrUtils::GetListInt(op_desc_ptr, OUTPUT_FORMAT_AGNOSTIC_EXCEPTION, output_non_format_agnostic_index);
  if (format_agnostic == static_cast<int64_t>(FormatSelectionType::FORMAT_AGNOSTIC_FOR_ALL_INPUTS_AND_OUTPUTS)) {
    FE_LOGD("%s Op's format_agnostic is %ld", node_name.c_str(), format_agnostic);
    // for this node 's all input data
    Status res_input_all_type =
        UpdateInputForAllType(current_node, input_non_format_agnostic_index, output_non_format_agnostic_index);
    if (res_input_all_type == NOT_CHANGED) {
      return SUCCESS;
    } else if (res_input_all_type != SUCCESS) {
      return FAILED;
    }
    // for this node 's all output data
    Status res_output_all_type =
        UpdateOutputForAllType(current_node, input_non_format_agnostic_index, output_non_format_agnostic_index);
    if (res_output_all_type == NOT_CHANGED) {
      return SUCCESS;
    }
  } else if (format_agnostic == static_cast<int64_t>(
             FormatSelectionType::FORMAT_AGNOSTIC_FOR_PAIRED_INPUT_AND_OUTPUT)) {
    FE_LOGD("%s Op's format_agnostic is %ld", node_name.c_str(), format_agnostic);
    // for this node 's all input data
    if (UpdateInputForPairType(current_node, input_non_format_agnostic_index, output_non_format_agnostic_index) !=
        SUCCESS) {
      return FAILED;
    }
    // for this node 's all output data
    Status res_output_pair_yype =
        UpdateOutputForPairType(current_node, input_non_format_agnostic_index, output_non_format_agnostic_index);
    if (res_output_pair_yype == NOT_CHANGED) {
      return SUCCESS;
    }
  }  // end if
  return SUCCESS;
}

/* We need to take the following topological order into consideration.
 *
 *            Enter          NextIteration
 *               \             /
 *                \           /
 *                 \         /
 *                  \       /
 *                    Merge
 *                      |
 *                      |
 *                     op_a
 *     Topo order is Enter->Merge->NextIteration and they are all set with attr
 *     _format_continuous and _format_agnostic.
 *     So when the format of op_a and Merge is different. If Enter, Merge and
 *     NextIteration are both 5HD or Nz:
 *     1. Enter will be set before Merge, so it will keep 5HD or Nz.
 *     2. And then Merge will be set as op_a's format, we say ND. Merge will become
 *     ND and NextIteration will become ND as well.
 *     So Enter is different from Merge in format which is not allowed.
 *
 *     To solove this case, we need to do the traverse from both topological order and
 *     reversed topological order in which Enter will be set again.
 *
 *   */
Status HeavyFormatPropagation::UpdateInputAndOutputForFormatContinuous() {
  for (const auto &node_info : format_agnostic_nodes_info_) {
    if (UpdateForOneNode(node_info) != SUCCESS) {
      return FAILED;
    }
  }  // end for each node in normal order

  auto size_of_nodes = format_agnostic_nodes_info_.size();
  for (size_t i = size_of_nodes; i > 0; i--) {
    if (UpdateForOneNode(format_agnostic_nodes_info_[i - 1]) != SUCCESS) {
      return FAILED;
    }
  }  // end for each node in normal order
  return SUCCESS;
}

Status HeavyFormatPropagation::PropagateHeavyFormat(const ge::ComputeGraph &graph) {
  FE_TIMECOST_START(PropagateHeavyFormat);
  // use graph.GetAllNodes including sub graph nodes
  for (auto &current_node : graph.GetAllNodes()) {
    if (current_node == nullptr || current_node->GetOpDesc() == nullptr) {
      continue;
    }

    PropagationInfo propagation_info;
    NodeInfoPtr node_info;
    FE_MAKE_SHARED(node_info = std::make_shared<NodeInfo>(current_node, nullptr, ge::FORMAT_RESERVED, 0,
                                                          propagation_info, INVALID_LAST_NODE_ANCHOR_INDEX, true),
                   return FAILED);

    Status ret = SetOpKernelAndTensorMap(node_info);
    if (ret != SUCCESS) {
      continue;
    }

    if (IsHeavyOp(node_info->current_node_op_kernel_ptr)) {
      std::deque<NodeInfoPtr> next_node_queue;

      FE_LOGD("PropagateForwards from heavy node[%s, %s].", current_node->GetName().c_str(),
              current_node->GetType().c_str());

      next_node_queue.push_back(node_info);
      while (!next_node_queue.empty()) {
        NodeInfoPtr node_info_local = next_node_queue.back();
        next_node_queue.pop_back();

        PropagateFarther(node_info_local->last_node_info, node_info_local, next_node_queue);
      }
    }
  }
  if (UpdateInputAndOutputForFormatContinuous() != SUCCESS) {
    format_agnostic_nodes_info_.clear();
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate] Fail to update input and output for format continuous.");
    return FAILED;
  }
  format_agnostic_nodes_info_.clear();
  FE_TIMECOST_END(PropagateHeavyFormat, "PropagateHeavyFormat during FEGraphOptimizer::OptimizeOriginalGraph");
  return SUCCESS;
}

Status HeavyFormatPropagation::GetOpKernelInfo(const ge::NodePtr &current_node, OpKernelInfoPtr &op_kernel_info_ptr) {
  int32_t op_impl_type = -1;
  if (ge::AttrUtils::GetInt(*(current_node->GetOpDesc().get()), FE_IMPLY_TYPE, op_impl_type)) {
    string imply_type_str;
    if (!imply_type_string_vec_.empty() && static_cast<uint32_t>(op_impl_type) < imply_type_string_vec_.size() &&
        imply_type_string_vec_.at(static_cast<uint32_t>(op_impl_type)) != "") {
      /* we have found this imply type before */
      imply_type_str = imply_type_string_vec_.at(static_cast<uint32_t>(op_impl_type));
    } else {
      FEOpsStoreInfo op_store_info;
      FE_LOGD("Try to get Op str of imply type %d of node %s", op_impl_type, current_node->GetName().c_str());
      if (Configuration::Instance(engine_name_).GetOpStoreInfoByImplType(
          static_cast<OpImplType>(op_impl_type), op_store_info) != SUCCESS) {
        FE_LOGW("[GraphOptJdgInst][FmtPropagate][GetOpkrnInfo] Failed to get op store info by impl_type[%d].",
                op_impl_type);
        return FAILED;
      }
      imply_type_str = op_store_info.fe_ops_store_name;
      if (static_cast<uint32_t>(op_impl_type) < imply_type_string_vec_.size()) {
        imply_type_string_vec_[static_cast<uint32_t>(op_impl_type)] = imply_type_str;
      }
    }
    op_kernel_info_ptr =
        OpsKernelManager::Instance(engine_name_).GetOpKernelInfoByOpType(imply_type_str, current_node->GetType());
    return op_kernel_info_ptr == nullptr ? FAILED : SUCCESS;
  }
  return SUCCESS;
}

/* Input Version of GetHeavyFormatIndex */
Status HeavyFormatPropagation::GetHeavyFormatIndex(const NodeInfoPtr &node_info,
                                                   const HeavyFormatInfo &heavy_format_info,
                                                   int32_t &heavy_format_index) const {
  Status ret = heavy_format_selector_ptr_->GetMostSuitableFormatIndex(node_info->current_node_op_kernel_ptr,
                                                                      node_info->current_node, heavy_format_info,
                                                                      node_info->tensor_map, heavy_format_index);

  return ret;
}

Status HeavyFormatPropagation::GetNextNodesInfoForwards(std::deque<NodeInfoPtr> &next_node_queue,
                                                        const ge::InDataAnchorPtr &peer_in_data_anchor,
                                                        const ge::Format &heavy_format, const int32_t &sub_format,
                                                        const NodeInfoPtr &last_node_info,
                                                        PropagationInfo &propagation_info) {
  ge::NodePtr next_node = (peer_in_data_anchor == nullptr) ? nullptr : peer_in_data_anchor->GetOwnerNode();
  bool is_function_op = !next_node->GetOpDesc()->GetSubgraphInstanceNames().empty();

  if (is_function_op) {
    ge::RefCell key(next_node->GetName(), next_node, ge::NODE_IN, peer_in_data_anchor->GetIdx());

    std::unordered_set<ge::RefCell, ge::RefCellHash> reflections;
    if (reflection_builder_ptr_->LookUpRefRelations(key, reflections) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][GetNextNdInfoFwd] Node[%s]: LookUpRefRelations failed, \
                      the %d input edge", next_node->GetName().c_str(), key.in_out_idx);
      return FAILED;
    }
    std::vector<ge::OutDataAnchorPtr> vec_out_data_anchors;
    (void)FeGraphUtils::GetNextSubDatasOutAnchors(reflections, vec_out_data_anchors);

    FE_LOGD("Propagate forward to Function op, next_node:%s, inanchorIdx:%d, data out anchors:%zu",
            next_node->GetName().c_str(), peer_in_data_anchor->GetIdx(), vec_out_data_anchors.size());
    for (auto &peer_out_anchor : vec_out_data_anchors) {
      ge::NodePtr data_next_node = peer_out_anchor->GetOwnerNode();
      NodeInfoPtr next_node_info;
      CreateNextNodeInfo(data_next_node, last_node_info, heavy_format, sub_format, propagation_info,
                         peer_out_anchor->GetIdx(), false, next_node_info, next_node_queue);
    }
  } else {
    FE_LOGD("Add node %s into queue with reshape type %s", next_node->GetName().c_str(),
            last_node_info->propagation_info.reshape_type.c_str());
    NodeInfoPtr next_node_info;
    CreateNextNodeInfo(next_node, last_node_info, heavy_format, sub_format, propagation_info,
                       peer_in_data_anchor->GetIdx(), true, next_node_info, next_node_queue);
  }
  return SUCCESS;
}

bool HeavyFormatPropagation::IsAnchorFormatAgnostic(const TensorInfoPtr &tensor_info_ptr, bool is_input,
                                                    int64_t anchor_idex) const {
  if (tensor_info_ptr->format_selection_type != FormatSelectionType::FORMAT_AGNOSTIC_FOR_ALL_INPUTS_AND_OUTPUTS) {
    return true;
  }
  vector<int64_t> non_format_agnostic_index;
  if (is_input) {
    (void)ge::AttrUtils::GetListInt(tensor_info_ptr->op_desc_ptr, INPUT_FORMAT_AGNOSTIC_EXCEPTION,
                                    non_format_agnostic_index);
  } else {
    (void)ge::AttrUtils::GetListInt(tensor_info_ptr->op_desc_ptr, OUTPUT_FORMAT_AGNOSTIC_EXCEPTION,
                                    non_format_agnostic_index);
  }
  if (std::find(non_format_agnostic_index.begin(), non_format_agnostic_index.end(), anchor_idex) !=
      non_format_agnostic_index.end()) {
    return false;
  } else {
    return true;
  }
}

Status HeavyFormatPropagation::GetTensorKernelInfo(const ge::NodePtr &current_node,
                                                   const TensorInfoPtr &tensor_info_ptr,
                                                   const OpKernelInfoPtr &op_kernel_info_ptr,
                                                   InputOrOutputInfoPtr &input_or_output_info) const {
  if (op_kernel_info_ptr == nullptr) {
    FE_LOGD("The op kernel of node %s is null.", current_node->GetName().c_str());
    return FAILED;
  }
  std::map<uint32_t, std::string>::const_iterator iter = tensor_info_ptr->tensor_map.find(
      tensor_info_ptr->anchor_index);
  if (iter == tensor_info_ptr->tensor_map.end()) {
    FE_LOGW("anchor index %u is not in output index map which size is %zu", tensor_info_ptr->anchor_index,
            tensor_info_ptr->tensor_map.size());
    return FAILED;
  }

  if (tensor_info_ptr->is_input) {
    if (op_kernel_info_ptr->GetInputInfoByName(iter->second, input_or_output_info) != SUCCESS) {
      FE_LOGW("Tensor %u named %s isn't in in kernel %s,%s, which size is %zu", tensor_info_ptr->anchor_index,
              iter->second.c_str(), current_node->GetName().c_str(), current_node->GetType().c_str(),
              op_kernel_info_ptr->GetAllInputInfo().size());
      return FAILED;
    }
  } else {
    if (op_kernel_info_ptr->GetOutputInfoByName(iter->second, input_or_output_info) != SUCCESS) {
      FE_LOGW("Tensor %u name %s isn't in out kernel %s,%s, which size is %zu", tensor_info_ptr->anchor_index,
              iter->second.c_str(), current_node->GetName().c_str(), current_node->GetType().c_str(),
              op_kernel_info_ptr->GetAllOutputInfo().size());
      return FAILED;
    }
  }
  return SUCCESS;
}

bool HeavyFormatPropagation::CheckForwardPropagtionNecessary(const NodeInfoPtr &curr_node_info,
                                                             const ge::OpDescPtr &op_desc_ptr,
                                                             const ge::OutDataAnchorPtr &out_data_anchor,
                                                             const TensorInfoPtr &tensor_info_ptr) {
  if (!IsOutputAvailable(op_desc_ptr, out_data_anchor)) {
    return false;
  }
  int32_t out_anchor_index = out_data_anchor->GetIdx();

  if (!IsAnchorFormatAgnostic(tensor_info_ptr, false, out_anchor_index)) {
    FE_LOGD("Out anchor %u of %s is not format agnostic!", out_anchor_index, op_desc_ptr->GetName().c_str());
    return false;
  }

  if (tensor_info_ptr->only_to_paired_input_or_output &&
      out_anchor_index != curr_node_info->anchor_index_of_curr_node) {
    FE_LOGD("%s needs paried input and output! Out anchor index %u != %u", op_desc_ptr->GetName().c_str(),
            out_anchor_index, curr_node_info->anchor_index_of_curr_node);
    return false;
  }

  return true;
}

Status HeavyFormatPropagation::PropagateNormalNodeForwards(const NodeInfoPtr &curr_node_info, int32_t format_index,
                                                           const TensorInfoPtr &tensor_info_ptr,
                                                           std::deque<NodeInfoPtr> &next_node_queue) {
  auto current_node = curr_node_info->current_node;
  auto node_name = current_node->GetName();
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  std::deque<NodeInfoPtr> pending_next_nodes;
  for (auto &out_data_anchor : current_node->GetAllOutDataAnchors()) {
    if (!CheckForwardPropagtionNecessary(curr_node_info, op_desc_ptr, out_data_anchor, tensor_info_ptr)) {
      continue;
    }

    tensor_info_ptr->anchor_index = out_data_anchor->GetIdx();
    auto output_desc_ptr = op_desc_ptr->GetOutputDescPtr(tensor_info_ptr->anchor_index);
    if (output_desc_ptr == nullptr) {
      continue;
    }
    if (tensor_info_ptr->format_selection_type != FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO) {
      tensor_info_ptr->heavy_format = curr_node_info->propagation_info.heavy_format;
      tensor_info_ptr->sub_format = curr_node_info->propagation_info.sub_format;
      FE_LOGD("op[name:%s,type:%s]: use heavy_format: %s, sub_format: %d", node_name.c_str(),
              op_desc_ptr->GetType().c_str(), FormatToStr(tensor_info_ptr->heavy_format).c_str(),
              tensor_info_ptr->sub_format);
    } else if (format_index == -1) {
      /* At the beginning, we don't have the heavy format. */
      auto output_format = output_desc_ptr->GetFormat();
      tensor_info_ptr->heavy_format = static_cast<ge::Format>(ge::GetPrimaryFormat(output_format));
      tensor_info_ptr->sub_format = static_cast<ge::Format>(ge::GetSubFormat(output_format));
      (void)GetTensorKernelInfo(current_node, tensor_info_ptr, curr_node_info->current_node_op_kernel_ptr,
                                tensor_info_ptr->op_kernel_tensor_info);
    } else {
      tensor_info_ptr->sub_format = tensor_info_ptr->propagation_info.sub_format;
      if (GetFormatAndDtypeFromOpKernel(current_node, tensor_info_ptr, curr_node_info->current_node_op_kernel_ptr) !=
          SUCCESS) {
        continue;
      }
    }

    auto ori_format = output_desc_ptr->GetOriginFormat();
    if (SetReshapeType(op_desc_ptr, curr_node_info->current_node_op_kernel_ptr, ori_format, tensor_info_ptr) !=
            SUCCESS &&
        tensor_info_ptr->heavy_format != ge::FORMAT_FRACTAL_NZ) {
      FE_LOGD("Failed to get reshape type of op[name:%s,type:%s]", node_name.c_str(), op_desc_ptr->GetType().c_str());
      return SUCCESS;
    }

    Status ret = SetFormats(tensor_info_ptr, curr_node_info->current_node_op_kernel_ptr);
    if (ret != SUCCESS) {
      bool ret_invalid_flag = (ret == GRAPH_OPTIMIZER_STOP_TRAVERSING_SCALAR_WEIGHT ||
                               ret == GRAPH_OPTIMIZER_STOP_TRAVERSING_OTHER_ANCHORS);
      if (ret_invalid_flag) {
        FE_LOGD("We stop traversing from output %u of %s %s because of %u.", tensor_info_ptr->anchor_index,
                op_desc_ptr->GetType().c_str(), node_name.c_str(), ret);
        return SUCCESS;
      }
      continue;
    }

    std::vector<ge::OutDataAnchorPtr> vec_out_data_anchors;

    for (auto &peer_in_data_anchor : out_data_anchor->GetPeerInDataAnchors()) {
      /* Current node will become last node. */
      GetNextNodesInfoForwards(pending_next_nodes, peer_in_data_anchor, tensor_info_ptr->heavy_format,
                               tensor_info_ptr->sub_format, curr_node_info, tensor_info_ptr->propagation_info);
    }
  }
  if (JudgePropagationWorthy(current_node, pending_next_nodes, next_node_queue)) {
    return SUCCESS;
  } else {
    return STOP_PROPAGATION_FROM_WEIGHT;
  }
}

Status HeavyFormatPropagation::SetHeavyFormat(const ge::InDataAnchorPtr &in_data_anchor,
                                              const NodeInfoPtr &curr_node_info, const ge::OpDescPtr &op_desc_ptr,
                                              const int32_t &format_index, const TensorInfoPtr &tensor_info_ptr) const {
  int32_t in_anchor_index = in_data_anchor->GetIdx();
  tensor_info_ptr->anchor_index = in_anchor_index;
  auto input_desc_ptr = op_desc_ptr->GetInputDescPtr(in_anchor_index);
  if (input_desc_ptr == nullptr) {
    FE_LOGD("Input_desc_ptr is null.");
    return FAILED;
  }
  if (tensor_info_ptr->format_selection_type != FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO) {
    tensor_info_ptr->heavy_format = curr_node_info->propagation_info.heavy_format;
    tensor_info_ptr->sub_format = curr_node_info->propagation_info.sub_format;
    tensor_info_ptr->data_type = input_desc_ptr->GetDataType();
  } else if (format_index == -1) {
    /* At the beginning, we don't have the heavy format. */
    auto input_format = input_desc_ptr->GetFormat();
    tensor_info_ptr->heavy_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_format));
    tensor_info_ptr->sub_format = static_cast<ge::Format>(ge::GetSubFormat(input_format));
  } else {
    FE_LOGD("Invalid para, node:%s, format_index:%d", op_desc_ptr->GetName().c_str(), format_index);
    return FAILED;
  }
  return SUCCESS;
}

Status HeavyFormatPropagation::PropagateSubNetoutputForwards(const NodeInfoPtr &curr_node_info, int32_t format_index,
                                                             const TensorInfoPtr &tensor_info_ptr,
                                                             std::deque<NodeInfoPtr> &next_node_queue) {
  auto current_node = curr_node_info->current_node;
  auto node_name = current_node->GetName();

  FE_LOGD("Propagate forward from function netoutput:%s", node_name.c_str());

  // set netoutput in_data_anchor
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  for (auto &in_data_anchor : current_node->GetAllInDataAnchors()) {
    if (in_data_anchor != nullptr && in_data_anchor->GetIdx() == curr_node_info->anchor_index_of_curr_node) {
      FE_LOGD("sub netoutput peer inanchor, node:%s, anchor_idx:%d", node_name.c_str(), in_data_anchor->GetIdx());
      if (SetHeavyFormat(in_data_anchor, curr_node_info, op_desc_ptr, format_index, tensor_info_ptr) != SUCCESS) {
        continue;
      }

      auto ori_format = op_desc_ptr->GetInputDescPtr(tensor_info_ptr->anchor_index)->GetOriginFormat();
      if (SetReshapeType(op_desc_ptr, curr_node_info->current_node_op_kernel_ptr, ori_format, tensor_info_ptr) !=
              SUCCESS &&
          tensor_info_ptr->heavy_format != ge::FORMAT_FRACTAL_NZ) {
        FE_LOGD("Get reshape type of op[name:%s,type:%s] not successful", op_desc_ptr->GetName().c_str(),
                op_desc_ptr->GetType().c_str());
        return SUCCESS;
      }
      Status ret = SetFormats(tensor_info_ptr, curr_node_info->current_node_op_kernel_ptr);
      if (ret != SUCCESS) {
        bool ret_invalid_flag = (ret == GRAPH_OPTIMIZER_STOP_TRAVERSING_SCALAR_WEIGHT ||
                                 ret == GRAPH_OPTIMIZER_STOP_TRAVERSING_OTHER_ANCHORS);
        if (ret_invalid_flag) {
          FE_LOGD("We stop traversing from output %u of %s %s because of %u.", in_data_anchor->GetIdx(),
                  op_desc_ptr->GetType().c_str(), node_name.c_str(), ret);
          return SUCCESS;
        }
        continue;
      }
      // get function op's all successor in_data_anchor
      // ge::OutDataAnchor::Vistor<ge::InDataAnchorPtr> vec_in_data_anchors;
      std::vector<ge::InDataAnchorPtr> vec_in_data_anchors;
      if (FeGraphUtils::GetNextInAnchorsOfSubNetOutput(current_node, tensor_info_ptr->anchor_index,
                                                       vec_in_data_anchors) != SUCCESS) {
        FE_LOGW("Failed to get sub netoutput peer inanchor, node:%s, anchorIdx:%d", node_name.c_str(),
                tensor_info_ptr->anchor_index);
        return FAILED;
      }

      FE_LOGD("Get netoutput peer inanchor, node:%s, anchor_idx:%d, inanchor size:%zu", node_name.c_str(),
              tensor_info_ptr->anchor_index, vec_in_data_anchors.size());

      for (const auto &peer_in_data_anchor : vec_in_data_anchors) {
        /* Current node -> last node. */
        GetNextNodesInfoForwards(next_node_queue, peer_in_data_anchor, tensor_info_ptr->heavy_format,
                                 tensor_info_ptr->sub_format, curr_node_info, tensor_info_ptr->propagation_info);
      }
    }
  }
  return SUCCESS;
}

Status HeavyFormatPropagation::GetNextNodesInfoBackWards(std::deque<NodeInfoPtr> &next_node_queue,
                                                         const ge::OutDataAnchorPtr &peer_out_anchor,
                                                         const ge::Format &heavy_format, const int32_t &sub_format,
                                                         const NodeInfoPtr &last_node_info,
                                                         PropagationInfo &propagation_info) {
  FE_CHECK_NOTNULL(peer_out_anchor);
  auto next_node = peer_out_anchor->GetOwnerNode();
  FE_CHECK_NOTNULL(next_node);
  auto next_node_name = next_node->GetName();
  bool is_function_op = !next_node->GetOpDesc()->GetSubgraphInstanceNames().empty();
  if (is_function_op) {
    FE_LOGD("Propagate backward to Function op, next_node:%s", next_node_name.c_str());

    ge::RefCell key(next_node_name, next_node, ge::NODE_OUT, peer_out_anchor->GetIdx());

    std::unordered_set<ge::RefCell, ge::RefCellHash> reflections;

    if (reflection_builder_ptr_->LookUpRefRelations(key, reflections) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][GetNextNdInfoBkwd] Node[%s]: LookUpRefRelations failed, the %d \
                      input edge", next_node_name.c_str(), key.in_out_idx);
      return FAILED;
    }

    std::vector<ge::InDataAnchorPtr> vec_netoutput_in_anchor;
    if (FeGraphUtils::GetPreSubNetoutputInAnchor(reflections, vec_netoutput_in_anchor) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][GetNextNdInfoBkwd] node %s's in data anchor is empty",
                      next_node_name.c_str());
      return FAILED;
    }

    for (auto &peer_in_anchor : vec_netoutput_in_anchor) {
      ge::NodePtr data_next_node = peer_in_anchor->GetOwnerNode();
      NodeInfoPtr next_node_info;
      CreateNextNodeInfo(data_next_node, last_node_info, heavy_format, sub_format, propagation_info,
                         peer_in_anchor->GetIdx(), true, next_node_info, next_node_queue);
    }
  } else {
    NodeInfoPtr next_node_info;
    CreateNextNodeInfo(next_node, last_node_info, heavy_format, sub_format, propagation_info, peer_out_anchor->GetIdx(),
                       false, next_node_info, next_node_queue);
  }
  if (next_node_queue.empty()) {
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][GetNextNdInfoBkwd] next node info list is empty for node %s",
                    next_node_name.c_str());
    return FAILED;
  }
  return SUCCESS;
}

bool HeavyFormatPropagation::CheckBackwardPropagtionNecessary(const NodeInfoPtr &curr_node_info,
                                                              const ge::InDataAnchorPtr &in_data_anchor,
                                                              const TensorInfoPtr &tensor_info_ptr,
                                                              uint32_t &count_anchor) {
  auto current_node = curr_node_info->current_node;
  auto node_name = current_node->GetName();
  auto op_desc_ptr = current_node->GetOpDesc();
  FE_LOGD("inDataAnchor size is %zu, count is %u, node is %s", current_node->GetAllInDataAnchors().size(),
          count_anchor++, node_name.c_str());
  if (!IsInputAvailable(op_desc_ptr, in_data_anchor)) {
    return false;
  }
  int32_t in_anchor_index = in_data_anchor->GetIdx();

  if (!IsAnchorFormatAgnostic(tensor_info_ptr, true, in_anchor_index)) {
    FE_LOGD("Out anchor %u of %s is not format agnostic!", in_anchor_index, node_name.c_str());
    return false;
  }

  bool sub_not = FeGraphUtils::IsSubGraphNetOutput(op_desc_ptr);
  if ((tensor_info_ptr->only_to_paired_input_or_output || sub_not) &&
      in_anchor_index != curr_node_info->anchor_index_of_curr_node) {
    FE_LOGD("%s need paried input and output! In anchor index %u != %u", node_name.c_str(), in_anchor_index,
            curr_node_info->anchor_index_of_curr_node);
    return false;
  }
  return true;
}

Status HeavyFormatPropagation::PropagateNormalNodeBackwards(const NodeInfoPtr &curr_node_info, int32_t format_index,
                                                            TensorInfoPtr &tensor_info_ptr,
                                                            std::deque<NodeInfoPtr> &next_node_queue) {
  uint32_t count_anchor = 0;
  auto current_node = curr_node_info->current_node;
  ge::OpDescPtr op_desc_ptr = current_node->GetOpDesc();
  auto node_name = current_node->GetName();

  for (auto &in_data_anchor : current_node->GetAllInDataAnchors()) {
    if (!CheckBackwardPropagtionNecessary(curr_node_info, in_data_anchor, tensor_info_ptr, count_anchor)) {
      continue;
    }
    tensor_info_ptr->anchor_index = in_data_anchor->GetIdx();
    auto input_desc_ptr = op_desc_ptr->GetInputDescPtr(tensor_info_ptr->anchor_index);
    if (input_desc_ptr == nullptr) {
      continue;
    }
    if (tensor_info_ptr->format_selection_type != FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO) {
      tensor_info_ptr->heavy_format = curr_node_info->propagation_info.heavy_format;
      tensor_info_ptr->sub_format = curr_node_info->propagation_info.sub_format;
    } else if (format_index == -1) {
      /* At the beginning, we don't have the heavy format. */
      auto input_format = input_desc_ptr->GetFormat();
      tensor_info_ptr->heavy_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_format));
      tensor_info_ptr->sub_format = static_cast<ge::Format>(ge::GetSubFormat(input_format));
      (void)GetTensorKernelInfo(current_node, tensor_info_ptr, curr_node_info->current_node_op_kernel_ptr,
                                tensor_info_ptr->op_kernel_tensor_info);
    } else {
      tensor_info_ptr->sub_format = tensor_info_ptr->propagation_info.sub_format;
      if (GetFormatAndDtypeFromOpKernel(current_node, tensor_info_ptr, curr_node_info->current_node_op_kernel_ptr) !=
          SUCCESS) {
        continue;
      }
    }

    auto ori_format = input_desc_ptr->GetOriginFormat();
    if (SetReshapeType(op_desc_ptr, curr_node_info->current_node_op_kernel_ptr, ori_format, tensor_info_ptr) !=
            SUCCESS &&
        tensor_info_ptr->heavy_format != ge::FORMAT_FRACTAL_NZ) {
      FE_LOGD("Get reshape type of op[name:%s,type:%s] not successful", op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str());
      return SUCCESS;
    }
    Status ret = SetFormats(tensor_info_ptr, curr_node_info->current_node_op_kernel_ptr);
    if (ret != SUCCESS) {
      if (ret == GRAPH_OPTIMIZER_STOP_TRAVERSING_OTHER_ANCHORS) {
        FE_LOGD("We stop traversing from input %u of %s %s", tensor_info_ptr->anchor_index,
                op_desc_ptr->GetType().c_str(), node_name.c_str());
        return SUCCESS;
      }
      continue;
    }

    ge::OutDataAnchorPtr peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
    GetNextNodesInfoBackWards(next_node_queue, peer_out_anchor, tensor_info_ptr->heavy_format,
                              tensor_info_ptr->sub_format, curr_node_info, tensor_info_ptr->propagation_info);
  }
  return SUCCESS;
}

Status HeavyFormatPropagation::PropagateSubDataBackwards(const NodeInfoPtr &curr_node_info,
                                                         const TensorInfoPtr &tensor_info_ptr,
                                                         std::deque<NodeInfoPtr> &next_node_queue) {
  auto current_node = curr_node_info->current_node;
  auto node_name = current_node->GetName();
  FE_LOGD("Propagate backward from Function data:%s", node_name.c_str());

  /* 1. Check whether this sub data has been back-propagated */
  int64_t infer_format = -1;
  /* Sub-graph data has only one output desc. */
  auto current_tensor = current_node->GetOpDesc()->MutableOutputDesc(0);
  (void)ge::AttrUtils::GetInt(current_tensor, INFER_FORMAT, infer_format);

  if (infer_format != -1) {
    FE_LOGD("This sub data %s has been back-propagated by format %ld.", node_name.c_str(), infer_format);
    return SUCCESS;
  }
  tensor_info_ptr->heavy_format = curr_node_info->propagation_info.heavy_format;
  tensor_info_ptr->sub_format = curr_node_info->propagation_info.sub_format;
  if (!IsHeavyFormatConsistentWithOriFormat(current_tensor, tensor_info_ptr)) {
    FE_LOGD("Original format %u is not consistent with heavy format %u.", current_tensor->GetOriginFormat(),
            tensor_info_ptr->heavy_format);
    return FAILED;
  }

  /* 2. Get the node in front of the function op. */
  ge::OutDataAnchorPtr peer_out_anchor;
  Status ret = FeGraphUtils::GetPreOutAnchorOfSubData(current_node, peer_out_anchor);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][PropagateSubDataBkwd] Failed to get pre out anchor for node %s",
                    current_node->GetName().c_str());
    return ret;
  }

  /* 3. Set attr INFER_FORMAT */
  (void)ge::AttrUtils::SetInt(current_tensor, INFER_FORMAT, tensor_info_ptr->heavy_format);

  /* 4. Propate backwards this sub data. */
  GetNextNodesInfoBackWards(next_node_queue, peer_out_anchor, tensor_info_ptr->heavy_format,
                            tensor_info_ptr->sub_format, curr_node_info, curr_node_info->propagation_info);

  return SUCCESS;
}

std::string HeavyFormatPropagation::GetPropagationReshapeType(const TensorInfoPtr &tensor_info_ptr,
                                                              const OpKernelInfoPtr &op_kernel_info_ptr,
                                                              const ge::GeTensorDescPtr &current_tensor) const {
  std::string reshape_type;
  if (tensor_info_ptr->op_kernel_tensor_info == nullptr || op_kernel_info_ptr == nullptr) {
    return reshape_type;
  }
  if (!tensor_info_ptr->op_kernel_tensor_info->GetReshapeType().empty()) {
    reshape_type = tensor_info_ptr->op_kernel_tensor_info->GetReshapeType();
  } else if (tensor_info_ptr->op_desc_ptr->GetAllInputsSize() == 1 &&
             tensor_info_ptr->op_desc_ptr->GetAllOutputsDescSize() == 1 &&
             op_kernel_info_ptr->GetOpPattern() == OP_PATTERN_FORMAT_AGNOSTIC) {
    reshape_type = tensor_info_ptr->propagation_info.reshape_type;
    (void)ge::AttrUtils::SetStr(current_tensor, INFER_RESHAPE_TYPE, reshape_type);
  }

  if (op_kernel_info_ptr->GetOpPattern() == OP_PATTERN_REDUCE) {
    vector<int64_t> axis_values;
    (void)ge::AttrUtils::GetListInt(tensor_info_ptr->op_desc_ptr, AXES_ATTR_NAME, axis_values);
    auto ori_format = current_tensor->GetOriginFormat();
    reshape_type = AxisNameUtil::GetReshapeType(ori_format, axis_values);
    FE_LOGD("Op[name:%s,type:%s] reshape type is %s, original format is %d.",
            tensor_info_ptr->op_desc_ptr->GetName().c_str(), tensor_info_ptr->op_desc_ptr->GetType().c_str(),
            reshape_type.c_str(), ori_format);
    (void)ge::AttrUtils::SetStr(current_tensor, INFER_RESHAPE_TYPE, reshape_type);
  }
  return reshape_type;
}

Status HeavyFormatPropagation::SetReshapeType(const ge::OpDescPtr &op_desc_ptr,
                                              const OpKernelInfoPtr &op_kernel_info_ptr,
                                              const ge::Format &ori_format,
                                              const TensorInfoPtr &tensor_info_ptr) const {
  if (op_kernel_info_ptr == nullptr || tensor_info_ptr->op_kernel_tensor_info == nullptr) {
    FE_LOGD("The op_kernel_info_ptr of op[name:%s,type:%s] is null!", op_desc_ptr->GetName().c_str(),
            op_desc_ptr->GetType().c_str());
    return SUCCESS;
  }

  if (op_kernel_info_ptr->GetOpPattern() == OP_PATTERN_REDUCE) {
    vector<int64_t> axis_values;
    (void)ge::AttrUtils::GetListInt(op_desc_ptr, AXES_ATTR_NAME, axis_values);
    std::string reshape_type = AxisNameUtil::GetReshapeType(ori_format, axis_values);
    tensor_info_ptr->propagation_info.reshape_type = reshape_type;
    FE_LOGD("Op[name:%s,type:%s] new reshape type is %s, original format is %d.", op_desc_ptr->GetName().c_str(),
            op_desc_ptr->GetType().c_str(), reshape_type.c_str(), ori_format);
  } else {
    bool kernel_reshape_empty = tensor_info_ptr->op_kernel_tensor_info->GetReshapeType().empty();
    bool prop_reshape_empty = tensor_info_ptr->propagation_info.reshape_type.empty();

    size_t dimension;
    if (tensor_info_ptr->is_input) {
      dimension = op_desc_ptr->MutableInputDesc(tensor_info_ptr->anchor_index)->MutableShape().GetDimNum();
    } else {
      dimension = op_desc_ptr->MutableOutputDesc(tensor_info_ptr->anchor_index)->MutableShape().GetDimNum();
    }
    bool needReshape = (dimension != NCHW_DIMENSION_NUM);
    if (!needReshape) {
      /* if the dimension is 4, that means we will not use reshape type, we just clean the
       * propagation reshape type. */
      tensor_info_ptr->propagation_info.reshape_type = "";
      FE_LOGD("Reshape is required for Op[name:%s,type:%s].", tensor_info_ptr->op_desc_ptr->GetName().c_str(),
              tensor_info_ptr->op_desc_ptr->GetType().c_str());
      return SUCCESS;
    }

    if (!kernel_reshape_empty && !prop_reshape_empty &&
        tensor_info_ptr->op_kernel_tensor_info->GetReshapeType() != tensor_info_ptr->propagation_info.reshape_type) {
      FE_LOGD("Op[name:%s,type:%s] reshape type [%s] is not same with propagate reshape type [%s].",
              tensor_info_ptr->op_desc_ptr->GetName().c_str(), tensor_info_ptr->op_desc_ptr->GetType().c_str(),
              tensor_info_ptr->op_kernel_tensor_info->GetReshapeType().c_str(),
              tensor_info_ptr->propagation_info.reshape_type.c_str());
      return FAILED;
    }

    if (!kernel_reshape_empty && prop_reshape_empty) {
      tensor_info_ptr->propagation_info.reshape_type = tensor_info_ptr->op_kernel_tensor_info->GetReshapeType();
      FE_LOGD("Use %s as the propagation reshape type for node %s.",
              tensor_info_ptr->propagation_info.reshape_type.c_str(), op_desc_ptr->GetName().c_str());
      return SUCCESS;
    }

    if (!prop_reshape_empty) {
      FE_LOGD("Op[name:%s,type:%s] has reshape type : %s. op pattern is %d. ", op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str(), tensor_info_ptr->propagation_info.reshape_type.c_str(),
              op_kernel_info_ptr->GetOpPattern());
      return SUCCESS;
    }
  }
  FE_LOGD("Use default reshape type for node %s.", op_desc_ptr->GetName().c_str());
  return SUCCESS;
}

bool HeavyFormatPropagation::IsInputAvailable(const ge::OpDescPtr &op_desc_ptr,
                                              const ge::InDataAnchorPtr &in_data_anchor_ptr) const {
  if (in_data_anchor_ptr == nullptr || in_data_anchor_ptr->GetPeerOutAnchor() == nullptr) {
    return false;
  }

  int32_t in_anchor_index = in_data_anchor_ptr->GetIdx();
  if (op_desc_ptr->MutableInputDesc(in_anchor_index) == nullptr) {
    return false;
  }
  return true;
}

bool HeavyFormatPropagation::IsOutputAvailable(const ge::OpDescPtr &op_desc_ptr,
                                               const ge::OutDataAnchorPtr &out_data_anchor_ptr) const {
  if (out_data_anchor_ptr == nullptr) {
    return false;
  }

  int32_t out_anchor_index = out_data_anchor_ptr->GetIdx();
  if (op_desc_ptr->MutableOutputDesc(out_anchor_index) == nullptr) {
    return false;
  }
  return true;
}

bool HeavyFormatPropagation::IsFormatAgnosticAnchor(const ge::NodePtr &current_node,
                                                    const NodeInfoPtr &next_node) const {
  auto next_op_desc = next_node->current_node->GetOpDesc();
  auto heavy_format = next_node->propagation_info.heavy_format;
  /* Check whether the node is format agnostic. If it is, we can keep doing the propagation. */
  if (next_node->format_selection != FormatSelectionType::FORMAT_DEPENDS_ON_OP_KERNEL_INFO) {
    if (next_node->format_selection == FormatSelectionType::FORMAT_AGNOSTIC_FOR_ALL_INPUTS_AND_OUTPUTS) {
      std::vector<int64_t> invalid_anchors;
      auto index = next_node->anchor_index_of_curr_node;
      if (ge::AttrUtils::GetListInt(next_op_desc, INPUT_FORMAT_AGNOSTIC_EXCEPTION, invalid_anchors) &&
          std::find(invalid_anchors.begin(), invalid_anchors.end(), index) != invalid_anchors.end()) {
        FE_LOGD("Stop propagation by heavy format %u from weight %s because its user %s's input %u is in exception.",
                heavy_format, current_node->GetName().c_str(), next_op_desc->GetName().c_str(), index);
        return false;
      }
    }
    return true;
  } else {
    FE_LOGD("Stop propagation by heavy format %u from weight %s because its user %s cannot support this format.",
            heavy_format, current_node->GetName().c_str(), next_op_desc->GetName().c_str());
    return false;
  }
}

bool HeavyFormatPropagation::IsHeavyFormatSupported(const ge::NodePtr &current_node,
                                                    const NodeInfoPtr &next_node_info) const {
  auto next_op_desc = next_node_info->current_node->GetOpDesc();
  if (next_node_info->tensor_map.empty()) {
    FE_LOGW("Tensor map of node %s is empty.", next_op_desc->GetName().c_str());
    return false;
  }

  InputOrOutputInfoPtr input_info;
  auto index = next_node_info->anchor_index_of_curr_node;
  std::map<uint32_t, std::string>::const_iterator iter = next_node_info->tensor_map[INPUT_INDEX].find(index);
  if (iter == next_node_info->tensor_map[INPUT_INDEX].end()) {
    FE_LOGW("Can not find input %u of node %s in tensor map!", index, next_op_desc->GetName().c_str());
    return false;
  }

  Status ret = next_node_info->current_node_op_kernel_ptr->GetInputInfoByName(iter->second, input_info);
  if (ret != SUCCESS) {
    FE_LOGW("Can not get input %u's info of node %s in ops kernel store.", index, next_op_desc->GetName().c_str());
    return false;
  }

  vector<ge::Format> input_formats;
  if (format_dtype_querier_ptr_->GetSupportFormats(next_node_info->current_node_op_kernel_ptr, input_info,
                                                   *(next_op_desc.get()), input_formats) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][IsHeavyFmtSupted] Failed to get the support formats for node %s.",
                    next_op_desc->GetName().c_str());
    return false;
  }
  auto heavy_format = next_node_info->propagation_info.heavy_format;
  if (std::find(input_formats.begin(), input_formats.end(), heavy_format) == input_formats.end()) {
    FE_LOGD(
        "Stop propagation by heavy format %s, sub_format %d from weight %s because its user %s cannot support this "
        "format.",
        FormatToStr(heavy_format).c_str(), next_node_info->propagation_info.sub_format, current_node->GetName().c_str(),
        next_op_desc->GetName().c_str());
    return false;
  }
  return true;
}

bool HeavyFormatPropagation::IsNextNodeQualified(const ge::NodePtr &current_node, const NodeInfoPtr &next_node) {
  if (next_node->current_node_op_kernel_ptr == nullptr) {
    FE_LOGD("Check format agnostic attr for node %s", next_node->current_node->GetName().c_str());
    if (IsFormatAgnosticAnchor(current_node, next_node)) {
      return true;
    }
  } else {
    FE_LOGD("Check format in op kernel for node %s", next_node->current_node->GetName().c_str());
    if (IsHeavyFormatSupported(current_node, next_node)) {
      return true;
    }
  }
  return false;
}

/* If one of the peer out node of switch cannot support the heavy format,
 * we stop propagation. */
bool HeavyFormatPropagation::CheckSwitchQualified(const std::vector<NodeInfoPtr> &node_list) {
  for (auto &node_info : node_list) {
    auto node = node_info->current_node;
    for (auto &out_anchor : node->GetAllOutDataAnchors()) {
      if (out_anchor->GetPeerInDataAnchors().empty()) {
        continue;
      }
      for (auto &peer_in_anchor : out_anchor->GetPeerInDataAnchors()) {
        if (peer_in_anchor == nullptr) {
          continue;
        }
        auto successor = peer_in_anchor->GetOwnerNode();
        NodeInfoPtr successor_info;
        auto heavy_format = node_info->propagation_info.heavy_format;
        auto sub_format = node_info->propagation_info.sub_format;
        std::deque<NodeInfoPtr> successor_queue;
        CreateNextNodeInfo(successor, node_info, heavy_format, sub_format, node_info->propagation_info,
                           peer_in_anchor->GetIdx(), true, successor_info, successor_queue);
        /* Check whether there is a unqualified successor. */
        if (IsNextNodeQualified(node, successor_info)) {
          continue;
        }
        FE_LOGD("The only successor(%s) of format agnostic node %s cannot support heavy format %u.",
                successor->GetName().c_str(), node->GetName().c_str(), heavy_format);
        return false;
      }
    }
  }
  return true;
}

void HeavyFormatPropagation::RollBackFormatAndShape(const ge::NodePtr &node, bool is_sub_graph_weight) const {
  if (!is_sub_graph_weight) {
    return;
  }

  auto output_desc = node->GetOpDesc()->MutableOutputDesc(0);
  if (output_desc == nullptr) {
    return;
  }

  auto sub_format = ge::GetSubFormat(output_desc->GetFormat());
  auto ori_format = output_desc->GetOriginFormat();
  auto ori_shape = output_desc->GetOriginShape();
  output_desc->SetFormat(ori_format);
  output_desc->SetShape(ori_shape);

  auto input_desc = node->GetOpDesc()->MutableInputDesc(0);
  if (input_desc != nullptr) {
    input_desc->SetFormat(static_cast<ge::Format>(ge::GetFormatFromSub(ori_format, sub_format)));
    input_desc->SetShape(ori_shape);
  }

  std::unordered_set<ge::RefCell, ge::RefCellHash> reflections;
  ge::RefCell key(node->GetName(), node, ge::NODE_IN, 0);

  (void)reflection_builder_ptr_->LookUpRefRelations(key, reflections);

  RelationUpdateInfo relation_update_info = {ori_format, sub_format, ori_shape, INFER_FORMAT, 1};

  (void)FeGraphUtils::UpdateFormatOfRelatedEdges(reflections, relation_update_info);
}

bool HeavyFormatPropagation::JudgePropagationWorthy(const ge::NodePtr &node,
                                                    const std::deque<NodeInfoPtr> &pending_next_nodes,
                                                    std::deque<NodeInfoPtr> &next_nodes) {
  vector<NodeInfoPtr> switch_list;
  /* In current stage, we only handle variable/const. And Other nodes we
   * will always do the propagation. */
  bool is_sub_graph_weight = IsConstNodeInSubGraph(node);
  if (IsWeight(node->GetOpDesc()) || is_sub_graph_weight) {
    FE_LOGD("Judge whether propagation is worthy for weight node %s.", node->GetName().c_str());
    for (auto &next_node : pending_next_nodes) {
      FE_CHECK(next_node == nullptr, FE_LOGW("next_node is null."), return false);
      auto op_desc = next_node->current_node->GetOpDesc();
      FE_CHECK(op_desc == nullptr, FE_LOGW("op_desc is null."), return false);
      if (op_desc->GetType() == SWITCH) {
        switch_list.emplace_back(next_node);
      }
      if (IsNextNodeQualified(node, next_node)) {
        continue;
      }

      RollBackFormatAndShape(node, is_sub_graph_weight);
      return false;
    }
    FE_LOGD("Propagation through this weight is worthy.");
  }

  if (!CheckSwitchQualified(switch_list)) {
    RollBackFormatAndShape(node, is_sub_graph_weight);
    return false;
  }

  for (auto &next_node : pending_next_nodes) {
    next_nodes.emplace_back(next_node);
  }
  return true;
}
}  // namespace fe