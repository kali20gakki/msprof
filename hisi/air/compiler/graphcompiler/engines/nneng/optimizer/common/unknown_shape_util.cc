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

#include "common/unknown_shape_util.h"
#include "common/aicore_util_attr_define.h"
#include "common/fe_inner_attr_define.h"
#include "graph/ge_context.h"

namespace fe {
std::string ShapeRangeToStr(const std::vector<std::pair<int64_t, int64_t>> &shape_range) {
  std::string s;
  s += "[";
  for (size_t i = 0; i < shape_range.size(); i++) {
    if (i != shape_range.size() - 1) {
      s = s + "[" + std::to_string(shape_range[i].first) + "," + std::to_string(shape_range[i].second) + "], ";
    } else {
      s = s + "[" + std::to_string(shape_range[i].first) + "," + std::to_string(shape_range[i].second) + "]";
    }
  }
  s += "]";
  return s;
}

std::vector<std::pair<int64_t, int64_t>> GetShapeRange(const ge::GeTensorDesc &tensor_desc) {
  std::vector<std::pair<int64_t, int64_t>> shape_range;
  auto status = tensor_desc.GetShapeRange(shape_range);
  if (status != ge::GRAPH_SUCCESS) {
    FE_LOGW("Shape range is not legal.");
    shape_range.clear();
    return shape_range;
  }
  return shape_range;
}

std::vector<std::pair<int64_t, int64_t>> GetValueRange(const ge::GeTensorDesc &tensor_desc) {
  std::vector<std::pair<int64_t, int64_t>> value_range;
  if (tensor_desc.GetValueRange(value_range) != ge::GRAPH_SUCCESS) {
    FE_LOGW("[SubGraphOpt][SetTbeTensor][GetValueRange] Get value range fail from ge.");
    value_range.clear();
    return value_range;
  }
  return value_range;
}

std::vector<std::pair<int64_t, int64_t>> GetAlignShapeRange(
    const std::vector<std::pair<int64_t, int64_t>> &ori_shape_range, const ge::GeShape &shape) {
  if (ori_shape_range.empty()) {
    return ori_shape_range;
  }
  if (ori_shape_range.size() == shape.GetDimNum()) {
    FE_LOGD("Size of range : %zu, is equal to size of shape %zu.", ori_shape_range.size(), shape.GetDimNum());
    return ori_shape_range;
  }
  std::vector<std::pair<int64_t, int64_t>> shape_range(shape.GetDimNum());
  size_t range_index = 0;
  for (size_t i = 0; i < shape.GetDimNum(); i++) {
    int64_t dim = shape.GetDim(i);
    if (dim < 0) {  // unknown shape
      if (ori_shape_range.size() - 1 < range_index) {
        FE_LOGW("Size of shape : %zu, is less than %zu.", ori_shape_range.size(), range_index);
        return ori_shape_range;
      }
      shape_range[i] =
          std::pair<int64_t, int64_t>(ori_shape_range[range_index].first, ori_shape_range[range_index].second);
      range_index++;
    } else if (dim == 0) {
      shape_range[i] = std::pair<int64_t, int64_t>(1, 1);
    } else {
      shape_range[i] = std::pair<int64_t, int64_t>(dim, dim);
    }
  }
  return shape_range;
}

Status SetShapeRange(const ge::OpDesc &op_desc, RangeAndFormat &range_and_format, ge::GeTensorDesc &tensor_desc) {
  if (range_and_format.old_format == range_and_format.new_format) {
    return SUCCESS;
  }
  if (IsFeSupportedDynamicOp(op_desc, true)) {
    FE_LOGD(
        "Op[name:%s,type:%s] is unknown shape op, now set shape range."
        " old format is %u, new format is %u",
        op_desc.GetName().c_str(), op_desc.GetType().c_str(), range_and_format.old_format, range_and_format.new_format);

    Status ret = RangeTransferAccordingToFormat::GetRangeAccordingToFormat(range_and_format);

    if (ret != SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][FmtPropagate][SetShpRange] Failed to get shape range. old format is %s, new format is %s",
          ge::TypeUtils::FormatToSerialString(range_and_format.old_format).c_str(),
          ge::TypeUtils::FormatToSerialString(range_and_format.new_format).c_str());
      return FAILED;
    }

    std::vector<std::pair<int64_t, int64_t>> new_shape_range = range_and_format.new_range;
    if (tensor_desc.SetShapeRange(new_shape_range) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOptJdgInst][FmtPropagate][SetShpRange] Set shape range of op[name:%s,type:%s] failed.",
                      op_desc.GetName().c_str(), op_desc.GetType().c_str());
      return FAILED;
    }
    FE_LOGD(
        "Set shape range of op[name:%s,type:%s] success. old format is %u, "
        "new format is %u. old range is %s, new range is %s.",
        op_desc.GetName().c_str(), op_desc.GetType().c_str(), range_and_format.old_format, range_and_format.new_format,
        ShapeRangeToStr(range_and_format.old_range).c_str(), ShapeRangeToStr(new_shape_range).c_str());
  }
  return SUCCESS;
}

bool IsFuzzBuild() {
  std::string build_mode;
  if (ge::GetContext().GetOption("ge.shape_generalized_build_mode", build_mode) != ge::GRAPH_SUCCESS) {
    FE_LOGI("[SubGraphOpt][Compile]Get fuzz_build flag from ge failed.");
    return false;
  }

  FE_LOGD("[SubGraphOpt][Compile]Get fuzz_build flag from ge, build mode is %s.", build_mode.c_str());
  if (build_mode == SHAPE_GENERALIZED) {
    return true;
  }

  return false;
}

bool IsFuzzBuildOp(const ge::OpDesc &op_desc) {
  bool support_dyn_shape = false;
  if (IsFuzzBuild() &&
      ge::AttrUtils::GetBool(op_desc, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, support_dyn_shape) && support_dyn_shape) {
    return true;
  } else {
    return false;
  }
}

bool IsFeSupportedDynamicOp(const ge::OpDesc &op_desc, bool is_use_attr_check) {
  return IsUnKnownShapeOp(op_desc, is_use_attr_check);
}

bool IsUnKnownShapeOp(const ge::OpDesc &op_desc, const bool is_use_attr_check) {
  const ge::OpDesc *temp_op_desc = &op_desc;
  ge::OpDesc *no_const_op_desc = const_cast<ge::OpDesc *>(temp_op_desc);
  bool unknow_shape_status = false;
  if (is_use_attr_check) {
    if (ge::AttrUtils::GetBool(op_desc, ATTR_NAME_UNKNOWN_SHAPE, unknow_shape_status)) {
      return unknow_shape_status;
    }
  }

  if (op_desc.GetAllInputsSize() != 0 || op_desc.GetOutputsSize() != 0) {
    unknow_shape_status = IsUnKnownShapeTensor(op_desc);
  }

  (void)ge::AttrUtils::SetBool(*no_const_op_desc, fe::ATTR_NAME_UNKNOWN_SHAPE, unknow_shape_status);
  FE_LOGD("Op[%s, %s] Set attr unknown_shape [%d].", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
          unknow_shape_status);
  return unknow_shape_status;
}

bool IsUnKnownShapeTensor(const ge::OpDesc &op_desc) {
  for (auto &tenosr_desc_ptr : op_desc.GetAllInputsDescPtr()) {
    if (tenosr_desc_ptr == nullptr) {
      continue;
    }
    if (tenosr_desc_ptr->MutableShape().IsUnknownShape()) {
      return true;
    }
  }

  for (auto &tenosr_desc_ptr : op_desc.GetAllOutputsDescPtr()) {
    if (tenosr_desc_ptr == nullptr) {
      continue;
    }
    if (tenosr_desc_ptr->MutableShape().IsUnknownShape()) {
      return true;
    }
  }

  return false;
}

bool IsContainUnknownDimNum(const ge::OpDesc &op_desc) {
  for (auto &ptr : op_desc.GetAllInputsDescPtr()) {
    if (IsShapeContainUnknownDimNum(ptr->GetShape())) {
      FE_LOGD("Op[name:%s,type:%s] has input tensor whose shape contains -2.", op_desc.GetName().c_str(),
              op_desc.GetType().c_str());
      return true;
    }
  }

  for (auto &ptr : op_desc.GetAllOutputsDescPtr()) {
    if (IsShapeContainUnknownDimNum(ptr->GetShape())) {
      FE_LOGD("Op[name:%s,type:%s] has output tensor whose shape contains -2.", op_desc.GetName().c_str(),
              op_desc.GetType().c_str());
      return true;
    }
  }

  return false;
}

bool IsShapeContainUnknownDimNum(const ge::GeShape &shape) {
  std::vector<int64_t> dim_vec = shape.GetDims();
  if (dim_vec.empty()) {
    return false;
  }
  if (std::any_of(dim_vec.begin(), dim_vec.end(), [](int64_t dim) { return dim == SHAPE_UNKNOWN_DIM_NUM; })) {
    FE_LOGI("Tensor shape contains -2.");
    return true;
  }
  return false;
}

bool IsUnknownShapeValue(const int64_t &value) {
  if (value == UNKNOWN_SHAPE_VALUE || value == SHAPE_UNKNOWN_DIM_NUM) {
    return true;
  }
  return false;
}
}  // namespace fe
