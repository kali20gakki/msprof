/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef INC_MEMORY_GROUP_MANAGER_H_
#define INC_MEMORY_GROUP_MANAGER_H_

#include <string>
#include "ge/ge_api_error_codes.h"
#include "runtime/rt_mem_queue.h"
#include "framework/common/debug/log.h"
#include "ge/ge_api_error_codes.h"

namespace ge {
class MemoryGroupManager {
 public:
  static MemoryGroupManager &GetInstance() {
    static MemoryGroupManager instance;
    return instance;
  }

  /*
   *  @ingroup ge
   *  @brief   init memory group manager
   *  @return: SUCCESS or FAILED
   */
  Status Initialize();

  /*
   *  @ingroup ge
   *  @brief   get qs memory group name
   *  @return: std::string
   */
  const std::string &GetQsMemGroupName() const;

  /*
   *  @ingroup ge
   *  @brief   create memory group
   *  @return: SUCCESS or FAILED
   */
  Status MemGrpCreate(const std::string &group_name);

  /*
   *  @ingroup ge
   *  @brief   add process into memory group
   *  @return: SUCCESS or FAILED
   */
  Status MemGrpAddProc(const std::string &group_name, const pid_t pid, bool is_admin, bool is_alloc);
 private:
  MemoryGroupManager() = default;
  ~MemoryGroupManager() = default;

  /*
   *  @ingroup ge
   *  @brief   init memory group
   *  @return: SUCCESS or FAILED
   */
  Status MemGroupInit(const std::string &group_name);

  std::string qs_mem_group_name_;
};
}
#endif // INC_EMEMORY_GROUP_MANAGER_H_
