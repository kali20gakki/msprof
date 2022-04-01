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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_UNKNOWN_SHAPE_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_UNKNOWN_SHAPE_UTIL_H_

#include <map>
#include <string>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "graph_optimizer/range_format_transfer/transfer_range_according_to_format.h"
#include "ops_store/op_kernel_info.h"

namespace fe {
/*
 *  @ingroup fe
 *  @brief   get sting of shape range.
 *  @param   [in]  input or output tensor.
 *  @return  shape range
 */
std::string ShapeRangeToStr(const std::vector<std::pair<int64_t, int64_t>> &shape_range);

/*
 *  @ingroup fe
 *  @brief   get shape range of unknown shape op.
 *  @param   [in]  input or output tensor.
 *  @return  shape range
 */
std::vector<std::pair<int64_t, int64_t>> GetShapeRange(const ge::GeTensorDesc &tensor_desc);

std::vector<std::pair<int64_t, int64_t>> GetValueRange(const ge::GeTensorDesc &tensor_desc);
/*
 *  @ingroup fe
 *  @brief   get new shape range aligned to shape.
 *  @param   [in]  input or output tensor.
 *  @return  shape range
 */
std::vector<std::pair<int64_t, int64_t>> GetAlignShapeRange(
    const std::vector<std::pair<int64_t, int64_t>> &ori_shape_range, const ge::GeShape &shape);

/*
 *  @ingroup fe
 *  @brief   set shape range of unknown shape op.
 *  @param   [in]  input or output tensor.
 *  @return  shape range
 */
Status SetShapeRange(const ge::OpDesc &op_desc, RangeAndFormat &range_and_format, ge::GeTensorDesc &tensor_desc);

/*
 *  @ingroup fe
 *  @brief   check whether the node is unknown shape.
 *  @param   [in]  input or output tensor.
 *  @return  true: unknown; false: known
 */
bool IsUnKnownShapeOp(const ge::OpDesc &op_desc, const bool is_use_attr_check = false);

/*
 *  @ingroup fe
 *  @brief   check whether the tensors of node is unknown shape.
 *  @param   [in]  input or output tensor.
 *  @return  true: unknown; false: known
 */
bool IsUnKnownShapeTensor(const ge::OpDesc &op_desc);

/*
*  @ingroup fe
*  @brief   check whether the input or output shape contains -2.
*  @param   op_desc input or output desc.
*  @return  true: contains; false: not contains
*/
bool IsContainUnknownDimNum(const ge::OpDesc &op_desc);

bool IsShapeContainUnknownDimNum(const ge::GeShape &shape);

/** to check whether one value of tensor's shape is unknown or not
 *
 * @param one value of tensor's shape
 *
 * @return true: unknown, false: known
 */
bool IsUnknownShapeValue(const int64_t &value);

/*
 *  @ingroup fe
 *  @brief   check whether fuzzy build.
 *  @param   [in]  nothing.
 *  @return  true: is fuzzy build; false: is not fuzzy build
 */
bool IsFuzzBuild();

bool IsFuzzBuildOp(const ge::OpDesc &op_desc);

bool IsFeSupportedDynamicOp(const ge::OpDesc &op_desc, bool is_use_attr_check = false);
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_UNKNOWN_SHAPE_UTIL_H_
