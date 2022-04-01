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

#include "ops_store/op_kernel_info_constructor.h"
#include "register/op_registry.h"
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "common/aicore_util_constants.h"
#include "common/fe_type_utils.h"
#include "common/string_utils.h"
#include "ops_store/ops_kernel_constants.h"
#include "ops_store/ops_kernel_utils.h"
#include "ops_store/ops_kernel_error_codes.h"
#include "common/util/error_manager/error_manager.h"
#include "common/fe_error_code.h"

namespace fe {

using ge::GeAttrValue;

static std::map<std::string, ge::GeAttrValue::ValueType> g_attr_type_map {
        {STR_INT, ge::GeAttrValue::VT_INT},
        {STR_FLOAT, ge::GeAttrValue::VT_FLOAT},
        {STR_BOOL, ge::GeAttrValue::VT_BOOL},
        {STR_STR, ge::GeAttrValue::VT_STRING},
        {STR_LIST_INT, ge::GeAttrValue::VT_LIST_INT},
        {STR_LIST_FLOAT, ge::GeAttrValue::VT_LIST_FLOAT},
        {STR_LIST_BOOL, ge::GeAttrValue::VT_LIST_BOOL},
        {STR_LIST_STR, ge::GeAttrValue::VT_LIST_STRING},
        {STR_LIST_LIST_INT, ge::GeAttrValue::VT_LIST_LIST_INT}};

OpKernelInfoConstructor::OpKernelInfoConstructor() {}

OpKernelInfoConstructor::~OpKernelInfoConstructor() {}

Status OpKernelInfoConstructor::ParseBasicParameter(const OpContent &op_content,
                                                    OpKernelInfoPtr op_kernel_info) const {
  // parse the op.pattern of the op
  string op_pattren_str;
  Status result = GetStrFromOpContent(op_content, STR_OP, STR_PATTERN, op_pattren_str);
  bool ret_status = (result == SUCCESS && !op_pattren_str.empty());
  if (ret_status) {
    auto op_pattern_iter = OP_PATTERN_MAP.find(StringUtils::Trim(op_pattren_str));
    if (op_pattern_iter == OP_PATTERN_MAP.end()) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s: invalid op.pattern %s, The value %s of %s for op %s is "
          "invalid, it should be one of formatAgnostic, broadcast or reduce.", op_kernel_info->op_type_.c_str(),
          op_pattren_str.c_str(), op_pattren_str.c_str(), STR_PATTERN.c_str(), op_content.op_type_.c_str());
      return FAILED;
    }
    op_kernel_info->op_pattern_ = op_pattern_iter->second;
  }

  // parse the imp_path.path of the op
  string op_imp_path_str;
  result = GetStrFromOpContent(op_content, STR_IMP_PATH, STR_PATH, op_imp_path_str);
  ret_status = (result == SUCCESS && !op_imp_path_str.empty());
  if (ret_status) {
    op_kernel_info->op_imp_path_ = op_imp_path_str;
  }

  // parse the dynamic_format.flag of the op
  string dynamic_format_str;
  result = GetStrFromOpContent(op_content, STR_DYNAMIC_FORMAT, STR_FLAG, dynamic_format_str);
  ret_status = (result == SUCCESS && !dynamic_format_str.empty());
  if (ret_status) {
    if (StringUtils::MatchString(kStrTrue, dynamic_format_str)) {
      op_kernel_info->op_pattern_ = OP_PATTERN_OP_CUSTOMIZE;
    }
  }

  // parse the dynamic_shape_support of the op
  string dynamic_shape_support_str;
  result = GetStrFromOpContent(op_content, STR_DYNAMIC_SHAPE_SUPPORT, STR_FLAG, dynamic_shape_support_str);
  ret_status = (result == SUCCESS && !dynamic_shape_support_str.empty());
  if (ret_status) {
    if (StringUtils::MatchString(kStrTrue, dynamic_shape_support_str)) {
      FE_LOGD("Op[type:%s] is support dynamic shape.", op_kernel_info->op_type_.c_str());
      op_kernel_info->is_support_dynamic_shape_ = true;
    }
  }

  // parse the dynamic_rank_support of the op
  string dynamic_rank_support_str;
  result = GetStrFromOpContent(op_content, STR_DYNAMIC_RANK_SUPPORT, STR_FLAG, dynamic_rank_support_str);
  ret_status = (result == SUCCESS && !dynamic_rank_support_str.empty());
  if (ret_status) {
    if (StringUtils::MatchString(kStrTrue, dynamic_rank_support_str)) {
      FE_LOGD("Op[type:%s] is support dynamic rank.", op_kernel_info->op_type_.c_str());
      op_kernel_info->is_support_dynamic_rank_ = true;
    }
  }
  // input_mem_continues.flag
  string input_mem_continues_str;
  (void)GetStrFromOpContent(op_content, STR_INPUT_MEM_CONTINUES, STR_FLAG, input_mem_continues_str);
  ret_status = (!input_mem_continues_str.empty() && StringUtils::MatchString(kStrTrue, input_mem_continues_str));
  if (ret_status) {
    op_kernel_info->input_mem_continues_ = true;
  }
  // output_mem_continues.flag
  string output_mem_continues_str;
  (void)GetStrFromOpContent(op_content, STR_OUTPUT_MEM_CONTINUES, STR_FLAG, output_mem_continues_str);
  ret_status = (!output_mem_continues_str.empty() && StringUtils::MatchString(kStrTrue, output_mem_continues_str));
  if (ret_status) {
    op_kernel_info->output_mem_continues_ = true;
  }

  // output_mem_continues.flag
  string lx_core_type;
  (void)GetStrFromOpContent(op_content, kCoreType, STR_VALUE, lx_core_type);
  FE_LOGD("Use core type[%s] for operator %s.", lx_core_type.c_str(), op_kernel_info->GetOpType().c_str());
  op_kernel_info->core_type_ = lx_core_type;

  return SUCCESS;
}

Status OpKernelInfoConstructor::InitializeOpKernelInfo(std::string engine_name, const OpContent &op_content,
                                                       OpKernelInfoPtr op_kernel_info) {
  FE_CHECK(op_kernel_info == nullptr,
           FE_LOGW("OpKernelInfo is null pointer in function InitializeOpKernelInfo!"),
           return SUCCESS);

  if (op_kernel_info->init_flag_) {
    FE_LOGD("OpKernelInfo has been initialized.");
    return SUCCESS;
  }

  op_kernel_info->op_type_ = op_content.op_type_;

  if (ParseBasicParameter(op_content, op_kernel_info) != SUCCESS) {
    return FAILED;
  }

  // parse the input and output tensor_desc_info
  if (ParseInputAndOutputFromOpContent(op_content, op_kernel_info) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s init input and output failed.",
                    op_kernel_info->op_type_.c_str());
    return FAILED;
  }

  if (CheckFormatAgnosticOp(op_kernel_info) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s check the format agnostic failed.",
                    op_kernel_info->op_type_.c_str());
    return FAILED;
  }

  // parse the attr_info
  if (InitAttrValue(op_content, op_kernel_info) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s init AttrValue failed.", op_kernel_info->op_type_.c_str());
    return FAILED;
  }

  // parse OpInfo
  if (InitOpInfo(engine_name, op_content, op_kernel_info) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s init OpInfo failed.", op_kernel_info->op_type_.c_str());
    return FAILED;
  }
  op_kernel_info->init_flag_ = true;
  return SUCCESS;
}

Status OpKernelInfoConstructor::CheckFormatAgnosticOp(OpKernelInfoPtr op_kernel_info) const {
  if (op_kernel_info->GetOpPattern() != OP_PATTERN_FORMAT_AGNOSTIC) {
    return SUCCESS;
  }
  std::vector<InputOrOutputInfoPtr> input_infos = op_kernel_info->input_infos_;
  std::vector<InputOrOutputInfoPtr> output_infos = op_kernel_info->output_infos_;
  if (input_infos.size() != 1 || output_infos.size() != 1) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s: the op.pattern is format_agnostic, \
                    but the input_infos size [%zu] or the output_infos size [%zu] is not 1.",
                    op_kernel_info->op_type_.c_str(), input_infos.size(), output_infos.size());
    return FAILED;
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::FinalizeOpKernelInfo(OpKernelInfoPtr op_kernel_info) const {
  if (op_kernel_info == nullptr) {
    FE_LOGW("OpKernelInfo is null pointer and it does not need to be finalized");
    return SUCCESS;
  }

  op_kernel_info->op_type_ = "";
  /* The following two loop can be ommited because when a vector
   * is cleared, the shared pointers in this vector will automatically
   * destroy themselves. */
  for (size_t i = 0; i < op_kernel_info->input_infos_.size(); i++) {
    if (op_kernel_info->input_infos_[i] == nullptr) {
      FE_LOGW("FinalizeOpKernelInfo pointer index %zu in input_infos_ should not be nullptr.", i);
      continue;
    }
    FinalizeInputAndOutput(op_kernel_info->input_infos_[i]);
  }

  for (size_t i = 0; i < op_kernel_info->output_infos_.size(); i++) {
    if (op_kernel_info->output_infos_[i] == nullptr) {
      FE_LOGW("FinalizeOpKernelInfo pointer index %zu in output_infos_ should not be nullptr!", i);
      continue;
    }
    FinalizeInputAndOutput(op_kernel_info->output_infos_[i]);
  }

  op_kernel_info->input_infos_.clear();
  op_kernel_info->output_infos_.clear();
  op_kernel_info->attrs_info_.clear();
  op_kernel_info->init_flag_ = false;
  return SUCCESS;
}

void GetIntputAndOutputFlag(const OpContent &op_content, vector<string> &input_vec, vector<string> &output_vec,
                            bool &input_found, bool &output_found) {
  for (auto &content : op_content.map_kernel_info_) {
    if (CheckInputSubStr(content.first, STR_INPUT_LOWERCASE) ||
        CheckInputSubStr(content.first, STR_INPUT_FIRST_LETTER_UPPERCASE)) {
      input_vec.push_back(content.first);
      input_found = true;
    }
    if (CheckInputSubStr(content.first, STR_OUTPUT_LOWERCASE)||
        CheckInputSubStr(content.first, STR_OUTPUT_FIRST_LETTER_UPPERCASE)) {
      output_vec.push_back(content.first);
      output_found = true;
    }
  }
}

Status OpKernelInfoConstructor::ParseInputAndOutputFromOpContent(const OpContent &op_content,
                                                                 OpKernelInfoPtr op_kernel_info) {
  vector<string> input_vec;
  vector<string> output_vec;
  bool input_found = false;
  bool output_found = false;
  GetIntputAndOutputFlag(op_content, input_vec, output_vec, input_found, output_found);

  sort(input_vec.begin(), input_vec.end(), CmpInputsNum);
  sort(output_vec.begin(), output_vec.end(), CmpOutputsNum);

  if (!input_found || !output_found) {
    FE_LOGW("input[%u] or output[%u] is empty in op optype [%s]!", input_found, output_found,
            op_content.op_type_.c_str());
  }
  uint32_t input_idx = 0;
  uint32_t dtype_and_format_size_of_first_input = INVALID_DTYPE_AND_FORMAT_SIZE;
  for (auto &input_index : input_vec) {
    auto input_pos = op_content.map_kernel_info_.find(input_index);
    if (input_pos == op_content.map_kernel_info_.end()) {
      return OP_STORE_MAP_KEY_FIND_FAILED;
    }
    string input_name;
    if (GetStrFromOpContent(op_content, input_index, STR_NAME, input_name) != SUCCESS) {
      REPORT_FE_ERROR(
          "[InitOpInfoLib][InitOpKernel] Get op %s InputName %s failed, The input%d's name of op %s does not exist.",
          op_kernel_info->op_type_.c_str(), input_index.c_str(), input_idx, op_content.op_type_.c_str());
      return FAILED;
    }
    InputOrOutputInfoPtr input_info_ptr = nullptr;
    FE_MAKE_SHARED(input_info_ptr = std::make_shared<InputOrOutputInfo>(input_name), return FAILED);
    FE_CHECK_NOTNULL(input_info_ptr);
    input_info_ptr->is_input_ = true;
    if (InitializeInputAndOutput(op_kernel_info->op_pattern_, op_kernel_info->op_type_, input_pos->second, input_idx++,
                                 input_info_ptr, dtype_and_format_size_of_first_input, op_kernel_info) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel]  Op %s input_desc_info %s initialize failed.",
                      op_kernel_info->op_type_.c_str(), input_index.c_str());
      return FAILED;
    }
    op_kernel_info->input_infos_.push_back(input_info_ptr);
  }
  uint32_t output_idx = 0;
  for (auto &output_index : output_vec) {
    auto output_pos = op_content.map_kernel_info_.find(output_index);
    if (output_pos == op_content.map_kernel_info_.end()) {
      return OP_STORE_MAP_KEY_FIND_FAILED;
    }
    string output_name;
    if (GetStrFromOpContent(op_content, output_index, STR_NAME, output_name) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel]  Op %s get OutputName %s failed.",
                      op_kernel_info->op_type_.c_str(), output_index.c_str());
      return FAILED;
    }
    InputOrOutputInfoPtr output_info_ptr = nullptr;
    FE_MAKE_SHARED(output_info_ptr = std::make_shared<InputOrOutputInfo>(output_name), return FAILED);
    FE_CHECK_NOTNULL(output_info_ptr);

    output_info_ptr->is_input_ = false;
    if (InitializeInputAndOutput(op_kernel_info->op_pattern_, op_kernel_info->op_type_, output_pos->second,
                                 output_idx++, output_info_ptr, dtype_and_format_size_of_first_input,
                                 op_kernel_info) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s output_desc_info %s initialize failed.",
                      op_kernel_info->op_type_.c_str(), output_index.c_str());
      return FAILED;
    }
    op_kernel_info->output_infos_.push_back(output_info_ptr);
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitFormatAndDtypeForSingleInputAndOutput(
    OpPattern op_pattren, const map<string, string> &map_info,
    const InputOrOutputInfoPtr &input_or_output_info,
    OpKernelInfoPtr op_kernel_info, uint32_t &dtype_and_format_size_of_first_input) {
  string format_str;
  string unknown_format_str;
  Status format_stat = GetStrFormMap(map_info, STR_FORMAT, format_str);
  Status unknown_format_stat = GetStrFormMap(map_info, STR_UNKNOWN_SHAPE_FORMAT, unknown_format_str);

  string unique_name = input_or_output_info->GetUniqueName();
  string op_type = op_kernel_info->GetOpType();
  bool init_by_op_kernel =
      op_pattren == OP_PATTERN_OP_KERNEL || format_stat == SUCCESS || unknown_format_stat == SUCCESS;
  if (init_by_op_kernel) {
    if (format_stat != SUCCESS && unknown_format_stat != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] op (type [%s]): format and unknownshape_format can not be \
                      found in kernel. Input or output is %s.", op_type.c_str(), unique_name.c_str());
      return FAILED;
    }
    if (format_stat == SUCCESS) {
      if (InitDtypeAndFormat(map_info, input_or_output_info, dtype_and_format_size_of_first_input) != SUCCESS) {
        REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] op (type [%s]): init dtype and format failed. \
                        The inputOrOutput is %s.", op_type.c_str(), unique_name.c_str());
        return FAILED;
      }
    }
    if (unknown_format_stat == SUCCESS) {
      if (InitDtypeByPattern(map_info, input_or_output_info, dtype_and_format_size_of_first_input, op_pattren) !=
          SUCCESS) {
        REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] op (type [%s]): init dtypes failed. The input_or_output is %s.",
                        op_type.c_str(), unique_name.c_str());
        return FAILED;
      }
      if (InitUnknownFormatAndDtype(map_info, input_or_output_info, dtype_and_format_size_of_first_input) != SUCCESS) {
        REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] op (type [%s]): init unknown shape dtype and format failed. \
                        The input_or_output is %s.", op_type.c_str(), unique_name.c_str());
        return FAILED;
      }
    }
  } else if (op_pattren == OP_PATTERN_OP_CUSTOMIZE) {
    FE_LOGD("op (type [%s]): the dynamic_format.flag is true, no need to init dtype or format. Input or output is %s.",
            op_type.c_str(), unique_name.c_str());
  } else if (op_pattren == OP_PATTERN_FORMAT_AGNOSTIC) {
    FE_LOGD("op (type [%s]): the op.pattern is format_agnostic, init dtype and all format. Input or output is %s.",
            op_type.c_str(), unique_name.c_str());
    if (InitDtypeAndAllFormat(map_info, input_or_output_info, dtype_and_format_size_of_first_input, op_kernel_info) !=
        SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] op (type [%s]): init dtype and all formats failed. \
                      The inputOrOutput is %s.", op_type.c_str(), unique_name.c_str());
      return FAILED;
    }
  } else {
    FE_LOGD("op (type [%s]): the op.pattern is %s, init dtypes. The inputOrOutput is %s.",
            op_type.c_str(), GetOpPatternString(op_pattren).c_str(), unique_name.c_str());
    if (InitDtype(map_info, input_or_output_info, dtype_and_format_size_of_first_input) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] op (type [%s]): init dtypes failed. The input_or_output is %s.",
                      op_type.c_str(), unique_name.c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitializeInputAndOutput(OpPattern op_pattren, const string &op_type,
                                                         const map<string, string> &map_info, uint32_t index,
                                                         InputOrOutputInfoPtr input_or_output_info,
                                                         uint32_t &dtype_and_format_size_of_first_input,
                                                         OpKernelInfoPtr op_kernel_info) {
  FE_CHECK(input_or_output_info == nullptr, FE_LOGW("param input_or_output_info is null!"),
           return OP_KERNEL_INFO_NULL_PTR);

  // 1. set unique_name
  input_or_output_info->index_ = index;
  SetUniqueName(input_or_output_info);

  // 2. init formats and dtypes
  if (InitFormatAndDtypeForSingleInputAndOutput(op_pattren, map_info, input_or_output_info,
                                                op_kernel_info, dtype_and_format_size_of_first_input) != SUCCESS) {
    return FAILED;
  }
  // 3. init param_type
  string param_type_str;
  if (GetStrFormMap(map_info, STR_PARAM_TYPE, param_type_str) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] TensorDescInfo::Initialize param_type_str key invalid.");
    return FAILED;
  }
  auto param_type_iter = PARAM_TYPE_MAP.find(StringUtils::Trim(param_type_str));
  if (param_type_iter == PARAM_TYPE_MAP.end()) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] invalid param_type.");
    return OP_STORE_MAP_KEY_FIND_FAILED;
  }
  input_or_output_info->op_param_type_ = param_type_iter->second;

  // 4. init reshape_type
  string reshape_type_str;
  if (GetStrFormMap(map_info, STR_RESHAPE_TYPE, reshape_type_str) == SUCCESS) {
    FE_LOGD("Op (type [%s]) has reshape type [%s]. Tensor index is [%u], name is [%s] and is_input_ is [%u]).",
        op_type.c_str(), reshape_type_str.c_str(), index, input_or_output_info->name_.c_str(),
        input_or_output_info->is_input_);
    input_or_output_info->reshape_type_ = reshape_type_str;
  }

  // 5. init const_value_depend
  string const_value_depend_str;
  if (GetStrFormMap(map_info, STR_CONST_VALUE_DEPEND, const_value_depend_str) == SUCCESS) {
    FE_LOGD("Tensor [%s, %u] of op[%s] has const value depend [%s].",
            input_or_output_info->name_.c_str(), index, op_type.c_str(), const_value_depend_str.c_str());
    // const depend param is optional
    auto const_depend_iter = CONST_VALUE_DEPEND_MAP.find(StringUtils::Trim(const_value_depend_str));
    if (const_depend_iter != CONST_VALUE_DEPEND_MAP.end()) {
      input_or_output_info->op_const_value_depend_ = const_depend_iter->second;
    }
  }

  return SUCCESS;
}

Status OpKernelInfoConstructor::FinalizeInputAndOutput(InputOrOutputInfoPtr input_or_output_info) const {
  /* null pointer means it is already finalized, so return SUCCESS here. */
  if (input_or_output_info == nullptr) {
    FE_LOGW("inputOrOutputInfo is null pointer in function FinalizeInputAndOutput");
    return SUCCESS;
  }
  input_or_output_info->supported_dtypes_.clear();
  input_or_output_info->supported_formats_.clear();
  return SUCCESS;
}

template <typename T>
Status ConvertStr2VecNumber(const string &attr_str, vector<T> &temp_plate_vec) {
  vector<string> template_str_vec = StringUtils::Split(attr_str, ',');
  for (auto el : template_str_vec) {
    // std::remove_cv: get rid of prefix like const or volatile
    T attr_elem;
    using DT = typename std::remove_cv<T>::type;
    if (std::is_same<int64_t, DT>::value) {
      try {
        attr_elem = static_cast<int64_t>(stol(el));
      } catch (...) {
        REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Int Attr %s invalid.", attr_str.c_str());
        return OP_STORE_STRING_CONVERT_FAILED;
      }
    } else if (std::is_same<bool, DT>::value) {
      attr_elem = StringUtils::MatchString(kStrTrue, el);
    } else {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Attr %s invalid. Attr type is not one of int/bool.",
                      attr_str.c_str());
      return OP_STORE_STRING_CONVERT_FAILED;
    }
    temp_plate_vec.push_back(attr_elem);
  }
  return SUCCESS;
}

Status ConvertStr2VecNumber(const string &attr_str, vector<float> &temp_plate_vec) {
  vector<string> template_str_vec = StringUtils::Split(attr_str, ',');
  for (auto &el : template_str_vec) {
    float attr_elem;
    try {
      attr_elem = std::stof(el);
    } catch (...) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Float Attr %s invalid.", attr_str.c_str());
      return OP_STORE_STRING_CONVERT_FAILED;
    }
    temp_plate_vec.emplace_back(attr_elem);
  }
  return SUCCESS;
}

/* string type */
Status ConvertStr2VecNumber(const string &attr_str, vector<string> &temp_plate_vec) {
  vector<string> template_str_vec = StringUtils::Split(attr_str, ',');
  for (auto const &el : template_str_vec) {
    temp_plate_vec.push_back(el);
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitDtype(const map<string, string> &map_info,
                                          InputOrOutputInfoPtr input_or_output_info,
                                          uint32_t &dtype_and_format_size_of_first_input) {
  string dtype_str;
  if (GetStrFormMap(map_info, STR_DTYPE, dtype_str) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] InputOrOutputInfo ::Initialize data_type key invalid.");
    return FAILED;
  }
  vector<string> dtype_str_vec = StringUtils::Split(StringUtils::Trim(dtype_str), ',');
  uint32_t dtype_size = static_cast<uint32_t>(dtype_str_vec.size());

  // check th dtype size and format size of inputs or outputs should be the same as
  // the size of the first input or output
  if (dtype_and_format_size_of_first_input == INVALID_DTYPE_AND_FORMAT_SIZE) {
    dtype_and_format_size_of_first_input = dtype_size;
  }
  if (dtype_size != dtype_and_format_size_of_first_input) {
    REPORT_FE_ERROR(
        "[InitOpInfoLib][InitOpKernel] "
        "The dtype size of input [%u] named [%s] is [%u], which is not as same as other input or output [%u].",
        input_or_output_info->GetIndex(), input_or_output_info->GetName().c_str(), dtype_size,
        dtype_and_format_size_of_first_input);
    return FAILED;
  }

  for (uint32_t i = 0; i < dtype_size; i++) {
    ge::DataType dtype;
    if (String2DataType(StringUtils::Trim(dtype_str_vec[i]), dtype) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] String2DataType %s failed. dtype is illegal!",
                      dtype_str_vec[i].c_str());
      return FAILED;
    }
    input_or_output_info->supported_dtypes_.push_back(dtype);
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitDtypeAndAllFormat(const map<string, string> &map_info,
                                                      InputOrOutputInfoPtr input_or_output_info,
                                                      uint32_t &dtype_of_first_input, OpKernelInfoPtr op_kernel_info) {
  string dtype_str;
  if (GetStrFormMap(map_info, STR_DTYPE, dtype_str) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] InputOrOutputInfo ::Initialize data_type key invalid.");
    return FAILED;
  }
  vector<string> dtype_str_vec = StringUtils::Split(StringUtils::Trim(dtype_str), ',');
  size_t dtype_size = dtype_str_vec.size();

  // 1. check th dtype size of inputs or outputs should be the same as
  // the size of the first input or output
  if (dtype_of_first_input == INVALID_DTYPE_AND_FORMAT_SIZE) {
    dtype_of_first_input = static_cast<uint32_t >(dtype_size);
  }
  if (dtype_size != dtype_of_first_input) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The dtype size of input [%u] named [%s] is [%zu], \
                    which is not as same as other input or output [%u].", input_or_output_info->GetIndex(),
                    input_or_output_info->GetName().c_str(), dtype_size, dtype_of_first_input);
    return FAILED;
  }

  // 2. generate the new data_types and the new formats
  vector<ge::DataType> old_data_types;
  for (size_t i = 0; i < dtype_size; i++) {
    ge::DataType dtype;
    if (String2DataType(StringUtils::Trim(dtype_str_vec[i]), dtype) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] String2DataType %s failed. dtype is illegal!",
                      dtype_str_vec[i].c_str());
      return FAILED;
    }
    old_data_types.push_back(dtype);
  }
  vector<ge::Format> old_formats(FE_HEAVY_FORMAT_VECTOR);
  old_formats.insert(old_formats.cend(), FE_ORIGIN_FORMAT_VECTOR.cbegin(), FE_ORIGIN_FORMAT_VECTOR.cend());
  vector<ge::Format> new_formats;
  vector<ge::DataType> new_data_types;
  if (GenerateUnionFormatAndDtype(old_formats, old_data_types, new_formats, new_data_types) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] fail to GenerateUnionFormatDtype");
    return FAILED;
  }
  input_or_output_info->supported_formats_.insert(input_or_output_info->supported_formats_.cend(),
                                                  new_formats.cbegin(), new_formats.cend());
  input_or_output_info->supported_dtypes_.insert(input_or_output_info->supported_dtypes_.cend(),
                                                 new_data_types.cbegin(), new_data_types.cend());

  // unknown shape format and dtype
  if (op_kernel_info->GetSupportDynamicShape()) {
    FE_LOGD("Op[type:%s] is support unknown shape, set its format and dtype.", op_kernel_info->GetOpType().c_str());
    input_or_output_info->supported_unknown_shape_formats_.insert(
        input_or_output_info->supported_unknown_shape_formats_.cend(), new_formats.cbegin(), new_formats.cend());
    input_or_output_info->supported_unknown_shape_dtypes_.insert(
        input_or_output_info->supported_unknown_shape_dtypes_.cend(), new_data_types.cbegin(), new_data_types.cend());
  }
  FE_LOGD("The new_formats is [%s], the new_data_types is [%s].", GetStrByFormatVec(new_formats).c_str(),
          GetStrByDataTypeVec(new_data_types).c_str());
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitDtypeAndFormat(const map<string, string> &map_info,
                                                   InputOrOutputInfoPtr input_or_output_info,
                                                   uint32_t &dtype_and_format_size_of_first_input) {
  string dtype_str;
  string format_str;
  // convert d_type
  if (GetStrFormMap(map_info, STR_DTYPE, dtype_str) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] InputOrOutputInfo ::Initialize data_type key invalid.");
    return FAILED;
  }

  vector<string> dtype_str_vec = StringUtils::Split(StringUtils::Trim(dtype_str), ',');

  // convert Format
  if (GetStrFormMap(map_info, STR_FORMAT, format_str) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] InputOrOutputInfo::Initialize format key invalid.");
    return FAILED;
  }
  vector<string> format_str_vec = StringUtils::Split(format_str, ',');

  // check whether size of dtype is the same as size of format
  size_t format_size = format_str_vec.size();
  size_t dtype_size = dtype_str_vec.size();
  if (dtype_size != format_size) {
    REPORT_FE_ERROR("[GraphOpt][InitDtypeAndFormat]:InitDtypeAndFormat:: inconsistent format size [%zu] \
                    and dtype size [%zu]!", format_size, dtype_size);
    return FAILED;

  }
  /* The Following dtype size and format size of inputs or outputs should be the
   * same as
   * the size of the first input or output */
  if (dtype_and_format_size_of_first_input == INVALID_DTYPE_AND_FORMAT_SIZE) {
    dtype_and_format_size_of_first_input = static_cast<uint32_t>(format_size);
  }
  if (format_size != dtype_and_format_size_of_first_input) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The format and dtype size of input [%u] named [%s] is [%zu], \
                    which is not as same as [%u].", input_or_output_info->GetIndex(),
                    input_or_output_info->GetName().c_str(), format_size, dtype_and_format_size_of_first_input);
    return FAILED;
  }

  for (size_t i = 0; i < format_size; i++) {
    ge::DataType dtype;
    if (String2DataType(StringUtils::Trim(dtype_str_vec[i]), dtype) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] String2DataType %s failed. dtype is illegal!",
                      dtype_str_vec[i].c_str());
      return FAILED;
    }
    input_or_output_info->supported_dtypes_.push_back(dtype);

    ge::Format format = ge::TypeUtils::SerialStringToFormat(StringUtils::Trim(format_str_vec[i]));
    if (format == ge::FORMAT_RESERVED) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Invalid format: whole [%s], format [%zu] = [%s], \
                      can not convert to ge::Format enum.", format_str.c_str(), i, format_str_vec[i].c_str());
      return FAILED;
    }
    input_or_output_info->supported_formats_.push_back(format);
  }

  return SUCCESS;
}

Status OpKernelInfoConstructor::InitUnknownFormatAndDtype(const map<string, string> &map_info,
                                                          InputOrOutputInfoPtr input_or_output_info,
                                                          uint32_t &dtype_and_format_size_of_first_input) {
  string format_str;
  Status status = GetStrFormMap(map_info, STR_UNKNOWN_SHAPE_FORMAT, format_str);
  if (status == OP_STORE_MAP_KEY_FIND_FAILED) {
    FE_LOGD("Can not find unknownshape_format.");
    return SUCCESS;
  } else if (status == SUCCESS) {
    string dtype_str;
    // convert d_type
    if (GetStrFormMap(map_info, STR_DTYPE, dtype_str) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] InputOrOutputInfo ::Initialize data_type key invalid.");
      return FAILED;
    }

    vector<string> dtype_str_vec = StringUtils::Split(StringUtils::Trim(dtype_str), ',');

    // convert Format
    if (GetStrFormMap(map_info, STR_UNKNOWN_SHAPE_FORMAT, format_str) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] InputOrOutputInfo::Initialize format key invalid.");
      return FAILED;
    }
    vector<string> format_str_vec = StringUtils::Split(format_str, ',');

    // check whether size of dtype is the same as size of format
    size_t format_size = format_str_vec.size();
    size_t dtype_size = dtype_str_vec.size();
    if (dtype_size != format_size) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Inconsistent format size [%zu] and dtype size [%zu]!",
          format_size, dtype_size);
      return FAILED;
    }
    /* The Following dtype size and format size of inputs or outputs should be the
     * same as
     * the size of the first input or output */
    if (dtype_and_format_size_of_first_input == INVALID_DTYPE_AND_FORMAT_SIZE) {
      dtype_and_format_size_of_first_input = static_cast<uint32_t>(format_size);
    }
    if (format_size != dtype_and_format_size_of_first_input) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The format and dtype size of input [%u] named[%s] is [%zu], \
                      which is not as same as [%u].", input_or_output_info->GetIndex(),
                      input_or_output_info->GetName().c_str(), format_size, dtype_and_format_size_of_first_input);
      return FAILED;
    }

    for (size_t i = 0; i < format_size; i++) {
      ge::Format format = ge::TypeUtils::SerialStringToFormat(StringUtils::Trim(format_str_vec[i]));
      if (format == ge::FORMAT_NULL) {
        FE_LOGD("Format is NULL, index is : %zu.", i);
        continue;
      }
      if (format == ge::FORMAT_RESERVED) {
        REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Invalid format: whole [%s], format [%zu] = [%s], \
                        can not convert to ge::Format enum.", format_str.c_str(), i, format_str_vec[i].c_str());
        return FAILED;
      }
      input_or_output_info->supported_unknown_shape_formats_.push_back(format);

      ge::DataType dtype;
      if (String2DataType(StringUtils::Trim(dtype_str_vec[i]), dtype) != SUCCESS) {
        REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] String2DataType %s failed. dtype is illegal!",
                        dtype_str_vec[i].c_str());
        return FAILED;
      }
      input_or_output_info->supported_unknown_shape_dtypes_.push_back(dtype);
    }
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::GetStrFormMap(const map<string, string> &map_info, string key, string &value) {
  std::map<string, string>::const_iterator find = map_info.find(key);
  if (find == map_info.end()) {
    return OP_STORE_MAP_KEY_FIND_FAILED;
  }
  value = find->second;
  return SUCCESS;
}

template <typename T>
Status OpKernelInfoConstructor::ConvertListAttrValue(const OpContent &op_content, string attr_name,
                                                     vector<vector<T>> &list_attr_vec, AttrInfoPtr attr_info) const {
  string list_value_str;
  string default_value_str;

  // Get Default Value for optional Attr
  if (!attr_info->is_required_) {
    if (GetStrFromOpContent(op_content, attr_name, STR_DEFAULT_VALUE, default_value_str) == SUCCESS) {
      vector<string> default_list_str_vec = StringUtils::Split(default_value_str, ';');
      for (auto &default_list_elem_str : default_list_str_vec) {
        std::vector<T> default_list_vec;
        default_list_elem_str.erase(std::remove(default_list_elem_str.begin(), default_list_elem_str.end(), '['),
                                    default_list_elem_str.end());
        default_list_elem_str.erase(std::remove(default_list_elem_str.begin(), default_list_elem_str.end(), ']'),
                                    default_list_elem_str.end());
        if (ConvertStr2VecNumber(default_list_elem_str, default_list_vec) != SUCCESS) {
          REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op[%s] Convert default AttrValue failed.",
                          op_content.op_type_.c_str());
          return FAILED;
        }
        list_attr_vec.push_back(default_list_vec);
      }

      /* we only takes the first default value. Here in function
        * GetStrFromOpContent we eliminate cases that default value string is
        * empty
      */
      if (!list_attr_vec.empty()) {
        attr_info->default_value_ = GeAttrValue::CreateFrom<vector<T>>(list_attr_vec[0]);
        attr_info->is_default_value_defined_ = true;
      }

    } else {
      attr_info->is_default_value_defined_ = false;
      FE_LOGD("Get %s Attr %s DefaultValue not success.", op_content.op_type_.c_str(), attr_name.c_str());
    }
  }

  if (attr_info->is_support_all_value_) {
    return SUCCESS;
  }

  if (GetStrFromOpContent(op_content, attr_name, STR_VALUE, list_value_str) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s get list value attr %s failed.",
                    op_content.op_type_.c_str(), attr_name.c_str());
    return FAILED;
  }

  vector<string> list_str_vec = StringUtils::Split(list_value_str, ';');
  for (auto &list_elem_str : list_str_vec) {
    std::vector<T> list_vec;
    list_elem_str.erase(std::remove(list_elem_str.begin(), list_elem_str.end(), '['), list_elem_str.end());
    list_elem_str.erase(std::remove(list_elem_str.begin(), list_elem_str.end(), ']'), list_elem_str.end());
    if (ConvertStr2VecNumber(list_elem_str, list_vec) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s Convert AttrValue faild.", op_content.op_type_.c_str());
      return FAILED;
    }
    list_attr_vec.push_back(list_vec);
  }
  return SUCCESS;
}

template <typename T>
Status OpKernelInfoConstructor::InitAttrTemplate(const OpContent &op_content, string attr, AttrInfoPtr attr_info)
    const {
  string attr_and_prefix = STR_ATTR_PREFIX + attr;
  vector<T> template_vec;
  string template_str;
  string default_int_str;

  /* Get Default Value for optional Attr */
  if (!attr_info->is_required_) {
    if (GetStrFromOpContent(op_content, attr_and_prefix, STR_DEFAULT_VALUE, default_int_str) == SUCCESS) {
      if (ConvertStr2VecNumber(default_int_str, template_vec) == SUCCESS) {
        /* we only takes the first default value. Here in function
         * GetStrFromOpContent we eliminate cases that default value string is
         * empty
         */
        if (!template_vec.empty()) {
          attr_info->default_value_ = GeAttrValue::CreateFrom<T>(template_vec[0]);
          attr_info->is_default_value_defined_ = true;
        }
      } else {
        FE_LOGW("Op %s convert DefaultValue string to int %s not success.", op_content.op_type_.c_str(),
                default_int_str.c_str());
      }

    } else {
      attr_info->is_default_value_defined_ = false;
      FE_LOGW("Fail to get default_value of attribute %s in op %s.", attr_and_prefix.c_str(),
              op_content.op_type_.c_str());
    }
  }

  if (attr_info->is_support_all_value_) {
    return SUCCESS;
  }

  if (GetStrFromOpContent(op_content, attr_and_prefix, STR_VALUE, template_str) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s get Int Attr %s failed.",
                    op_content.op_type_.c_str(), attr_and_prefix.c_str());
    return FAILED;
  }
  if (ConvertStr2VecNumber(template_str, template_vec) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s convert int %s failed.",
                    op_content.op_type_.c_str(), template_str.c_str());
    return FAILED;
  }

  for (const auto &it : template_vec) {
    ge::GeAttrValue attr_value = GeAttrValue::CreateFrom<T>(it);
    attr_info->supported_values_.push_back(attr_value);
  }
  return SUCCESS;
}
template <typename T>
Status OpKernelInfoConstructor::InitAttrListTemplate(const OpContent &op_content, string attr,
                                                     AttrInfoPtr attr_info) {
  string attr_and_prefix = STR_ATTR_PREFIX + attr;
  vector<vector<T>> list_attr_vec;
  vector<GeAttrValue> attr_value_vec;
  if (ConvertListAttrValue<T>(op_content, attr_and_prefix, list_attr_vec, attr_info) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s convert ListInt %s fail.",
                    op_content.op_type_.c_str(), attr_and_prefix.c_str());
    return FAILED;
  }
  for (auto &el : list_attr_vec) {
    GeAttrValue attr_value = GeAttrValue::CreateFrom<vector<T>>(el);
    attr_info->supported_values_.push_back(attr_value);
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitAttrValueSub(const OpContent &op_content, OpKernelInfoPtr op_kernel_info) {
  for (auto &attr : op_kernel_info->attrs_info_) {
    Status status = SUCCESS;
    switch (attr->dtype_) {
      case GeAttrValue::VT_INT: {
        status = InitAttrTemplate<int64_t>(op_content, attr->attr_name_, attr);
        break;
      }
      case GeAttrValue::VT_FLOAT: {
        status = InitAttrTemplate<float>(op_content, attr->attr_name_, attr);
        break;
      }
      case GeAttrValue::VT_BOOL: {
        status = InitAttrTemplate<bool>(op_content, attr->attr_name_, attr);
        break;
      }
      case GeAttrValue::VT_STRING: {
        status = InitAttrTemplate<string>(op_content, attr->attr_name_, attr);
        break;
      }
      case GeAttrValue::VT_LIST_INT: {
        status = InitAttrListTemplate<int64_t>(op_content, attr->attr_name_, attr);
        break;
      }
      case GeAttrValue::VT_LIST_FLOAT: {
        status = InitAttrListTemplate<float>(op_content, attr->attr_name_, attr);
        break;
      }
      case GeAttrValue::VT_LIST_BOOL: {
        status = InitAttrListTemplate<bool>(op_content, attr->attr_name_, attr);
        break;
      }
      default: {
        status = InitAttrListTemplate<string>(op_content, attr->attr_name_, attr);
        break;
      }
    }
    if (status != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s init attr %s value failed.",
                      op_content.op_type_.c_str(), attr->attr_name_.c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitAttrValue(const OpContent &op_content, OpKernelInfoPtr op_kernel_info) {
  vector<string> attr_vec;
  auto attr_pos = op_content.map_kernel_info_.find(STR_ATTR);
  if (op_content.map_kernel_info_.end() == attr_pos) {
    return SUCCESS;
  }
  string attr_str;
  if (GetStrFromOpContent(op_content, STR_ATTR, STR_LIST, attr_str) != SUCCESS) {
    FE_LOGW("Op %s get attr.list not success.", op_content.op_type_.c_str());
    return SUCCESS;
  }
  attr_vec = StringUtils::Split(attr_str, ',');
  for (auto &attr : attr_vec) {
    string attr_type_str;
    string attr_value_str;
    string param_type_str;

    string attr_name = STR_ATTR_PREFIX + StringUtils::Trim(attr);
    if (GetStrFromOpContent(op_content, attr_name, STR_TYPE, attr_type_str) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] OpKernelInfoConstructor::InitAttrValue, \
                      Op %s get Attr %s.type failed, check the ops_kernel_info.",
                      op_content.op_type_.c_str(), attr_name.c_str());
      return FAILED;
    }
    if (GetStrFromOpContent(op_content, attr_name, STR_VALUE, attr_value_str) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] OpKernelInfoConstructor::InitAttrValue, \
                      Op %s get Attr %s.value failed, check the ops_kernel_info.",
                      op_content.op_type_.c_str(), attr_name.c_str());
      return FAILED;
    }
    if (GetStrFromOpContent(op_content, attr_name, STR_PARAM_TYPE, param_type_str) != SUCCESS) {
      FE_LOGD("OpKernelInfoConstructor::InitAttrValue, Op %s get %s.param_type_str failed, check the ops_kernel_info.",
          op_content.op_type_.c_str(), attr_name.c_str());
      continue;
    }
    std::shared_ptr<AttrInfo> new_attr_info = nullptr;
    FE_MAKE_SHARED(new_attr_info = std::make_shared<AttrInfo>(StringUtils::Trim(attr)), return FAILED);
    if (new_attr_info == nullptr) {
      FE_LOGW("Op %s make shared ptr for new_attr_info failed", op_content.op_type_.c_str());
      continue;
    }

    if (param_type_str == STR_REQUIRED) {
      new_attr_info->is_required_ = true;
    } else {
      new_attr_info->is_required_ = false;
    }

    std::map<std::string, ge::GeAttrValue::ValueType>::const_iterator find_attr_type =
            g_attr_type_map.find(attr_type_str);
    if (find_attr_type == g_attr_type_map.end()) {
      FE_LOGW("Attr type [%s] of Attr [%s] is invalid for op [%s]", attr_type_str.c_str(), attr_name.c_str(),
              op_content.op_type_.c_str());
      continue;
    }

    new_attr_info->dtype_ = find_attr_type->second;
    bool is_all = StringUtils::MatchString(STR_ALL, attr_value_str);
    new_attr_info->is_support_all_value_ = is_all;
    op_kernel_info->attrs_info_.push_back(new_attr_info);
  }

  if (InitAttrValueSub(op_content, op_kernel_info) != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] OpKernelInfoConstructor::InitAttrValueSub failed.");
    return FAILED;
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::GetStrFromOpContent(const OpContent &op_content, const string &key1,
                                                    const string &key2, string &value) const {
  vector<string> attr_vec;
  auto key1_pos = op_content.map_kernel_info_.find(key1);
  if (op_content.map_kernel_info_.end() == key1_pos) {
    if (key1 != STR_RESHAPE_TYPE && key2 != STR_DEFAULT_VALUE) {
      FE_LOGD("Op %s not found %s in OpContent!", op_content.op_type_.c_str(), key1.c_str());
    }
    return OP_STORE_MAP_KEY_FIND_FAILED;
  }

  auto key2_pos = key1_pos->second.find(key2);
  if (key1_pos->second.end() == key2_pos) {
    if (key1 != STR_RESHAPE_TYPE && key2 != STR_DEFAULT_VALUE) {
      FE_LOGD("Op %s not found %s.%s in OpContent!", op_content.op_type_.c_str(), key1.c_str(), key2.c_str());
    }
    return OP_STORE_MAP_KEY_FIND_FAILED;
  }

  value = key2_pos->second;
  if (value == "") {
    if (key1 != STR_PRECISION_POLICY) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Op %s op_content %s.%s's value is invalid.",
                      op_content.op_type_.c_str(), key1.c_str(), key2.c_str());
    }
    return FAILED;
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitSlicePattern(const OpContent &op_content, OpKernelInfoPtr op_kernel_info) const {
  string slice_pattern_str;
  (void)GetStrFromOpContent(op_content, STR_SLICE_PATTERN, STR_VALUE, slice_pattern_str);
  if (!slice_pattern_str.empty()) {
    auto iter = STR_SLICE_PATTERN_MAP.find(slice_pattern_str);
    if (iter == STR_SLICE_PATTERN_MAP.end()) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The param value[%s] of [%s] in op[%s] is invalid.",
                      slice_pattern_str.c_str(), STR_SLICE_PATTERN.c_str(),
                      op_content.op_type_.c_str());
      return PARAM_INVALID;
    }
    op_kernel_info->slice_pattern_ = iter->second;
  }
  return SUCCESS;
}

Status OpKernelInfoConstructor::InitOpInfo(std::string engine_name, const OpContent &op_content,
                                           OpKernelInfoPtr op_kernel_info) {
  op_kernel_info->op_info_.engine = engine_name;
  op_kernel_info->op_info_.flagPartial = false;
  op_kernel_info->op_info_.flagAsync = false;
  op_kernel_info->op_info_.computeCost = COMPUTE_COST_DEFAULT;

  string is_heavy_op_str;
  (void)GetStrFromOpContent(op_content, STR_HEAVY_OP, STR_FLAG, is_heavy_op_str);
  if (!is_heavy_op_str.empty()) {
    auto iter = STR_BOOL_MAP.find(is_heavy_op_str);
    if (iter == STR_BOOL_MAP.end()) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The param value[%s] of [%s] in op[%s] is invalid.",
                      is_heavy_op_str.c_str(), STR_HEAVY_OP.c_str(), op_content.op_type_.c_str());
      return PARAM_INVALID;
    }
    op_kernel_info->is_heavy_o_p_ = iter->second;
  }

  string range_limit;
  (void)GetStrFromOpContent(op_content, STR_RANGE_LIMIT, STR_VALUE, range_limit);
  FE_LOGD("[InitOpInfoLib][InitOpKernel] Op %s get range_limit value is %s.", op_content.op_type_.c_str(),
          range_limit.c_str());
  if (!range_limit.empty()) {
    FE_CHECK(STR_RANGE_LIMIT_SET.count(range_limit) == 0,
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The param value[%s] of [%s] in op[%s] is invalid.",
                      range_limit.c_str(), STR_RANGE_LIMIT.c_str(), op_content.op_type_.c_str()),
      return PARAM_INVALID);
    op_kernel_info->is_limited_range_ = range_limit;
  }

  Status ret = InitSlicePattern(op_content, op_kernel_info);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Init SlicePattern for node[%s] failed.",
                    op_content.op_type_.c_str());
    return ret;
  }

  string need_check_support;
  (void)GetStrFromOpContent(op_content, STR_NEED_CHECK_SUPPORT, STR_FLAG, need_check_support);
  if (!need_check_support.empty()) {
    auto iter = STR_BOOL_MAP.find(need_check_support);
    if (iter == STR_BOOL_MAP.end()) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The param value[%s] of [%s] in op[%s] is invalid.",
                      need_check_support.c_str(), STR_NEED_CHECK_SUPPORT.c_str(), op_content.op_type_.c_str());
      return PARAM_INVALID;
    }
    op_kernel_info->need_check_support_ = iter->second;
  }

  Status result;
  string op_file;
  result = GetStrFromOpContent(op_content, STR_OP_FILE, STR_VALUE, op_file);
  bool op_file_empty_flag = (result != SUCCESS || op_file.empty());
  if (op_file_empty_flag) {
    FE_LOGD("Op %s can't get %s op_file value.", op_content.op_type_.c_str(), op_content.op_type_.c_str());
    op_kernel_info->op_info_.opFileName = "";
  } else {
    FE_LOGD("Op %s get op_file value is %s.", op_content.op_type_.c_str(), op_file.c_str());
    op_kernel_info->op_info_.opFileName = op_file;
  }

  string op_interface;
  result = GetStrFromOpContent(op_content, STR_OP_INTERFACE, STR_VALUE, op_interface);
  bool op_interface_empty_flag = (result != SUCCESS || op_interface.empty());
  if (op_interface_empty_flag) {
    FE_LOGD("Op %s can't get %s op_interface value.", op_content.op_type_.c_str(), op_content.op_type_.c_str());
    op_kernel_info->op_info_.opFuncName = "";
  } else {
    FE_LOGD("Op %s get op_interface value is %s.", op_content.op_type_.c_str(), op_interface.c_str());
    op_kernel_info->op_info_.opFuncName = op_interface;
  }

  result = GetPrecisionPolicyFromOpContent(op_content, op_kernel_info);
  if (result != SUCCESS) {
    return FAILED;
  }

  string dynamic_compile_static;
  (void)GetStrFromOpContent(op_content, STR_DYNAMIC_COMPILE_STATIC, STR_FLAG, dynamic_compile_static);
  if (!dynamic_compile_static.empty()) {
    auto iter = STR_BOOL_MAP.find(dynamic_compile_static);
    if (iter == STR_BOOL_MAP.end()) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] The param value[%s] of [%s] in op[%s] is invalid.",
                      dynamic_compile_static.c_str(), STR_DYNAMIC_COMPILE_STATIC.c_str(), op_content.op_type_.c_str());
      return PARAM_INVALID;
    }
    op_kernel_info->dynamic_compile_static_ = iter->second;
  } else {
    op_kernel_info->dynamic_compile_static_ = false;
  }

  return SUCCESS;
}

Status OpKernelInfoConstructor::GetPrecisionPolicyFromOpContent(const OpContent &op_content,
                                                                const OpKernelInfoPtr &op_kernel_info) const {
  std::vector<std::string> precision_policy_vec = {STR_PRECISION_POLICY, STR_PRECISION_POLICY_BF16};
  for (auto choose_precision : precision_policy_vec) {
    string precision_reduce;
    PrecisionPolicy precision_policy = GRAY;
    Status result = GetStrFromOpContent(op_content, choose_precision, STR_FLAG, precision_reduce);
    if (result != SUCCESS) {
      FE_LOGD("Op %s can't get precision_policy value, use default value.", op_content.op_type_.c_str());
      precision_reduce = "";
    }
    if (precision_reduce == "true") {
      precision_policy = WHITE;
    } else if (precision_reduce == "false") {
      precision_policy = BLACK;
    } else if (precision_reduce.empty()) {
      precision_policy = GRAY;
    } else {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] precision reduce flag (%s) of op %s is invalid",
                      precision_reduce.c_str(), op_content.op_type_.c_str());
      return FAILED;
    }
    if (choose_precision == STR_PRECISION_POLICY) {
      FE_LOGD("Op %s's precision_policy value is %s.", op_content.op_type_.c_str(),
              GetPrecisionPolicyString(precision_policy).c_str());
      op_kernel_info->op_store_info_.precision_policy = precision_policy;
    } else {
      FE_LOGD("Op %s's precision_policy_bf16 value is %s.", op_content.op_type_.c_str(),
              GetPrecisionPolicyString(precision_policy).c_str());
      op_kernel_info->op_store_info_bf16_.precision_policy = precision_policy;
    }
  }
  return SUCCESS;
}

void OpKernelInfoConstructor::SetUniqueName(InputOrOutputInfoPtr input_or_output_info_ptr) const {
  string unique_name_prefix = input_or_output_info_ptr->is_input_ ? STR_INPUT_LOWERCASE : STR_OUTPUT_LOWERCASE;
  input_or_output_info_ptr->unique_name_ =
      unique_name_prefix + to_string(input_or_output_info_ptr->index_) + "." + input_or_output_info_ptr->name_;
}

Status OpKernelInfoConstructor::InitDtypeByPattern(const map<string, string> &map_info,
                                                   InputOrOutputInfoPtr input_or_output_info,
                                                   uint32_t &dtype_and_format_size_of_first_input,
                                                   const OpPattern &op_pattern) {
  if (op_pattern == OP_PATTERN_BROADCAST || op_pattern == OP_PATTERN_REDUCE) {
    if (InitDtype(map_info, input_or_output_info, dtype_and_format_size_of_first_input) != SUCCESS) {
      REPORT_FE_ERROR("[InitOpInfoLib][InitOpKernel] Init dtypes failed by pattern.");
      return FAILED;
    }
  }
  return SUCCESS;
}
}  // namespace fe
