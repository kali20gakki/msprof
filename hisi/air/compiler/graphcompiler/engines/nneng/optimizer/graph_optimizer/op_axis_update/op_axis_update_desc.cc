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

#include "graph_optimizer/op_axis_update/op_axis_update_desc.h"
#include "common/aicore_util_attr_define.h"
#include "common/format/axis_name_util.h"
#include "common/op_info_common.h"
#include "ops_store/ops_kernel_manager.h"

namespace fe {
OpAxisUpdateDesc::OpAxisUpdateDesc(const std::string &engine_name) : engine_name_(engine_name) {}

OpAxisUpdateDesc::~OpAxisUpdateDesc() {}

Status OpAxisUpdateDesc::UpdateAxis(ge::ComputeGraph &graph) {
  FE_TIMECOST_START(UpdateAxis);
  for (auto &node : graph.GetAllNodes()) {
    FE_CHECK_NOTNULL(node);
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc_ptr);

    OpKernelInfoPtr op_kernel =
        OpsKernelManager::Instance(engine_name_).GetHighPrioOpKernelInfo(op_desc_ptr->GetType());
    if (op_kernel == nullptr) {
      continue;
    }
    OpPattern op_pattern = op_kernel->GetOpPattern();
    bool broadcast_need_reshape = (op_pattern == OP_PATTERN_BROADCAST && IsNeedReshape(op_desc_ptr));
    if (!(op_pattern == OP_PATTERN_REDUCE || broadcast_need_reshape)) {
      FE_LOGD("Op [Name:%s, Type:%s] is not reduce op or no need to reshape.", op_desc_ptr->GetName().c_str(),
              op_desc_ptr->GetType().c_str());
      continue;
    }

    int64_t op_imply_type = -1;
    FE_LOGW_IF(!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, op_imply_type),
               "Op[name=%s,type=%s]: get imply type not success.", op_desc_ptr->GetName().c_str(),
               op_desc_ptr->GetType().c_str());

    // get all output desc info and reshape fractal_z
    // we do not need to update axis once, so its update_axis_flag is false
    ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_out_put_desc_ptr = op_desc_ptr->GetAllOutputsDescPtr();
    if (ReshapeFz3DAndUpdateAxis(all_out_put_desc_ptr, op_imply_type, false, *(op_desc_ptr.get())) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][AxisUpd] Set reduce op [%s] input new shape failed!",
                      op_desc_ptr->GetName().c_str());
      return FAILED;
    }

    // get all input desc info and reshape fractal_z and update axis value
    // update axis value once, so its update_axis_flag is true
    bool is_update_axis = (op_pattern == OP_PATTERN_REDUCE) ? true : false;
    ge::OpDesc::Vistor<ge::GeTensorDescPtr> all_input_desc_ptr = op_desc_ptr->GetAllInputsDescPtr();
    if (ReshapeFz3DAndUpdateAxis(all_input_desc_ptr, op_imply_type, is_update_axis, *(op_desc_ptr.get())) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][AxisUpd] Set reduce op [%s] input new shape failed!",
                      op_desc_ptr->GetName().c_str());
      return FAILED;
    }
  }
  FE_TIMECOST_END(UpdateAxis, "UpdateAxis during FEGraphOptimizer::OptimizeOriginalGraph");
  return SUCCESS;
}

Status OpAxisUpdateDesc::SetAxisAttributeValue(ge::OpDesc &op_desc, ge::Format &origin_format,
                                               ge::Format &current_format, ge::GeShape &origin_shape) {
  FE_LOGD("Reduce op [%s] origin format is [%d], current format is [%d],", op_desc.GetName().c_str(), origin_format,
          current_format);

  // 1. get the new_axis_index_vec
  Status res;
  std::vector<int64_t> new_axis_index_vec;
  if (origin_format == current_format) {
    FE_LOGD("Reduce op [%s] origin format equals current format, does not need to change axis.",
            op_desc.GetName().c_str());
    return SUCCESS;
  }

  // NZ don't care the origin format
  if (current_format == ge::FORMAT_FRACTAL_NZ) {
    res = GetNewAxisForNz(op_desc, origin_shape, new_axis_index_vec);
  } else {
    // 5HD,6HD,6D,FZ
    if (origin_format == ge::FORMAT_ND) {
      FE_LOGD("Reduce op [%s] origin format equals current format, does not need to change axis.",
              op_desc.GetName().c_str());
      return SUCCESS;
    }
    res = AxisNameUtil::GetNewAxisAttributeValue(op_desc, origin_format, current_format, origin_shape,
                                                 new_axis_index_vec);
  }
  if (res != SUCCESS) {
    FE_LOGW("Get reduce op [%s] new axis info failed!", op_desc.GetName().c_str());
    return FAILED;
  }

  // 2. set new axis info
  if (!ge::AttrUtils::SetListInt(op_desc, AXES_ATTR_NAME, new_axis_index_vec)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][AxisUpd] Set reduce op [%s] axis failed!", op_desc.GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status OpAxisUpdateDesc::GetNewAxisForNz(const ge::OpDesc &op_desc, const ge::GeShape &origin_shape,
                                         vector<int64_t> &axis_index_vec) {
  vector<int64_t> origin_axis_index_vec;
  Status res = AxisUtil::GetOriginAxisAttribute(op_desc, origin_shape, origin_axis_index_vec);
  if (res != SUCCESS) {
    return res;
  }

  int64_t dim_num = static_cast<int64_t>(origin_shape.GetDimNum());
  for (auto &axis_index : origin_axis_index_vec) {
    if (axis_index == dim_num - 1) {
      axis_index_vec.emplace_back(axis_index - 1);
      axis_index_vec.emplace_back(axis_index + 2);
    } else if (axis_index == dim_num - 2) {
      axis_index_vec.emplace_back(axis_index + 1);
      axis_index_vec.emplace_back(axis_index + 2);
    } else {
      axis_index_vec.emplace_back(axis_index);
    }
  }
  return SUCCESS;
}

Status OpAxisUpdateDesc::ReshapeFz3DAndUpdateAxis(ge::OpDesc::Vistor<ge::GeTensorDescPtr> &input_or_output_tensor_desc,
                                                  const int64_t &op_imply_type, const bool &update_axis_flag,
                                                  ge::OpDesc &op_desc) {
  ge::Format primary_format = ge::FORMAT_ND;  // current format of the op
  ge::Format origin_format = ge::FORMAT_ND;  // original format of the op
  ge::GeShape origin_shape;                  // original shape of the op
  ge::DataType current_data_type;            // current data type of the op

  // 1.reset shape of fractal_z to 6d
  for (auto input_or_output_desc : input_or_output_tensor_desc) {
    int32_t sub_format = 0;
    primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_or_output_desc->GetFormat()));
    sub_format = static_cast<ge::Format>(ge::GetSubFormat(input_or_output_desc->GetFormat()));
    origin_format = input_or_output_desc->GetOriginFormat();
    origin_shape = input_or_output_desc->GetOriginShape();
    current_data_type = input_or_output_desc->GetDataType();

    // only when format is FRACTAL_Z or FRACTAL_Z_3D, we reset its shape.
    if (std::find(FE_GROUP_RELA_FORMAT_VECTOR.begin(), FE_GROUP_RELA_FORMAT_VECTOR.end(), primary_format) ==
        FE_GROUP_RELA_FORMAT_VECTOR.end()) {
      continue;
    }
    ge::GeShape new_shape =
        GetFractalZNewShape(origin_shape, origin_format, primary_format, sub_format, op_imply_type, current_data_type);
    input_or_output_desc->SetShape(new_shape);
  }
  // 2.update axis info according to current and original format.
  if (update_axis_flag) {
    return SetAxisAttributeValue(op_desc, origin_format, primary_format, origin_shape);
  }
  return SUCCESS;
}

ge::GeShape OpAxisUpdateDesc::GetFractalZNewShape(const ge::GeShape &origin_shape, const ge::Format &origin_format,
                                                  const ge::Format primary_format, const int32_t sub_format,
                                                  const int64_t &op_imply_type,
                                                  const ge::DataType &current_data_type) const {
  // 1. get the axis value by origin format
  std::vector<int64_t> axis_value(AXIS_BOTTOM, 1);
  std::vector<int64_t> nd_value;
  uint32_t c0 = GetC0(current_data_type);
  Status status = AxisUtil::GetAxisValueByOriginFormat(origin_format, origin_shape.GetDims(), c0, axis_value, nd_value);
  if (status != SUCCESS) {
    FE_LOGW("Failed to get the original shape: origin format is %s, origin shape is %s.",
            ge::TypeUtils::FormatToSerialString(origin_format).c_str(), GetShapeDims(origin_shape).c_str());
    return origin_shape;
  }

  // 2. get the FZ shape
  ge::GeShape new_fz_shape;
  ShapeAndFormat shape_and_format_info = {origin_shape,      new_fz_shape,  origin_format, primary_format,
                                          current_data_type, op_imply_type, sub_format};
  status = ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(shape_and_format_info);
  if (status != SUCCESS) {
    FE_LOGW(
        "Fail to GetShapeAccordingToFormat: origin format is %s, origin shape is %s, new primary_format is %s, "
        "new sub_format is %d.",
        ge::TypeUtils::FormatToSerialString(origin_format).c_str(), GetShapeDims(origin_shape).c_str(),
        ge::TypeUtils::FormatToSerialString(primary_format).c_str(), sub_format);
    return origin_shape;
  }

  if (CheckInt64MulOverflow(axis_value[AXIS_W], axis_value[AXIS_H]) != SUCCESS) {
    FE_LOGW("Int64 %ld and %ld addition can result in overflow!", axis_value[AXIS_W], axis_value[AXIS_H]);
    return origin_shape;
  }

  // 3. get the H*W or D*H*W
  int64_t axis_dhw = axis_value[AXIS_W] * axis_value[AXIS_H];
  if (primary_format == ge::FORMAT_FRACTAL_Z_3D) {
    if (CheckInt64MulOverflow(axis_dhw, axis_value[AXIS_D]) != SUCCESS) {
      FE_LOGW("Int64 %ld and %ld addition can result in overflow!", axis_dhw, axis_value[AXIS_D]);
      return origin_shape;
    }
    axis_dhw *= axis_value[AXIS_D];
  }

  // 4. get the G*C1*D*H*W or G*C1*D*H*W
  if (new_fz_shape.GetDims().size() < 4) {
    FE_LOGW("The new shape size of format %s is not 4.", ge::TypeUtils::FormatToSerialString(primary_format).c_str());
    return origin_shape;
  }
  if (axis_dhw == 0) {
    FE_LOGW("The d*h*w is 0: new_fz_shape is %s.", GetShapeDims(new_fz_shape).c_str());
    return origin_shape;
  }

  vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(new_fz_shape.GetDim(0) / axis_dhw);  // G*C1
  if (primary_format == ge::FORMAT_FRACTAL_Z_3D) {
    new_dim_vec.push_back(axis_value[AXIS_D]);
  }
  new_dim_vec.push_back(axis_value[AXIS_H]);
  new_dim_vec.push_back(axis_value[AXIS_W]);
  new_dim_vec.push_back(new_fz_shape.GetDim(1));
  new_dim_vec.push_back(new_fz_shape.GetDim(2));
  new_dim_vec.push_back(new_fz_shape.GetDim(3));
  ge::GeShape result = ge::GeShape(new_dim_vec);
  FE_LOGD("New shape of format %s is %s , old shape is %s.",
          ge::TypeUtils::FormatToSerialString(primary_format).c_str(),
          GetShapeDims(result).c_str(),
          GetShapeDims(origin_shape).c_str());
  return result;
}
}  // namespace fe
