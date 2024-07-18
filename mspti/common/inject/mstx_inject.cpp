/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.cpp
 * Description        : Injection of mstx.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "common/inject/mstx_inject.h"

#include <atomic>
#include <cstring>
#include <mutex>

#include "activity/activity_manager.h"
#include "common/inject/runtime_inject.h"
#include "common/utils.h"

std::atomic<uint64_t> g_markId{0};

void aclprofMarkEx(const char* message, size_t len, RT_STREAM stream)
{
    if (message == nullptr || strlen(message) != len || stream == nullptr) {
        printf("Mark Failed\n");
        return;
    }
    static const uint32_t HOST_ID = 64;
    static const uint32_t MARK_TAG_ID = 11;
    uint64_t markId = g_markId++;
    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARK;
    activity.mode = 0;
    activity.timestamp = Mspti::Common::Utils::GetClockMonotonicRawNs();
    activity.markId = markId;
    int32_t streamId = 0;
    rtGetStreamId(stream, &streamId);
    activity.streamId = static_cast<uint32_t>(streamId);
    activity.deviceId = HOST_ID;
    activity.name = message;
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));

    rtProfilerTraceEx(markId, 0xFFFFFFFFU, MARK_TAG_ID, stream);
}
