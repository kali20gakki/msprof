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

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_reformat_generator.h"
#include "common/fe_inner_error_codes.h"
#include "common/util/op_info_util.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"

namespace fe {
TransNodeReformatGenerator::TransNodeReformatGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr,
                                                       TransInfoPtr trans_info_ptr)
    : TransNodeBaseGenerator(fe_ops_store_ptr, trans_info_ptr) {}

TransNodeReformatGenerator::~TransNodeReformatGenerator() {}

Status TransNodeReformatGenerator::AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) {
  trans_info_ptr_ = trans_info_ptr;
  /* For reformat op, if the src out format is ND and dst in format is Nz or
   * src out format is Nz and dst in format is ND,
   * we skip this insertion case.
   *
   * If the src out format is ND and dst in format is NCHW or NHWC or HWCN,
   * we insert it directly and its input format is ND and its output format
   * is NCHW or NHWC or HWCN.
   * If the src out format is NCHW or NHWC or HWCN and dst in format is Nz,
   * we insert it with the input format as NCHW or NHWC or HWCN and the output
   * format as ND. */
  if ((trans_info_ptr_->src_out_primary_format == ge::FORMAT_ND &&
      trans_info_ptr_->dst_in_primary_format == ge::FORMAT_FRACTAL_NZ) ||
      (trans_info_ptr_->src_out_primary_format == ge::FORMAT_FRACTAL_NZ &&
      trans_info_ptr_->dst_in_primary_format == ge::FORMAT_ND)) {
    return SUCCESS;
  }

  /* Here if the trans_info_ptr_->src_out_primary_format is not ND, it
   * can only be NCHW/HWCN/NHWC. And if trans_info_ptr_->src_out_primary_format is ND,
   * the dst_in_primary_format can only be NCHW/HWCN/NHWC */
  ge::Format output_primary_format;
  int32_t output_sub_format = 0;
  if (trans_info_ptr_->src_out_primary_format != ge::FORMAT_ND) {
    output_primary_format = ge::FORMAT_ND;
    output_sub_format = 0;
  } else {
    output_primary_format = trans_info_ptr_->dst_in_primary_format;
    output_sub_format = trans_info_ptr_->dst_in_sub_format;
  }

  ge::GeShape output_shape = trans_info_ptr_->src_out_shape;
  ge::DataType output_dtype = trans_info_ptr_->src_out_data_type;

  return AddOpAndNode(fused_graph, output_shape, output_primary_format, output_sub_format, output_dtype);
}

Status TransNodeReformatGenerator::AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::GeShape &shape,
                                                const ge::Format &primary_format, const int32_t &sub_format,
                                                const ge::DataType &dtype) {
  ge::OpDescPtr op_desc_ptr = CreateBasicOpDescForTransNode(REFORMAT);
  FE_CHECK(op_desc_ptr == nullptr, FE_LOGW("AddOpAndNode : op_desc_ptr is mull."), return PARAM_INVALID);

  FE_LOGD("Create [%s] node between [%s] and [%s] success!", REFORMAT.c_str(),
          trans_info_ptr_->src_op_desc->GetName().c_str(), trans_info_ptr_->dst_op_desc->GetName().c_str());

  auto input_format = static_cast<ge::Format>(
      ge::GetFormatFromSub(trans_info_ptr_->src_out_primary_format, trans_info_ptr_->src_out_sub_format));
  if (op_desc_ptr->AddInputDesc(ge::GeTensorDesc(trans_info_ptr_->src_out_shape, input_format,
                                                 trans_info_ptr_->src_out_data_type)) != SUCCESS) {
    FE_LOGD("CreateReformatOp: op [REFORMAT]: add input desc fail.");
    return FAILED;
  }

  auto output_format = static_cast<ge::Format>(ge::GetFormatFromSub(primary_format, sub_format));
  if (op_desc_ptr->AddOutputDesc(ge::GeTensorDesc(shape, output_format, dtype)) != SUCCESS) {
    FE_LOGD("CreateReformatOp: op [REFORMAT]: add output desc fail.");
    return FAILED;
  }

  /* Reshape op is not belong to any ops store */
  (void)ge::AttrUtils::SetInt(op_desc_ptr, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::AI_CPU));
  (void)ge::AttrUtils::SetInt(op_desc_ptr, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);

  (void)SetTensorDescInfo(op_desc_ptr);

  SetTensorRealDimCountAndNewShape(op_desc_ptr, {trans_info_ptr_->src_out_shape}, shape);
  SetNewShapeRange(op_desc_ptr, trans_info_ptr_->src_out_range, trans_info_ptr_->dst_in_range);
  if (AddEdgesAndFreshTransInfo(fused_graph, op_desc_ptr) != SUCCESS) {
    FE_LOGW("Add edges and fresh trans info failed.");
    return FAILED;
  }

  return SUCCESS;
}
}
