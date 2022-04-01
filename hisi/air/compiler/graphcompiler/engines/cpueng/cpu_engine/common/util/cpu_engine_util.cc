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
#include "cpu_engine_util.h"

#include <dlfcn.h>
#include <regex.h>
#include <cstdlib>

#include <algorithm>
#include <fstream>

#include "graph/debug/ge_attr_define.h"
#include "util/constant.h"
#include "util/util.h"
#include "engine/base_engine.h"
#include "common/sgt_slice_type.h"

using namespace std;
using namespace ge;

namespace aicpu {
using AttrValueMap = google::protobuf::Map<string, aicpuops::AttrValue>;
ge::Status SetValueTypeString(const ge::OpDescPtr &op_desc_ptr,
                              const std::string &attr_name,
                              aicpuops::AttrValue &attr_value) {
  string string_value;
  CHECK_RES_BOOL(ge::AttrUtils::GetStr(op_desc_ptr, attr_name, string_value),
                 ErrorCode::DATA_TYPE_UNDEFILED,
                 AICPU_REPORT_CALL_ERROR(
                     "Call ge::AttrUtils::GetStr function failed to get"
                     " attr[%s], op[%s].",
                     attr_name.c_str(), op_desc_ptr->GetName().c_str()))

  attr_value.set_s(string_value);
  return ge::SUCCESS;
}

ge::Status SetValueTypeFloat(const ge::OpDescPtr &op_desc_ptr,
                             const std::string &attr_name,
                             aicpuops::AttrValue &attr_value) {
  float float_value = 0.0;
  CHECK_RES_BOOL(ge::AttrUtils::GetFloat(op_desc_ptr, attr_name, float_value),
                 ErrorCode::DATA_TYPE_UNDEFILED,
                 AICPU_REPORT_CALL_ERROR(
                     "Call ge::AttrUtils::GetFloat function failed to get"
                     " attr[%s], op[%s].",
                     attr_name.c_str(), op_desc_ptr->GetName().c_str()))
  attr_value.set_f(float_value);
  return ge::SUCCESS;
}

ge::Status SetValueTypeBool(const ge::OpDescPtr &op_desc_ptr,
                            const std::string &attr_name,
                            aicpuops::AttrValue &attr_value) {
  bool bool_value = false;
  CHECK_RES_BOOL(ge::AttrUtils::GetBool(op_desc_ptr, attr_name, bool_value),
                 ErrorCode::DATA_TYPE_UNDEFILED,
                 AICPU_REPORT_CALL_ERROR(
                     "Call ge::AttrUtils::GetBool function failed to get"
                     " attr[%s], op[%s].",
                     attr_name.c_str(), op_desc_ptr->GetName().c_str()))
  attr_value.set_b(bool_value);
  return ge::SUCCESS;
}

ge::Status SetValueTypeInt(const ge::OpDescPtr &op_desc_ptr,
                           const string &attr_name,
                           aicpuops::AttrValue &attr_value) {
  int64_t int_value = 0;
  CHECK_RES_BOOL(ge::AttrUtils::GetInt(op_desc_ptr, attr_name, int_value),
                 ErrorCode::DATA_TYPE_UNDEFILED,
                 AICPU_REPORT_CALL_ERROR(
                     "Call ge::AttrUtils::GetInt function failed to get"
                     " attr[%s], op[%s].",
                     attr_name.c_str(), op_desc_ptr->GetName().c_str()))
  attr_value.set_i(int_value);
  return ge::SUCCESS;
}

ge::Status SetValueTypeListFloat(const ge::OpDescPtr &op_desc_ptr,
                                 const std::string &attr_name,
                                 aicpuops::AttrValue &attr_value) {
  std::vector<float> float_list;
  CHECK_RES_BOOL(
      ge::AttrUtils::GetListFloat(op_desc_ptr, attr_name, float_list),
      ErrorCode::DATA_TYPE_UNDEFILED,
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetListFloat function failed"
                              " to get attr[%s], op[%s].",
                              attr_name.c_str(),
                              op_desc_ptr->GetName().c_str()))
  for (const float &f : float_list) {
    auto array = attr_value.mutable_array();
    AICPU_CHECK_NOTNULL(array)
    array->add_f(f);
  }
  return ge::SUCCESS;
}

ge::Status SetValueTypeAicpuTensor(const ge::OpDescPtr &op_desc_ptr,
                                   const std::string &attr_name,
                                   aicpuops::AttrValue &attr_value) {
  ge::ConstGeTensorPtr ge_tensor;
  CHECK_RES_BOOL(ge::AttrUtils::GetTensor(op_desc_ptr, attr_name, ge_tensor),
                 ErrorCode::DATA_TYPE_UNDEFILED,
                 AICPU_REPORT_CALL_ERROR(
                     "Call ge::AttrUtils::GetTensor function failed"
                     " to get attr[%s], op[%s].",
                     attr_name.c_str(), op_desc_ptr->GetName().c_str()))
  AICPU_CHECK_NOTNULL(ge_tensor)

  ge::DataType ge_data_type = ge_tensor->GetTensorDesc().GetDataType();
  aicpuops::Tensor *aicpu_tensor = attr_value.mutable_tensor();
  AICPU_CHECK_NOTNULL(aicpu_tensor)
  aicpu_tensor->set_tensor_type(static_cast<ge::DataType>(ge_data_type));

  aicpuops::TensorShape *aicpu_shape = aicpu_tensor->mutable_tensor_shape();
  AICPU_CHECK_NOTNULL(aicpu_shape)
  for (const auto &dim : ge_tensor->GetTensorDesc().GetShape().GetDims()) {
    aicpuops::TensorShape_Dim *aicpu_dims = aicpu_shape->add_dim();
    AICPU_CHECK_NOTNULL(aicpu_dims)
    aicpu_dims->set_size(dim);
  }

  return ge::SUCCESS;
}

ge::Status SetValueTypeAicpuDataType(const ge::OpDescPtr &op_desc_ptr,
                                     const std::string &attr_name,
                                     aicpuops::AttrValue &attr_value) {
  ge::DataType ge_type;
  CHECK_RES_BOOL(ge::AttrUtils::GetDataType(op_desc_ptr, attr_name, ge_type),
                 ErrorCode::DATA_TYPE_UNDEFILED,
                 AICPU_REPORT_CALL_ERROR(
                     "Call ge::AttrUtils::GetDataType function failed"
                     " to get attr[%s], op[%s].",
                     attr_name.c_str(), op_desc_ptr->GetName().c_str()))
  attr_value.set_type(static_cast<ge::DataType>(ge_type));
  return ge::SUCCESS;
}

ge::Status SetValueTypeListAicpuShape(const ge::OpDescPtr &op_desc_ptr,
                                      const std::string &attr_name,
                                      aicpuops::AttrValue &attr_value) {
  std::vector<std::vector<int64_t>> shape_value;
  CHECK_RES_BOOL(
      ge::AttrUtils::GetListListInt(op_desc_ptr, attr_name, shape_value),
      ErrorCode::DATA_TYPE_UNDEFILED,
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetListListInt function"
                              " failed to get attr[%s], op[%s].",
                              attr_name.c_str(),
                              op_desc_ptr->GetName().c_str()))

  auto attr_list = attr_value.mutable_array();
  AICPU_CHECK_NOTNULL(attr_list)
  for (const vector<int64_t> &shape : shape_value) {
    aicpuops::TensorShape *shape_tensor = attr_list->add_shape();
    AICPU_CHECK_NOTNULL(shape_tensor)
    for (const int64_t &dim : shape) {
      AICPU_IF_BOOL_EXEC(
          (dim < 0), AICPUE_LOGW("The dim [%ld] in shape is less than 0.", dim);
          return ErrorCode::DATA_TYPE_UNDEFILED)
      aicpuops::TensorShape_Dim *aicpu_dims = shape_tensor->add_dim();
      AICPU_CHECK_NOTNULL(aicpu_dims)
      aicpu_dims->set_size(dim);
    }
  }
  return ge::SUCCESS;
}

ge::Status SetValueTypeListAicpuDataType(const ge::OpDescPtr &op_desc_ptr,
                                         const std::string &attr_name,
                                         aicpuops::AttrValue &attr_value) {
  AICPUE_LOGD("Get attr_name[%s] data type list.", attr_name.c_str());
  std::vector<ge::DataType> ge_type_list;
  CHECK_RES_BOOL(
      ge::AttrUtils::GetListDataType(op_desc_ptr, attr_name, ge_type_list),
      ErrorCode::DATA_TYPE_UNDEFILED,
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetListDataType function"
                              " failed to get attr[%s], op[%s].",
                              attr_name.c_str(),
                              op_desc_ptr->GetName().c_str()))

  for (ge::DataType &ge_type : ge_type_list) {
    AICPUE_LOGD("AttrName[%s] getype[%d].", attr_name.c_str(), ge_type);
    AICPU_CHECK_NOTNULL(attr_value.mutable_array())
    attr_value.mutable_array()->add_type(static_cast<ge::DataType>(ge_type));
  }
  return ge::SUCCESS;
}

ge::Status SetValueTypeListInt(const ge::OpDescPtr &op_desc_ptr,
                               const std::string &attr_name,
                               aicpuops::AttrValue &attr_value) {
  // if not find in json file, the attr is truely list_int
  std::vector<int32_t> list_int_value;
  CHECK_RES_BOOL(
      ge::AttrUtils::GetListInt(op_desc_ptr, attr_name, list_int_value),
      ErrorCode::DATA_TYPE_UNDEFILED,
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetListInt function"
                              " failed to get attr[%s], op[%s].",
                              attr_name.c_str(),
                              op_desc_ptr->GetName().c_str()))
  for (int32_t value : list_int_value) {
    AICPU_CHECK_NOTNULL(attr_value.mutable_array())
    attr_value.mutable_array()->add_i(value);
  }
  return ge::SUCCESS;
}

ge::Status SetValueTypeListString(const ge::OpDescPtr &op_desc_ptr,
                                  const std::string &attr_name,
                                  aicpuops::AttrValue &attr_value) {
  std::vector<std::string> list_string_value;
  CHECK_RES_BOOL(
      ge::AttrUtils::GetListStr(op_desc_ptr, attr_name, list_string_value),
      ErrorCode::DATA_TYPE_UNDEFILED,
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetListStr function"
                              " failed to get attr[%s], op[%s].",
                              attr_name.c_str(),
                              op_desc_ptr->GetName().c_str()))

  for (std::string value : list_string_value) {
    AICPU_CHECK_NOTNULL(attr_value.mutable_array())
    attr_value.mutable_array()->add_s(value);
  }
  return ge::SUCCESS;
}

ge::Status GetAttrValueFromGe(const ge::OpDescPtr &op_desc_ptr,
                              const std::string &attr_name,
                              const ge::GeAttrValue::ValueType ge_value_type,
                              aicpuops::AttrValue &attr_value) {
  std::string op_type = op_desc_ptr->GetType();
  switch (ge_value_type) {
    case ge::GeAttrValue::ValueType::VT_STRING: {
      AICPU_CHECK_RES(SetValueTypeString(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_FLOAT: {
      AICPU_CHECK_RES(SetValueTypeFloat(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_BOOL: {
      AICPU_CHECK_RES(SetValueTypeBool(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_INT: {
      AICPU_CHECK_RES(SetValueTypeInt(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_TENSOR: {
      AICPU_CHECK_RES(
          SetValueTypeAicpuTensor(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_DATA_TYPE: {
      AICPU_CHECK_RES(
          SetValueTypeAicpuDataType(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_LIST_FLOAT: {
      AICPU_CHECK_RES(SetValueTypeListFloat(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_LIST_LIST_INT: {
      AICPU_CHECK_RES(
          SetValueTypeListAicpuShape(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_LIST_DATA_TYPE: {
      AICPU_CHECK_RES(
          SetValueTypeListAicpuDataType(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_LIST_INT: {
      AICPU_CHECK_RES(SetValueTypeListInt(op_desc_ptr, attr_name, attr_value))
      break;
    }
    case ge::GeAttrValue::ValueType::VT_LIST_STRING: {
      AICPU_CHECK_RES(
          SetValueTypeListString(op_desc_ptr, attr_name, attr_value))
      break;
    }
    default: {
      AICPUE_LOGW("op [%s] Currently can not support attr value type of [%d].",
                  op_type.c_str(), ge_value_type);
      return ge::SUCCESS;
    }
  }
  return ge::SUCCESS;
}

ge::Status BuildAicpuNodeDef(const ge::OpDescPtr &op_desc_ptr,
                             aicpuops::NodeDef &node_def) {
  std::string op_type = op_desc_ptr->GetType();
  node_def.set_op(op_type);

  bool is_unknow_shape = false;
  if (ge::AttrUtils::HasAttr(op_desc_ptr, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE)) {
    // ATTR_NAME_UNKNOWN_SHAPE_TYPE attr exist, means unknow shape
    is_unknow_shape = true;
  }

  size_t input_num = op_desc_ptr->GetInputsSize();
  for (size_t i = 0; i < input_num; ++i) {
    aicpuops::Tensor *input_tensor = node_def.add_inputs();
    AICPU_CHECK_NOTNULL(input_tensor)
    ge::GeTensorDesc input_desc = op_desc_ptr->GetInputDesc(i);
    auto aicpu_shape = input_tensor->mutable_tensor_shape();
    AICPU_CHECK_NOTNULL(aicpu_shape)
    for (const auto &dim : input_desc.GetShape().GetDims()) {
      aicpuops::TensorShape_Dim *aicpu_dims = aicpu_shape->add_dim();
      AICPU_CHECK_NOTNULL(aicpu_dims)
      aicpu_dims->set_size(dim);
    }
    aicpu_shape->set_data_format(
        static_cast<ge::Format>(input_desc.GetFormat()));
    aicpu_shape->set_unknown_rank(is_unknow_shape);
    input_tensor->set_tensor_type(
        static_cast<ge::DataType>(input_desc.GetDataType()));
  }

  size_t output_num = op_desc_ptr->GetOutputsSize();
  for (size_t i = 0; i < output_num; ++i) {
    aicpuops::Tensor *output_tensor = node_def.add_outputs();
    AICPU_CHECK_NOTNULL(output_tensor)
    ge::GeTensorDesc output_desc = op_desc_ptr->GetOutputDesc(i);
    auto aicpu_shape = output_tensor->mutable_tensor_shape();
    AICPU_CHECK_NOTNULL(aicpu_shape)
    for (const auto &dim : output_desc.GetShape().GetDims()) {
      aicpuops::TensorShape_Dim *aicpu_dims = aicpu_shape->add_dim();
      AICPU_CHECK_NOTNULL(aicpu_dims)
      aicpu_dims->set_size(dim);
    }
    aicpu_shape->set_data_format(
        static_cast<ge::Format>(output_desc.GetFormat()));
    aicpu_shape->set_unknown_rank(is_unknow_shape);
    output_tensor->set_tensor_type(
        static_cast<ge::DataType>(output_desc.GetDataType()));
  }

  auto attrs_map = op_desc_ptr->GetAllAttrs();
  for (auto iter = attrs_map.begin(); iter != attrs_map.end(); ++iter) {
    const std::string &attr_name = iter->first;
    aicpuops::AttrValue attr_value;
    const ge::GeAttrValue::ValueType ge_value_type =
        (iter->second).GetValueType();
    AICPUE_LOGD("Get attr:[%s] value from op [%s], ge_value_type is [%d].",
                attr_name.c_str(), op_type.c_str(), ge_value_type);

    AICPUE_LOGI("Get Attr name: [%s].", attr_name.c_str());
    // If get attr value failed, no need to insert the attr. Only print log
    if (GetAttrValueFromGe(op_desc_ptr, attr_name, ge_value_type, attr_value) !=
        ge::SUCCESS) {
      AICPUE_LOGW("GetAttrValueFromGe attr_name[%s] for op[%s] failed.",
                  attr_name.c_str(), op_type.c_str());
      continue;
    }

    AICPU_CHECK_NOTNULL(node_def.mutable_attrs())
    auto pair = node_def.mutable_attrs()->insert(
        AttrValueMap::value_type(attr_name, attr_value));
    AICPU_CHECK_FALSE_EXEC(
        pair.second,
        AICPUE_LOGW("Node [%s] insert attr [%s] to nodeDef failed.",
                    op_desc_ptr->GetName().c_str(), attr_name.c_str()));
  }

  return ge::SUCCESS;
}

/**
 * Set shape type
 * @param op_desc_ptr op desc
 * @param all_op_info op info
 * @return status whether this operation success
 */
ge::Status CheckAndSetUnknowType(ge::OpDescPtr &op_desc_ptr, const map<string, OpFullInfo> &all_op_info) {
  AICPU_CHECK_NOTNULL(op_desc_ptr);
  string op_type = op_desc_ptr->GetType();
  if (IsUnknowShape(op_desc_ptr)) {
    AICPU_CHECK_FALSE_EXEC(
        AttrUtils::SetBool(op_desc_ptr, kAttrNameUnknownShape, true),
        AICPU_REPORT_CALL_ERROR(
            "Call Set ge::AttrUtils::SetBool failed to set attr[%s], op[%s].",
            kAttrNameUnknownShape.c_str(), op_desc_ptr->GetName().c_str());
        return ErrorCode::ADD_ATTR_FAILED)
  }

  auto iter = all_op_info.find(op_type);
  if (iter == all_op_info.end()) {
    AICPU_REPORT_INNER_ERROR("not supported op type[%s], op[%s].",
        op_type.c_str(), op_desc_ptr->GetName().c_str());
    return ErrorCode::NONE_KERNELINFOSTORE;
  }

  OpFullInfo op_full_info = iter->second;

  int32_t shape_type = 0;
  shape_type = op_full_info.shapeType;
  AICPU_CHECK_FALSE_EXEC(
      AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_UNKNOWN_SHAPE_TYPE, shape_type),
      AICPU_REPORT_CALL_ERROR(
          "Call AttrUtils::SetInt failed to set attr[%s], op[%s].",
          ATTR_NAME_UNKNOWN_SHAPE_TYPE.c_str(), op_desc_ptr->GetName().c_str());
      return ErrorCode::ADD_ATTR_FAILED)

  AICPUE_LOGI("Set unknown shape type [%d] for op type[%s].", shape_type, op_type.c_str());
  return SUCCESS;
}

ge::Status FillAttrOfAicpuNodeDef(const ge::OpDescPtr &op_desc_ptr,
                                  aicpuops::NodeDef &node_def) {
  auto attrs_map = op_desc_ptr->GetAllAttrs();
  for (auto iter = attrs_map.begin(); iter != attrs_map.end(); ++iter) {
    const std::string &attr_name = iter->first;
    aicpuops::AttrValue attr_value;
    const ge::GeAttrValue::ValueType ge_value_type =
        (iter->second).GetValueType();
    AICPUE_LOGD("Get attr:[%s] value from op [%s], ge_value_type is [%d].",
                attr_name.c_str(), op_desc_ptr->GetType().c_str(), ge_value_type);

    AICPUE_LOGI("Get Attr name: [%s].", attr_name.c_str());
    // If get attr value failed, no need to insert the attr. Only print log
    if (GetAttrValueFromGe(op_desc_ptr, attr_name, ge_value_type, attr_value) !=
        ge::SUCCESS) {
      AICPUE_LOGW("GetAttrValueFromGe attr_name[%s] for op[%s] failed.",
                  attr_name.c_str(), op_desc_ptr->GetType().c_str());
      continue;
    }

    AICPU_CHECK_NOTNULL(node_def.mutable_attrs())
    auto pair = node_def.mutable_attrs()->insert(
        AttrValueMap::value_type(attr_name, attr_value));
    AICPU_CHECK_FALSE_EXEC(
        pair.second,
        AICPUE_LOGW("Node[%s] insert attr [%s] to nodeDef failed.",
                    op_desc_ptr->GetName().c_str(), attr_name.c_str()));
  }
  return ge::SUCCESS;
}

ge::Status GetAicpuFftsPlusInfo(const ge::OpDescPtr &op_desc_ptr, FftsPlusInfo &ffts_info) {
  if (!op_desc_ptr->HasAttr(kAttrNameThreadScopeId)) {
    return ge::FAILED;
  }
  if (ge::AttrUtils::HasAttr(op_desc_ptr, kAttrNameUnknownShape)) {
    return ge::FAILED;
  }
  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = op_desc_ptr->TryGetExtAttr(kAttrNameSgtStruct, slice_info_ptr);
  if (slice_info_ptr == nullptr) {
    AICPU_REPORT_CALL_ERROR("The Node[%s] has no _sgt_struct_info", op_desc_ptr->GetName().c_str());
    return ge::FAILED;
  }
  AICPUE_LOGD("Node[%s] ffts thread mode[%u]slicenum[%u]",
              op_desc_ptr->GetName().c_str(), slice_info_ptr->thread_mode,
              slice_info_ptr->slice_instance_num);
  if (!slice_info_ptr->thread_mode) {
    return ge::FAILED;
  }
  auto thread_input_range = slice_info_ptr->input_tensor_slice;
  GetThreadTensorShape(thread_input_range, ffts_info.thread_input_shape);
  auto thread_output_range = slice_info_ptr->output_tensor_slice;
  GetThreadTensorShape(thread_output_range, ffts_info.thread_output_shape);
  ffts_info.slice_instance_num = slice_info_ptr->slice_instance_num;
  return ge::SUCCESS;
}

ge::Status InsertAicpuNodeDefAttrToOp(const ge::OpDescPtr &op_desc_ptr,
                                      aicpuops::NodeDef &node_def,
                                      const std::string &attr_name) {
  std::string node_def_str;
  if (!(node_def.SerializeToString(&node_def_str))) {
    AICPU_REPORT_INNER_ERROR(
        "The serialization from nodedef probuf to str failed, op[%s].",
        op_desc_ptr->GetName().c_str());
    return ErrorCode::CREATE_NODEDEF_FAILED;
  }
  const uint8_t *node_def_data =
      reinterpret_cast<const uint8_t *>(node_def_str.data());
  if (!(ge::AttrUtils::SetZeroCopyBytes(op_desc_ptr, attr_name,
          ge::Buffer::CopyFrom(node_def_data, node_def_str.length())))) {
    AICPU_REPORT_CALL_ERROR(
        "Failed to call SetZeroCopyBytes to set node def, op[%s].",
        op_desc_ptr->GetName().c_str());
    return ErrorCode::CREATE_NODEDEF_FAILED;
  }
  return ge::SUCCESS;
}

ge::Status BuildFftsPlusNodeInputShapeInfo(const ge::OpDescPtr &op_desc_ptr,
                                           aicpuops::NodeDef &node_def,
                                           const vector<vector<int64_t>> &dims,
                                           const bool &is_unknow_shape) {
  size_t input_num = op_desc_ptr->GetInputsSize();
  if (dims.size() != input_num) {
    AICPU_REPORT_INNER_ERROR("op[%s] input_num[%zu] input_dims size[%zu]",
                             op_desc_ptr->GetName().c_str(), input_num,
                             dims.size());
    return CREATE_NODEDEF_FAILED;
  }
  for (size_t i = 0; i < input_num; ++i) {
    aicpuops::Tensor *input_tensor = node_def.add_inputs();
    AICPU_CHECK_NOTNULL(input_tensor)
    ge::GeTensorDesc input_desc = op_desc_ptr->GetInputDesc(i);
    auto shape = input_tensor->mutable_tensor_shape();
    AICPU_CHECK_NOTNULL(shape)
    for (const auto &dim : dims[i]) {
      aicpuops::TensorShape_Dim *aicpu_dims = shape->add_dim();
      AICPU_CHECK_NOTNULL(aicpu_dims)
      aicpu_dims->set_size(dim);
    }
    shape->set_data_format(static_cast<ge::Format>(input_desc.GetFormat()));
    shape->set_unknown_rank(is_unknow_shape);
    input_tensor->set_tensor_type(
        static_cast<ge::DataType>(input_desc.GetDataType()));
  }
  return ge::SUCCESS;
}

ge::Status BuildFftsPlusNodeOutputShapeInfo(const ge::OpDescPtr &op_desc_ptr,
                                            aicpuops::NodeDef &node_def,
                                            const vector<vector<int64_t>> &dims,
                                            const bool &is_unknow_shape) {
  size_t output_num = op_desc_ptr->GetOutputsSize();
  if (dims.size() != output_num) {
    AICPU_REPORT_INNER_ERROR("op[%s] output_num[%zu] output_dims size[%zu]",
                             op_desc_ptr->GetName().c_str(), output_num,
                             dims.size());
    return CREATE_NODEDEF_FAILED;
  }
  for (size_t i = 0; i < output_num; ++i) {
    aicpuops::Tensor *output_tensor = node_def.add_outputs();
    AICPU_CHECK_NOTNULL(output_tensor)
    ge::GeTensorDesc output_desc = op_desc_ptr->GetOutputDesc(i);
    auto shape = output_tensor->mutable_tensor_shape();
    AICPU_CHECK_NOTNULL(shape)
    for (const auto &dim : dims[i]) {
      aicpuops::TensorShape_Dim *aicpu_dims = shape->add_dim();
      AICPU_CHECK_NOTNULL(aicpu_dims)
      aicpu_dims->set_size(dim);
    }
    shape->set_data_format(static_cast<ge::Format>(output_desc.GetFormat()));
    shape->set_unknown_rank(is_unknow_shape);
    output_tensor->set_tensor_type(
        static_cast<ge::DataType>(output_desc.GetDataType()));
  }
  return ge::SUCCESS;
}

ge::Status BuildFftsPlusAicpuNodeShapeInfo(const ge::OpDescPtr &op_desc_ptr,
                                           aicpuops::NodeDef &node_def,
                                           const FftsPlusInfo &ffts_info) {
  std::string op_type = op_desc_ptr->GetType();
  node_def.set_op(op_type);
  const uint32_t index = ffts_info.slice_instance_index;
  if ((index >= ffts_info.thread_input_shape.size()) || (index >= ffts_info.thread_output_shape.size())) {
    AICPU_REPORT_INNER_ERROR(
        "op[%s] index[%u] input shape size[%zu], output shape size[%zu]",
        op_desc_ptr->GetName().c_str(), index,
        ffts_info.thread_input_shape.size(),
        ffts_info.thread_output_shape.size());
    return CREATE_NODEDEF_FAILED;
  }
  bool is_unknow_shape = false;
  if (ge::AttrUtils::HasAttr(op_desc_ptr, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE)) {
    is_unknow_shape = true;
  }
  auto input_dims = ffts_info.thread_input_shape[index];
  ge::Status state = BuildFftsPlusNodeInputShapeInfo(
      op_desc_ptr, node_def, input_dims, is_unknow_shape);
  if (state != ge::SUCCESS) {
    return state;
  }
  auto output_dims = ffts_info.thread_output_shape[index];
  state = BuildFftsPlusNodeOutputShapeInfo(op_desc_ptr, node_def, output_dims,
                                           is_unknow_shape);
  if (state != ge::SUCCESS) {
    return state;
  }

  state = FillAttrOfAicpuNodeDef(op_desc_ptr, node_def);
  if (state != ge::SUCCESS) {
    return state;
  }
  return ge::SUCCESS;
}

ge::Status BuildFftsPlusAicpuNodeDef(const ge::OpDescPtr &op_desc_ptr,
                                     FftsPlusInfo &ffts_info) {
  aicpuops::NodeDef node_def;
  ffts_info.slice_instance_index = 0;
  ge::Status state = BuildFftsPlusAicpuNodeShapeInfo(
      op_desc_ptr, node_def, ffts_info);
  if (state != ge::SUCCESS) {
    AICPU_REPORT_INNER_ERROR("op[%s] build shape info fail[%u]",
                             op_desc_ptr->GetName().c_str(), state);
    return CREATE_NODEDEF_FAILED;
  }
  aicpuops::NodeDef tail_node_def;
  if (ffts_info.slice_instance_num > kAicpuManualSliceNum) {
    ffts_info.slice_instance_index = ffts_info.slice_instance_num - kAicpuManualSliceNum;
    state = BuildFftsPlusAicpuNodeShapeInfo(
        op_desc_ptr, tail_node_def, ffts_info);
    if (state != ge::SUCCESS) {
      AICPU_REPORT_INNER_ERROR("op[%s] build shape info fail[%u]",
                               op_desc_ptr->GetName().c_str(), state);
      return CREATE_NODEDEF_FAILED;
    }
  }
  state = InsertAicpuNodeDefAttrToOp(op_desc_ptr, node_def, kCustomizedOpDef);
  if (state != ge::SUCCESS) {
    return state;
  }
  if (ffts_info.slice_instance_num > kAicpuManualSliceNum) {
    state = InsertAicpuNodeDefAttrToOp(op_desc_ptr, tail_node_def, kCustomizedTailOpDef);
    if (state != ge::SUCCESS) {
      return state;
    }
  }
  return ge::SUCCESS;
}
}  // namespace aicpu
