/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: command handle to acl/ge
 * Author: zcj
 * Create: 2020-11-26
 */
#ifndef ANALYSIS_DVVP_PROFILER_COMMAND_HANDLE_H
#define ANALYSIS_DVVP_PROFILER_COMMAND_HANDLE_H

#include <cstdint>

#include "acl/acl_base.h"

#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace ProfilerCommon {
#define PROF_INVALID_MODE_ID                0xFFFFFFFFUL
int32_t CommandHandleProfInit();
int32_t CommandHandleProfStart(const uint32_t devIdList[], uint32_t devNums, uint64_t profSwitch);
int32_t CommandHandleProfStop(const uint32_t devIdList[], uint32_t devNums, uint64_t profSwitch);
int32_t CommandHandleProfFinalize();
int32_t CommandHandleProfSubscribe(uint32_t modelId, uint64_t profSwitch);
int32_t CommandHandleProfUnSubscribe(uint32_t modelId);
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis

#endif
