/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "framework/common/profiling/ge_profiling.h"
#include "common/profiling/profiling_manager.h"

#include "runtime/rt.h"
#include "framework/common/debug/log.h"
#include "graph/load/graph_loader.h"
#include "graph/ge_context.h"
#include "framework/common/ge_types.h"

namespace {
const uint16_t kStepStart = 0U;
const uint16_t kStepEnd = 1U;
}  // namespace

// For: msprof/engine/src/prof_acl_core.cpp
ge::Status ProfSetStepInfo(const uint64_t index_id, const uint16_t tag_id, rtStream_t const stream) {
  if ((tag_id != kStepStart) && (tag_id != kStepEnd)) {
    GELOGE(ge::FAILED, "Param tag_id:%u invalid, [0: first run, 1: second run]", static_cast<uint32_t>(tag_id));
    REPORT_INPUT_ERROR("E10001", std::vector<std::string>({"value", "parameter", "reason"}),
                       std::vector<std::string>({std::to_string(static_cast<int32_t>(tag_id)), "tag_id",
                                                 "tag id must be 0 when first run, must be 1 when second run"}));
    return ge::FAILED;
  }

  int32_t device_id = 0;
  const rtError_t rt_ret = rtGetDevice(&device_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(ge::RT_FAILED, "[Get][LogicDeviceId]Failed, ret 0x%X", rt_ret);
    REPORT_CALL_ERROR("E19999", "Get logic device id failed, ret 0x%X", rt_ret);
    return ge::RT_FAILED;
  }

  const uint32_t prof_dev_id = static_cast<uint32_t>(device_id);
  auto &prof_mgr = ge::ProfilingManager::Instance();
  prof_mgr.SetStepInfoIndex(static_cast<int64_t>(index_id));
  return prof_mgr.ProfileStepInfo(index_id, ge::kInvalidModelId, tag_id, stream, prof_dev_id);
}

ge::Status ProfGetDeviceFormGraphId(const uint32_t graph_id, uint32_t &device_id) {
  return ge::ProfilingManager::Instance().GetDeviceIdFromGraph(graph_id, device_id);
}