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

#include "common/dump_util.h"
#include <sstream>
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "common/aicore_util_attr_define.h"

namespace fe {
using ToOpStructPtr = std::shared_ptr<ToOpStruct_t>;

void DumpOpInfoTensor(std::vector<te::TbeOpParam> &puts, std::string &debug_str) {
  te::TensorType type = te::TensorType::TT_REQ;
  std::vector<int64_t> shape;
  std::vector<int64_t> offset;
  std::vector<te::TbeOpTensor> tensors;
  std::string format;
  int32_t sub_format = 0;
  std::string dtype;
  size_t value = 0;
  te::ATTR_SHAPETYPE shapetype;
  std::ostringstream debug_oss;
  string str;
  for (unsigned int i = 0; i < puts.size(); i++) {
    tensors.clear();
    debug_oss << std::endl << "tensor[" << std::to_string(i) << "]";
    debug_oss << std::endl << "tensor type : " << std::to_string(type);
    puts.at(i).GetTensors(tensors);
    for (auto &tensor : tensors) {
      str.clear();
      tensor.GetName(str);
      debug_oss << std::endl << "name : " << str;

      str.clear();
      tensor.GetShape(shape);
      for (auto ele : shape) {
        str += std::to_string(ele);
        str += ", ";
      }
      debug_oss << std::endl << "shape : [" << str << "]";

      str.clear();
      tensor.GetOriginShape(shape);
      for (auto ele : shape) {
        str += std::to_string(ele);
        str += ", ";
      }
      debug_oss << std::endl << "origin shape : [" << str << "]";

      tensor.GetShapeType(shapetype);
      debug_oss << std::endl << "shape type : " << std::to_string(shapetype);

      tensor.GetFormat(format);
      debug_oss << std::endl << "format : " << format;

      tensor.GetSubFormat(sub_format);
      debug_oss << std::endl << "sub_format : " << sub_format;

      tensor.GetOriginFormat(format);
      debug_oss << std::endl << "origin format : " << format;

      str.clear();
      tensor.GetType(str);
      debug_oss << std::endl << "dtype : " << str;

      tensor.GetAddrType(value);
      debug_oss << std::endl << "addr type : " << std::to_string(value);

      tensor.GetL1WorkspaceFlag(value);

      debug_oss << std::endl << "L1WorkspaceFlag : " << std::to_string(value);

      str.clear();
      tensor.GetSliceOffset(offset);
      for (auto ele : offset) {
        str += std::to_string(ele);
        str += ", ";
      }
      debug_oss << std::endl << "slice offset : " << str;

      str.clear();
      tensor.GetValidShape(shape);
      for (auto ele : shape) {
        str += std::to_string(ele);
        str += ", ";
      }
      debug_oss << std::endl << "valid shape : " << str;

      str.clear();
      tensor.GetSgtSliceShape(shape);
      for (auto ele : shape) {
        str += std::to_string(ele);
        str += ", ";
      }
      debug_oss << std::endl << "sgt slice shape : " << str;
    }
  }
  debug_oss << std::endl;
  debug_str = debug_oss.str();
}

void DumpOpInfo(te::TbeOpInfo &op_info) {
  string str;

  std::ostringstream debug_oss;
  int64_t value = 0;
  (void)op_info.GetName(str);
  debug_oss << std::endl << "name : " << str;
  (void)op_info.GetOpFileName(str);
  debug_oss << std::endl << "OpFileName : " << str;
  (void)op_info.GetOpFuncName(str);
  debug_oss << std::endl << "OpFuncName : " << str;
  (void)op_info.GetCheckSupportedFunc(str);
  debug_oss << std::endl << "CheckSupportedFunc : " << str;
  (void)op_info.GetVersion(str);
  debug_oss << std::endl << "Version : " << str;
  (void)op_info.GetPattern(str);
  debug_oss << std::endl << "Pattern : " << str;
  (void)op_info.GetKernelName(str);
  debug_oss << std::endl << "KernelName : " << str;
  (void)op_info.GetL1Space(value);
  debug_oss << std::endl << "L1Space : " << std::to_string(value);
  debug_oss << std::endl;
  FE_LOGD("Dump Op info. %s.", debug_oss.str().c_str());

  str.clear();
  std::vector<te::TbeOpParam> inputs;
  std::vector<te::TbeOpParam> outputs;
  (void)op_info.GetInputs(inputs);
  string debug_str;
  DumpOpInfoTensor(inputs, debug_str);
  FE_LOGD("Dump Op info inputs, %s.", debug_str.c_str());

  (void)op_info.GetOutputs(outputs);
  debug_str.clear();
  DumpOpInfoTensor(outputs, debug_str);
  FE_LOGD("Dump Op info outputs, %s.", debug_str.c_str());
}

void DumpL1Attr(const ge::Node *node) {
  auto op_desc = node->GetOpDesc();
  vector<int32_t> in_memery_type_vec;
  vector<int32_t> out_memery_type_vec;
  ToOpStructPtr l1_info = std::shared_ptr<ToOpStruct_t>();
  (void)ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_INPUT_MEM_TYPE_LIST, in_memery_type_vec);
  (void)ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_OUTPUT_MEM_TYPE_LIST, out_memery_type_vec);
  l1_info = op_desc->TryGetExtAttr(ge::ATTR_NAME_L1_FUSION_EXTEND_PTR, l1_info);
  string in_mem_type_str;
  string out_mem_type_str;
  string op_l1_fusion_type_str;
  string slice_input_offset_str;
  string slice_output_offset_str;
  string slice_input_shape_str;
  string slice_output_shape_str;
  FE_CHECK(l1_info == nullptr, FE_LOGD("l1Info null."), return);
  for (unsigned int i = 0; i < in_memery_type_vec.size(); i++) {
    in_mem_type_str += std::to_string(in_memery_type_vec[i]) + " ";
  }

  for (unsigned int i = 0; i < out_memery_type_vec.size(); i++) {
    out_mem_type_str += std::to_string(out_memery_type_vec[i]) + " ";
  }

  for (unsigned int i = 0; i < l1_info->op_l1_fusion_type.size(); i++) {
    op_l1_fusion_type_str += " L1 fusion type" + std::to_string(i) + ": ";
    for (auto &type : l1_info->op_l1_fusion_type) {
      op_l1_fusion_type_str += std::to_string(type) + " ";
    }
  }

  for (unsigned int i = 0; i < l1_info->slice_input_shape.size(); i++) {
    slice_input_shape_str += " valid shape input" + std::to_string(i) + ": ";
    for (auto &valid_input_shape : l1_info->slice_input_shape.at(i)) {
      slice_input_shape_str += std::to_string(valid_input_shape) + " ";
    }
  }

  for (unsigned int i = 0; i < l1_info->slice_output_shape.size(); i++) {
    slice_output_shape_str += " valid shape output" + std::to_string(i) + ": ";
    for (auto &valid_output_shape : l1_info->slice_output_shape.at(i)) {
      slice_output_shape_str += std::to_string(valid_output_shape) + " ";
    }
  }

  for (unsigned int i = 0; i < l1_info->slice_input_offset.size(); i++) {
    slice_input_offset_str += " slice offset input" + std::to_string(i) + ": ";
    for (auto &slice_offset : l1_info->slice_input_offset.at(i)) {
      slice_input_offset_str += std::to_string(slice_offset) + " ";
    }
  }

  for (unsigned int i = 0; i < l1_info->slice_output_offset.size(); i++) {
    slice_output_offset_str += " slice offset output" + std::to_string(i) + ": ";
    for (auto &slice_offset : l1_info->slice_output_offset.at(i)) {
      slice_output_offset_str += std::to_string(slice_offset) + " ";
    }
  }

  FE_LOGD("Dump L1 Op  info[%s, %s], input size [%zu], type [%s], output size [%zu], type [%s].",
          op_desc->GetName().c_str(), op_desc->GetType().c_str(), in_memery_type_vec.size(), in_mem_type_str.c_str(),
          out_memery_type_vec.size(), out_mem_type_str.c_str());

  FE_LOGD("Dump L1 Op info. op_l1_fusion_type size [%zu], type [%s],"
          " slice_input_shape size [%zu], type [%s]. slice_output_shape size [%zu], type [%s],"
          " slice_input_offset size [%zu], type [%s]. slice_output_offset size [%zu], type [%s], l1space [%ld].",
          l1_info->op_l1_fusion_type.size(), op_l1_fusion_type_str.c_str(), l1_info->slice_input_shape.size(),
          slice_input_shape_str.c_str(), l1_info->slice_output_shape.size(), slice_output_shape_str.c_str(),
          l1_info->slice_input_offset.size(), slice_input_offset_str.c_str(), l1_info->slice_output_offset.size(),
          slice_output_offset_str.c_str(), l1_info->op_l1_space);
}

void DumpL2Attr(const ge::Node *node) {
  auto op_desc = node->GetOpDesc();
  vector<int32_t> in_memery_type_vec;
  vector<int32_t> out_memery_type_vec;
  ToOpStructPtr l2_info = std::shared_ptr<ToOpStruct_t>();
  (void)ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_INPUT_MEM_TYPE_LIST, in_memery_type_vec);
  (void)ge::AttrUtils::GetListInt(op_desc, ge::ATTR_NAME_OUTPUT_MEM_TYPE_LIST, out_memery_type_vec);
  l2_info = op_desc->TryGetExtAttr(ATTR_NAME_L2_FUSION_EXTEND_PTR, l2_info);
  string in_mem_type_str;
  string out_mem_type_str;
  string slice_input_offset_str;
  string slice_output_offset_str;
  string slice_input_shape_str;
  string slice_output_shape_str;
  FE_CHECK(l2_info == nullptr, FE_LOGD("l2Info null."), return);
  for (unsigned int i = 0; i < in_memery_type_vec.size(); i++) {
    in_mem_type_str += std::to_string(in_memery_type_vec[i]) + " ";
  }

  for (unsigned int i = 0; i < out_memery_type_vec.size(); i++) {
    out_mem_type_str += std::to_string(out_memery_type_vec[i]) + " ";
  }

  for (unsigned int i = 0; i < l2_info->slice_input_shape.size(); i++) {
    slice_input_shape_str += " valid shape input" + std::to_string(i) + ": ";
    for (auto &valid_input_shape : l2_info->slice_input_shape.at(i)) {
      slice_input_shape_str += std::to_string(valid_input_shape) + " ";
    }
  }

  for (unsigned int i = 0; i < l2_info->slice_output_shape.size(); i++) {
    slice_output_shape_str += " valid shape output" + std::to_string(i) + ": ";
    for (auto &valid_output_shape : l2_info->slice_output_shape.at(i)) {
      slice_output_shape_str += std::to_string(valid_output_shape) + " ";
    }
  }

  for (unsigned int i = 0; i < l2_info->slice_input_offset.size(); i++) {
    slice_input_offset_str += " slice offset input" + std::to_string(i) + ": ";
    for (auto &slice_offset : l2_info->slice_input_offset.at(i)) {
      slice_input_offset_str += std::to_string(slice_offset) + " ";
    }
  }

  for (unsigned int i = 0; i < l2_info->slice_output_offset.size(); i++) {
    slice_output_offset_str += " slice offset output" + std::to_string(i) + ": ";
    for (auto &slice_offset : l2_info->slice_output_offset.at(i)) {
      slice_output_offset_str += std::to_string(slice_offset) + " ";
    }
  }

  FE_LOGD("Dump L2 Op info[%s, %s], input size [%zu], type [%s], output size [%zu], type [%s].",
          op_desc->GetName().c_str(), op_desc->GetType().c_str(), in_memery_type_vec.size(), in_mem_type_str.c_str(),
          out_memery_type_vec.size(), out_mem_type_str.c_str());

  FE_LOGD("Dump L2 Op info."
          " slice_input_shape size [%zu], type [%s]. slice_output_shape size [%zu], type [%s],"
          " slice_input_offset size [%zu], type [%s]. slice_output_offset size [%zu], type [%s].",
          l2_info->slice_input_shape.size(), slice_input_shape_str.c_str(), l2_info->slice_output_shape.size(),
          slice_output_shape_str.c_str(), l2_info->slice_input_offset.size(), slice_input_offset_str.c_str(),
          l2_info->slice_output_offset.size(), slice_output_offset_str.c_str());
}
}  // namespace fe
