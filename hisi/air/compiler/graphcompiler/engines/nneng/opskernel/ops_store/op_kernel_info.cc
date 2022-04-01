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

#include "ops_store/op_kernel_info.h"
#include <algorithm>
#include "common/fe_log.h"
#include "ops_store/ops_kernel_error_codes.h"
#include "graph/utils/type_utils.h"

namespace fe {

using ge::GeAttrValue;

InputOrOutputInfo::InputOrOutputInfo(string name)
    : is_input_(true),
      name_(std::move(name)),
      index_(),
      reshape_type_(""),
      propagate_reshape_type_(""),
      op_param_type_(RESERVED),
      op_const_value_depend_(CONST_IGNORE),
      supported_formats_(),
      supported_dtypes_(),
      supported_unknown_shape_formats_(),
      supported_unknown_shape_dtypes_() {}

uint32_t InputOrOutputInfo::GetIndex() const { return index_; }

InputOrOutputInfo::~InputOrOutputInfo() {}

const string InputOrOutputInfo::GetName() const { return name_; }

const bool& InputOrOutputInfo::GetIsInput() { return is_input_; }

vector<ge::DataType> InputOrOutputInfo::GetDataType() { return supported_dtypes_; }

vector<ge::Format> InputOrOutputInfo::GetFormat() { return supported_formats_; }

vector<ge::DataType> InputOrOutputInfo::GetUnknownShapeDataType() { return supported_unknown_shape_dtypes_; }

vector<ge::Format> InputOrOutputInfo::GetUnknownShapeFormat() { return supported_unknown_shape_formats_; }

OpParamType InputOrOutputInfo::GetParamType() const { return op_param_type_; }

OpConstValueDepend InputOrOutputInfo::GetConstValueDepend() const { return op_const_value_depend_; }

std::string InputOrOutputInfo::GetReshapeType() { return reshape_type_; }

void InputOrOutputInfo::SetReshapeType(std::string reshape_type) { reshape_type_ = reshape_type; }

const string InputOrOutputInfo::GetUniqueName() { return unique_name_; }

/*---------------------------------------Below is definition of AttrInfo-----------------------------------------*/
AttrInfo::AttrInfo(string attr_name)
    : attr_name_(attr_name),
      is_required_(false),
      dtype_(),
      is_support_all_value_(false),
      supported_values_(),
      is_default_value_defined_(false),
      default_value_() {}

AttrInfo::~AttrInfo() {}

const ge::GeAttrValue::ValueType& AttrInfo::GetAttrDType() const { return dtype_; }

const bool& AttrInfo::GetSupportAllValue() const { return is_support_all_value_; }

bool AttrInfo::GetIsRequired() const { return is_required_; }

const std::vector<ge::GeAttrValue>& AttrInfo::GetSupportedAttrValueVector() const { return supported_values_; }

const bool& AttrInfo::GetDefaultValueDefinedFlag() const { return is_default_value_defined_; }

const ge::GeAttrValue& AttrInfo::GetDefaultValue() const { return default_value_; }

const std::string& AttrInfo::GetAttrName() const { return attr_name_; }

/*-------------------------------Below is definition of OpKernelInfo--------------------------------------------------*/
OpKernelInfo::OpKernelInfo(string op_type)
    : init_flag_(false),
      op_type_(op_type),
      op_info_(),
      input_infos_(),
      output_infos_(),
      attrs_info_(),
      must_check_support_flag_(false),
      need_check_support_(false),
      impl_type_(EN_RESERVED),
      is_heavy_o_p_(false),
      op_pattern_(OP_PATTERN_OP_KERNEL),
      op_store_info_(),
      op_store_info_bf16_(),
      slice_pattern_(PATTERN_RESERVED) {
}

OpKernelInfo::~OpKernelInfo() {}

const vector<InputOrOutputInfoPtr>& OpKernelInfo::GetAllInputInfo() const { return input_infos_; }

const vector<InputOrOutputInfoPtr>& OpKernelInfo::GetAllOutputInfo() const { return output_infos_; }

Status OpKernelInfo::GetTensorInfoByName(const bool& isinput, const string& tensor_name,
                                         InputOrOutputInfoPtr& tensor_info_ptr) {
  if (isinput) {
    return GetInputInfoByName(tensor_name, tensor_info_ptr);
  } else {
    return GetOutputInfoByName(tensor_name, tensor_info_ptr);
  }
}

Status OpKernelInfo::GetInputInfoByName(const string& input_name, InputOrOutputInfoPtr& input_info_ptr) const {
  for (auto& input_iter : input_infos_) {
    if (input_iter->GetName() == input_name) {
      input_info_ptr = input_iter;
      return SUCCESS;
    }
  }
  return OP_INPUT_NOT_FOUND_IN_OP_KERNEL_INFO;
}

Status OpKernelInfo::GetOutputInfoByName(const string& output_name, InputOrOutputInfoPtr& output_info_ptr) const {
  for (auto& output_iter : output_infos_) {
    if (output_iter->GetName() == output_name) {
      output_info_ptr = output_iter;
      return SUCCESS;
    }
  }
  return OP_OUTPUT_NOT_FOUND_IN_OP_KERNEL_INFO;
}

string OpKernelInfo::GetOpType() const { return op_type_; }

const Status OpKernelInfo::GetAttrTypeByAttrName(const std::string& attr_name,
                                                 ge::GeAttrValue::ValueType& dtype) const {
  if (attrs_info_.empty()) {
    return OP_ATTR_EMPTY_IN_OP_KERNEL_INFO;
  }
  for (auto attr_info_iter : attrs_info_) {
    if (attr_info_iter->GetAttrName() == attr_name) {
      dtype = attr_info_iter->GetAttrDType();
      return SUCCESS;
    }
  }
  return OP_ATTR_NOT_FOUND_IN_OP_KERNEL_INFO;
}

const Status OpKernelInfo::GetAttrValueByAttrName(const std::string& attr_name,
                                                  std::vector<ge::GeAttrValue>& attr_value) const {
  for (auto attr_info_iter : attrs_info_) {
    if (attr_info_iter->GetAttrName() == attr_name) {
      attr_value = attr_info_iter->GetSupportedAttrValueVector();
      return SUCCESS;
    }
  }
  return OP_ATTR_NOT_FOUND_IN_OP_KERNEL_INFO;
}

const std::vector<AttrInfoPtr> OpKernelInfo::GetVecAttrInfo() const { return attrs_info_; }

ge::OpInfo OpKernelInfo::GetOpInfo() { return op_info_; }

OpStoreInfo OpKernelInfo::GetOpStoreInfo() { return op_store_info_; }
OpStoreInfo OpKernelInfo::GetOpStoreInfoBf16() { return op_store_info_bf16_; }

OpImplType OpKernelInfo::GetOpStoreImplType() const { return impl_type_; }

bool OpKernelInfo::GetHeavyOpFlag() const { return is_heavy_o_p_; };

OpPattern OpKernelInfo::GetOpPattern() const { return op_pattern_; };

bool OpKernelInfo::GetInputMemContinuesFlag() const { return input_mem_continues_; };

bool OpKernelInfo::GetOutputMemContinuesFlag() const { return output_mem_continues_; };

std::string OpKernelInfo::GetOpImpPath() const { return op_imp_path_; };

void OpKernelInfo::SetOpImpPath(std::string op_imp_path) { op_imp_path_ = op_imp_path; }

SlicePattern OpKernelInfo::GetOpSlicePattern() const {return slice_pattern_;};

void OpKernelInfo::SetImplType(const OpImplType impl_type) { impl_type_ = impl_type; }

bool OpKernelInfo::GetSupportDynamicShape() const { return is_support_dynamic_shape_; }

bool OpKernelInfo::GetFlagDynamicCompileStatic() const { return dynamic_compile_static_; }

void OpKernelInfo::SetSupportDynamicShape(const bool is_support_dynamic_shape) {
  is_support_dynamic_shape_ = is_support_dynamic_shape;
}

bool OpKernelInfo::GetSupportDynamicRank() const { return is_support_dynamic_rank_; }

std::string OpKernelInfo::GetCoreType() const {return core_type_;}
bool OpKernelInfo::GetNeedCheckSupportFlag() const { return need_check_support_; }
std::string OpKernelInfo::GetRangeLimitValue() const { return is_limited_range_; }
}  // namespace fe
