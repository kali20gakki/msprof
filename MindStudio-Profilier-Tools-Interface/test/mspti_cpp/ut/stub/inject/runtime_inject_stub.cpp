/**
* @file profapi_inject_stub.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
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
