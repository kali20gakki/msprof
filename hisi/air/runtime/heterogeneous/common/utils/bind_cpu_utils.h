/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.SUCCESS (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.SUCCESS
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef AIR_RUNTIME_COMMON_UTILS_BIND_CPU_UTILS_H_
#define AIR_RUNTIME_COMMON_UTILS_BIND_CPU_UTILS_H_
#include "framework/common/debug/log.h"
#include "pthread.h"
#include "sched.h"

namespace ge {
class BindCpuUtils {
 public:
  static Status BindCore(uint32_t cpu_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    int32_t ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
    if (ret != 0) {
      GELOGW("[Set][Affinity] set affinity with cpu[%u] failed, ret=%d", cpu_id, ret);
    }

    if (CPU_ISSET(cpu_id, &mask) == 0) {
      GELOGW("[Check][Bind] check process bind to cpu[%u] failed.", cpu_id);
    }
    return SUCCESS;
  }
};
}  // namespace ge
#endif  // AIR_RUNTIME_COMMON_UTILS_BIND_CPU_UTILS_H_
