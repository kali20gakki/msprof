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

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_cast_generator.h"
#include "common/fe_inner_error_codes.h"
#include "common/util/op_info_util.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"

namespace fe {
TransNodeCastGenerator::TransNodeCastGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr, TransInfoPtr trans_info_ptr)
    : TransNodeBaseGenerator(fe_ops_store_ptr, trans_info_ptr) {}

TransNodeCastGenerator::~TransNodeCastGenerator() {}

Status TransNodeCastGenerator::AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) {
  trans_info_ptr_ = trans_info_ptr;
  if (trans_info_ptr->src_out_data_type == trans_info_ptr->dst_in_data_type) {
    return SUCCESS;
  }

  /* After Inserting Cast Op, we will insert TransData op. So for Cast op, its
   input and output format can just
   * be the same as the output format of its source node.
   e.g. SOURCE(5HD-5HD, Fp16) -> CAST(5HD-5HD, FP16 to FP32) ->
   DESTINATION(4D-4D, FP32) After inserting TransData Op, it will become:
   SOURCE(5HD-5HD, Fp16) -> TRANSDATA(5HD to 4D, FP32) -> CAST(5HD-5HD, FP16 to
   FP32) -> DESTINATION(4D-4D, FP32)
   */
  return AddOpAndNode(fused_graph, trans_info_ptr->src_out_shape, trans_info_ptr->src_out_primary_format,
                      trans_info_ptr->src_out_sub_format, trans_info_ptr->dst_in_data_type);
}

Status TransNodeCastGenerator::AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::GeShape &shape,
                                            const ge::Format &primary_format, const int32_t &sub_format,
                                            const ge::DataType &dtype) {
  TransInfoPtr trans_info_ptr = trans_info_ptr_;
  ge::OpDescPtr op_desc_ptr = CreateBasicOpDescForTransNode(CAST);
  FE_CHECK_NOTNULL(op_desc_ptr);

  FE_LOGD("Create [%s] node between [%s] and [%s] success!", CAST.c_str(),
          trans_info_ptr->src_op_desc->GetName().c_str(), trans_info_ptr->dst_op_desc->GetName().c_str());

  (void)ge::AttrUtils::SetInt(op_desc_ptr, CAST_ATTR_SRCT, static_cast<int64_t>(trans_info_ptr->src_out_data_type));
  (void)ge::AttrUtils::SetInt(op_desc_ptr, CAST_ATTR_DSTT, static_cast<int64_t>(dtype));
  (void)ge::AttrUtils::SetInt(op_desc_ptr, CAST_ATTR_DST_TYPE, static_cast<int64_t>(dtype));
  if (!ge::AttrUtils::SetBool(op_desc_ptr, CAST_ATTR_TRUNCATE, false)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][AddOpAndNd] Set op[%s] attr CAST_ATTR_TRUNCATE failed.",
                    op_desc_ptr->GetName().c_str());
    return FAILED;
  }

  auto input_format = static_cast<ge::Format>(
      ge::GetFormatFromSub(trans_info_ptr_->src_out_primary_format, trans_info_ptr_->src_out_sub_format));
  if (op_desc_ptr->AddInputDesc(CAST_INPUT_NAME, ge::GeTensorDesc(trans_info_ptr_->src_out_shape, input_format,
                                                                  trans_info_ptr->src_out_data_type)) != SUCCESS) {
    FE_LOGD("CreateReshapeOp by cast: op [RESHAPE]: add input desc fail.");
    return FAILED;
  }

  auto output_format = static_cast<ge::Format>(ge::GetFormatFromSub(primary_format, sub_format));
  if (op_desc_ptr->AddOutputDesc(CAST_OUTPUT_NAME, ge::GeTensorDesc(shape, output_format, dtype)) != SUCCESS) {
    FE_LOGD("CreateReshapeOp by cast: op [RESHAPE]: add output desc fail.");
    return FAILED;
  }
  if (!ge::AttrUtils::SetBool(op_desc_ptr, ge::ATTR_NEED_COMPILE, true)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][AddOpAndNd] Set op[%s] attr need compile failed.",
                    op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  (void)SetTensorDescInfo(op_desc_ptr);

  (void)ge::AttrUtils::SetInt(op_desc_ptr, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);

  // insert new op need add attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES
  // for data dump
  std::vector<std::string> original_names;
  if (!ge::AttrUtils::SetListStr(op_desc_ptr, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_names)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][AddOpAndNd] Set op[%s] attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES \
                    failed.", op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  SetTensorRealDimCountAndNewShape(op_desc_ptr, {trans_info_ptr_->src_out_shape}, shape);
  SetNewShapeRange(op_desc_ptr, trans_info_ptr_->src_out_range, trans_info_ptr_->src_out_range);
  if (AddEdgesAndFreshTransInfo(fused_graph, op_desc_ptr) != SUCCESS) {
    FE_LOGD("Add edge failed!");
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace fe
