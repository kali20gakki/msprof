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

/** @defgroup FE_ERROR_CODES_GROUP FE Error Code Interface */
/** FE error code definition
 *  @ingroup FE_ERROR_CODES_GROUP
 */

#ifndef FUSION_ENGINE_UTILS_COMMON_COMM_ERROR_CODES_H_
#define FUSION_ENGINE_UTILS_COMMON_COMM_ERROR_CODES_H_

#include <map>
#include <string>
#include "graph_optimizer/graph_optimize_register_error_codes.h"

namespace fe {
/** Task Builder module ID */
const uint8_t FE_MODID_TASK_BUILDER = 57;
/** Op calculator module ID */
const uint8_t FE_MODID_OP_CALCULATOR = 62;

#define FE_DEF_ERRORNO_OP_CALCULATOR(name, value, desc) \
  FE_DEF_ERRORNO(SYSID_FE, FE_MODID_OP_CALCULATOR, name, value, desc)
#define FE_DEF_ERRORNO_TASK_BUILDER(name, value, desc) \
  FE_DEF_ERRORNO(SYSID_FE, FE_MODID_TASK_BUILDER, name, value, desc)

FE_DEF_ERRORNO_COMMON(INVALID_FILE_PATH, 8, "Failed to get the valid file path.");                        // 0x3320008
FE_DEF_ERRORNO_COMMON(LACK_MANDATORY_CONFIG_KEY, 9, "The mandatory key is not configured in files.");     // 0x3320009
FE_DEF_ERRORNO_COMMON(OPSTORE_NAME_EMPTY, 10, "The name of opstore config is empty.");                    // 0x3320010
FE_DEF_ERRORNO_COMMON(OPSTORE_VALUE_EMPTY, 11, "The value of opstore config is empty.");                  // 0x3320011
FE_DEF_ERRORNO_COMMON(OPSTORE_VALUE_ITEM_SIZE_INCORRECT, 12, "The size of opstore items is incorrect.");  // 0x3320012
FE_DEF_ERRORNO_COMMON(OPSTORE_VALUE_ITEM_EMPTY, 13, "At least one of the opstore item is empty.");        // 0x3320013
FE_DEF_ERRORNO_COMMON(OPSTORE_EMPTY, 14, "There is no OP store in configuration files.");                 // 0x3320014
FE_DEF_ERRORNO_COMMON(OPSTORE_OPIMPLTYPE_REPEAT, 15, "OP imple type of OP stores can not be repeated.");  // 0x3320015
FE_DEF_ERRORNO_COMMON(OPSTORE_OPIMPLTYPE_INVALID, 16, "The OP imple type of OP store is invalid.");       // 0x3320016
FE_DEF_ERRORNO_COMMON(OPSTORE_PRIORITY_INVALID, 17, "The priority of OP stores is invalid.");             // 0x3320017
FE_DEF_ERRORNO_COMMON(OPSTORE_CONFIG_NOT_INTEGRAL, 18, "The content of ops store is not integral.");      // 0x3320018

FE_DEF_ERRORNO_OP_CALCULATOR(TENSOR_FORMAT_INVALID, 1, "This format is not valid.");                        // 0x3320019
FE_DEF_ERRORNO_OP_CALCULATOR(TENSOR_DATATYPE_INVALID, 2, "This data type is not valid.");                   // 0x3320020
FE_DEF_ERRORNO_OP_CALCULATOR(DIM_VALUE_INVALID, 3, "The dim value must be great than zero.");               // 0x3320021
FE_DEF_ERRORNO_OP_CALCULATOR(TENSOR_DATATYPE_NOT_SUPPORT, 4, "This tensor format is not supported.");       // 0x3320026

/** Task Builder error code define */
FE_DEF_ERRORNO_TASK_BUILDER(TASK_BUILDER_CREATE_ADAPTER_FAILED, 0,
                            "Create task builder adapter failed!");                                       // 0x03390000
FE_DEF_ERRORNO_TASK_BUILDER(TASK_BUILDER_RUN_ADAPTER_FAILED, 1, "Run task builder adapter failed!");      // 0x03390001
FE_DEF_ERRORNO_TASK_BUILDER(TASK_BUILDER_STATUS_BAD_PARAM, 2, "Run task builder bad param!");             // 0x03390002
FE_DEF_ERRORNO_TASK_BUILDER(TASK_BUILDER_STATUS_INTERNAL_ERROR, 3, "Run task builder internal failed!");  // 0x03390003
FE_DEF_ERRORNO_TASK_BUILDER(TASK_BUILDER_STATUS_KERNEL_ERROR, 4, "Run task builder kernel failed!");      // 0x03390004
FE_DEF_ERRORNO_TASK_BUILDER(TASK_BUILDER_STATUS_RUNTIME_ERROR, 5, "Run task builder runtime failed!");    // 0x03390005
FE_DEF_ERRORNO_TASK_BUILDER(TASK_BUILDER_CALL_RT_FAILED, 6, "Failed to call runtime API!");               // 0x03390006

}  // namespace fe

#endif  // FUSION_ENGINE_UTILS_COMMON_COMM_ERROR_CODES_H_
