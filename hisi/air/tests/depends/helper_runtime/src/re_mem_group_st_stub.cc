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

#include <string.h>
#include <string>
#include "rt_mem_queue.h"

RTS_API rtError_t rtMemGrpCreate(const char *name, const rtMemGrpConfig_t *cfg) {
  const std::string group_name(name);
  const std::string qs_name("DM_QS_GROUP");
  if (group_name.find(qs_name) == group_name.npos) {
    return 1;
  }
  return 0;
}

RTS_API rtError_t rtMemGrpAddProc(const char *name, int32_t pid, const rtMemGrpShareAttr_t *attr) {
  const std::string group_name(name);
  const std::string qs_name("DM_QS_GROUP");
  if (group_name.find(qs_name) == group_name.npos) {
    return 1;
  }
  return 0;
}

RTS_API rtError_t rtMemGrpAttach(const char *name, int32_t timeout) {
  if (timeout < 1000) {
    return 1;
  }
  return 0;
}

RTS_API rtError_t rtMemGrpQuery(int32_t cmd, const rtMemGrpQueryInput_t *input, rtMemGrpQueryOutput_t *output)
{
  return 0;
}

RTS_API rtError_t rtMemQueueInit(int32_t devId) {
  return 0;
}

RTS_API rtError_t rtQueryDevPid(rtBindHostpidInfo_t *info, int32_t *devPid) {
  if ((info != nullptr) && (info->chipId == 0xff)) {
    return 1;
  }
  *devPid = 100;
  return 0;
}

RTS_API rtError_t rtMemQueueInitQS(int32_t devId, const char* grpName) {
  const std::string group_name(grpName);
  const std::string qs_name("DM_QS_GROUP");
  if (group_name.find(qs_name) == group_name.npos) {
    return 1;
  }
  return 0;
}

RTS_API rtError_t rtMemQueueGrant(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr) {
  if (devId == 0xff) {
    return 1;
  }
  return 0;
}

RTS_API rtError_t rtMemQueueCreate(int32_t device, const rtMemQueueAttr_t *queAttr, uint32_t *qid) {
  return 0;
}


RTS_API rtError_t rtMemQueueDestroy(int32_t device, uint32_t qid) {
  return 0;
}

RTS_API rtError_t rtMemQueueEnQueueBuff(int32_t device, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout) {
  return 0;
}

RTS_API rtError_t rtMemQueueDeQueueBuff(int32_t device, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout) {
  return 0;
}

RTS_API rtError_t rtMemQueuePeek(int32_t device, uint32_t qid, size_t *bufLen, int32_t timeout) {
  return 0;
}

RTS_API rtError_t rtBufEventTrigger(const char *name) {
  return 0;
}

RTS_API rtError_t rtMemQueueInitFlowGw(int32_t devId, const rtInitFlowGwInfo_t *const initInfo) {
  const std::string group_name(initInfo->groupName);
  const std::string qs_name("DM_QS_GROUP");
  if (group_name.find(qs_name) == group_name.npos) {
    return 1;
  }
  return 0;
}