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
#ifndef MSPROF_ENGINE_GE_CORE_H
#define MSPROF_ENGINE_GE_CORE_H

#include <cstdint>
#include "ge/ge_prof.h"
#include "prof_api_common.h"

namespace ge {
using namespace analysis::dvvp::common::utils;
int32_t GeOpenDeviceHandle(const uint32_t devId);
void GeFinalizeHandle();
size_t aclprofGetGraphId(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index);
void EraseDevRecord(const uint32_t devId);
}

#endif  // MSPROF_ENGINE_GE_CORE_H
