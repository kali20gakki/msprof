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

#include "graph_optimizer/op_judge/format_and_dtype/update_desc/op_format_dtype_update_desc_base.h"
#include <common/format/axis_name_util.h>
#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/unknown_shape_util.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_attr_value.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "graph_optimizer/range_format_transfer/transfer_range_according_to_format.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {
OpFormatDtypeUpdateDescBase::OpFormatDtypeUpdateDescBase(FormatDtypeQuerierPtr format_dtype_querier_ptr)
    : format_dtype_querier_ptr_(format_dtype_querier_ptr) {}
OpFormatDtypeUpdateDescBase::~OpFormatDtypeUpdateDescBase() {}

Status SetDstTypeForOpCast(ge::OpDescPtr op_desc_ptr, ge::DataType& op_kernel_data_type, const bool& is_input) {
  string attr_name = is_input ? CAST_ATTR_SRC_TYPE : CAST_ATTR_DST_TYPE;
  if (IsDtypeSensitiveOp(op_desc_ptr->GetType()) &&
      !ge::AttrUtils::SetInt(op_desc_ptr, attr_name, static_cast<int64_t>(op_kernel_data_type))) {
    return FAILED;
  } else {
    return SUCCESS;
  }
}

Status OpFormatDtypeUpdateDescBase::GetAndCheckSupportedDtype(const UpdateInfo& update_info,
                                                              ge::DataType& op_kernel_data_type) {
  ge::OpDescPtr op_desc_ptr = update_info.node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  string op_type = op_desc_ptr->GetType();
  string op_name = op_desc_ptr->GetName();
  std::vector<ge::DataType> op_kernel_data_type_vec;
  FE_CHECK_NOTNULL(format_dtype_querier_ptr_);
  if (format_dtype_querier_ptr_->GetSupportDataTypes(update_info.op_kernel_info_ptr,
                                                     update_info.input_or_output_info_ptr, *(op_desc_ptr.get()),
                                                     op_kernel_data_type_vec) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][GetChkSptDtype] Fail to get the support data_types for %s.",
                    op_name.c_str());
    return FAILED;
  }

  uint32_t op_kernel_data_type_vec_size = op_kernel_data_type_vec.size();
  if (op_kernel_data_type_vec.empty() || update_info.matched_index >= op_kernel_data_type_vec_size) {
    REPORT_FE_ERROR(
        "[GraphOptJdgInst][UpdFmtAndDtype][GetChkSptDtype] "
        "Op[name=%s,type=%s]: update the op_input_or_output_desc %u, the matched "
        "index %u is larger than or equals to the size of "
        "opKernelDataTypeVec %u.",
        op_name.c_str(), op_type.c_str(), update_info.index, update_info.matched_index, op_kernel_data_type_vec_size);
    return FAILED;
  }
  op_kernel_data_type = op_kernel_data_type_vec[update_info.matched_index];
  return SUCCESS;
}

Status OpFormatDtypeUpdateDescBase::UpdateDtype(const UpdateInfo& update_info) {
  ge::OpDescPtr op_desc_ptr = update_info.node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  string op_type = op_desc_ptr->GetType();
  string op_name = op_desc_ptr->GetName();

  // 1. check the matched_index and op_kernel_data_type_vec_size
  ge::DataType op_kernel_data_type;
  if (GetAndCheckSupportedDtype(update_info, op_kernel_data_type) != SUCCESS) {
    return FAILED;
  }
  // 2. if the data type in op kernel is not equal to the original data type,
  // then update op_input_or_output_desc
  ge::DataType op_input_or_output_desc_origin_dtype = update_info.op_input_or_output_desc.GetDataType();
  if (op_kernel_data_type != op_input_or_output_desc_origin_dtype) {
    update_info.op_input_or_output_desc.SetDataType(op_kernel_data_type);
    /* If the dtype of cast is changed, we need to set attr */
    if (SetDstTypeForOpCast(op_desc_ptr, op_kernel_data_type, update_info.input_or_output_info_ptr->GetIsInput()) !=
        SUCCESS) {
      FE_LOGW("Set src or dst type for op %s", op_desc_ptr->GetName().c_str());
      return SUCCESS;
    }
    FE_LOGD(
        "Op[name=%s,type=%s]: update the %s tensor desc %u,"
        "new data type is [%s], origin data type is [%s].",
        op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(update_info.is_input), update_info.index,
        DTypeToStr(op_kernel_data_type).c_str(), DTypeToStr(op_input_or_output_desc_origin_dtype).c_str());
  }
  return SUCCESS;
}

ge::GeShape OpFormatDtypeUpdateDescBase::GetNewShape(const ge::OpDescPtr& op_desc_ptr, const ge::GeShape& old_shape,
                                                     const ge::Format& old_format, const ge::Format& new_format,
                                                     const int64_t& op_imply_type,
                                                     const ge::DataType& current_data_type,
                                                     const int64_t& group) const {
  auto old_dim_vec = old_shape.GetDims();
  uint32_t old_dim_vec_size = old_dim_vec.size();
  /* other format to Nz or ND to (FRACTAL_Z / FRACTAL_ZN_RNN / ND_RNN_BIAS),
   * the size of original shape does not need to be 4 */
  if (old_dim_vec_size < NCHW_DIMENSION_NUM && new_format != ge::FORMAT_FRACTAL_NZ &&
      ((new_format != ge::FORMAT_FRACTAL_Z && new_format != ge::FORMAT_FRACTAL_ZN_RNN &&
        new_format != ge::FORMAT_ND_RNN_BIAS) || old_format != ge::FORMAT_ND)) {
    FE_LOGW("Get shape not success: old format is %s, new format is %s, old dimension is %u",
        ge::TypeUtils::FormatToSerialString(old_format).c_str(),
        ge::TypeUtils::FormatToSerialString(new_format).c_str(), old_dim_vec_size);
    return old_shape;
  }
  /* Assemble shape and format transmission information */
  ge::GeShape new_shape;
  int64_t hidden_size = 1;
  int64_t input_size = 1;
  int64_t state_size = -1;
  (void)ge::AttrUtils::GetInt(op_desc_ptr, "hidden_size", hidden_size);
  (void)ge::AttrUtils::GetInt(op_desc_ptr, "input_size", input_size);
  (void)ge::AttrUtils::GetInt(op_desc_ptr, "state_size", state_size);
  CalcShapeExtraAttr extra_attr = {hidden_size, input_size, state_size};
  ShapeAndFormat shape_and_format_info = {old_shape, new_shape, old_format, new_format,
                                          current_data_type, op_imply_type, group, extra_attr};

  Status ret = ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(shape_and_format_info);
  if (ret != SUCCESS) {
    FE_LOGW("Old format is %s, new format is %s, old dimension is %u and opImplyType is %ld.",
        ge::TypeUtils::FormatToSerialString(old_format).c_str(),
        ge::TypeUtils::FormatToSerialString(new_format).c_str(), old_dim_vec_size, op_imply_type);
    return old_shape;
  }
  return new_shape;
}

Status OpFormatDtypeUpdateDescBase::GetFormatAndDtypeVec(const OpKernelInfoPtr& op_kernel_info_ptr,
                                                         const InputOrOutputInfoPtr& input_or_output_info_ptr,
                                                         const ge::OpDescPtr& op_desc_ptr,
                                                         std::vector<ge::Format>& op_kernel_format_vec,
                                                         std::vector<ge::DataType>& op_kernel_data_type_vec) {
  if (format_dtype_querier_ptr_->GetSupportFormats(op_kernel_info_ptr, input_or_output_info_ptr, *(op_desc_ptr.get()),
                                                   op_kernel_format_vec) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][GetFmtDtypeVec] Fail to get the support formats, \
                    return FAILED.");
    return FAILED;
  }

  if (format_dtype_querier_ptr_->GetSupportDataTypes(op_kernel_info_ptr, input_or_output_info_ptr, *(op_desc_ptr.get()),
                                                     op_kernel_data_type_vec) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][GetFmtDtypeVec] Fail to get the support data_types, \
                    return FAILED.");
    return FAILED;
  }
  return SUCCESS;
}

Status CheckMatchedIndexValid(const std::vector<ge::Format>& op_kernel_format_vec,
                              const std::vector<ge::DataType>& op_kernel_data_type_vec, const uint32_t& matched_index,
                              const ge::OpDescPtr& op_desc_ptr, const uint32_t& index, const bool& is_input) {
  FE_CHECK_NOTNULL(op_desc_ptr);
  uint32_t op_kernel_format_vec_size = op_kernel_format_vec.size();
  uint32_t op_kernel_data_type_vec_size = op_kernel_data_type_vec.size();
  if (op_kernel_format_vec.empty() || matched_index >= op_kernel_format_vec_size) {
    REPORT_FE_ERROR("[GraphOpt][FmtJdg][ChkMtcIdxValid] Op[%s,type=%s]: update the %s [%u], "
                    "the matched index [%u] is larger than or equals to the size of op_kernel_format_vec [%u].",
                    op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), IS_INPUT_TO_STRING(is_input), index,
                    matched_index, op_kernel_format_vec_size);
    return FAILED;
  }

  if (op_kernel_data_type_vec.empty() || matched_index >= op_kernel_data_type_vec_size) {
    REPORT_FE_ERROR("[GraphOpt][FmtJdg][ChkMtcIdxValid] Op[%s,type=%s]: update the %s [%u], the matched "
                    "index [%u] is larger than or equals to the size of opKernelDataTypeVecSize [%u].",
                    op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), IS_INPUT_TO_STRING(is_input), index,
                    matched_index, op_kernel_data_type_vec_size);
    return FAILED;
  }
  return SUCCESS;
}

void UpdateNewShapeAndFormat(const UpdateInfo& update_info, ge::Format op_kernel_format, const int64_t& group,
                             const ge::GeShape& original_shape, const ge::GeShape& new_shape,
                             const std::string& op_type, const std::string& op_name) {
  auto old_format = update_info.op_input_or_output_desc.GetFormat();
  auto new_format = op_kernel_format;
  if (std::find(FE_GROUP_RELA_FORMAT_VECTOR.begin(), FE_GROUP_RELA_FORMAT_VECTOR.end(), op_kernel_format) !=
          FE_GROUP_RELA_FORMAT_VECTOR.end() &&
      group > GROUPS_DEFAULT_VALUE) {
    FE_LOGD("Op[name=%s,type=%s]: the %s [%u], the group is more than 1, set sub_format to be %ld.", op_name.c_str(),
            op_type.c_str(), IS_INPUT_TO_STRING(update_info.is_input), update_info.index, group);
    new_format = static_cast<ge::Format>(ge::GetFormatFromSub(op_kernel_format, group));
  }

  FE_LOGD(
      "Op[name=%s,type=%s]: update the %s [%u], new format is [%s], group is [%ld], old format is [%s], origin format "
      "is [%s].",
      op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(update_info.is_input), update_info.index,
      FormatToStr(new_format).c_str(), group, FormatToStr(old_format).c_str(),
      FormatToStr(update_info.op_input_or_output_desc.GetOriginFormat()).c_str());

  update_info.op_input_or_output_desc.SetFormat(new_format);

  FE_LOGD("Op[name=%s,type=%s]: update the %s [%u], new shape is [%s], origin shape is [%s].",
          op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(update_info.is_input), update_info.index,
          GetShapeDims(new_shape).c_str(),
          GetShapeDims(original_shape).c_str());
  update_info.op_input_or_output_desc.SetShape(new_shape);
}

bool IsScalar(const ge::GeShape &original_shape, ge::Format original_format) {
  if (original_shape.IsScalar()) {
    return true;
  }

  return original_shape.GetDimNum() == 1 && original_shape.GetDims().at(0) == 1 && original_format == ge::FORMAT_ND;

}

Status OpFormatDtypeUpdateDescBase::PadNDToOtherFormatAndGetReshapeType(const UpdateInfo &update_info,
                                                                        const ge::Format &op_kernel_format,
                                                                        ge::Format &ori_format,
                                                                        std::string &reshape_type) {
  FE_LOGD("before PadNDToOtherFormatAndGetReshapeType node name: %s, op_kernel_format: %d, orin_format: %d",
          update_info.node_ptr->GetName().c_str(), op_kernel_format, ori_format);
  if (op_kernel_format == ge::FORMAT_NC1HWC0 &&
      update_info.op_input_or_output_desc.GetOriginFormat() == ge::FORMAT_ND &&
      update_info.is_input) {
    ge::NodePtr node = update_info.node_ptr;
    int64_t input_nd_other_format = -1;
    if (ge::AttrUtils::GetInt(node->GetOpDesc(), INPUT_ND_TO_OTHER_FORMAT, input_nd_other_format) &&
        input_nd_other_format != -1) {
      ge::Format other_format = static_cast<ge::Format>(input_nd_other_format);
      ori_format = other_format;
      update_info.op_input_or_output_desc.SetOriginFormat(ori_format);
    } else {
      size_t count_input_is_nd = 0;
      for (size_t i = 0; i < node->GetAllInDataAnchors().size(); i++) {
        auto input_desc_ptr = node->GetOpDesc()->GetInputDescPtr(i);
        if (input_desc_ptr == nullptr) {
          continue;
        }
        ge::Format node_input_ori_format = input_desc_ptr->GetOriginFormat();
        if (node_input_ori_format != ge::FORMAT_ND) {
          ori_format = node_input_ori_format;
          update_info.op_input_or_output_desc.SetOriginFormat(ori_format);
          (void)ge::AttrUtils::SetInt(update_info.node_ptr->GetOpDesc(), INPUT_ND_TO_OTHER_FORMAT, ori_format);
          break;
        }
        count_input_is_nd++;
      }
      if (count_input_is_nd != 1 && count_input_is_nd == node->GetAllInDataAnchors().size()) {
        REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][PadNDToOtherFmt] [Node: %s] All input original format is \
                        ge::FORMAT_ND", node->GetName().c_str());
        return FAILED;
      }
    }
  }
  FE_LOGD("after PadNDToOtherFormatAndGetReshapeType node name: %s, op_kernel_format: %d, orin_format: %d",
          update_info.node_ptr->GetName().c_str(), op_kernel_format, ori_format);
  reshape_type = update_info.input_or_output_info_ptr->GetReshapeType();
  if (update_info.input_or_output_info_ptr->GetReshapeType().empty() && update_info.op_kernel_info_ptr != nullptr &&
      update_info.op_kernel_info_ptr->GetOpPattern() == OP_PATTERN_REDUCE) {
    vector<int64_t> axis_values;
    (void)ge::AttrUtils::GetListInt(update_info.node_ptr->GetOpDesc(), AXES_ATTR_NAME, axis_values);
    reshape_type = AxisNameUtil::GetReshapeType(ori_format, axis_values);
  }
  return SUCCESS;
}

Status OpFormatDtypeUpdateDescBase::UpdateNewShape(const UpdateInfo& update_info, ge::Format op_kernel_format,
    ge::DataType op_kernel_dtype, int64_t group, int64_t op_imply_type_input) {
  auto original_shape = update_info.op_input_or_output_desc.GetShape();
  auto new_shape = update_info.op_input_or_output_desc.GetShape();
  auto ori_format = GetCurOpOriginFormat(update_info.op_input_or_output_desc);

  if (IsScalar(original_shape, ori_format)) {
    FE_LOGD("%s %u of %s is scalar, we do not change its format and shape.",
            IS_INPUT_TO_STRING(update_info.is_input), update_info.index, update_info.node_ptr->GetName().c_str());
    return SUCCESS;
  }
  string op_name = update_info.node_ptr->GetName();
  string op_type = update_info.node_ptr->GetType();
  // 3.3 if the op_kernel_format is ND or ALL, use the ori_format
  if (IsNd(op_kernel_format)) {
    FE_LOGD("the op_kernel_format is ND, get the origin format.");
    op_kernel_format = ori_format;
  } else {
    // 3.4 pad the origin shape and get the new shape
    FE_LOGD("The %s op_kernel_format is [%s] and tensor original format is [%s]",
            IS_INPUT_TO_STRING(update_info.is_input), FormatToStr(op_kernel_format).c_str(),
            FormatToStr(ori_format).c_str());
    if (!IsShapeContainUnknownDimNum(original_shape) && op_kernel_format != ori_format) {
      FE_LOGD("Format of op_kernel is not equal with origin format of input.");
      std::string reshape_type;
      // deal with Format_NC1HWC0, pad ND to other not ND format
      if (PadNDToOtherFormatAndGetReshapeType(update_info, op_kernel_format, ori_format, reshape_type) != SUCCESS) {
        FE_LOGD("PadNDToOtherFormatAndGetReshapeType failed");
        return FAILED;
      }

      std::vector<int64_t> dims = original_shape.GetDims();
      ExpandDimension(dims, op_type, ori_format, op_kernel_format, update_info.index, reshape_type);
      (void)ge::AttrUtils::SetStr(update_info.op_input_or_output_desc, INFER_RESHAPE_TYPE, reshape_type);
      ge::GeShape origin_shape_afer_pad(dims);

      new_shape = GetNewShape(update_info.node_ptr->GetOpDesc(), origin_shape_afer_pad, ori_format, op_kernel_format,
                              op_imply_type_input, op_kernel_dtype, group);
      /* update shape range for unknown op */
      vector<std::pair<int64_t, int64_t>> new_range_shape;
      vector<std::pair<int64_t, int64_t>> ori_shape_range = GetShapeRange(update_info.op_input_or_output_desc);
      vector<std::pair<int64_t, int64_t>> old_shape_range = GetAlignShapeRange(ori_shape_range, origin_shape_afer_pad);
      int64_t hidden_size = 1;
      int64_t input_size = 1;
      int64_t state_size = -1;
      (void)ge::AttrUtils::GetInt(update_info.node_ptr->GetOpDesc(), "hidden_size", hidden_size);
      (void)ge::AttrUtils::GetInt(update_info.node_ptr->GetOpDesc(), "input_size", input_size);
      (void)ge::AttrUtils::GetInt(update_info.node_ptr->GetOpDesc(), "state_size", state_size);
      CalcShapeExtraAttr extra_attr = {hidden_size, input_size, state_size};
      RangeAndFormat shape_and_format_info = {
          origin_shape_afer_pad, old_shape_range, new_range_shape, ori_format,
          op_kernel_format, op_kernel_dtype, op_imply_type_input, extra_attr};
      if (SetShapeRange(*update_info.node_ptr->GetOpDesc(), shape_and_format_info,
                        update_info.op_input_or_output_desc) != SUCCESS) {
        FE_LOGI("Set shape range of op[name:%s,type:%s] failed.", update_info.node_ptr->GetOpDesc()->GetName().c_str(),
                update_info.node_ptr->GetOpDesc()->GetType().c_str());
        return FAILED;
      }
    }
  }

  // 4. update the shape and format
  UpdateNewShapeAndFormat(update_info, op_kernel_format, group, original_shape, new_shape, op_type, op_name);
  return SUCCESS;
}

Status OpFormatDtypeUpdateDescBase::CalcNewShapeAndUpdate(const UpdateInfo& update_info,
                                                          ge::Format op_kernel_format, ge::DataType op_kernel_dtype) {
  /* 3.1 Get the origianl shape and original format */
  ge::Format op_input_or_output_desc_origin_format = GetCurOpOriginFormat(update_info.op_input_or_output_desc);

  string op_name = update_info.node_ptr->GetName();
  string op_type = update_info.node_ptr->GetType();
  /* 3.2 Get the op imply type */
  int64_t op_imply_type_input = -1;
  FE_LOGW_IF(!ge::AttrUtils::GetInt(update_info.node_ptr->GetOpDesc(), FE_IMPLY_TYPE, op_imply_type_input),
             "Op[name=%s,type=%s]: get imply type not success.", op_name.c_str(), op_type.c_str());
  FE_LOGD("Op[name=%s,type=%s]: get the new format and shape for the %s [%u], \
          the op_kernel_format is %s, origin_format is %s.",
      op_name.c_str(), op_type.c_str(), IS_INPUT_TO_STRING(update_info.is_input), update_info.index,
      FormatToStr(op_kernel_format).c_str(), FormatToStr(op_input_or_output_desc_origin_format).c_str());

  // if the input or output tensor has chose format fraz_g,
  // then verify whether the opdesc has attr groups or _fe_group
  // and the group value should be greater than zero.
  int64_t group = GROUPS_DEFAULT_VALUE;
  if (std::find(FE_GROUP_RELA_FORMAT_VECTOR.begin(), FE_GROUP_RELA_FORMAT_VECTOR.end(), op_kernel_format) !=
      FE_GROUP_RELA_FORMAT_VECTOR.end()) {
    if (GetGroupAttributeWithVerify(update_info.node_ptr->GetOpDesc(), group) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CalcNewShpAndUpd] The groups attribute value of node[%s, %s] \
                      is invalid.", update_info.node_ptr->GetOpDesc()->GetName().c_str(),
                      update_info.node_ptr->GetOpDesc()->GetType().c_str());
      return FAILED;
    }
  }

  Status ret = UpdateNewShape(update_info, op_kernel_format, op_kernel_dtype, group, op_imply_type_input);
  if (ret != SUCCESS) {
    return ret;
  }

  return SUCCESS;
}

Status OpFormatDtypeUpdateDescBase::UpdateFormat(const UpdateInfo& update_info) {
  ge::OpDescPtr op_desc_ptr = update_info.node_ptr->GetOpDesc();
  string op_type = op_desc_ptr->GetType();
  string op_name = op_desc_ptr->GetName();
  std::vector<ge::Format> op_kernel_format_vec;
  std::vector<ge::DataType> op_kernel_data_type_vec;
  /* 1. Get all supported format and dtype */
  FE_LOGD("Update format for %s tensor %u of op %s", IS_INPUT_TO_STRING(update_info.is_input), update_info.index,
          update_info.node_ptr->GetName().c_str());
  Status ret = GetFormatAndDtypeVec(update_info.op_kernel_info_ptr, update_info.input_or_output_info_ptr, op_desc_ptr,
                                    op_kernel_format_vec, op_kernel_data_type_vec);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][UpdFmt] Failed to get supported dtype and format for %s %u of \
                    %s", IS_INPUT_TO_STRING(update_info.is_input), update_info.index, op_name.c_str());
    return ret;
  }
  /* 2. Check whether the matched index is valid for this op kernel */
  ret = CheckMatchedIndexValid(op_kernel_format_vec, op_kernel_data_type_vec, update_info.matched_index, op_desc_ptr,
                               update_info.index, update_info.is_input);
  /* Log is enough in func CheckMatchedIndexValid so the following check omits
   * the log. */
  if (ret != SUCCESS) {
    return ret;
  }
  if (op_kernel_format_vec.empty()) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][UpdFmt] opKernelFormatVec is empty");
    return FAILED;
  }
  if (op_kernel_data_type_vec.empty()) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][UpdFmt] opKernelDataTypeVec is empty");
    return FAILED;
  }
  ge::Format op_kernel_format = op_kernel_format_vec[update_info.matched_index];
  ge::DataType op_kernel_dtype = op_kernel_data_type_vec[update_info.matched_index];
  /* 3. update the tensor current format & dtype & shape according to the
   * selected format and dtype */
  ret = CalcNewShapeAndUpdate(update_info, op_kernel_format, op_kernel_dtype);

  return ret;
}
}  // namespace fe
