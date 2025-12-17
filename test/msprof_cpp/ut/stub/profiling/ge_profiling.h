/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef INC_FRAMEWORK_COMMON_GE_PROFILING_H_
#define INC_FRAMEWORK_COMMON_GE_PROFILING_H_

#include "ge/ge_api_error_codes.h"
#include "toolchain/prof_callback.h"

using aclrtStream = void *;

ge::Status ProfSetStepInfo(uint64_t index_id, uint16_t tag_id, aclrtStream stream);

#endif  // INC_FRAMEWORK_COMMON_GE_PROFILING_H_
