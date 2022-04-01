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

#include "adapter/tbe_adapter/tbe_info/tbe_info_assembler.h"
#include <sstream>
#include "common/aicore_util_constants.h"
#include "common/configuration.h"
#include "common/dump_util.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_log.h"
#include "common/fe_type_utils.h"
#include "common/graph/fe_graph_utils.h"
#include "common/lxfusion_json_util.h"
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/string_utils.h"
#include "common/unknown_shape_util.h"
#include "ge/ge_api_types.h"
#include "graph/compute_graph.h"
#include "graph/ge_attr_value.h"
#include "graph/ge_local_context.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

namespace fe {
const std::map<ge::DataType, SetConstValueWithDtypePtr> TbeInfoAssembler::set_const_value_func_map = {
    {ge::DT_FLOAT16, std::make_shared<SetConstValueWithDtype>(SetConstValueWithFloat16)},
    {ge::DT_FLOAT, std::make_shared<SetConstValueWithDtype>(SetConstValue<float>)},
    {ge::DT_DOUBLE, std::make_shared<SetConstValueWithDtype>(SetConstValue<double>)},
    {ge::DT_INT8, std::make_shared<SetConstValueWithDtype>(SetConstValue<int8_t>)},
    {ge::DT_UINT8, std::make_shared<SetConstValueWithDtype>(SetConstValue<uint8_t>)},
    {ge::DT_INT16, std::make_shared<SetConstValueWithDtype>(SetConstValue<int16_t>)},
    {ge::DT_UINT16, std::make_shared<SetConstValueWithDtype>(SetConstValue<uint16_t>)},
    {ge::DT_INT32, std::make_shared<SetConstValueWithDtype>(SetConstValue<int32_t>)},
    {ge::DT_UINT32, std::make_shared<SetConstValueWithDtype>(SetConstValue<uint32_t>)},
    {ge::DT_INT64, std::make_shared<SetConstValueWithDtype>(SetConstValue<int64_t>)},
    {ge::DT_UINT64, std::make_shared<SetConstValueWithDtype>(SetConstValue<uint64_t>)}};

const string kDebugMode = "2";
/*
 *  @ingroup fe
 *  @brief   set parameter infos to tbe_op_info
 *  @param   [in]      node           op node pointer
 *  @param   [in]      input_info_ptr   op info store pointer
 *  @param   [in/out]  input          global temp param
 *  @param   [in]      input_tensor    tensor from const node
 *  @param   [in/out]  op_info         tbe data item
 *  @param   [in]      input_size     number of inputs or outputs of op
 *  @param   [in]      i             index to input
 *  @param   [in]      is_input_or_output input or output
 *  @return  SUCCESS or FAILED
 */
Status TbeInfoAssembler::FeedParameterInfoForInput(const ge::Node *node, const InputOrOutputInfoPtr &info_ptr,
                                                   int index_in_opdesc, bool last_item_flag,
                                                   te::TbeOpTensor &tbe_op_tensor, te::TbeOpParam &tbe_op_param,
                                                   te::TbeOpInfo &tbe_op_info) const {
  OpParamType input_type = info_ptr->GetParamType();
  tbe_op_param.SetType(te::TensorType(input_type));
  OpConstValueDepend value_depend = info_ptr->GetConstValueDepend();
  auto iter = VALUE_DEPEND_MAP.find(value_depend);
  if (iter != VALUE_DEPEND_MAP.end()) {
    tbe_op_param.SetValueDepend(iter->second);
  } else {
    FE_LOGW("Node[type=%s,name=%s]: ValueDepend is %d, is invalid.", node->GetOpDesc()->GetType().c_str(),
            node->GetOpDesc()->GetName().c_str(), value_depend);
    tbe_op_param.SetValueDepend(te::VALUE_DEPEND_IGNORE);
  }

  // dynamic input contains multiple sub-inputs, should be packaged together
  if (input_type == OpParamType::DYNAMIC) {
    tbe_op_param.SetTensor(tbe_op_tensor);
    if (last_item_flag) {
      tbe_op_info.AddInput(tbe_op_param);
      tbe_op_param.Clear();
    }
  } else {
    bool need_tensor =
        ((input_type == OpParamType::OPTIONAL || input_type == OpParamType::REQUIRED) && (index_in_opdesc >= 0) &&
         (node->GetInDataAnchor(index_in_opdesc) != nullptr &&
          node->GetInDataAnchor(index_in_opdesc)->GetPeerOutAnchor() != nullptr));
    if (need_tensor) {
      tbe_op_param.SetTensor(tbe_op_tensor);
    }
    tbe_op_info.AddInput(tbe_op_param);
    tbe_op_param.Clear();
  }
  return SUCCESS;
}

Status TbeInfoAssembler::FeedParameterInfoForOutput(const ge::OpDesc &op_desc, const ge::GeTensorDesc &output_desc,
                                                    const InputOrOutputInfoPtr &info_ptr, bool last_item_flag,
                                                    te::TbeOpTensor &tbe_op_tensor, te::TbeOpParam &tbe_op_param,
                                                    te::TbeOpInfo &tbe_op_info) const {
  OpParamType input_type = info_ptr->GetParamType();
  tbe_op_param.SetType(te::TensorType(input_type));
  OpConstValueDepend value_depend = info_ptr->GetConstValueDepend();
  auto iter = VALUE_DEPEND_MAP.find(value_depend);
  if (iter != VALUE_DEPEND_MAP.end()) {
    tbe_op_param.SetValueDepend(iter->second);
  } else {
    FE_LOGW("Node[type=%s,name=%s]: ValueDepend is %d, is invalid.", op_desc.GetType().c_str(),
            op_desc.GetName().c_str(), value_depend);
    tbe_op_param.SetValueDepend(te::VALUE_DEPEND_IGNORE);
  }
  // dynamic input contains multiple sub-inputs, should be packaged together
  if (input_type == OpParamType::DYNAMIC) {
    tbe_op_param.SetTensor(tbe_op_tensor);
    if (last_item_flag) {
      tbe_op_info.AddOutput(tbe_op_param);
      tbe_op_param.Clear();
    }
  } else {
    bool need_tensor =
        (input_type == OpParamType::OPTIONAL && !IsMemoryEmpty(output_desc)) || input_type == OpParamType::REQUIRED;
    if (need_tensor) {
      tbe_op_param.SetTensor(tbe_op_tensor);
    } else {
      FE_LOGI("Node[type=%s,name=%s]: the output %s needs an empty tensor, the paramType=%d.",
              op_desc.GetType().c_str(), op_desc.GetName().c_str(), info_ptr->GetName().c_str(), input_type);
    }
    tbe_op_info.AddOutput(tbe_op_param);
    tbe_op_param.Clear();
  }
  return SUCCESS;
}

Status TbeInfoAssembler::FeedParameterInfoForNotFound(const InputOrOutputInfoPtr &info_ptr,
                                                      const string &is_input_or_output, te::TbeOpParam &tbe_op_param,
                                                      te::TbeOpInfo &tbe_op_info) const {
  tbe_op_param.SetType(te::TensorType(info_ptr->GetParamType()));
  if (is_input_or_output == STR_INPUT_LOWERCASE) {
    tbe_op_info.AddInput(tbe_op_param);
  } else if (is_input_or_output == STR_OUTPUT_LOWERCASE) {
    tbe_op_info.AddOutput(tbe_op_param);
  }
  tbe_op_param.Clear();
  return SUCCESS;
}

Status TbeInfoAssembler::ConvertParameterInfoForInput(InputOrOutputInfoPtr info_ptr, te::TbeOpParam &input,
                                                      te::TbeOpTensor &input_tensor, te::TbeOpInfo &op_info,
                                                      bool last_item_flag) const {
  OpConstValueDepend value_depend = info_ptr->GetConstValueDepend();
  auto iter = VALUE_DEPEND_MAP.find(value_depend);
  if (iter == VALUE_DEPEND_MAP.end()) {
    FE_LOGW("Node[input name=%s]: ValueDepend is %d, is invalid.", info_ptr->GetName().c_str(), value_depend);
    input.SetValueDepend(te::VALUE_DEPEND_IGNORE);
  } else {
    input.SetValueDepend(iter->second);
  }

  OpParamType input_type = info_ptr->GetParamType();
  input.SetType(te::TensorType(input_type));
  if (input_type == OpParamType::DYNAMIC) {
    input.SetTensor(input_tensor);
    if (last_item_flag) {
      op_info.AddInput(input);
      input.Clear();
    }
  } else {
    bool need_tensor = input_type == OpParamType::OPTIONAL || input_type == OpParamType::REQUIRED;
    if (need_tensor) {
      input.SetTensor(input_tensor);
    }
    op_info.AddInput(input);
    input.Clear();
  }
  return SUCCESS;
}

void TbeInfoAssembler::FeedL1InputTensor(const ToOpStructPtr &l1_info, const ge::OpDescPtr &op_desc,
                                         IndexNameMap &input_idx_name_map, const uint32_t &index_in_opdesc,
                                         te::TbeOpTensor &input_tensor) const {
  // update opinfo
  input_tensor.SetL1WorkspaceFlag(l1_info->op_l1_workspace_flag);
  input_tensor.SetL1WorkspaceSize(l1_info->op_l1_workspace_size);
  input_tensor.SetTotalShape(l1_info->total_shape);
  input_tensor.SetSplitIndex(l1_info->split_index);
  FE_LOGD("FeedL1InputTensor begin, op name is %s.", op_desc->GetName().c_str());
  if (!l1_info->op_l1_fusion_type.empty()) {
    input_tensor.SetL1FusionType(l1_info->op_l1_fusion_type[0]);
  }

  size_t input_idx_name_size = input_idx_name_map.size();
  size_t l1_slice_input_shape_size = l1_info->slice_input_shape.size();
  if (l1_slice_input_shape_size == input_idx_name_size) {
    if (index_in_opdesc >= l1_slice_input_shape_size) {
      FE_LOGW("index_in_opdesc > l1_slice_input_shape_size, index_in_opdesc:%u, l1_slice_input_shape_size:%zu.",
              index_in_opdesc, l1_slice_input_shape_size);
      return;
    }

    input_tensor.SetValidShape(l1_info->slice_input_shape[index_in_opdesc]);
  }

  size_t l1_slice_input_offset_size = l1_info->slice_input_offset.size();
  if (l1_slice_input_offset_size == input_idx_name_size) {
    if (index_in_opdesc >= l1_slice_input_offset_size) {
      FE_LOGW("index_in_opdesc >= l1_slice_input_offset_size, index_in_opdesc:%u, l1_slice_input_offset_size:%zu.",
              index_in_opdesc, l1_slice_input_offset_size);
      return;
    }
    input_tensor.SetSliceOffset(l1_info->slice_input_offset[index_in_opdesc]);
  }

  /* Set addr  offset */
  size_t input_num = op_desc->GetInputsSize();
  vector<int64_t> input_offsets = op_desc->GetInputOffset();
  size_t input_offset_size = input_offsets.size();
  if (input_num > input_offset_size) {
    FE_LOGW("input_num > input_offset_size, input_num:%lu, input_offset_size:%lu.", input_num, input_offset_size);
  } else {
    if (index_in_opdesc >= input_offset_size) {
      FE_LOGW("index_in_opdesc >= input_offsets_size, index_in_opdesc:%u, input_offset_size:%zu.", index_in_opdesc,
              input_offset_size);
      return;
    }
    input_tensor.SetAddrOffset(input_offsets[index_in_opdesc]);
  }
}

void TbeInfoAssembler::FeedL2InputTensor(const ToOpStructPtr &l2_info, const ge::OpDescPtr &op_desc,
                                         IndexNameMap &input_idx_name_map, const uint32_t &index_in_opdesc,
                                         te::TbeOpTensor &input_tensor) const {
  if (l2_info == nullptr) {
    return;
  }
  // update opinfo
  size_t input_idx_name_size = input_idx_name_map.size();
  size_t l2_slice_input_shape_size = l2_info->slice_input_shape.size();
  if (l2_slice_input_shape_size == input_idx_name_size) {
    if (index_in_opdesc >= l2_slice_input_shape_size) {
      FE_LOGW("index_in_opdesc >= l2_slice_input_shape_size, index_in_opdesc:%u, l2_slice_input_shape_size:%zu.",
              index_in_opdesc, l2_slice_input_shape_size);
      return;
    }
    input_tensor.SetValidShape(l2_info->slice_input_shape[index_in_opdesc]);
  }

  size_t l2_slice_input_offset_size = l2_info->slice_input_offset.size();
  if (l2_slice_input_offset_size == input_idx_name_size) {
    if (index_in_opdesc >= l2_slice_input_offset_size) {
      FE_LOGW("index_in_opdesc >= l2_slice_input_offset_size, index_in_opdesc:%u, l2_slice_input_offset_size:%zu.",
              index_in_opdesc, l2_slice_input_offset_size);
      return;
    }
    input_tensor.SetSliceOffset(l2_info->slice_input_offset[index_in_opdesc]);
  }
  /* Set addr  offset */
  size_t input_num = op_desc->GetInputsSize();
  vector<int64_t> input_offsets = op_desc->GetInputOffset();
  size_t input_offset_size = input_offsets.size();
  if (input_num > input_offset_size) {
    FE_LOGW("intput_desc_size > input_offset_size, input_desc_size:%lu, input_offset_size:%lu.", input_num,
            input_offset_size);
  } else {
    if (index_in_opdesc >= input_offset_size) {
      FE_LOGW("index_in_opdesc >= input_offsets_size, index_in_opdesc:%u, input_offset_size:%zu.", index_in_opdesc,
              input_offset_size);
      return;
    }
    input_tensor.SetAddrOffset(input_offsets[index_in_opdesc]);
  }
}

void SetTbeTensorValueRange(const ge::OpDesc &op_desc, const ge::GeTensorDesc &tensor_desc,
                            te::TbeOpTensor &tbe_tensor) {
  std::vector<std::pair<int64_t, int64_t>> value_range = GetValueRange(tensor_desc);
  FE_LOGD("[SubGraphOpt][SetTbeTensor][SetTValueRange] Value range of op [name : %s, type : %s] is [%s].",
          op_desc.GetName().c_str(), op_desc.GetType().c_str(), ShapeRangeToStr(value_range).c_str());
  tbe_tensor.SetValueRange(value_range);
}

Status TbeInfoAssembler::SetInputTensorBaseInfo(const ge::OpDescPtr &op_desc,
                                                const uint32_t &index_in_opdesc,
                                                te::TbeOpTensor &input_tensor) const {
  if (op_desc->MutableInputDesc(index_in_opdesc) == nullptr) {
    return SUCCESS;
  }
  string op_name = op_desc->GetName();
  string op_type = op_desc->GetType();
  auto input_i = op_desc->MutableInputDesc(index_in_opdesc);
  auto shape = input_i->MutableShape();
  ge::DataType dtype = input_i->GetDataType();
  auto format = input_i->GetFormat();
  auto primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(format));
  auto sub_format = static_cast<ge::Format>(ge::GetSubFormat(format));
  auto data_type_iter = DATATYPE_STRING_MAP.find(dtype);
  if (data_type_iter == DATATYPE_STRING_MAP.end()) {
    FE_LOGD("Current data type[%u] of input index = %u of op (name [%s], type [%s]) is not found.", dtype,
            index_in_opdesc, op_name.c_str(), op_type.c_str());
    return FAILED;
  }
  FE_LOGD("Op[name=%s,type=%s]: index_in_opdesc is [%u], input format is [%s].", op_name.c_str(), op_type.c_str(),
          index_in_opdesc, ge::TypeUtils::FormatToSerialString(format).c_str());
  string dim_string;
  // If empty shape of scalar, the op will compile fail. So need set {1} to shape.
  vector<int64_t> dims;
  if (input_i->MutableShape().IsScalar()) {
    dim_string = "1, ";
    dims = {1};
  } else {
    dim_string = GetShapeDims(input_i->GetShape());
    dims = shape.GetDims();
  }
  FE_LOGD("Op[name=%s,type=%s]: current input shape is [%s].", op_name.c_str(), op_type.c_str(), dim_string.c_str());
  string primary_format_str = ge::TypeUtils::FormatToSerialString(primary_format);
  // set full shape to StridedRead
  bool is_strided_read =
      op_desc->GetType() == STRIDEDREAD && op_desc->GetInputDesc(0).GetFormat() == ge::FORMAT_NC1HWC0;
  if (is_strided_read && dims.size() >= 2) {
    int64_t c1 = dims[1];
    (void)ge::AttrUtils::GetInt(op_desc, ATTR_STRIDE_ATTR_STRIDE, c1);
    dims[1] = c1;
  }
  // aicore not support bool
  string dtype_str = data_type_iter->second;
  if (dtype_str == STR_BOOL) {
    dtype_str = STR_INT8;
  }

  input_tensor.SetShape(dims);
  input_tensor.SetType(dtype_str);
  input_tensor.SetFormat(primary_format_str);
  input_tensor.SetSubFormat(sub_format);
  if (IsFeSupportedDynamicOp(*op_desc)) {
    std::vector<std::pair<int64_t, int64_t>> shape_range = GetShapeRange(*input_i);
    std::vector<std::pair<int64_t, int64_t>> ori_shape_range;
    (void)input_i->GetOriginShapeRange(ori_shape_range);
    FE_LOGD("Shape range of op[name:%s,type:%s] is %s, origin range is %s.", op_name.c_str(), op_type.c_str(),
            ShapeRangeToStr(shape_range).c_str(), ShapeRangeToStr(ori_shape_range).c_str());
    input_tensor.SetShapeRange(shape_range);
    input_tensor.SetOriginShapeRange(ori_shape_range);
    SetTbeTensorValueRange(*op_desc, *input_i, input_tensor);
  }
  return SUCCESS;
}

void TbeInfoAssembler::GetOpInputL1Attr(const ge::OpDescPtr &op_desc, std::vector<int64_t> &op_input_l1_flag,
                                        std::vector<int64_t> &op_input_l1_addr,
                                        std::vector<int64_t> &op_input_l1_valid_size) const {
  if (!ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_OP_INPUT_L1_FLAG, op_input_l1_flag)) {
    FE_LOGD("Fail to get attribute op_input_l1_flag of op[%s, %s].", op_desc->GetName().c_str(),
            op_desc->GetType().c_str());
    return;
  }
  bool is_l1_flag = false;
  if (std::any_of(op_input_l1_flag.begin(), op_input_l1_flag.end(),
                  [](int64_t input_l1_flag) { return input_l1_flag >= 0; })) {
    is_l1_flag = true;
  }
  if (is_l1_flag) {
    if (!ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_OP_INPUT_L1_ADDR, op_input_l1_addr)) {
      FE_LOGD("Fail to get attribute op_input_l1_addr of op[%s, %s].", op_desc->GetName().c_str(),
              op_desc->GetType().c_str());
      return;
    }
    if (!ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_OP_INPUT_L1_VALID_SIZE, op_input_l1_valid_size)) {
      FE_LOGD("Fail to get attr op_input_l1_valid_size of op[%s, %s].", op_desc->GetName().c_str(),
              op_desc->GetType().c_str());
      return;
    }
  }
  FE_LOGD("The value of OP_INPUT_L1_FLAG, OP_INPUT_L1_ADDR, OP_INPUT_L1_VALID_SIZE of node[%s] are [%s], [%s], [%s].",
          op_desc->GetName().c_str(), StringUtils::IntegerVecToString(op_input_l1_flag).c_str(),
          StringUtils::IntegerVecToString(op_input_l1_addr).c_str(),
          StringUtils::IntegerVecToString(op_input_l1_valid_size).c_str());
}

bool GetAddTensorFlag(const ge::OpDesc &op_desc, const vector<uint32_t> &specific_input_index_vec) {
  bool res = false;
  if (!specific_input_index_vec.empty()) {
    auto input_desc = op_desc.GetInputDescPtr(specific_input_index_vec.at(0));
    auto primary_format = ge::GetPrimaryFormat(input_desc->GetFormat());
    res = primary_format != ge::FORMAT_RESERVED && input_desc->GetDataType() != ge::DT_UNDEFINED
            && input_desc->GetDataType() < ge::DT_MAX;
  }
  return res;
}

bool GetAddTensorFlag(const ge::OpDescPtr &op_desc, const vector<uint32_t> &specific_input_index_vec) {
  return GetAddTensorFlag(*op_desc.get(), specific_input_index_vec);
}

/*
 *  @ingroup fe
 *  @brief   set inputs to tbe_op_info
 *  @param   [in]  node            input node pointer
 *  @param   [in]  input_idx_name_map map with input index as key and input name as
 * value
 *  @param   [in]  op_kernel_info_ptr tensor from const node
 *  @param   [in/out]  op_info      tbe data item
 *  @return  SUCCESS or FAILED
 */
Status TbeInfoAssembler::FeedInputsToTbeOpInfo(const ge::Node *node, IndexNameMap &input_idx_name_map,
                                               OpKernelInfoPtr op_kernel_info_ptr, te::TbeOpInfo &op_info) const {
  auto op_desc = node->GetOpDesc();
  auto input_info_in_opkernel = op_kernel_info_ptr->GetAllInputInfo();
  auto input_size_in_op_kernel = input_info_in_opkernel.size();
  vector<int32_t> memery_type_vec;
  ToOpStructPtr l1_info;
  FE_MAKE_SHARED(l1_info = std::make_shared<ToOpStruct_t>(), return FAILED);
  ToOpStructPtr l2_info;
  FE_MAKE_SHARED(l2_info = std::make_shared<ToOpStruct_t>(), return FAILED);
  (void)ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_INPUT_MEM_TYPE_LIST, memery_type_vec);
  GetL1ToOpStructFromJson(op_desc, l1_info);
  GetL2ToOpStructFromJson(op_desc, l2_info);

  std::vector<int64_t> op_input_l1_flag;
  std::vector<int64_t> op_input_l1_addr;
  std::vector<int64_t> op_input_l1_valid_size;
  GetOpInputL1Attr(op_desc, op_input_l1_flag, op_input_l1_addr, op_input_l1_valid_size);

  auto memery_type_vec_size = memery_type_vec.size();
  auto input_size = op_desc->GetInputsSize();
  te::TbeOpParam input;
  for (uint32_t index_in_op_kernel = 0; index_in_op_kernel < input_size_in_op_kernel; index_in_op_kernel++) {
    auto input_info_ptr = input_info_in_opkernel.at(index_in_op_kernel);
    FE_CHECK(input_info_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][FeedInputs] inputInfoPtr is \
                    nullptr."),
             return FAILED);
    string input_name_in_op_kernel = input_info_ptr->GetName();

    vector<uint32_t> specific_input_index_vec_in_op_desc;
    if (GetSpecificIndex(*op_desc.get(), input_idx_name_map, input_name_in_op_kernel, true,
                         specific_input_index_vec_in_op_desc) != SUCCESS) {
      REPORT_FE_ERROR(
          "[SubGraphOpt][PreCompileOp][FeedInputs][Op %s, type %s]: Get input name [%s] from \
                      inputIdxNameMap failed.",
          op_desc->GetName().c_str(), op_desc->GetType().c_str(), input_name_in_op_kernel.c_str());
      return FAILED;
    }

    auto size_of_this_input = specific_input_index_vec_in_op_desc.size();
    bool add_tensor_info = GetAddTensorFlag(op_desc, specific_input_index_vec_in_op_desc);
    if (add_tensor_info) {
      uint32_t count = 0;
      for (uint32_t index_in_opdesc : specific_input_index_vec_in_op_desc) {
        auto input_i = op_desc->GetInputDesc(index_in_opdesc);
        vector<int64_t> dims;
        te::TbeOpTensor input_tensor(input_name_in_op_kernel, dims, "", "");
        if (SetInputTensorBaseInfo(op_desc, index_in_opdesc, input_tensor) != SUCCESS) {
          REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][FeedInputs][Op %s, type %s]: failed to setInputTensorBaseInfo.",
                          op_desc->GetName().c_str(), op_desc->GetType().c_str());
          return FAILED;
        }
        FE_LOGD("get attr fusion userinfo, info size %zu, input size %zu.", memery_type_vec_size, input_size);
        bool add_addr_type = (memery_type_vec_size == input_size && index_in_opdesc < memery_type_vec_size);
        if (add_addr_type) {
          input_tensor.SetAddrType(memery_type_vec[index_in_opdesc]);
        }
        if (l1_info != nullptr) {
          FeedL1InputTensor(l1_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);
          if (op_input_l1_flag.size() > index_in_opdesc) {
            input_tensor.SetL1AddrFlag(op_input_l1_flag[index_in_opdesc]);
          }
          if (op_input_l1_addr.size() > index_in_opdesc) {
            input_tensor.SetAddrOffset(op_input_l1_addr[index_in_opdesc]);
          }
          if (op_input_l1_valid_size.size() > index_in_opdesc) {
            input_tensor.SetL1ValidSize(op_input_l1_valid_size[index_in_opdesc]);
          }
        }

        FeedL2InputTensor(l2_info, op_desc, input_idx_name_map, index_in_opdesc, input_tensor);

        /* Set original format and dtype */
        ge::Format primary_origin_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_i.GetOriginFormat()));
        input_tensor.SetOriginFormat(ge::TypeUtils::FormatToSerialString(primary_origin_format));
        input_tensor.SetOriginShape(input_i.GetOriginShape().GetDims());
        FE_LOGD("Op[name=%s,type=%s]: origin input format is [%s], the index_in_op_kernel is [%u].",
                op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                ge::TypeUtils::FormatToSerialString(input_i.GetOriginFormat()).c_str(), index_in_op_kernel);
        string input_origin_shape_dims = GetShapeDims(input_i.GetOriginShape());
        FE_LOGD("Op[name=%s,type=%s]: origin input shape is [%s].", op_desc->GetName().c_str(),
                op_desc->GetType().c_str(), input_origin_shape_dims.c_str());

        if (SetTensorConstValue(node, index_in_opdesc, input_info_ptr, input_tensor) != SUCCESS) {
          REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][FeedInputs][Op %s, type %s]: Fail to set value for input[%u].",
                          op_desc->GetName().c_str(), op_desc->GetType().c_str(), index_in_opdesc);
          return FAILED;
        }
        // take care that danamic input have multiple sub-inputs.
        if (FeedParameterInfoForInput(node, input_info_ptr, static_cast<int>(index_in_opdesc),
                                      (count == size_of_this_input - 1), input_tensor, input, op_info) != SUCCESS) {
          REPORT_FE_ERROR(
              "[SubGraphOpt][PreCompileOp][FeedInputs][Op %s, type %s]: failed to \
                        feedDynamicInputsToTbeOpInfo.",
              op_desc->GetName().c_str(), op_desc->GetType().c_str());
          return FAILED;
        }
        count++;
      }
    } else {
      if (FeedParameterInfoForNotFound(input_info_ptr, STR_INPUT_LOWERCASE, input, op_info) != SUCCESS) {
        REPORT_FE_ERROR(
            "[SubGraphOpt][PreCompileOp][FeedInputs][Op %s, type %s]: failed to \
                        feedDynamicInputsToTbeOpInfo.",
            op_desc->GetName().c_str(), op_desc->GetType().c_str());
        return FAILED;
      }
      FE_LOGD("Optional input %u named [%s] is missing in Opdesc, we set a empty TbeOpTensor when feeds input.",
              index_in_op_kernel, input_info_ptr->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status TbeInfoAssembler::FindAndCheckEndNodeForConstValue(const ge::Node *node, const uint32_t &tensor_index,
                                                          InputOrOutputInfoPtr tensor_info_ptr,
                                                          ge::NodePtr &other_end_node, bool &is_const_node) const {
  // find the node of other end
  is_const_node = FeGraphUtils::IsPeerOutConst(node, static_cast<int>(tensor_index), other_end_node);
  bool required_check_invalid = (tensor_info_ptr->GetConstValueDepend() == CONST_REQUIRED && !is_const_node);
  if (required_check_invalid) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][PreCompileOp][SetTensorConstVal][Op %s, type %s]: The const value input[%u] \
                    is required, but the input is not linked to a const node.",
        node->GetName().c_str(), node->GetType().c_str(), tensor_index);
    return FAILED;
  }
  return SUCCESS;
}

Status TbeInfoAssembler::SetTensorConstValue(const ge::Node *node, const uint32_t &tensor_index,
                                             InputOrOutputInfoPtr tensor_info_ptr, te::TbeOpTensor &op_tensor) const {
  auto op_desc = node->GetOpDesc();
  FE_LOGD("Begin to set const value for input[%s] of node[%s, %s].", tensor_info_ptr->GetName().c_str(),
          node->GetName().c_str(), node->GetType().c_str());

  ge::GeTensorPtr const_tensor_ptr;
  auto tensor_desc = op_desc->MutableInputDesc(tensor_index);
  FE_CHECK_NOTNULL(tensor_desc);
  // fuzz build, GE set const value to op_desc by tensor attr, change const node to data node
  if (ge::AttrUtils::MutableTensor(tensor_desc, ge::ATTR_NAME_VALUE, const_tensor_ptr)) {
    if (AssembleConstValue(const_tensor_ptr, op_desc, op_tensor) == SUCCESS) {
      FE_LOGD("Set index[%u] of node[%s] const value success.", tensor_index, node->GetName().c_str());
      return SUCCESS;
    }
  }

  if (tensor_info_ptr->GetConstValueDepend() == CONST_IGNORE) {
    return SUCCESS;
  }

  // find the node of other end
  ge::NodePtr other_end_node = nullptr;
  bool is_const_node = false;
  if (FindAndCheckEndNodeForConstValue(node, tensor_index, tensor_info_ptr, other_end_node, is_const_node) != SUCCESS) {
    return FAILED;
  }

  bool optional_check_invalid = (tensor_info_ptr->GetConstValueDepend() == CONST_OPTIONAL && !is_const_node);
  if (optional_check_invalid) {
    FE_LOGD("The const value input[%u] in node[%s, %s] is optional, and the input is not linked to a const node.",
            tensor_index, node->GetName().c_str(), node->GetType().c_str());
    return SUCCESS;
  }

  FE_CHECK_NOTNULL(other_end_node);
  auto const_op_desc = other_end_node->GetOpDesc();
  FE_CHECK_NOTNULL(const_op_desc);
  FE_LOGD("Begin to get const data from node[%s, %s].",
          other_end_node->GetName().c_str(), other_end_node->GetType().c_str());
  vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(other_end_node);
  if (weights.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Const node does not have weight.",
                    const_op_desc->GetName().c_str(), const_op_desc->GetType().c_str());
    return FAILED;
  }
  const_tensor_ptr = weights[0];
  if (AssembleConstValue(const_tensor_ptr, const_op_desc, op_tensor) != SUCCESS) {
    return FAILED;
  }

  return SUCCESS;
}

Status TbeInfoAssembler::AssembleConstValue(ge::GeTensorPtr const_tensor_ptr, const ge::OpDescPtr &op_desc,
                                            te::TbeOpTensor &op_tensor) const {
  FE_LOGD("Begin AssembleConstValue of node[%s].", op_desc->GetName().c_str());
  string tensor_name;
  (void)op_tensor.GetName(tensor_name);
  FE_CHECK_NOTNULL(const_tensor_ptr);
  ge::GeTensorDesc out_tensor_desc = const_tensor_ptr->GetTensorDesc();
  FE_CHECK_NOTNULL(const_tensor_ptr->GetData().GetData());

  ge::DataType data_type = out_tensor_desc.GetDataType();
  auto iter_set_const_value_func = set_const_value_func_map.find(data_type);
  if (iter_set_const_value_func == set_const_value_func_map.end()) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Data type[%s] is not supported yet \
                    while assembling const value.",
        op_desc->GetName().c_str(), op_desc->GetType().c_str(), DTypeToStr(data_type).c_str());
    return FAILED;
  }

  SetConstValueWithDtypePtr set_const_value_func = nullptr;
  FE_MAKE_SHARED(set_const_value_func = iter_set_const_value_func->second, return FAILED);
  FE_CHECK_NOTNULL(set_const_value_func);

  Status status = (*set_const_value_func)(const_tensor_ptr, tensor_name, op_tensor);
  if (status != SUCCESS) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Fail to set const value for op \
                    tensor with Data type[%s].",
        op_desc->GetName().c_str(), op_desc->GetType().c_str(), DTypeToStr(data_type).c_str());
    return FAILED;
  }

  FE_LOGD("Op tensor[%s] has been set const value whose data type is %s.", tensor_name.c_str(),
          DTypeToStr(data_type).c_str());
  return SUCCESS;
}

Status CreateTbeTensor(const ge::OpDesc &op_desc, const TensorDescAndIndex &tensor_info, te::TbeOpTensor &tbe_tensor) {
  string is_input = IS_INPUT_TO_STRING(tensor_info.is_input);
  auto shape = tensor_info.tensor_desc_ptr->MutableShape();
  ge::DataType dtype = tensor_info.tensor_desc_ptr->GetDataType();
  auto data_type_iter = DATATYPE_STRING_MAP.find(dtype);
  if (data_type_iter == DATATYPE_STRING_MAP.end()) {
    REPORT_INNER_ERROR(EM_INNER_ERROR, "Current data type[%u] of %s %u of op (name [%s], type [%s]) is not found.",
                       dtype, is_input.c_str(), tensor_info.index_in_opdesc, op_desc.GetName().c_str(),
                       op_desc.GetType().c_str());
    FE_LOGD("Current data type[%u] of %s %u of op (name [%s], type [%s]) is not found.", dtype, is_input.c_str(),
            tensor_info.index_in_opdesc, op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  string dtype_str = data_type_iter->second;
  if (dtype_str == STR_BOOL) {
    dtype_str = STR_INT8;
    FE_LOGD("Update dtype bool to int8 successfully. OpName: %s, OpType: %s.",
            op_desc.GetName().c_str(), op_desc.GetType().c_str());
  }

  auto format = tensor_info.tensor_desc_ptr->GetFormat();
  auto primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(format));
  auto sub_format = ge::GetSubFormat(format);
  if (tensor_info.propagat_heavy_format != ge::FORMAT_RESERVED) {
    sub_format = tensor_info.propagat_sub_format;
  }
  FE_LOGD("Op[name=%s,type=%s]: current %s primary_format is [%s], sub_format is [%d], the index_in_op_kernel \
          is [%zu].", op_desc.GetName().c_str(), op_desc.GetType().c_str(), is_input.c_str(),
          ge::TypeUtils::FormatToSerialString(primary_format).c_str(), sub_format, tensor_info.index_in_op_kernel);
  string dim_string = GetShapeDims(tensor_info.tensor_desc_ptr->GetShape());
  FE_LOGD("Op[name=%s,type=%s]: current %s shape is [%s].", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
          is_input.c_str(), dim_string.c_str());

  string primary_format_str = ge::TypeUtils::FormatToSerialString(primary_format);
  // If dim_num is 0, the op will compile fail. So need set {1} to shape.
  vector<int64_t> dims = shape.GetDims();
  if (tensor_info.tensor_desc_ptr->MutableShape().IsScalar()) {
    dims = {1};
  }

  tbe_tensor = te::TbeOpTensor(tensor_info.name_in_op_kernel, dims, dtype_str, primary_format_str);
  tbe_tensor.SetSubFormat(sub_format);
  return SUCCESS;
}

void SetTbeTensorShape(const ge::OpDesc &op_desc, const TensorDescAndIndex &tensor_info, te::TbeOpTensor &tbe_tensor) {
  if (IsFeSupportedDynamicOp(op_desc)) {
    std::vector<std::pair<int64_t, int64_t>> shape_range = GetShapeRange(*tensor_info.tensor_desc_ptr.get());
    std::vector<std::pair<int64_t, int64_t>> ori_shape_range;
    (void)tensor_info.tensor_desc_ptr->GetOriginShapeRange(ori_shape_range);
    FE_LOGD("Shape range of op[name:%s,type:%s] is %s, origin range is %s.", op_desc.GetName().c_str(),
            op_desc.GetType().c_str(), ShapeRangeToStr(shape_range).c_str(), ShapeRangeToStr(ori_shape_range).c_str());
    tbe_tensor.SetShapeRange(shape_range);
    tbe_tensor.SetOriginShapeRange(ori_shape_range);
    SetTbeTensorValueRange(op_desc, *tensor_info.tensor_desc_ptr.get(), tbe_tensor);
  }

  ge::Format primary_origin_format =
          static_cast<ge::Format>(ge::GetPrimaryFormat(tensor_info.tensor_desc_ptr->GetOriginFormat()));
  tbe_tensor.SetOriginFormat(ge::TypeUtils::FormatToSerialString(primary_origin_format));
  tbe_tensor.SetOriginShape(tensor_info.tensor_desc_ptr->GetOriginShape().GetDims());
  string is_input = IS_INPUT_TO_STRING(tensor_info.is_input);
  FE_LOGD("Op[name=%s,type=%s]: origin %s format is [%s], the index_in_op_kernel is [%zu].", op_desc.GetName().c_str(),
          op_desc.GetType().c_str(), is_input.c_str(),
          ge::TypeUtils::FormatToSerialString(tensor_info.tensor_desc_ptr->GetOriginFormat()).c_str(),
          tensor_info.index_in_op_kernel);
  string origin_shape_dims = GetShapeDims(tensor_info.tensor_desc_ptr->GetOriginShape());
  FE_LOGD("Op[name=%s,type=%s]: origin %s shape is [%s].", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
          is_input.c_str(), origin_shape_dims.c_str());
}

Status SetTbeTensor(const ge::OpDesc &op_desc, const TensorDescAndIndex &tensor_info, te::TbeOpTensor &input_tensor) {
  Status ret = CreateTbeTensor(op_desc, tensor_info, input_tensor);
  if (ret != SUCCESS) {
    return ret;
  }

  if (tensor_info.is_input) {
    input_tensor.SetFirstLayer(tensor_info.is_first_layer_conv);
  }

  /* Set original format and dtype */
  SetTbeTensorShape(op_desc, tensor_info, input_tensor);
  return SUCCESS;
}

Status TbeInfoAssembler::ConvertInputsToTbeOpInfo(const ge::OpDesc &op_desc, IndexNameMap &input_idx_name_map,
                                                  OpKernelInfoPtr op_kernel_info_ptr,
                                                  const HeavyFormatInfo &heavy_format_info,
                                                  te::TbeOpInfo &op_info) const {
  auto input_info_in_opkernel = op_kernel_info_ptr->GetAllInputInfo();
  auto input_size_in_op_kernel = input_info_in_opkernel.size();
  bool is_first_layer_conv = false;
  (void)ge::AttrUtils::GetBool(op_desc, IS_FIRST_LAYER_CONV, is_first_layer_conv);

  te::TbeOpParam input;
  for (size_t index_in_op_kernel = 0; index_in_op_kernel < input_size_in_op_kernel; index_in_op_kernel++) {
    auto input_info_ptr = input_info_in_opkernel.at(index_in_op_kernel);
    FE_CHECK(input_info_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleConstVal]inputInfoPtr is \
                    nullptr."),
             return FAILED);
    string input_name_in_op_kernel = input_info_ptr->GetName();

    vector<uint32_t> specific_input_index_vec_in_op_desc;
    if (GetSpecificIndex(op_desc, input_idx_name_map, input_name_in_op_kernel, true,
                         specific_input_index_vec_in_op_desc) != SUCCESS) {
      REPORT_FE_ERROR(
          "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Get input name [%s] from \
                      inputIdxNameMap failed.",
          op_desc.GetName().c_str(), op_desc.GetType().c_str(), input_name_in_op_kernel.c_str());
      return FAILED;
    }

    auto size_of_this_input = specific_input_index_vec_in_op_desc.size();
    bool add_tensor_info = GetAddTensorFlag(op_desc, specific_input_index_vec_in_op_desc);
    if (add_tensor_info) {
      uint32_t count = 0;
      for (uint32_t index_in_opdesc : specific_input_index_vec_in_op_desc) {
        te::TbeOpTensor input_tensor;
        auto input_i = op_desc.MutableInputDesc(index_in_opdesc);
        if (input_i == nullptr) {
          FE_LOGD("[SubGraphOpt][PreCompileOp][AssembleConstVal]input_i is nullptr.");
          continue;
        }
        TensorDescAndIndex tensor_info = {input_i,
                                          input_name_in_op_kernel,
                                          index_in_op_kernel,
                                          index_in_opdesc,
                                          true,
                                          heavy_format_info.expected_heavy_format,
                                          heavy_format_info.sub_format,
                                          is_first_layer_conv};
        Status ret = SetTbeTensor(op_desc, tensor_info, input_tensor);
        if (ret != SUCCESS) {
          return ret;
        }

        // take care that danamic input have multiple sub-inputs.
        if (ConvertParameterInfoForInput(input_info_ptr, input, input_tensor, op_info,
                                         (count == size_of_this_input - 1)) != SUCCESS) {
          REPORT_FE_ERROR(
              "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Failed to feed normal \
                          input %zu to tbe.",
              op_desc.GetName().c_str(), op_desc.GetType().c_str(), index_in_op_kernel);
          return FAILED;
        }
        count++;
      }
    } else {
      if (FeedParameterInfoForNotFound(input_info_ptr, STR_INPUT_LOWERCASE, input, op_info) != SUCCESS) {
        REPORT_FE_ERROR(
            "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Failed to feed dummy input %zu \
                          to tbe.",
            op_desc.GetName().c_str(), op_desc.GetType().c_str(), index_in_op_kernel);
        return FAILED;
      }
      FE_LOGI("Optional input %zu named [%s] is missing in Opdesc, we set a empty TbeOpTensor.", index_in_op_kernel,
              input_info_ptr->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status TbeInfoAssembler::ConvertInputsToTbeOpInfo(const ge::NodePtr &node, IndexNameMap &input_idx_name_map,
                                                  OpKernelInfoPtr op_kernel_info_ptr,
                                                  const HeavyFormatInfo &heavy_format_info,
                                                  te::TbeOpInfo &op_info) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  auto input_info_in_opkernel = op_kernel_info_ptr->GetAllInputInfo();
  auto input_size_in_op_kernel = input_info_in_opkernel.size();
  bool is_first_layer_conv = false;
  (void)ge::AttrUtils::GetBool(op_desc, IS_FIRST_LAYER_CONV, is_first_layer_conv);

  te::TbeOpParam input;
  for (size_t index_in_op_kernel = 0; index_in_op_kernel < input_size_in_op_kernel; index_in_op_kernel++) {
    auto input_info_ptr = input_info_in_opkernel.at(index_in_op_kernel);
    FE_CHECK(input_info_ptr == nullptr,
             REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleConstVal]inputInfoPtr is nullptr."),
             return FAILED);
    string input_name_in_op_kernel = input_info_ptr->GetName();

    vector<uint32_t> specific_input_index_vec_in_op_desc;
    if (GetSpecificIndex(op_desc, input_idx_name_map, input_name_in_op_kernel, true,
                         specific_input_index_vec_in_op_desc) != SUCCESS) {
      REPORT_FE_ERROR(
          "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Get input name [%s] from \
                      inputIdxNameMap failed.",
          op_desc.GetName().c_str(), op_desc.GetType().c_str(), input_name_in_op_kernel.c_str());
      return FAILED;
    }

    auto size_of_this_input = specific_input_index_vec_in_op_desc.size();
    bool add_tensor_info = GetAddTensorFlag(op_desc, specific_input_index_vec_in_op_desc);
    if (add_tensor_info) {
      uint32_t count = 0;
      for (uint32_t index_in_opdesc : specific_input_index_vec_in_op_desc) {
        te::TbeOpTensor input_tensor;
        auto input_i = op_desc.MutableInputDesc(index_in_opdesc);
        if (input_i == nullptr) {
          FE_LOGD("[SubGraphOpt][PreCompileOp][AssembleConstVal]input_i is nullptr.");
          continue;
        }
        TensorDescAndIndex tensor_info = {input_i,
                                          input_name_in_op_kernel,
                                          index_in_op_kernel,
                                          index_in_opdesc,
                                          true,
                                          heavy_format_info.expected_heavy_format,
                                          heavy_format_info.sub_format,
                                          is_first_layer_conv};
        Status ret = SetTbeTensor(op_desc, tensor_info, input_tensor);
        if (ret != SUCCESS) {
          return ret;
        }

        if (SetTensorConstValue(node.get(), index_in_opdesc, input_info_ptr, input_tensor) != SUCCESS) {
          REPORT_FE_ERROR(
              "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s, type %s]: Fail to set value for input[%u].",
              op_desc.GetName().c_str(), op_desc.GetType().c_str(), index_in_opdesc);
          return FAILED;
        }

        // take care that danamic input have multiple sub-inputs.
        if (ConvertParameterInfoForInput(input_info_ptr, input, input_tensor, op_info,
                                         (count == size_of_this_input - 1)) != SUCCESS) {
          REPORT_FE_ERROR(
              "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Failed to feed normal \
                          input %zu to tbe.",
              op_desc.GetName().c_str(), op_desc.GetType().c_str(), index_in_op_kernel);
          return FAILED;
        }
        count++;
      }
    } else {
      if (FeedParameterInfoForNotFound(input_info_ptr, STR_INPUT_LOWERCASE, input, op_info) != SUCCESS) {
        REPORT_FE_ERROR(
            "[SubGraphOpt][PreCompileOp][AssembleConstVal][Op %s,type %s]: Failed to feed dummy input %zu \
                          to tbe.",
            op_desc.GetName().c_str(), op_desc.GetType().c_str(), index_in_op_kernel);
        return FAILED;
      }
      FE_LOGI("Optional input %zu named [%s] is missing in Opdesc, we set a empty TbeOpTensor.", index_in_op_kernel,
              input_info_ptr->GetName().c_str());
    }
  }
  return SUCCESS;
}

void TbeInfoAssembler::FeedFusionOutputTensor(const ToOpStructPtr &fusion_info, const ge::OpDescPtr &op_desc,
                                              IndexNameMap &output_idx_name_map, const uint32_t &index_in_opdesc,
                                              te::TbeOpTensor &output_tensor) const {
  // update opinfo
  if (!fusion_info->op_l1_fusion_type.empty()) {
    output_tensor.SetL1FusionType(fusion_info->op_l1_fusion_type[0]);
  }

  size_t output_idx_name_size = output_idx_name_map.size();
  size_t slice_output_shape_size = fusion_info->slice_output_shape.size();
  if (slice_output_shape_size == output_idx_name_size) {
    if (index_in_opdesc >= slice_output_shape_size) {
      FE_LOGW("index_in_opdesc >= valid_output_shape_size, index_in_opdesc:%u, valid_output_shape_size:%zu.",
              index_in_opdesc, slice_output_shape_size);
      return;
    }
    output_tensor.SetValidShape(fusion_info->slice_output_shape[index_in_opdesc]);
  }

  size_t slice_output_offset_size = fusion_info->slice_output_offset.size();
  if (slice_output_offset_size == output_idx_name_size) {
    if (index_in_opdesc >= slice_output_offset_size) {
      FE_LOGW("index_in_opdesc >= slice_output_offset_size, index_in_opdesc:%u, slice_output_offset_size:%zu.",
              index_in_opdesc, slice_output_offset_size);
      return;
    }
    output_tensor.SetSliceOffset(fusion_info->slice_output_offset[index_in_opdesc]);
  }

  /* Set addr  offset */
  size_t output_num = op_desc->GetOutputsSize();
  vector<int64_t> output_offsets = op_desc->GetOutputOffset();
  size_t output_offset_size = output_offsets.size();
  if (output_num > output_offset_size) {
    FE_LOGW("output_desc_size > output_offset_size, output_desc_size:%lu, output_offset_size:%lu.", output_num,
            output_offset_size);
  } else {
    if (index_in_opdesc >= output_offset_size) {
      FE_LOGW("index_in_opdesc >= output_offset_size, index_in_opdesc:%u, output_offset_size:%lu.", index_in_opdesc,
              output_offset_size);
      return;
    }
    output_tensor.SetAddrOffset(output_offsets[index_in_opdesc]);
  }
}

Status TbeInfoAssembler::FeedOutputsToTbeOpInfo(const ge::Node *node, IndexNameMap &output_idx_name_map,
                                                OpKernelInfoPtr op_kernel_info_ptr, te::TbeOpInfo &op_info) const {
  auto op = node->GetOpDesc();
  auto output_info_in_opkernel = op_kernel_info_ptr->GetAllOutputInfo();
  auto output_size_in_op_kernel = output_info_in_opkernel.size();
  vector<int32_t> memery_type_vec;
  ToOpStructPtr l1_info;
  FE_MAKE_SHARED(l1_info = std::make_shared<ToOpStruct_t>(), return FAILED);
  ToOpStructPtr l2_info;
  FE_MAKE_SHARED(l2_info = std::make_shared<ToOpStruct_t>(), return FAILED);
  (void)ge::AttrUtils::GetListInt(op, ge::ATTR_NAME_OUTPUT_MEM_TYPE_LIST, memery_type_vec);
  auto memery_type_vec_size = memery_type_vec.size();
  (void)GetL1ToOpStructFromJson(op, l1_info);
  (void)GetL2ToOpStructFromJson(op, l2_info);

  te::TbeOpParam output;
  auto output_size = op->GetOutputsSize();
  for (uint32_t index_in_op_kernel = 0; index_in_op_kernel < output_size_in_op_kernel; index_in_op_kernel++) {
    auto output_info_ptr = output_info_in_opkernel.at(index_in_op_kernel);
    FE_CHECK(output_info_ptr == nullptr, REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][FeedOutputs] OutputInfoPtr \
                    is nullptr."),
             return FAILED);
    string output_name_in_op_kernel = output_info_ptr->GetName();

    vector<uint32_t> specific_output_index_vec_in_op_desc;
    if (GetSpecificIndex(*op.get(), output_idx_name_map, output_name_in_op_kernel, false,
                         specific_output_index_vec_in_op_desc) != SUCCESS) {
      REPORT_FE_ERROR(
          "[SubGraphOpt][PreCompileOp][FeedOutputs][Op %s,type %s]: Get output name [%s] from \
                      output_idx_name_map failed.",
          output_name_in_op_kernel.c_str(), op->GetName().c_str(), op->GetType().c_str());
      return FAILED;
    }
    auto size_of_this_output = specific_output_index_vec_in_op_desc.size();
    // dynamic, required, optional
    if (size_of_this_output != 0) {
      uint32_t count = 0;
      for (uint32_t index_in_opdesc : specific_output_index_vec_in_op_desc) {
        te::TbeOpTensor output_tensor;
        auto output_desc = op->MutableOutputDesc(index_in_opdesc);
        if (output_desc == nullptr) {
          continue;
        }
        TensorDescAndIndex tensor_info = {output_desc, output_name_in_op_kernel, index_in_op_kernel, index_in_opdesc,
                                          false};
        Status ret = CreateTbeTensor(*op.get(), tensor_info, output_tensor);
        if (ret != SUCCESS) {
          return ret;
        }
        FE_LOGD("get attr l1 userinfo, l1 info size %zu, input size %zu.", memery_type_vec_size, output_size);

        if (memery_type_vec_size == output_size) {
          output_tensor.SetAddrType(memery_type_vec[index_in_opdesc]);
        }
        if (l1_info != nullptr) {
          FeedFusionOutputTensor(l1_info, op, output_idx_name_map, index_in_opdesc, output_tensor);
        }

        if (l2_info != nullptr) {
          FeedFusionOutputTensor(l2_info, op, output_idx_name_map, index_in_opdesc, output_tensor);
        }

        SetTbeTensorShape(*op.get(), tensor_info, output_tensor);
        // take care that danamic output have multiple sub-outputs.
        if (FeedParameterInfoForOutput(*(node->GetOpDesc().get()), *(output_desc.get()), output_info_ptr,
                                       (count == size_of_this_output - 1), output_tensor, output, op_info) != SUCCESS) {
          REPORT_FE_ERROR(
              "[SubGraphOpt][PreCompileOp][FeedOutputs][Op %s,type %s]: FeedDynamicOutputsToTbeOpInfo \
                          failed.",
              op->GetName().c_str(), op->GetType().c_str());
          return FAILED;
        }
        count++;
      }
    } else {
      if (FeedParameterInfoForNotFound(output_info_ptr, STR_OUTPUT_LOWERCASE, output, op_info) != SUCCESS) {
        REPORT_FE_ERROR(
            "[SubGraphOpt][PreCompileOp][FeedOutputs][Op %s,type %s]: FeedDynamicOutputsToTbeOpInfo \
                          failed.",
            op->GetName().c_str(), op->GetType().c_str());
        return FAILED;
      }
      FE_LOGD("Optional output %u named [%s] is missing in Opdesc, we set a empty TbeOpTensor.", index_in_op_kernel,
              output_info_ptr->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status TbeInfoAssembler::ConvertOutputsToTbeOpInfo(const ge::OpDesc &op_desc, IndexNameMap &output_idx_name_map,
                                                   OpKernelInfoPtr op_kernel_info_ptr,
                                                   const HeavyFormatInfo &heavy_format_info,
                                                   te::TbeOpInfo &op_info) const {
  auto output_info_in_opkernel = op_kernel_info_ptr->GetAllOutputInfo();
  auto output_size_in_op_kernel = output_info_in_opkernel.size();

  te::TbeOpParam output;
  for (uint32_t index_in_op_kernel = 0; index_in_op_kernel < output_size_in_op_kernel; index_in_op_kernel++) {
    auto output_info_ptr = output_info_in_opkernel.at(index_in_op_kernel);
    FE_CHECK(output_info_ptr == nullptr,
             REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][ConvertOutputs] OutputInfoPtr is nullptr."),
             return FAILED);
    string output_name = output_info_ptr->GetName();

    vector<uint32_t> specific_output_index_vec_in_op_desc;
    if (GetSpecificIndex(op_desc, output_idx_name_map, output_name, false, specific_output_index_vec_in_op_desc) !=
        SUCCESS) {
      REPORT_FE_ERROR(
          "[SubGraphOpt][PreCompileOp][ConvertOutputs][Op %s,type %s]: Get output name [%s] of from \
                      output_idx_name_map failed.",
          op_desc.GetName().c_str(), op_desc.GetType().c_str(), output_name.c_str());
      return FAILED;
    }
    auto size_of_this_output = specific_output_index_vec_in_op_desc.size();
    if (size_of_this_output != 0) {
      uint32_t count = 0;
      for (uint32_t index_in_opdesc : specific_output_index_vec_in_op_desc) {
        te::TbeOpTensor output_tensor;
        auto output_desc = op_desc.MutableOutputDesc(index_in_opdesc);
        if (output_desc == nullptr) {
          continue;
        }
        TensorDescAndIndex tensor_info = {output_desc,
                                          output_name,
                                          index_in_op_kernel,
                                          index_in_opdesc,
                                          false,
                                          heavy_format_info.expected_heavy_format,
                                          heavy_format_info.sub_format};
        Status ret = SetTbeTensor(op_desc, tensor_info, output_tensor);
        if (ret != SUCCESS) {
          return ret;
        }

        // take care that danamic output have multiple sub-outputs.
        if (FeedParameterInfoForOutput(op_desc, *(output_desc.get()), output_info_ptr,
                                       (count == size_of_this_output - 1), output_tensor, output, op_info) != SUCCESS) {
          REPORT_FE_ERROR(
              "[SubGraphOpt][PreCompileOp][ConvertOutputs][Op %s,type %s]: FeedDynamicOutputsToTbeOpInfo \
                          failed.",
              op_desc.GetName().c_str(), op_desc.GetType().c_str());
          return FAILED;
        }
        count++;
      }
    } else {
      if (FeedParameterInfoForNotFound(output_info_ptr, STR_OUTPUT_LOWERCASE, output, op_info) != SUCCESS) {
        REPORT_FE_ERROR(
            "[SubGraphOpt][PreCompileOp][ConvertOutputs][Op %s,type %s]: FeedDynamicOutputsToTbeOpInfo \
                          failed.",
            op_desc.GetName().c_str(), op_desc.GetType().c_str());
        return FAILED;
      }
      FE_LOGI("Optional output %u named [%s] is missing in Opdesc, we set a empty TbeOpTensor.", index_in_op_kernel,
              output_info_ptr->GetName().c_str());
    }
  }
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   set Attrs to tbe_op_info
 *  @param   [in]  op              op desc
 *  @param   [in]  op_kernel_info_ptr op kernel info
 *  @param   [in/out]  op_info      tbe data item
 *  @return  SUCCESS or FAILED
 */
Status TbeInfoAssembler::FeedAttrsToTbeOpInfo(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                                              te::TbeOpInfo &op_info) const {
  // load op info store and get all attr list_fe_ops_kernel_info_store
  std::vector<AttrInfoPtr> attrs_info = op_kernel_info_ptr->GetVecAttrInfo();
  // loop over attr list and set each of them to TbeOpInfo
  for (auto iter : attrs_info) {
    te::TbeAttrValue attr_value;
    string attr_name = iter->GetAttrName();
    auto func = k_attr_get_funcs.find(iter->GetAttrDType());
    if (func == k_attr_get_funcs.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][FeedAttr][Op %s, type=%s]: dtype %u of attr %s is invalid.",
                      op_desc.GetName().c_str(), op_desc.GetType().c_str(), iter->GetAttrDType(), attr_name.c_str());
      return FAILED;
    } else {
      if (func->second(op_desc, attr_name, attr_value, iter) != SUCCESS) {
        return FAILED;
      }
    }
    bool has_default_value = iter->GetDefaultValueDefinedFlag();
    bool support_all_value = iter->GetSupportAllValue();
    attr_value.SetAttrSupAllFlag(support_all_value);
    attr_value.SetAttrHasDefaultValFlag(has_default_value);
    op_info.AddAttrValue(attr_value);
  }
  return SUCCESS;
}

Status TbeInfoAssembler::JudgeShapeToSetFlag(const ge::OpDescPtr &op_desc, te::TbeOpInfo &op_info,
                                             bool &flag, string in_out) const {
  if (flag) {
    return SUCCESS;
  }
  size_t size = 0;
  if (in_out == "in") {
    size = op_desc->GetInputsSize();
  } else {
    size = op_desc->GetOutputsSize();
  }
  for (size_t i = 0; i < size; i++) {
    auto in_out_desc = in_out == "in" ? op_desc->MutableInputDesc(i) : op_desc->MutableOutputDesc(i);
    if (in_out_desc != nullptr) {
      vector<int64_t> dims = in_out_desc->MutableShape().GetDims();
      ge::DataType data_type = in_out_desc->GetDataType();
      FE_CHECK(data_type == ge::DT_UNDEFINED || data_type >= ge::DT_MAX,
               REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][JudgeShape] dataType UNDEFINED."),
               return FAILED);
      int64_t data_size = static_cast<int64_t>(ge::GetSizeByDataType(data_type));
      int64_t sum = 1;
      int64_t max_int32 = INT32_MAX;
      for (int64_t dim : dims) {
        FE_INT64_MULCHECK(sum, dim);
        sum *= dim;
        if (sum > max_int32) {
          flag = true;
          op_info.SetFlagUseInt64(true);
          break;
        }
      }
      FE_LOGD("Shape multiplication of %s-%zu is %ld, data type size is %ld.", in_out.c_str(), i, sum, data_size);
      FE_INT64_MULCHECK(sum, data_size);
      if (!flag && sum * data_size > max_int32) {
        flag = true;
        op_info.SetFlagUseInt64(true);
      }
    }
    if (flag) {
      break;
    }
  }
  return SUCCESS;
}

Status TbeInfoAssembler::FeedFlagInt64ToTbeOpInfo(const ge::Node *node, te::TbeOpInfo &op_info) const {
  auto op_desc = node->GetOpDesc();
  bool flag = false;
  (void)JudgeShapeToSetFlag(op_desc, op_info, flag, "in");
  (void)JudgeShapeToSetFlag(op_desc, op_info, flag, "out");
  FE_LOGD("[node : %s] AssembleTbeInfo FlagUseInt64 : %d", node->GetName().c_str(), op_info.GetFlagUseInt64());
  return SUCCESS;
}

Status TbeInfoAssembler::FeedIsUnknownShapeToTbeOpInfo(const ge::OpDesc &op_desc,
                                                       te::TbeOpInfo &op_info) const {
  int64_t is_unknown_shape_value = 0;
  (void)ge::AttrUtils::GetInt(op_desc, ATTR_NAME_IS_UNKNOWN_SHAPE_OP, is_unknown_shape_value);
  FE_LOGD("Op[name=%s,type=%s]: is_unknown_shape flag is %ld.", op_desc.GetName().c_str(), op_desc.GetType().c_str(),
          is_unknown_shape_value);
  if (is_unknown_shape_value == IS_UNKNOWN_SHAPE_VALUE) {
    op_info.SetIsUnknownShape(true);
  }
  return SUCCESS;
}

void TbeInfoAssembler::SetTbeInfoLimitedRange(const OpKernelInfoPtr &op_kernel_info_ptr,
    te::TbeOpInfo &op_info) const {
  const std::string limited_range = op_kernel_info_ptr->GetRangeLimitValue();
  FE_LOGD("Limited_range is %s.", limited_range.c_str());
  op_info.SetLimitedRange(limited_range);
}

void TbeInfoAssembler::SetExtraParams(const ge::OpDesc &op_desc, te::TbeOpInfo &op_info) const {
  std::string extra_params;
  (void)ge::AttrUtils::GetStr(op_desc, kAttrExtraParams, extra_params);
  if (!extra_params.empty()) {
    FE_LOGD("Node[%s] has extra_params[%s].", op_desc.GetName().c_str(), extra_params.c_str());
  }
  op_info.SetExtraParams(extra_params);
}

Status TbeInfoAssembler::AssembleTbeInfo(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                                         const std::string &engine_name, te::TbeOpInfo &tbe_op_info) {
  HeavyFormatInfo heavy_format_info;
  return AssembleTbeInfo(node, op_kernel_info_ptr, heavy_format_info, engine_name, tbe_op_info);
}

Status TbeInfoAssembler::AssembleTbeInfo(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                                         const HeavyFormatInfo &heavy_format_info, const std::string &engine_name,
                                         te::TbeOpInfo &tbe_op_info) {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  ge::OpDesc &op_desc = *(op_desc_ptr.get());
  // 1. set the opFileName and the op_func_name
  string opFileName = op_kernel_info_ptr->GetOpInfo().opFileName;
  string opFuncName = op_kernel_info_ptr->GetOpInfo().opFuncName;
  tbe_op_info.SetOpFileName(opFileName);
  tbe_op_info.SetOpFuncName(opFuncName);
  FE_LOGD("Op[name=%s,type=%s]: tbe_op_info.opFileName=[%s], tbe_op_info.opFuncName=[%s].", op_desc.GetName().c_str(),
          op_desc.GetType().c_str(), opFileName.c_str(), opFuncName.c_str());

  IndexNameMap input_map;
  IndexNameMap output_map;
  if (GetInputOutputNameMap(op_desc, op_kernel_info_ptr, input_map, output_map) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s,type %s]: GetInputOutputNameMap failed.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // 2. feed all inputs to TbeOpInfo
  if (ConvertInputsToTbeOpInfo(node, input_map, op_kernel_info_ptr, heavy_format_info, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to assemble input info.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // 3. feed all outputs to TbeOpInfo
  if (ConvertOutputsToTbeOpInfo(op_desc, output_map, op_kernel_info_ptr, heavy_format_info, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to assemble output info.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // 4. feed all attrs to TbeOpInfo
  if (FeedAttrsToTbeOpInfo(op_desc, op_kernel_info_ptr, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to feedAttrsToTbeOpInfo.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // feed all options to TbeOpInfo
  map<std::string, std::string> options = GetAllOptionsForTBE(engine_name);
  tbe_op_info.SetOptions(options);

  SetExtraParams(*op_desc_ptr, tbe_op_info);
  SetTbeInfoLimitedRange(op_kernel_info_ptr, tbe_op_info);

  if (IsFuzzBuildOp(op_desc)) {
    tbe_op_info.SetBuildType(te::FUZZILY_BUILD);
  }

  return SUCCESS;
}

Status TbeInfoAssembler::AssembleTbeInfo(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                                         const HeavyFormatInfo &heavy_format_info, te::TbeOpInfo &tbe_op_info,
                                         const std::string &engine_name) {
  // 1. set the opFileName and the op_func_name
  string opFileName = op_kernel_info_ptr->GetOpInfo().opFileName;
  string opFuncName = op_kernel_info_ptr->GetOpInfo().opFuncName;
  tbe_op_info.SetOpFileName(opFileName);
  tbe_op_info.SetOpFuncName(opFuncName);
  FE_LOGD("Op[name=%s,type=%s]: tbe_op_info.opFileName=[%s], tbe_op_info.opFuncName=[%s].", op_desc.GetName().c_str(),
          op_desc.GetType().c_str(), opFileName.c_str(), opFuncName.c_str());

  IndexNameMap input_map;
  IndexNameMap output_map;
  if (GetInputOutputNameMap(op_desc, op_kernel_info_ptr, input_map, output_map) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s,type %s]: GetInputOutputNameMap failed.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // 2. feed all inputs to TbeOpInfo
  if (ConvertInputsToTbeOpInfo(op_desc, input_map, op_kernel_info_ptr, heavy_format_info, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to assemble input info.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // 3. feed all outputs to TbeOpInfo
  if (ConvertOutputsToTbeOpInfo(op_desc, output_map, op_kernel_info_ptr, heavy_format_info, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to assemble output info.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // 4. feed all attrs to TbeOpInfo
  if (FeedAttrsToTbeOpInfo(op_desc, op_kernel_info_ptr, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to feedAttrsToTbeOpInfo.",
                    op_desc.GetName().c_str(), op_desc.GetType().c_str());
    return FAILED;
  }

  // feed all options to TbeOpInfo
  map<std::string, std::string> options = GetAllOptionsForTBE(engine_name);
  tbe_op_info.SetOptions(options);

  SetExtraParams(op_desc, tbe_op_info);
  SetTbeInfoLimitedRange(op_kernel_info_ptr, tbe_op_info);

  if (IsFuzzBuildOp(op_desc)) {
    tbe_op_info.SetBuildType(te::FUZZILY_BUILD);
  }

  return SUCCESS;
}

void TbeInfoAssembler::GetOpPrecisionMode(const map<std::string, std::string> &options,
                                          std::string &op_precision_mode_str) const {
  auto iter_op_implmode = options.find(ge::OP_SELECT_IMPL_MODE);
  auto iter_optypelist = options.find(ge::OPTYPELIST_FOR_IMPLMODE);
  if (iter_op_implmode == options.end() || iter_optypelist == options.end()) {
    return;
  }
  std::string op_implmode = iter_op_implmode->second;
  if (op_implmode.empty() || iter_optypelist->second.empty()) {
    return;
  }
  if (op_implmode != "high_precision" && op_implmode != "high_performance") {
    return;
  }
  std::vector<std::string> op_type_list = StringUtils::Split(iter_optypelist->second, ',');
  if (op_type_list.empty()) {
    return;
  }
  std::ostringstream oss;
  bool is_first = true;
  for (std::string &op_type : op_type_list) {
    if (is_first) {
      is_first = false;
    } else {
      oss << ",";
    }
    oss << StringUtils::Trim(op_type) << ":" << op_implmode;
  }
  op_precision_mode_str = oss.str();
}

map<std::string, std::string> TbeInfoAssembler::GetAllOptionsForTBE(const string &engine_name) const {
  map<std::string, std::string> options = ge::GetThreadLocalContext().GetAllOptions();
  auto iter = options.find(ge::AUTO_TUNE_MODE);
  if (iter != options.end()) {
    options.insert(std::pair<string, string>(TBE_AUTO_TILING_MODE, iter->second));
  }

  iter = options.find(ge::OPTION_EXEC_DEVICE_ID);
  if (iter != options.end()) {
    options.insert(std::pair<string, string>(TBE_DEVICE_ID, iter->second));
  }

  iter = options.find(OP_STATUS_CHECK);
  if (iter != options.end()) {
    options.insert(std::pair<string, string>(OP_STATUS_CHECK, iter->second));
  } else {
    options.insert(std::pair<string, string>(OP_STATUS_CHECK, "false"));
  }

  AppendArgsMode append_args_mode = Configuration::Instance(engine_name).GetAppendArgsMode();
  string append_args_mode_str = std::to_string(static_cast<int>(append_args_mode));
  options.insert(std::pair<string, string>(TBE_APPEND_ARGS_MODE, append_args_mode_str));

  std::string op_precision_mode_str;
  GetOpPrecisionMode(options, op_precision_mode_str);
  if (op_precision_mode_str.empty()) {
    Configuration::Instance(engine_name).GetOpSelectImplModeStr(op_precision_mode_str);
  }
  options.insert(std::pair<std::string, std::string>(kOpPrecisionModeStr, op_precision_mode_str));

  options.insert(std::pair<string, string>(kFeEngineType, engine_name));
  FE_LOGD("fe.engineType is %s", engine_name.c_str());

  /* Alter op debug level when the global
   * debug switch is configurated. */
  const char * const npu_collect_path_env = std::getenv("NPU_COLLECT_PATH");
  if (npu_collect_path_env != nullptr) {
    const std::string npu_collect_path(npu_collect_path_env);
    if (!npu_collect_path.empty()) {
      FE_LOGD("Set op debug level as 2 when npu collect path is %s.",
              npu_collect_path.c_str());
      options[ge::OP_DEBUG_LEVEL] = kDebugMode;
    }
  }
  return options;
}

Status TbeInfoAssembler::AssembleTbeInfo(ge::Node *node, const OpKernelInfoPtr &op_kernel_info_ptr,
                                         te::TbeOpInfo &tbe_op_info, const string &engine_name) {
  // set op_info
  tbe_op_info.SetOpFileName(op_kernel_info_ptr->GetOpInfo().opFileName);
  tbe_op_info.SetOpFuncName(op_kernel_info_ptr->GetOpInfo().opFuncName);
  tbe_op_info.SetDyRankSupFlag(op_kernel_info_ptr->GetSupportDynamicRank());
  const std::map<OpPattern, std::string>::const_iterator iter =
      PATTERN_TO_STRING_MAP.find(op_kernel_info_ptr->GetOpPattern());
  if (iter == PATTERN_TO_STRING_MAP.end()) {
    FE_LOGW("[AssembleTbeInfo] Fail to set op[%s] pattern", node->GetName().c_str());
  } else {
    const std::string &pattern_str = iter->second;
    FE_LOGD("[AssembleTbeInfo] Succeed to set op[%s] pattern[%s]", node->GetName().c_str(), pattern_str.c_str());
    tbe_op_info.SetPattern(pattern_str);
  }

  IndexNameMap input_map;
  IndexNameMap output_map;
  auto op = node->GetOpDesc();
  if (GetInputOutputNameMap(*(op.get()), op_kernel_info_ptr, input_map, output_map) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to getInputOutputNameMap.",
                    op->GetName().c_str(), op->GetType().c_str());
    return FAILED;
  }

  // feed all inputs to TbeOpInfo
  if (FeedInputsToTbeOpInfo(node, input_map, op_kernel_info_ptr, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to feedInputsToTbeOpInfo.",
                    op->GetName().c_str(), op->GetType().c_str());
    return FAILED;
  }

  // feed all outputs to TbeOpInfo
  if (FeedOutputsToTbeOpInfo(node, output_map, op_kernel_info_ptr, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to feedOutputsToTbeOpInfo.",
                    op->GetName().c_str(), op->GetType().c_str());
    return FAILED;
  }

  // feed all outputs to TbeOpInfo
  if (FeedAttrsToTbeOpInfo(*(op.get()), op_kernel_info_ptr, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to feedAttrsToTbeOpInfo.",
                    op->GetName().c_str(), op->GetType().c_str());
    return FAILED;
  }

  if (FeedFlagInt64ToTbeOpInfo(node, tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: AssembleTbeInfo \
                    FeedFlagInt64ToTbeOpInfo failed.",
        op->GetName().c_str(), op->GetType().c_str());
    return FAILED;
  }

  // feed all IsUnknownShape to TbeOpInfo
  if (FeedIsUnknownShapeToTbeOpInfo(*(op.get()), tbe_op_info) != SUCCESS) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][PreCompileOp][AssembleInput][Op %s, type %s]: failed to \
                    feedIsUnknownShapeToTbeOpInfo.",
        op->GetName().c_str(), op->GetType().c_str());
    return FAILED;
  }

  // feed all options to TbeOpInfo
  map<std::string, std::string> options = GetAllOptionsForTBE(engine_name);
  ge::AttrUtils::SetStr(op, kAttrEngineType, engine_name);
  FE_LOGD("Set Node %s's engine_name as %s.", op->GetName().c_str(), engine_name.c_str());
  tbe_op_info.SetOptions(options);
  SetExtraParams(*op, tbe_op_info);
  ToOpStructPtr l1_info;
  FE_MAKE_SHARED(l1_info = std::make_shared<ToOpStruct_t>(), return FAILED);
  ToOpStructPtr l2_info;
  FE_MAKE_SHARED(l2_info = std::make_shared<ToOpStruct_t>(), return FAILED);

  GetL1ToOpStructFromJson(op, l1_info);
  GetL2ToOpStructFromJson(op, l2_info);

  if (l1_info != nullptr) {
    FE_LOGD("get attr l1 userinfo, op name = %s, op type = %s, op_l1_space = %ld.", op->GetName().c_str(),
            op->GetType().c_str(), l1_info->op_l1_space);
    if (!l1_info->op_l1_fusion_type.empty()) {
      FE_LOGD("opL1fusionType = %ld.", l1_info->op_l1_fusion_type[0]);
    }
    tbe_op_info.SetL1Space(l1_info->op_l1_space);
    DumpL1Attr(node);
    DumpOpInfo(tbe_op_info);
  } else {
    FE_LOGD("l1Info is null_ptr, op name = %s, op type = %s.", op->GetName().c_str(), op->GetType().c_str());
  }
  if (l2_info != nullptr) {
    DumpL2Attr(node);
    DumpOpInfo(tbe_op_info);
  }
  return SUCCESS;
}

Status TbeInfoAssembler::GetSpecificIndex(const ge::OpDesc &op_desc, const IndexNameMap &name_map,
                                          const std::string &input_name_in_op_kernel, bool is_input,
                                          vector<uint32_t> &specific_input_index) const {
  for (const auto &inOrOutDesc : name_map) {
    if (inOrOutDesc.second == input_name_in_op_kernel) {
      if (is_input && inOrOutDesc.first < op_desc.GetAllInputsSize() &&
          op_desc.MutableInputDesc(inOrOutDesc.first) != nullptr) {
        specific_input_index.push_back(inOrOutDesc.first);
      }
      if (!is_input && inOrOutDesc.first < op_desc.GetAllOutputsDescSize() &&
          op_desc.MutableOutputDesc(inOrOutDesc.first) != nullptr) {
        specific_input_index.push_back(inOrOutDesc.first);
      }
    }
  }
  return SUCCESS;
}

template <typename T>
void TbeInfoAssembler::GetConstValueVec(ge::GeTensorPtr &const_tensor_ptr, vector<T> &const_data_vec) {
  T *const_data_ptr = (T *)const_tensor_ptr->GetData().GetData();
  if (const_data_ptr == nullptr) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][GetConstVal] const data ptr is null.");
    return;
  }
  size_t size = const_tensor_ptr->GetData().GetSize() / sizeof(T);
  for (size_t i = 0; i < size; ++i) {
    T const_data = *(const_data_ptr + i);
    const_data_vec.push_back(const_data);
  }
}

Status TbeInfoAssembler::SetConstValueWithFloat16(ge::GeTensorPtr tensor_ptr, const std::string &tensor_name,
                                                  te::TbeOpTensor &op_tensor) {
  uint16_t *const_data_ptr = (uint16_t *)tensor_ptr->GetData().GetData();
  FE_CHECK_NOTNULL(const_data_ptr);
  size_t size = tensor_ptr->GetData().GetSize() / sizeof(uint16_t);
  vector<float> const_data_vec;
  for (size_t i = 0; i < size; ++i) {
    uint16_t const_data = *(const_data_ptr + i);
    float const_data_fp32 = Uint16ToFloat(const_data);
    const_data_vec.push_back(const_data_fp32);
    FE_LOGD("Float16 value of const data[%zu] is %f.", i, const_data_fp32);
  }
  te::TbeAttrValue const_value(tensor_name, const_data_vec);
  op_tensor.SetConstValue(const_value);
  return SUCCESS;
}

template <typename T>
Status TbeInfoAssembler::SetConstValue(ge::GeTensorPtr tensor_ptr, const std::string &tensor_name,
                                       te::TbeOpTensor &op_tensor) {
  vector<T> const_data_vec;
  GetConstValueVec(tensor_ptr, const_data_vec);
  te::TbeAttrValue const_value(tensor_name, const_data_vec);
  op_tensor.SetConstValue(const_value);
  return SUCCESS;
}
}  // namespace fe
