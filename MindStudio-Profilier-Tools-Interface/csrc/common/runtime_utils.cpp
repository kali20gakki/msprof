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

#include "csrc/common/runtime_utils.h"
#include "csrc/common/inject/acl_inject.h"
#include "csrc/include/mspti_activity.h"
#include "csrc/include/mspti_result.h"

namespace Mspti {
namespace Common {
namespace {
#pragma pack(1)
struct TraceData {
    uint64_t indexId;
    uint64_t modelId;
    uint16_t tagId;
};
}
uint32_t GetDeviceId()
{
    int32_t deviceId = 0;
    if (aclrtGetDevice(&deviceId) != MSPTI_SUCCESS) {
        return MSPTI_INVALID_DEVICE_ID;
    }
    return static_cast<uint32_t>(deviceId);
}

uint32_t GetStreamId(AclrtStream stm)
{
    int32_t streamId = 0;
    if (aclrtStreamGetId(stm, &streamId) != MSPTI_SUCCESS) {
        return MSPTI_INVALID_STREAM_ID;
    }
    return static_cast<uint32_t>(streamId);
}

AclError profTrace(uint64_t indexId, uint64_t modelId, uint16_t tagId, AclrtStream stream)
{
    TraceData traceData{.indexId = indexId, .modelId = modelId, .tagId = tagId};
    return aclrtProfTrace((void *)&traceData, sizeof(TraceData), stream);
}

} // Common
} // Mspti
