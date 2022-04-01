/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#ifndef AICPU_ENGINE_UTIL_H_
#define AICPU_ENGINE_UTIL_H_

#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

#include "error_code/error_code.h"
#include "external/ge/ge_api_error_codes.h"
#include "graph/compute_graph.h"
#include "proto/cpu_node_def.pb.h"
#include "aicpu_ops_kernel_info_store/op_struct.h"
#include "common/aicpu_ops_kernel_builder/kernel_builder.h"
#include "common/sgt_slice_type.h"

namespace aicpu {

ge::Status BuildAicpuNodeDef(const ge::OpDescPtr &op_desc_ptr,
                             aicpuops::NodeDef &node_def);
ge::Status GetAttrValueFromGe(const ge::OpDescPtr &op_desc_ptr,
                              const std::string &attr_name,
                              const ge::GeAttrValue::ValueType ge_value_type,
                              aicpuops::AttrValue &attr_value);

ge::Status FillAttrOfAicpuNodeDef(const ge::OpDescPtr &op_desc_ptr, aicpuops::NodeDef &node_def);

ge::Status GetAicpuFftsPlusInfo(const ge::OpDescPtr &op_desc_ptr,
                                FftsPlusInfo &ffts_info);

ge::Status InsertAicpuNodeDefAttrToOp(const ge::OpDescPtr &op_desc_ptr,
                                     aicpuops::NodeDef &node_def,
                                     const std::string &attr_name);

ge::Status BuildFftsPlusAicpuNodeShapeInfo(const ge::OpDescPtr &op_desc_ptr,
                                           aicpuops::NodeDef &node_def,
                                           const FftsPlusInfo &ffts_info);

ge::Status BuildFftsPlusAicpuNodeDef(const ge::OpDescPtr &op_desc_ptr,
                                     FftsPlusInfo &ffts_info);
                                     
/**
 * Set shape type
 * @param op_desc_ptr op desc
 * @param all_op_info op info
 * @return status whether this operation success
 */
ge::Status CheckAndSetUnknowType(ge::OpDescPtr &op_desc_ptr, const map<string, OpFullInfo> &all_op_info);
}  // namespace aicpu

#endif
