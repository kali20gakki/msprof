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
#include "common/plog_manager.h"
#include "common/inject/runtime_inject.h"
#include "common/utils.h"

std::atomic<uint64_t> g_markId{0};

void aclprofMarkEx(const char* message, size_t len, RT_STREAM stream)
{
    if (message == nullptr || strlen(message) != len || stream == nullptr) {
        MSPTI_LOGE("[Mark]Invalid input param for markEx");
        return;
    }
    static uint32_t processId = Mspti::Common::Utils::GetPid();
    thread_local static uint32_t threadId = Mspti::Common::Utils::GetTid();
    static const uint32_t HOST_ID = 64;
    static const uint32_t MARK_TAG_ID = 11;
    uint64_t markId = g_markId++;
    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS;
    activity.mode = MSPTI_ACTIVITY_MARKER_MODE_HOST;
    activity.timestamp = Mspti::Common::Utils::GetClockMonotonicRawNs();
    activity.id = markId;
    activity.msptiObjectId.pt.processId = processId;
    activity.msptiObjectId.pt.threadId = threadId;
    activity.name = message;

    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));

    rtProfilerTraceEx(markId, 0xFFFFFFFFU, MARK_TAG_ID, stream);
}

void aclprofMark(void* stamp)
{
    if (stamp == nullptr) {
        MSPTI_LOGE("[Mark]aclprofStamp is nullptr");
        return;
    }
    static uint32_t processId = Mspti::Common::Utils::GetPid();
    thread_local static uint32_t threadId = Mspti::Common::Utils::GetTid();
    static const uint32_t HOST_ID = 64;
    uint64_t markId = g_markId++;
    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS;
    activity.mode = MSPTI_ACTIVITY_MARKER_MODE_HOST;
    activity.timestamp = Mspti::Common::Utils::GetClockMonotonicRawNs();
    activity.id = markId;
    activity.msptiObjectId.pt.processId = processId;
    activity.msptiObjectId.pt.threadId = threadId;
    auto stampInstancePtr = static_cast<MsprofStampInstance*>(stamp);
    activity.name = stampInstancePtr->stampInfo.message;
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));
}