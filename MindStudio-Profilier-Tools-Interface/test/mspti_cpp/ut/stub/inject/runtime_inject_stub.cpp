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
 * -------------------------------------------------------------------------
*/
#include "csrc/common/inject/runtime_inject.h"

RtErrorT rtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, RtStreamT stm)
{
    return 0;
}

RtErrorT rtGetDevice(int32_t* devId)
{
    if (devId != nullptr) {
        *devId = 0;
    }
    return 0;
}

RtErrorT rtGetStreamId(RtStreamT stm, int32_t *streamId)
{
    if (streamId != nullptr) {
        *streamId = 0;
    }
    return 0;
}
