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

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdata_generator.h"
#include <memory>
#include <string>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/op_info_common.h"
#include "common/unknown_shape_util.h"
#include "common/util/op_info_util.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"

namespace fe {

static const trans_data_map kTransdataMap = {
    {CalcStrategyId(ge::FORMAT_FRACTAL_Z, ge::FORMAT_NHWC, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_FRACTAL_Z, ge::FORMAT_CHWN, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_FRACTAL_Z, ge::FORMAT_NC1HWC0, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_FRACTAL_Z, ge::FORMAT_NCHW, kStrategyIdExtThree), ge::FORMAT_HWCN},
    {CalcStrategyId(ge::FORMAT_NC1HWC0, ge::FORMAT_HWCN, kStrategyIdExtDeft), ge::FORMAT_NCHW},
    {CalcStrategyId(ge::FORMAT_NC1HWC0, ge::FORMAT_FRACTAL_Z, kStrategyIdExtDeft), ge::FORMAT_NCHW},
};

TransNodeTransdataGenerator::TransNodeTransdataGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr,
                                                         TransInfoPtr trans_info_ptr)
    : TransNodeBaseGenerator(fe_ops_store_ptr, trans_info_ptr) {}
TransNodeTransdataGenerator::~TransNodeTransdataGenerator() {}

Status TransNodeTransdataGenerator::AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) {
  /* The two anchors of inserting TransData op should be same in data type.
   * If it is 4D to 5HD, 4D format should be NCHW.
   * Thas requires we do Permute and Cast before TransData. */
  trans_info_ptr_ = trans_info_ptr;

  TransformDimTo4(true);
  ge::Format out_format_new_node = trans_info_ptr->dst_in_primary_format;
  auto out_sub_format = trans_info_ptr->dst_in_sub_format;

  auto trans_data = kTransdataMap.find(trans_info_ptr->strategy_id);
  if (trans_data != kTransdataMap.end()) {
    out_format_new_node = trans_data->second;
    out_sub_format = 0;
  } else if (trans_info_ptr->src_out_primary_format == ge::FORMAT_FRACTAL_NZ) {
    /* To ensure that Format Nz will only be tranformed to
     * its original format */
    /* For Nz to NCHW or NHWC or HWCN, we set the output
     * format as ND, to let the trans-nodes support more scenario and
     * let two trans-nodes of ND->Nz and Nz->ND merge themselves.
     * Because NHWC->NZ and NZ->HWCN will not merge automatically althought
     * their shape is the same */
    out_format_new_node = ge::FORMAT_ND;
    out_sub_format = 0;
  } else if (trans_info_ptr->dst_in_primary_format == ge::FORMAT_FRACTAL_NZ) {
    /* For NCHW or NHWC or HWCN to Nz, we set the input format of trans-nodes
     * as ND, to let the trans-nodes support more scenario and
     * let two trans-nodes of ND->Nz and Nz->ND merge themselves.
     * Because NHWC->NZ and NZ->HWCN will not merge automatically althought
     * their shape is the same */
    trans_info_ptr->src_out_primary_format = ge::FORMAT_ND;
  } else if (trans_info_ptr->src_out_primary_format == ge::FORMAT_C1HWNCoC0) {
    out_format_new_node = ge::FORMAT_HWCN;
    out_sub_format = 0;
  }

  return AddOpAndNode(fused_graph, ge::GeShape(), out_format_new_node, out_sub_format,
                      trans_info_ptr->src_out_data_type);
}

bool IsNeedToUpdateOutputShapeByOriginalShape(TransInfoPtr trans_info_ptr) {
  bool is_common_format = false;
  for (auto iter : FE_ORIGIN_FORMAT_VECTOR) {
    if (iter == trans_info_ptr->src_out_primary_format) {
      is_common_format = true;
      break;
    }
  }
  if (is_common_format) {
    return false;
  } else if ((trans_info_ptr->src_out_primary_format == ge::FORMAT_NC1HWC0 ||
              trans_info_ptr->src_out_primary_format == ge::FORMAT_NC1HWC0_C04) &&
             (trans_info_ptr->src_out_original_shape.GetDimNum() > DIM_DEFAULT_SIZE ||
              trans_info_ptr->src_out_original_shape.GetDimNum() == 0)) {
    /* Src out format is 5HD and it's original shape is already 5HD or empty,
     * we don't need to calculate the new shape by original shape. */
    return false;
  } else {
    return true;
  }
}

/* Get input and output shape of op transdata. */
Status TransNodeTransdataGenerator::GetShapeOfTransdata(const ge::OpDescPtr op_desc_ptr, ge::GeShape &new_in_shape,
                                                        ge::GeShape &new_out_shape,
                                                        vector<std::pair<int64_t, int64_t>> &new_in_range,
                                                        vector<std::pair<int64_t, int64_t>> &new_out_range,
                                                        const ge::Format &output_format,
                                                        const ge::DataType &output_dtype) {
  /* CCE transdata node will be ignored in our progress,
     but GE constant folding depends on transdata's shape,
     to resolve this, set imply_type as TBE. */
  int64_t imply_type = EN_IMPL_HW_TBE;

  ge::GeShape temp_src_out_shape;
  ge::Format temp_src_out_format;
  int64_t group_value = GROUPS_DEFAULT_VALUE;
  if (trans_info_ptr_->src_out_primary_format == ge::FORMAT_FRACTAL_Z) {
    /* Update shape of input of transdata base on its imply type.
     * For Fragz, we will change the shape based on its
     * father's original shape */
    temp_src_out_shape = trans_info_ptr_->src_out_original_shape;
    temp_src_out_format = trans_info_ptr_->src_out_original_format;
    std::vector<int64_t> new_dims = temp_src_out_shape.GetDims();
    ExpandDimension(new_dims, trans_info_ptr_->src_op_desc_type, temp_src_out_format,
                    trans_info_ptr_->src_out_primary_format, 0, trans_info_ptr_->src_reshape_type);
    temp_src_out_shape = ge::GeShape(new_dims);
    group_value = trans_info_ptr_->src_out_sub_format;

  } else {
    temp_src_out_shape = trans_info_ptr_->src_out_shape;
    temp_src_out_format = trans_info_ptr_->src_out_primary_format;
  }

  CalcShapeExtraAttr extra_attr = {trans_info_ptr_->hidden_size, trans_info_ptr_->input_size,
                                   trans_info_ptr_->state_size};
  ShapeAndFormat input_shape_and_format_info = {temp_src_out_shape,
                                                new_in_shape,
                                                temp_src_out_format,
                                                trans_info_ptr_->src_out_primary_format,
                                                trans_info_ptr_->src_out_data_type,
                                                imply_type,
                                                group_value,
                                                extra_attr};
  (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(input_shape_and_format_info);
  if (IsFeSupportedDynamicOp(*op_desc_ptr, true)) {
    RangeAndFormat input_range_and_format_info = {temp_src_out_shape,
                                                  trans_info_ptr_->src_out_range,
                                                  new_in_range,
                                                  temp_src_out_format,
                                                  trans_info_ptr_->src_out_primary_format,
                                                  trans_info_ptr_->src_out_data_type,
                                                  imply_type,
                                                  extra_attr};
    (void)RangeTransferAccordingToFormat::GetRangeAccordingToFormat(input_range_and_format_info);
  }
  /* If 5D to 4D we use source node's original shape as final
   * 4D shape, and do transpose if necessary.
   * If source node's original shape is 5D, than we just
   * use formula {C = C1 * C0, N,H,W remain the same as 5D} to get the new 4D
   * shape */
  if (IsNeedToUpdateOutputShapeByOriginalShape(trans_info_ptr_) &&
      !IsShapeContainUnknownDimNum(trans_info_ptr_->src_out_shape)) {
    /* Update output shape of transdata based on original shape and
     * original format of its father node */
    temp_src_out_shape = trans_info_ptr_->src_out_original_shape;
    temp_src_out_format = trans_info_ptr_->src_out_original_format;
    std::vector<int64_t> new_dims = temp_src_out_shape.GetDims();

    ExpandDimension(new_dims, trans_info_ptr_->src_op_desc_type, temp_src_out_format,
                    trans_info_ptr_->src_out_primary_format, 0, trans_info_ptr_->src_reshape_type);

    temp_src_out_shape = ge::GeShape(new_dims);
  } else {
    /* Update output shape of transdata based on its input. */
    temp_src_out_shape = new_in_shape;
    temp_src_out_format = trans_info_ptr_->src_out_primary_format;
  }

  int64_t dst_group_value = trans_info_ptr_->dst_in_sub_format;
  ShapeAndFormat output_shape_and_format_info = {temp_src_out_shape, new_out_shape, temp_src_out_format, output_format,
                                                 output_dtype,       imply_type,    dst_group_value,     extra_attr};
  (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(output_shape_and_format_info);
  if (IsFeSupportedDynamicOp(*op_desc_ptr, true)) {
    RangeAndFormat output_range_and_format_info = {
        new_in_shape,  new_in_range, new_out_range, trans_info_ptr_->src_out_primary_format,
        output_format, output_dtype, imply_type,    extra_attr};
    (void)RangeTransferAccordingToFormat::GetRangeAccordingToFormat(output_range_and_format_info);
  }
  return SUCCESS;
}

void TransNodeTransdataGenerator::SetAttr(const TransInfoPtr &trans_info_ptr, const ge::Format &primary_format,
                                          const int32_t &sub_format, ge::OpDescPtr &op_desc_ptr) const {
  (void)ge::AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_INPUT_FORMAT,
                              static_cast<int64_t>(trans_info_ptr->src_out_primary_format));
  (void)ge::AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_OUTPUT_FORMAT, static_cast<int64_t>(primary_format));
  string src_format = ge::TypeUtils::FormatToSerialString(trans_info_ptr->src_out_primary_format);
  string dst_format = ge::TypeUtils::FormatToSerialString(primary_format);
  (void)ge::AttrUtils::SetStr(op_desc_ptr, ATTR_NAME_SRC_FORMAT, src_format);
  (void)ge::AttrUtils::SetStr(op_desc_ptr, ATTR_NAME_DST_FORMAT, dst_format);

  // src
  if (std::find(FE_GROUP_RELA_FORMAT_VECTOR.begin(), FE_GROUP_RELA_FORMAT_VECTOR.end(),
                trans_info_ptr->src_out_primary_format) != FE_GROUP_RELA_FORMAT_VECTOR.end()) {
    FE_LOGD(
        "Transdata[%s]: src_out_primary_format=%s, src_out_sub_format=%d, dst_out_primary_format=%s,\
            dst_out_sub_format=%d.",
        op_desc_ptr->GetName().c_str(), FormatToStr(trans_info_ptr->src_out_primary_format).c_str(),
        trans_info_ptr->src_out_sub_format, FormatToStr(primary_format).c_str(), sub_format);
    int64_t group = trans_info_ptr->src_out_sub_format;
    if (group < GROUPS_DEFAULT_VALUE) {
      group = GROUPS_DEFAULT_VALUE;
    }

    (void)ge::AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_GROUPS, group);
    FE_LOGD("Set groups attribute[%ld] for transdata node[%s].", group, op_desc_ptr->GetName().c_str());
  }

  // dst
  if (std::find(FE_GROUP_RELA_FORMAT_VECTOR.begin(), FE_GROUP_RELA_FORMAT_VECTOR.end(), primary_format) !=
      FE_GROUP_RELA_FORMAT_VECTOR.end()) {
    FE_LOGD(
        "Transdata[%s]: src_out_primary_format=%s, src_out_sub_format=%d, dst_out_primary_format=%s,\
            dst_out_sub_format=%d.",
        op_desc_ptr->GetName().c_str(), FormatToStr(trans_info_ptr->src_out_primary_format).c_str(),
        trans_info_ptr->src_out_sub_format, FormatToStr(primary_format).c_str(), sub_format);
    int64_t group = sub_format;
    if (group < GROUPS_DEFAULT_VALUE) {
      group = GROUPS_DEFAULT_VALUE;
    }

    (void)ge::AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_GROUPS, group);
    FE_LOGD("Set groups attribute[%ld] for transdata node[%s].", group, op_desc_ptr->GetName().c_str());
  }
}  // namespace fe

Status TransNodeTransdataGenerator::AddAndSetTensor(const ge::GeShape &shape, const ge::Format &primary_format,
                                                    const int32_t &sub_format, const ge::DataType &dtype,
                                                    ge::OpDescPtr &op_desc_ptr) {
  ge::Format input_format = static_cast<ge::Format>(
      ge::GetFormatFromSub(trans_info_ptr_->src_out_primary_format, trans_info_ptr_->src_out_sub_format));
  ge::GeTensorDesc input =
      ge::GeTensorDesc(trans_info_ptr_->src_out_shape, input_format, trans_info_ptr_->src_out_data_type);
  if (op_desc_ptr->AddInputDesc(TRANSDATA_INPUT_NAME, input) != SUCCESS) {
    FE_LOGD("CreateTransdataOp: op [Transdata]: add input desc fail.");
    return FAILED;
  }

  ge::Format output_format = static_cast<ge::Format>(ge::GetFormatFromSub(primary_format, sub_format));
  if (op_desc_ptr->AddOutputDesc(TRANSDATA_OUTPUT_NAME, ge::GeTensorDesc(shape, output_format, dtype)) != SUCCESS) {
    FE_LOGD("CreateTransdataOp: op [Transdata]: add output desc fail.");
    return FAILED;
  }

  (void)SetTensorDescInfo(op_desc_ptr);
  return SUCCESS;
}

Status TransNodeTransdataGenerator::AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::GeShape &shape,
                                                 const ge::Format &out_primary_format, const int32_t &out_sub_format,
                                                 const ge::DataType &dtype) {
  TransInfoPtr trans_info_ptr = trans_info_ptr_;
  ge::OpDescPtr op_desc_ptr = CreateBasicOpDescForTransNode(TRANSDATA);
  FE_CHECK_NOTNULL(op_desc_ptr);

  FE_LOGD("Create [%s] node between [%s] and [%s] success!", TRANSDATA.c_str(),
          trans_info_ptr->src_op_desc->GetName().c_str(), trans_info_ptr->dst_op_desc->GetName().c_str());

  SetAttr(trans_info_ptr, out_primary_format, out_sub_format, op_desc_ptr);

  if (AddAndSetTensor(shape, out_primary_format, out_sub_format, dtype, op_desc_ptr) != SUCCESS) {
    return FAILED;
  }
  /*  If we can not find specific transdata, we just skip this operation
   * and keep running. */
  /* There are no trans-op for FRACTAL_Z_3D and FORMAT_FRACTAL_Z_3D_TRANSPOSE,
   * so if we encouter them, we just skip the check accuracy support and
   * still insert that kind of TransData into the graph and GE will then
   * fuse them will const node. */
  bool always_insert_flag = trans_info_ptr->src_out_primary_format == ge::FORMAT_FRACTAL_NZ ||
                            trans_info_ptr->dst_in_primary_format == ge::FORMAT_FRACTAL_NZ ||
                            (dtype != ge::DT_INT32 && dtype != ge::DT_DOUBLE);

  /*  If we can not find specific transdata, we just skip this operation
   * and keep running. */
  if (!TransNodeCheckAccuracySupported(op_desc_ptr, true, always_insert_flag)) {
    FE_LOGW("[GraphOpt][Trans][TransData] Format %u to %u of op %s to %s is not supported by %s!",
            trans_info_ptr_->src_out_primary_format, trans_info_ptr_->dst_in_primary_format,
            trans_info_ptr_->src_op_desc->GetName().c_str(), trans_info_ptr_->dst_op_desc->GetName().c_str(),
            op_desc_ptr->GetName().c_str());
    return SUCCESS;
  }

  // insert new op need add attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES
  // for data dump
  std::vector<std::string> original_names;
  if (!ge::AttrUtils::SetListStr(op_desc_ptr, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_names)) {
    REPORT_FE_ERROR(
        "[GraphOptJdgInst][ShapeTrans][AddOpAndNd] Set op[%s] attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES \
                    failed.",
        op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  ge::GeShape new_out_shape;
  ge::GeShape new_in_shape;
  vector<std::pair<int64_t, int64_t>> new_in_range;
  vector<std::pair<int64_t, int64_t>> new_out_range;
  GetShapeOfTransdata(op_desc_ptr, new_in_shape, new_out_shape, new_in_range, new_out_range, out_primary_format, dtype);
  SetTensorRealDimCountAndNewShape(op_desc_ptr, {new_in_shape}, new_out_shape);
  SetNewShapeRange(op_desc_ptr, new_in_range, new_out_range);
  if (AddEdgesAndFreshTransInfo(fused_graph, op_desc_ptr) != SUCCESS) {
    FE_LOGD("Add edge failed!");
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace fe