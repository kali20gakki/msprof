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

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transpose_generator.h"
#include "common/fe_inner_error_codes.h"
#include "common/util/op_info_util.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"

namespace fe {
static const trans_pose_map kTransposeMap = {
    /* dst format is FORMAT_C1HWNCoC0 */
    {CalcStrategyId(ge::FORMAT_NCHW, ge::FORMAT_C1HWNCoC0, kStrategyIdExtDeft), ge::FORMAT_HWCN},
    {CalcStrategyId(ge::FORMAT_NHWC, ge::FORMAT_C1HWNCoC0, kStrategyIdExtDeft), ge::FORMAT_HWCN},
    {CalcStrategyId(ge::FORMAT_HWCN, ge::FORMAT_C1HWNCoC0, kStrategyIdExtDeft), ge::FORMAT_HWCN},
    {CalcStrategyId(ge::FORMAT_C1HWNCoC0, ge::FORMAT_C1HWNCoC0, kStrategyIdExtDeft), ge::FORMAT_HWCN},
    {CalcStrategyId(ge::FORMAT_FRACTAL_NZ, ge::FORMAT_C1HWNCoC0, kStrategyIdExtDeft), ge::FORMAT_HWCN},
    /* dst format is FORMAT_NC1HWC0 */
    {CalcStrategyId(ge::FORMAT_HWCN, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_CHWN, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_NCHW, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_NHWC, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_NC1HWC0, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_ND, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_FRACTAL_NZ, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_MAX},
    /* dst format is FORMAT_FRACTAL_Z */
    {CalcStrategyId(ge::FORMAT_NCHW, ge::FORMAT_FRACTAL_Z, kStrategyIdExtThree), ge::FORMAT_HWCN},
    {CalcStrategyId(ge::FORMAT_NHWC, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_CHWN, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_HWCN, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_NC1HWC0, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_FRACTAL_Z, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_ND, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_MAX},
    {CalcStrategyId(ge::FORMAT_FRACTAL_NZ, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_MAX},
};

Status SetTransposeOrder(ge::Format input_format, ge::Format output_format, ge::OpDescPtr op_desc_ptr) {
  auto input_iter = FORMAT_INDEX_MAP.find(input_format);
  if (input_iter == FORMAT_INDEX_MAP.end()) {
    FE_LOGW("Can not find input format %u in format map!", input_format);
    return SUCCESS;
  }
  auto output_iter = FORMAT_INDEX_MAP.find(output_format);
  if (output_iter == FORMAT_INDEX_MAP.end()) {
    FE_LOGW("Can not find output format0 %u in format map!", input_format);
    return SUCCESS;
  }
  if (PERM_VALUE_VECTOR.empty()) {
    FE_LOGW("Perm value vector is empty!");
    return SUCCESS;
  }

  if (input_iter->second >= PERM_VALUE_VECTOR.size()) {
    FE_LOGW("Index %u is larger than first dimension of Perm vector.", input_iter->second);
    return SUCCESS;
  }

  if (PERM_VALUE_VECTOR[input_iter->second].empty()) {
    FE_LOGW("Perm value vector %u is empty!", input_iter->second);
    return SUCCESS;
  }

  if (output_iter->second >= PERM_VALUE_VECTOR[input_iter->second].size()) {
    FE_LOGW("Index %u is larger than second dim of Perm vector %u.", output_iter->second, input_iter->second);
    return SUCCESS;
  }

  vector<int64_t> attr_order = PERM_VALUE_VECTOR[input_iter->second][output_iter->second];

  if (!(ge::AttrUtils::SetListInt(op_desc_ptr, ge::PERMUTE_ATTR_ORDER, attr_order))) {
    FE_LOGW(
        "Failed to set transpose order for op (name [%s] type [%s]). And the "
        "input format is [%u].",
        op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), input_format);
  }
  if (!(ge::AttrUtils::SetListInt(op_desc_ptr, PERM, attr_order))) {
    FE_LOGW(
        "Failed to set transpose perm for op (name [%s] type [%s]). And the "
        "input format is [%u].",
        op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), input_format);
  }

  return SUCCESS;
}

TransNodeTransposeGenerator::TransNodeTransposeGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr,
                                                         TransInfoPtr trans_info_ptr)
    : TransNodeBaseGenerator(fe_ops_store_ptr, trans_info_ptr) {}

TransNodeTransposeGenerator::~TransNodeTransposeGenerator() {}

Status TransNodeTransposeGenerator::AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) {
  trans_info_ptr_ = trans_info_ptr;
  TransformDimTo4(true);
  /* output format and dtype of source node should be the same as permute op's
   * input format and dtype! */
  ge::Format out_format_new_node = trans_info_ptr->dst_in_primary_format;
  int32_t out_sub_format = trans_info_ptr->dst_in_sub_format;

  auto trans_pose = kTransposeMap.find(trans_info_ptr->strategy_id);
  if (trans_pose != kTransposeMap.end()) {
    if (trans_pose->second == ge::FORMAT_MAX) {
      /* In Other Cases, we will not tranform by transpose op */
      out_format_new_node = trans_info_ptr->src_out_primary_format;
      out_sub_format = trans_info_ptr->src_out_sub_format;
    } else {
      out_format_new_node = trans_pose->second;
      out_sub_format = 0;
    }
  }

  return AddOpAndNode(fused_graph, out_format_new_node, out_sub_format, trans_info_ptr->src_out_data_type);
}

void TransNodeTransposeGenerator::GetNewShape(ge::OpDescPtr &op_desc_ptr, ge::Format format, ge::DataType dtype,
                                              int64_t imply_type, ge::GeShape &newshape) {
  ShapeAndFormat output_shape_and_format_info = {trans_info_ptr_->src_out_shape,
                                                 newshape,
                                                 trans_info_ptr_->src_out_primary_format,
                                                 format,
                                                 dtype,
                                                 imply_type,
                                                 GROUPS_DEFAULT_VALUE};
  /* Update output shape of transpose based on it's imple type */
  (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(output_shape_and_format_info);

  SetTensorRealDimCountAndNewShape(op_desc_ptr, {trans_info_ptr_->src_out_shape}, newshape);
  SetNewShapeRange(op_desc_ptr, trans_info_ptr_->src_out_range, trans_info_ptr_->dst_in_range);
}

Status TransNodeTransposeGenerator::AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::Format &primary_format,
                                                 const int32_t &sub_format, const ge::DataType &dtype) {
  TransInfoPtr trans_info_ptr = trans_info_ptr_;
  if (trans_info_ptr_->src_out_primary_format == primary_format) {
    FE_LOGD("format %u of %s is equal to format %u of %s.", trans_info_ptr_->src_out_primary_format,
            trans_info_ptr->src_op_desc->GetName().c_str(), primary_format,
            trans_info_ptr->dst_op_desc->GetName().c_str());
    return SUCCESS;
  }
  ge::OpDescPtr op_desc_ptr = CreateBasicOpDescForTransNode(TRANSPOSE);
  FE_CHECK_NOTNULL(op_desc_ptr);

  FE_LOGD("Create [%s] node between [%s] and [%s] success!", TRANSPOSE.c_str(),
          trans_info_ptr->src_op_desc->GetName().c_str(), trans_info_ptr->dst_op_desc->GetName().c_str());

  ge::GeShape newshape = trans_info_ptr_->src_out_shape;
  ge::Format input_format = static_cast<ge::Format>(
      ge::GetFormatFromSub(trans_info_ptr_->src_out_primary_format, trans_info_ptr_->src_out_sub_format));
  if (op_desc_ptr->AddInputDesc(TRANSPOSE_INPUT_NAME, ge::GeTensorDesc(trans_info_ptr_->src_out_shape, input_format,
                                                                       trans_info_ptr->src_out_data_type)) != SUCCESS) {
    FE_LOGD("CreateReshapeOp: op [RESHAPE]: add input desc fail.");
    return FAILED;
  }

  ge::Format output_format = static_cast<ge::Format>(ge::GetFormatFromSub(primary_format, sub_format));
  if (op_desc_ptr->AddOutputDesc(TRANSPOSE_OUTPUT_NAME, ge::GeTensorDesc(newshape, output_format, dtype)) != SUCCESS) {
    FE_LOGD("CreateReshapeOp: op [RESHAPE]: add output desc fail.");
    return FAILED;
  }
  /* Set required attr perm first */
  SetTransposeOrder(trans_info_ptr_->src_out_primary_format, primary_format, op_desc_ptr);

  (void)SetTensorDescInfo(op_desc_ptr);

  /*  If we can not find specific transpose, we just skip this operation
   * and keep running. */
  if (!TransNodeCheckAccuracySupported(op_desc_ptr, true)) {
    FE_LOGW("Format %u to %u is not supported by %s!", trans_info_ptr_->src_out_primary_format,
            trans_info_ptr_->dst_in_primary_format, op_desc_ptr->GetName().c_str());
    return SUCCESS;
  }

  // insert new op need add attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES for data dump
  std::vector<std::string> original_names;
  if (!ge::AttrUtils::SetListStr(op_desc_ptr, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_names)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][AddOpAndNd] Set op[%s] attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES \
                    failed.", op_desc_ptr->GetName().c_str());
    return FAILED;
  }

  int64_t imply_type = -1;
  if (!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, imply_type)) {
    FE_LOGW("Get imply_type of op %s not success!", op_desc_ptr->GetName().c_str());
    return SUCCESS;
  }

  GetNewShape(op_desc_ptr, primary_format, dtype, imply_type, newshape);
  if (AddEdgesAndFreshTransInfo(fused_graph, op_desc_ptr) != SUCCESS) {
    FE_LOGD("Add edge failed!");
    return FAILED;
  }

  return SUCCESS;
}

}  // namespace fe
