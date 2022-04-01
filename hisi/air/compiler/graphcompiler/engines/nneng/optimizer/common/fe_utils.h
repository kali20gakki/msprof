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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_FE_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_FE_UTIL_H_

#include <memory>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "common/util/constants.h"
#include "common/util/op_info_util.h"
#include "graph/utils/type_utils.h"
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_base.h"
#include "register/graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "common/lxfusion_json_util.h"

namespace fe {
// send error message to error manager
void LogErrorMessage(std::string error_code, const std::map<std::string, std::string> &args_map);

// Save error message to error manager
void SaveErrorMessage(const std::string &graph_name, const std::map<std::string, std::string> &args_map);

int64_t GetMicroSecondTime();

uint64_t GetCurThreadId();

uint64_t GetAtomicId();

std::string FormatToStr(ge::Format format);
std::string DTypeToStr(ge::DataType d_type);

std::string GetBoolString(bool &bool_value);

std::string GetCheckSupportedString(te::CheckSupportedResult &check_supported);

std::string GetImplTypeString(OpImplType op_impl_type);

std::string GetGeImplTypeString(domi::ImplyType ge_impl_type);

bool IsInvalidImplType(const std::string &engine_name, const OpImplType &op_impl_type);

std::string ConstFormatToStr(const ge::Format &format);

std::string GetPassTypeString(GraphFusionPassType pass_type);

std::string GetRuleTypeString(RuleType rule_type);

std::string GetBufferFusionPassTypeString(BufferFusionPassType pass_type);

/**
 * check file path is right or not
 * @param filepath
 * @return
 */
bool CheckFilePath(std::string file_path, std::string delimiter);

int64_t GetGreatestCommonDivisor(int64_t x, int64_t y);

int64_t GetLeastCommonMultiple(int64_t x, int64_t y);

uint32_t GetPlatformSCubeVecSplitFlag();

CubeVecState CheckCubeVecState();

AICoreMode GetPlatformAICoreMode();

/**
 * check file is empty or not
 * @param file_name
 * @return
 */
bool CheckFileEmpty(const std::string &file_name);

// Record the start time of stage.
#define FE_TIMECOST_START(stage) int64_t start_usec_##stage = GetMicroSecondTime();

// Print the log of time cost of stage.
#define FE_TIMECOST_END(stage, stage_name)                                              \
  {                                                                                     \
    int64_t end_usec_##stage = GetMicroSecondTime();                                    \
    FE_LOGV("[FE_PERFORMANCE]The time cost of %s is [%ld] micro second.", (stage_name), \
            (end_usec_##stage - start_usec_##stage));                                   \
  }

template <typename T, typename... Args>
static inline std::shared_ptr<T> FeComGraphMakeShared(Args &&... args) {
  using T_nc = typename std::remove_const<T>::type;
  std::shared_ptr<T> ret(new (std::nothrow) T_nc(std::forward<Args>(args)...));
  return ret;
}
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_FE_UTIL_H_
