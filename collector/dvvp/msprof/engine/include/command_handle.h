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
#ifndef ANALYSIS_DVVP_PROFILER_COMMAND_HANDLE_H
#define ANALYSIS_DVVP_PROFILER_COMMAND_HANDLE_H

#include <cstdint>

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
uint64_t GetProfSwitchHi(const uint64_t &dataTypeConfig);
}  // namespace ProfilerCommon
}  // namespace Dvvp
}  // namespace Analysis

#endif
