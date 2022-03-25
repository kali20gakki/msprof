/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle acl request
 * Author: xiepeng
 * Create: 2020-07-30
 */
#ifndef MSPROF_ENGINE_GE_CORE_H
#define MSPROF_ENGINE_GE_CORE_H

#include <cstdint>
#include "ge/ge_prof.h"
#include "prof_api_common.h"

namespace ge {
using namespace analysis::dvvp::common::utils;
void GeOpenDeviceHandle(const uint32_t devId);
void GeFinalizeHandle();
size_t aclprofGetGraphId(CONST_VOID_PTR opInfo, size_t opInfoLen, uint32_t index);
}

#endif  // MSPROF_ENGINE_GE_CORE_H
