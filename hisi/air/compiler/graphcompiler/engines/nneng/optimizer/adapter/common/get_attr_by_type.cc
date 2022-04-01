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
#include "get_attr_by_type.h"

namespace fe {
template <typename T>
Status GetAttrValue(const AttrInfoPtr& attrs_info, T &value) {
  if (attrs_info->GetIsRequired()) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Get attr value failed.");
    return FAILED;
  } else {
    if (!attrs_info->GetDefaultValueDefinedFlag()) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] No default value, get attr value failed.");
      return FAILED;
    } else {
      if (attrs_info->GetDefaultValue().GetValue<T>(value) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Get default value failed.");
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status GetStrAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                       te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  string str_value;
  if (!ge::AttrUtils::GetStr(&op_desc, attr_name, str_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, str_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get string attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, str_value);
  return SUCCESS;
}

Status GetIntAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                       te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  int64_t int_value = 0;
  if (!ge::AttrUtils::GetInt(&op_desc, attr_name, int_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, int_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get int attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, int_value);
  return SUCCESS;
}

Status GetFloatAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                         te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  float float_value = 0.0;
  if (!ge::AttrUtils::GetFloat(&op_desc, attr_name, float_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, float_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get float attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, float_value);
  return SUCCESS;
}

Status GetBoolAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                        te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  bool bool_value = true;
  if (!ge::AttrUtils::GetBool(&op_desc, attr_name, bool_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, bool_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get bool attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, bool_value);
  return SUCCESS;
}

Status GetListStrAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                           te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  vector<string> vec_str_value;
  if (!ge::AttrUtils::GetListStr(&op_desc, attr_name, vec_str_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, vec_str_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get vec_str attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, vec_str_value);
  return SUCCESS;
}

Status GetListIntAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                           te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  vector<int64_t> vec_int_value;
  if (!ge::AttrUtils::GetListInt(&op_desc, attr_name, vec_int_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, vec_int_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get vec_int attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, vec_int_value);
  return SUCCESS;
}

Status GetListListIntAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                               te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  vector<vector<int64_t>> vecvec_int_value;
  FE_LOGD("ListListInt attr_name is %s.", attr_name.c_str());
  if (!ge::AttrUtils::GetListListInt(&op_desc, attr_name, vecvec_int_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, vecvec_int_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get list_list_int attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, vecvec_int_value);
  return SUCCESS;
}

Status GetListFloatAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                             te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  vector<float> vec_float_value;
  if (!ge::AttrUtils::GetListFloat(&op_desc, attr_name, vec_float_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, vec_float_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get vec_float attr [%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, vec_float_value);
  return SUCCESS;
}

Status GetListBoolAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                            te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  vector<bool> vec_bool_value;
  if (!ge::AttrUtils::GetListBool(&op_desc, attr_name, vec_bool_value)) {
    if (attrs_info == nullptr || GetAttrValue(attrs_info, vec_bool_value) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Op [%s] get vec_bool attr[%s] value failed.",
                      op_desc.GetName().c_str(), attr_name.c_str());
      return FAILED;
    }
  }
  attr_value = te::TbeAttrValue(attr_name, vec_bool_value);
  return SUCCESS;
}

Status GetTensorAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                          const te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] Tensor attr [%s] value not supported in op [%s].",
                  attr_name.c_str(), op_desc.GetName().c_str());
  return FAILED;
}

Status GetListTensorAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                              const te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info) {
  REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp] ListTensor attr [%s] value not supported in op [%s].",
                  attr_name.c_str(), op_desc.GetName().c_str());
  return FAILED;
}

} // namespace fe