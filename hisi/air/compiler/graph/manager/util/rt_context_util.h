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

#ifndef GE_GRAPH_MANAGER_UTIL_RT_CONTEXT_UTIL_H_
#define GE_GRAPH_MANAGER_UTIL_RT_CONTEXT_UTIL_H_

#include <vector>
#include <map>
#include <mutex>

#include "runtime/context.h"
#include "ge/ge_api_error_codes.h"

namespace ge {
class RtContextUtil {
 public:
  static RtContextUtil &GetInstance();

  Status SetRtContext(const uint64_t session_id, const uint32_t graph_id, const int32_t device_id,
                      const rtCtxMode_t mode, rtContext_t rt_context) const;
  void AddRtContext(uint64_t session_id, rtContext_t context);
  void AddRtContext(uint64_t session_id, uint32_t graph_id, rtContext_t context);
  void DestroyRtContexts(uint64_t session_id);
  void DestroyRtContexts(uint64_t session_id, uint32_t graph_id);
  void DestroyAllRtContexts();

  RtContextUtil &operator=(const RtContextUtil &) = delete;
  RtContextUtil(const RtContextUtil &RtContextUtil) = delete;

 private:
  RtContextUtil() = default;
  ~RtContextUtil() {}

  void DestroyRtContexts(uint64_t session_id, int64_t graph_id, std::vector<rtContext_t> &contexts) const;

  std::map<uint64_t, std::map<int64_t, std::vector<rtContext_t>>> rt_contexts_;
  std::mutex ctx_mutex_;
};
}  // namespace ge

#endif  // GE_GRAPH_MANAGER_UTIL_RT_CONTEXT_UTIL_H_

