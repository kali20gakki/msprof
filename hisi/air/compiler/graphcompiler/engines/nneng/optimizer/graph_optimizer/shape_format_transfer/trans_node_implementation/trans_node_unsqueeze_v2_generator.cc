/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_unsqueeze_v2_generator.h"
#include <sstream>
#include "common/fe_inner_error_codes.h"
#include "common/util/op_info_util.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"

namespace fe {
TransNodeUnsqueezeV2Generator::TransNodeUnsqueezeV2Generator(FEOpsKernelInfoStorePtr fe_ops_store_ptr,
                                                             TransInfoPtr trans_info_ptr)
    : TransNodeBaseGenerator(fe_ops_store_ptr, trans_info_ptr) {}

TransNodeUnsqueezeV2Generator::~TransNodeUnsqueezeV2Generator() {}

Status TransNodeUnsqueezeV2Generator::AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) {
  trans_info_ptr_ = trans_info_ptr;
  auto src_out_shape_dim = trans_info_ptr->src_out_shape.GetDimNum();
  auto dst_in_shape_dim = trans_info_ptr->dst_in_shape.GetDimNum();
  /*
   * Now we only support 1,2,3D to 4D. UnsqueezeV2 op requires
   * programmer to elucidate how to shape will change.
   * e.g. To unsqueeze 2D(NH) -> 4D(NCHW), we will add dimension c,
   * w to 2D op and shape of c,w is 1.
   * So 2D(NH) becomes 4D(n,1(c),h,1(w).
   */
  if (src_out_shape_dim < dst_in_shape_dim && src_out_shape_dim <= LOW_DIMENSION_NUM_THD && src_out_shape_dim > 0) {
    /* 1,2,3D -> NCHW */
    std::vector<int64_t> dim_vec_new;
    dim_vec_new = trans_info_ptr_->src_out_shape.GetDims();
    ExpandDimension(dim_vec_new, trans_info_ptr_->dst_op_desc->GetType(), trans_info_ptr_->src_out_primary_format,
                    trans_info_ptr_->src_out_primary_format, trans_info_ptr_->dst_anchor->GetIdx(),
                    trans_info_ptr_->dst_reshape_type);

    ge::GeShape new_shape = ge::GeShape(dim_vec_new);
    string new_shapestr = GetShapeDims(new_shape);
    FE_LOGD("After unsqueeze the new shape is %s.", new_shapestr.c_str());
    FE_LOGD("Source node is %s, dst node is %s.", trans_info_ptr_->src_op_desc->GetName().c_str(),
            trans_info_ptr_->dst_op_desc->GetName().c_str());
    return AddOpAndNode(fused_graph, new_shape, trans_info_ptr_->src_out_primary_format,
                        trans_info_ptr_->src_out_sub_format, trans_info_ptr->src_out_data_type);
  } else {
    FE_LOGI("[GraphOpt][Trans][Unsqueeze] can not unsqueeze from node[%s] to node[%s].",
            trans_info_ptr_->src_op_desc->GetName().c_str(), trans_info_ptr_->dst_op_desc->GetName().c_str());
    return SUCCESS;
  }
}

Status TransNodeUnsqueezeV2Generator::SetAttr(const ge::OpDescPtr &op_desc_ptr) const {
  std::vector<int32_t> axis;

  auto reshape_type = trans_info_ptr_->dst_reshape_type;
  if (reshape_type.empty()) {
    if (GetDefaultReshapeType(trans_info_ptr_->src_out_primary_format, trans_info_ptr_->src_out_shape.GetDims().size(),
                              reshape_type) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Trans][UnsqueezeV2] GetDefaultReshapeType failed.");
      return FAILED;
    }
  }

  std::string final_format = ConstFormatToStr(trans_info_ptr_->src_out_primary_format);
  for (size_t i = 0; i < final_format.size(); ++i) {
    if (reshape_type.find(final_format[i]) != std::string::npos) {
      continue;
    }

    std::string axis_str(1, final_format[i]);
    int32_t dim_index = GetAxisIndexByFormat(trans_info_ptr_->src_out_primary_format, axis_str);
    if (dim_index < 0) {
      REPORT_FE_ERROR("[GraphOpt][Trans][Unsqueeze] dim_index is less than 0.");
      return FAILED;
    }
    axis.push_back(dim_index);
  }

  // 2. set attr axis info
  if (!ge::AttrUtils::SetListInt(op_desc_ptr, AXIS_ATTR_NAME, axis)) {
    REPORT_FE_ERROR("[GraphOpt][Trans][Unsqueeze] Set unsqueeze op [%s] axis failed!", op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status TransNodeUnsqueezeV2Generator::SetTensorDescInfo(ge::OpDescPtr &op_desc_ptr) const {
  FE_CHECK_NOTNULL(op_desc_ptr);
  auto input_tensor_0 = op_desc_ptr->MutableInputDesc(0);
  FE_CHECK_NOTNULL(input_tensor_0);

  input_tensor_0->SetOriginFormat(trans_info_ptr_->src_out_original_format);
  input_tensor_0->SetOriginShape(trans_info_ptr_->src_out_original_shape);

  auto output_tensor_0 = op_desc_ptr->MutableOutputDesc(0);
  FE_CHECK_NOTNULL(output_tensor_0);

  output_tensor_0->SetOriginFormat(trans_info_ptr_->src_out_original_format);
  output_tensor_0->SetOriginShape(trans_info_ptr_->src_out_original_shape);

  return SUCCESS;
}

Status TransNodeUnsqueezeV2Generator::AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::GeShape &shape,
                                                   const ge::Format &primary_format, const int32_t &sub_format,
                                                   const ge::DataType &dtype) {
  ge::OpDescPtr op_desc_ptr = CreateBasicOpDescForTransNode(UNSQUEEZE_V2);
  FE_CHECK_NOTNULL(op_desc_ptr);

  FE_LOGD("Create [%s] node between [%s] and [%s] success!", UNSQUEEZE_V2.c_str(),
          trans_info_ptr_->src_op_desc->GetName().c_str(), trans_info_ptr_->dst_op_desc->GetName().c_str());

  auto input_format = static_cast<ge::Format>(
      ge::GetFormatFromSub(trans_info_ptr_->src_out_primary_format, trans_info_ptr_->src_out_sub_format));
  if (op_desc_ptr->AddInputDesc(UNSQUEEZE_V2_INPUT_NAME, ge::GeTensorDesc(trans_info_ptr_->src_out_shape, input_format,
                                                                          trans_info_ptr_->src_out_data_type)) !=
      SUCCESS) {
    FE_LOGD("CreateReshapeOp: op [UNSQUEEZE_V2]: add input desc fail.");
    return FAILED;
  }

  auto output_format = static_cast<ge::Format>(ge::GetFormatFromSub(primary_format, sub_format));
  if (op_desc_ptr->AddOutputDesc(UNSQUEEZE_V2_OUTPUT_NAME, ge::GeTensorDesc(shape, output_format, dtype)) != SUCCESS) {
    FE_LOGD("CreateReshapeOp: op [UNSQUEEZE_V2]: add output desc fail.");
    return FAILED;
  }

  /* The output shape of unsqueeze depends on its attr value which name is "axes". */
  if (SetAttr(op_desc_ptr) != SUCCESS) {
    FE_LOGD("CreateReshapeOp: op [UNSQUEEZE_V2]: set attr axes failed.");
    return FAILED;
  }

  (void)SetTensorDescInfo(op_desc_ptr);

  // insert new op need add attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES
  // for data dump
  std::vector<std::string> original_names;
  if (!ge::AttrUtils::SetListStr(op_desc_ptr, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_names)) {
    REPORT_FE_ERROR("[GraphOpt][Trans][Unsqueeze] Set op[%s] attr ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES failed.",
                    op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  SetTensorRealDimCountAndNewShape(op_desc_ptr, {trans_info_ptr_->src_out_shape}, shape);
  SetNewShapeRange(op_desc_ptr, trans_info_ptr_->src_out_range, trans_info_ptr_->dst_in_range);
  if (AddEdgesAndFreshTransInfo(fused_graph, op_desc_ptr) != SUCCESS) {
    FE_LOGD("Add edge failed!");
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace fe