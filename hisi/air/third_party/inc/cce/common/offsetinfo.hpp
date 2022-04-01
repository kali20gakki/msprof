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

#ifndef __OFFSETINFO_HPP_
#define __OFFSETINFO_HPP_
#include <stdint.h>
struct OffsetInfo {
  uint32_t start;
  uint32_t len;

  OffsetInfo() : start(0), len(0) {}
  OffsetInfo(uint32_t _start, uint32_t _len) : start(_start), len(_len) {}

  bool operator==(const OffsetInfo& rhs) {
    return start == rhs.start && len == rhs.len;
  }
};
#endif  // CATCH_HPP_
