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

#include "hybrid/executor/hybrid_profiler.h"
#include <iomanip>
#include <iostream>
#include <cstdarg>
#include "framework/common/debug/ge_log.h"
#include "securec.h"

namespace ge {
namespace hybrid {
namespace {
const int32_t kMaxEvents = 1024 * 500;
const int32_t kEventDescMax = 512;
const int32_t kMaxEventTypes = 8;
const int32_t kIndent = 8;
}

HybridProfiler::HybridProfiler(): counter_(0) {
  Reset();
}

void HybridProfiler::RecordEvent(const EventType event_type, const char_t *const fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char_t buf[kEventDescMax];
  if (vsnprintf_s(&buf[0], static_cast<size_t>(kEventDescMax), static_cast<size_t>(kEventDescMax - 1),
                  fmt, args) == -1) {
    GELOGE(FAILED, "[Parse][Param:fmt]Format %s failed.", fmt);
    REPORT_CALL_ERROR("E19999", "Parse Format %s failed.", fmt);
    va_end(args);
    return;
  }

  va_end(args);
  const auto index = counter_++;
  if (index >= static_cast<int32_t>(events_.size())) {
    GELOGE(INTERNAL_ERROR,
           "[Check][Range]index out of range. index = %d, max event size = %zu", index, events_.size());
    REPORT_INNER_ERROR("E19999", "index out of range. index = %d, max event size = %zu", index, events_.size());
    return;
  }
  auto &evt = events_[static_cast<size_t>(index)];
  evt.timestamp = std::chrono::system_clock::now();
  evt.desc = std::string(buf);
  evt.event_type = event_type;
}

void HybridProfiler::Dump(std::ostream &output_stream) {
  if (events_.empty()) {
    return;
  }

  const auto start_dump = std::chrono::system_clock::now();
  const auto first_evt = events_[0U];
  auto start = first_evt.timestamp;
  std::vector<decltype(start)> prev_timestamps;
  prev_timestamps.resize(static_cast<size_t>(kMaxEventTypes), start);

  for (int32_t i = 0; i < counter_; ++i) {
    auto &evt = events_[static_cast<size_t>(i)];
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(evt.timestamp - start).count();
    auto &prev_ts = prev_timestamps[static_cast<size_t>(evt.event_type)];
    const auto cost = std::chrono::duration_cast<std::chrono::microseconds>(evt.timestamp - prev_ts).count();
    prev_ts = evt.timestamp;
    output_stream << std::setw(kIndent) << elapsed << "\t\t" << cost << "\t\t" << evt.desc << std::endl;
  }
  const auto end_dump = std::chrono::system_clock::now();
  const auto elapsed_dump = std::chrono::duration_cast<std::chrono::microseconds>(end_dump - start).count();
  const auto cost_dump = std::chrono::duration_cast<std::chrono::microseconds>(end_dump - start_dump).count();
  output_stream << std::setw(kIndent) << elapsed_dump << "\t\t" << cost_dump
                << "\t\t" << "[Dump profiling]" << std::endl;
  Reset();
}

void HybridProfiler::Reset() {
  counter_ = 0;
  events_.clear();
  events_.resize(static_cast<size_t>(kMaxEvents));
}
}  // namespace hybrid
}  // namespace ge
