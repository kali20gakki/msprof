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

#include "common/mem_grp/memory_group_manager.h"
#include "external/runtime/rt_error_codes.h"
#include "mmpa/mmpa_api.h"

namespace ge {
namespace {
constexpr int32_t KTimeout = 3000;
}

Status MemoryGroupManager::MemGrpCreate(const std::string &group_name) {
  rtMemGrpConfig_t cfg = {0};
  GE_CHK_RT_RET(rtMemGrpCreate(group_name.c_str(), &cfg));
  GELOGD("[Create][MemoryGrp] success, group name=%s.", group_name.c_str());
  return SUCCESS;
}

Status MemoryGroupManager::MemGrpAddProc(const std::string &group_name, const pid_t pid, bool is_admin, bool is_alloc) {
  rtMemGrpShareAttr_t attr = {0};
  attr.admin = is_admin ? 1 : 0;
  attr.alloc = is_alloc ? 1 : 0;
  attr.read = 1;
  attr.write = 1;
  const auto ret = rtMemGrpAddProc(group_name.c_str(), pid, &attr);
  if (ret != RT_ERROR_NONE && ret != ACL_ERROR_RT_REPEATED_INIT) {
    REPORT_CALL_ERROR("E19999", "Call rtMemGrpAddProc fail, ret: 0x%X", ret);
    GELOGE(RT_FAILED, "[rtMemGrpAddProc] failed, rt_err = %d, group_name is[%s]", ret, group_name.c_str());
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GELOGD("[Add][Process] success.");
  return SUCCESS;
}

Status MemoryGroupManager::Initialize() {
  qs_mem_group_name_ = std::string("DM_QS_GROUP_") + std::to_string(mmGetPid());
  return MemGroupInit(qs_mem_group_name_);
}

Status MemoryGroupManager::MemGroupInit(const std::string &group_name) {
  ge::Status res = MemGrpCreate(group_name);
  if (res != SUCCESS) {
    return FAILED;
  }

  pid_t pid = getpid();
  GE_CHK_STATUS_RET(MemGrpAddProc(group_name, pid, true, true),
                    "[Add][Pid] add leader pid[%d] into memory group[%s] error.", pid, group_name.c_str());
  GE_CHK_RT_RET(rtMemGrpAttach(group_name.c_str(), KTimeout));
  GELOGD("[Attach][MemoryGrp] success.");
  return SUCCESS;
}

const std::string &MemoryGroupManager::GetQsMemGroupName() const {
  return qs_mem_group_name_;
}
}
