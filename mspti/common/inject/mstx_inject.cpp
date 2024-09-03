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
#include "common/context_manager.h"

namespace MsptiMstxApi {
std::atomic<uint64_t> g_markId{1};
constexpr uint32_t MARK_TAG_ID = 11;
static std::mutex g_mutex;
static std::unordered_map<uint64_t, RT_STREAM> g_rangeIds;

void MstxMarkAFunc(const char* msg, RT_STREAM stream)
{
    if (msg == nullptr) {
        MSPTI_LOGE("Input params is invalid.");
        return;
    }
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    uint64_t markId = g_markId++;
    if (stream != nullptr && rtProfilerTraceEx(markId,
        static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE), MARK_TAG_ID, stream) !=
        MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to run markA func.");
        return;
    }
    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = (stream != nullptr) ? MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE :
        MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.id = markId;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity.name = msg;
    activity.timestamp = timestamp;
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));
}
 
uint64_t MstxRangeStartAFunc(const char* msg, RT_STREAM stream)
{
    if (msg == nullptr) {
        MSPTI_LOGE("Input params is invalid.");
        return 0;
    }
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    uint64_t markId = g_markId++;
    if (stream != nullptr && rtProfilerTraceEx(markId,
        static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE),
        MARK_TAG_ID, stream) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to run range startA func.");
        return 0;
    }
    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = (stream != nullptr) ? MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE :
        MSPTI_ACTIVITY_FLAG_MARKER_START;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.id = markId;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity.name = msg;
    activity.timestamp = timestamp;
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_rangeIds.insert({markId, stream});
    }

    return markId;
}
 
void MstxRangeEndFunc(uint64_t rangeId)
{
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    bool withStream = false;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto iter = g_rangeIds.find(rangeId);
        if (iter == g_rangeIds.end()) {
            MSPTI_LOGW("Input rangeId[%lu] is invalid.", rangeId);
            return;
        }
        if (iter->second) {
            if (rtProfilerTraceEx(rangeId, static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE),
                MARK_TAG_ID, iter->second) != MSPTI_SUCCESS) {
                MSPTI_LOGE("Failed to run range end func.");
                return;
            }
            withStream = true;
        }
        g_rangeIds.erase(iter);
    }
    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = withStream ? MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE : MSPTI_ACTIVITY_FLAG_MARKER_END;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.id = rangeId;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity.name = "";
    activity.timestamp = timestamp;

    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));
}
}

using namespace MsptiMstxApi;

int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable)
{
    unsigned int outSize = 0;
    MstxFuncTable outTable;
    if (getFuncTable == nullptr || getFuncTable(MSTX_API_MODULE_CORE, &outTable, &outSize) != MSPTI_SUCCESS ||
        outTable == nullptr) {
        MSPTI_LOGE("Failed to init mstx funcs.");
        return 1; // 1 : init failed
    }
    *(outTable[MSTX_FUNC_MARKA]) = (MstxFuncPointer)MsptiMstxApi::MstxMarkAFunc;
    *(outTable[MSTX_FUNC_RANGE_STARTA]) = (MstxFuncPointer)MsptiMstxApi::MstxRangeStartAFunc;
    *(outTable[MSTX_FUNC_RANGE_END]) = (MstxFuncPointer)MsptiMstxApi::MstxRangeEndFunc;
    return MSPTI_SUCCESS;
}

void aclprofMarkEx(const char* message, size_t len, RT_STREAM stream)
{
    if (message == nullptr || strlen(message) != len || stream == nullptr) {
        MSPTI_LOGE("Input params are invalid.");
        return;
    }
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    uint64_t markId = g_markId++;
    if (rtProfilerTraceEx(markId, static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE),
        MARK_TAG_ID, stream) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to run markex func.");
        return;
    }

    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.id = markId;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity.name = message;
    activity.timestamp = timestamp;
 
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));
}
 
void aclprofMark(void* stamp)
{
    if (stamp == nullptr) {
        MSPTI_LOGE("Input params is invalid.");
        return;
    }
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    msptiActivityMark activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.timestamp = timestamp;
    activity.id = -1;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    auto stampInstancePtr = static_cast<MsprofStampInstance*>(stamp);
    activity.name = stampInstancePtr->stampInfo.message;
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity*>(&activity), sizeof(msptiActivityMark));
}
