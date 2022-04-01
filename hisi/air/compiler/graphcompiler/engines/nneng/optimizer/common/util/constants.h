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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_CONSTANTS_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_CONSTANTS_H_
#include <map>
#include <string>
#include "common/aicore_util_constants.h"

namespace fe {
const std::string FUSION_SWITCH_ON = "on";
const std::string FUSION_SWITCH_OFF = "off";

/* type of AI core and vector core */
const std::string AI_CORE_TYPE = "AiCore";
const std::string VECTOR_CORE_TYPE = "VectorCore";
const std::string TBE_APPEND_ARGS_MODE = "l2_mode";
const std::string TBE_AUTO_TILING_MODE = "auto_tune_mode";
const std::string TBE_DEVICE_ID = "device_id";
const std::string OP_STATUS_CHECK = "ge.status_check";

const std::string STR_SOC_VERSION = "soc_version";
const std::string kFeEngineType = "fe.engineType";
// fusion switch
const std::string UB_FUSION = "UBFusion";
const std::string GRAPH_FUSION = "GraphFusion";
// min coefficients of reduce op which needed ub size
const int REDUCE_COEFFICIENTS = 2;
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_CONSTANTS_H_
