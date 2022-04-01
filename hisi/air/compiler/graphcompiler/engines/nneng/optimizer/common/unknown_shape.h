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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_UNKNOWN_SHAPE_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_UNKNOWN_SHAPE_H_

#include <map>
#include <map>
#include <string>
#include <string>
#include <vector>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "ops_store/op_kernel_info.h"

namespace fe {

/*
 *  @ingroup fe
 *  @brief   check c axis is unknown shape or not
 *  @param   [in]  format
 *  @param   [in]  shape
 *  @return  true: unknown, false: known
 */
bool IsCUnknownShape(const ge::Format &format, const ge::GeShape &shape);

/*
 *  @ingroup fe
 *  @brief   check last two axis is unknown shape or not
 *  @param   [in]  format
 *  @param   [in]  shape
 *  @return  true: unknown, false: known
 */
bool IsLastTwoUnknownShape(const ge::Format &format, const ge::GeShape &shape);
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_UNKNOWN_SHAPE_H_
