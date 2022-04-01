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
#ifndef FUSION_ENGINE_ADAPTER_COMMON_GET_ATTR_BY_TYPE_H_
#define FUSION_ENGINE_ADAPTER_COMMON_GET_ATTR_BY_TYPE_H_

#include "graph/compute_graph.h"
#include "graph/ge_attr_value.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/fe_inner_error_codes.h"
#include "common/op_info_common.h"
#include "tensor_engine/fusion_api.h"

namespace fe{
using GetAttrValueByType =
    std::function<Status(const ge::OpDesc &, const string &, te::TbeAttrValue &, AttrInfoPtr)>;

template <typename T>
Status GetAttrValue(const AttrInfoPtr& attrs_info, T &value);

Status GetStrAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                       te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetIntAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                       te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetFloatAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                         te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetBoolAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                        te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetListStrAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                           te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetListIntAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                           te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetListListIntAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                               te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetListFloatAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                             te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetListBoolAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                            te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetTensorAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                          const te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

Status GetListTensorAttrValue(const ge::OpDesc &op_desc, const string &attr_name,
                              const te::TbeAttrValue &attr_value, const AttrInfoPtr& attrs_info);

const std::map<ge::GeAttrValue::ValueType, GetAttrValueByType> k_attr_get_funcs = {
    {ge::GeAttrValue::VT_STRING, GetStrAttrValue},
    {ge::GeAttrValue::VT_INT, GetIntAttrValue},
    {ge::GeAttrValue::VT_FLOAT, GetFloatAttrValue},
    {ge::GeAttrValue::VT_BOOL, GetBoolAttrValue},
    {ge::GeAttrValue::VT_LIST_STRING, GetListStrAttrValue},
    {ge::GeAttrValue::VT_LIST_INT, GetListIntAttrValue},
    {ge::GeAttrValue::VT_LIST_FLOAT, GetListFloatAttrValue},
    {ge::GeAttrValue::VT_LIST_BOOL, GetListBoolAttrValue},
    {ge::GeAttrValue::VT_LIST_LIST_INT, GetListListIntAttrValue},
    {ge::GeAttrValue::VT_TENSOR, GetTensorAttrValue},
    {ge::GeAttrValue::VT_LIST_TENSOR, GetListTensorAttrValue},
};
} // namespace fe
#endif // FUSION_ENGINE_ADAPTER_COMMON_GET_ATTR_BY_TYPE_H_
