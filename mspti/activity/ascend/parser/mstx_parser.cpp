/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_parser.cpp
 * Description        : Manage mark activity.
 * Author             : msprof team
 * Creation Date      : 2025/05/06
 * *****************************************************************************
*/

#include "mstx_parser.h"

#include "activity/activity_manager.h"
#include "external/mspti_activity.h"
#include "common/plog_manager.h"
#include "common/context_manager.h"
#include "common/config.h"
#include "common/utils.h"
#include "common/thread_local.h"
#include "hccl_reporter.h"

namespace Mspti {
namespace Parser {
namespace {
inline Mspti::Common::ThreadLocal<msptiActivityMarker> GetDefaultMarkActivity()
{
    static Mspti::Common::ThreadLocal<msptiActivityMarker> instance(
        [] () {
            auto* marker = new(std::nothrow) msptiActivityMarker();
            if (UNLIKELY(marker == nullptr)) {
                MSPTI_LOGE("create default marker activity failed");
                return marker;
            }
            marker->kind = MSPTI_ACTIVITY_KIND_MARKER;
            marker->objectId.pt.processId = Mspti::Common::Utils::GetPid();
            marker->objectId.pt.threadId = Mspti::Common::Utils::GetTid();
            return marker;
        });
    return instance;
}
}

std::unordered_map<uint64_t, std::string> MstxParser::hashMarkMsg_;
std::mutex MstxParser::markMsgMtx_;

// InnerDeviceMarker
std::mutex MstxParser::innerMarkerMutex_;
std::unordered_map<uint64_t, RtStreamT> MstxParser::innerMarkIds;

MstxParser *MstxParser::GetInstance()
{
    static MstxParser instance;
    return &instance;
}

const std::string* MstxParser::TryCacheMarkMsg(const char* msg)
{
    // msg字符串已在对外接口进行判空和长度判断操作
    std::lock_guard<std::mutex> lk(markMsgMtx_);
    if (hashMarkMsg_.size() > MARK_MAX_CACHE_NUM) {
        MSPTI_LOGE("Cache mark msg failed, current size: %u, limit size: %u", hashMarkMsg_.size(), MARK_MAX_CACHE_NUM);
        return nullptr;
    }
    uint64_t hashId = Common::GetHashIdImple(msg);
    auto iter = hashMarkMsg_.find(hashId);
    if (iter == hashMarkMsg_.end()) {
        iter = hashMarkMsg_.emplace(hashId, std::string(msg)).first;
    }
    return &iter->second;
}

msptiResult MstxParser::ReportMark(const char* msg, RtStreamT stream, const char* domain)
{
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    auto msgPtr = TryCacheMarkMsg(msg);
    if (msgPtr == nullptr) {
        MSPTI_LOGE("Try Cache Mark msg failed.");
        return MSPTI_ERROR_INNER;
    }
    uint64_t markId = ++gMarkId_;
    if (stream != nullptr && rtProfilerTraceEx(markId,
        static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE), MARK_TAG_ID, stream) !=
                              MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to run markA func.");
        return MSPTI_ERROR_INNER;
    }
    msptiActivityMarker* activity = GetDefaultMarkActivity().Get();
    if (UNLIKELY(activity == nullptr)) {
        MSPTI_LOGE("Get Default MarkActivity is nullptr");
        return MSPTI_ERROR_INNER;
    }
    activity->kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity->flag = (stream != nullptr) ? MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE :
                    MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS;
    activity->sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity->id = markId;
    activity->objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity->objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity->name = msgPtr->c_str();
    activity->domain = domain;
    activity->timestamp = timestamp;
    return Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(activity), sizeof(msptiActivityMarker));
}

msptiResult MstxParser::ReportRangeStartA(const char* msg, RtStreamT stream, uint64_t& markId, const char* domain)
{
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    auto msgPtr = TryCacheMarkMsg(msg);
    if (msgPtr == nullptr) {
        MSPTI_LOGE("Try Cache Mark msg failed.");
        return MSPTI_ERROR_INNER;
    }
    markId = ++gMarkId_;
    if (stream != nullptr && rtProfilerTraceEx(markId,
                                               static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE),
                                               MARK_TAG_ID, stream) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to run range startA func.");
        return MSPTI_ERROR_INNER;
    }
    msptiActivityMarker* activity = GetDefaultMarkActivity().Get();
    if (UNLIKELY(activity == nullptr)) {
        MSPTI_LOGE("Get Default MarkActivity is nullptr");
        return MSPTI_ERROR_INNER;
    }
    activity->kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity->flag = (stream != nullptr) ? MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE :
                    MSPTI_ACTIVITY_FLAG_MARKER_START;
    activity->sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity->id = markId;
    activity->objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity->objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity->name = msgPtr->c_str();
    activity->domain = domain;
    activity->timestamp = timestamp;
    auto ret = Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(activity), sizeof(msptiActivityMarker));
    {
        std::lock_guard<std::mutex> lock(rangeInfoMtx_);
        markId2Stream_.insert({markId, stream});
    }
    return ret;
}

msptiResult MstxParser::ReportRangeEnd(uint64_t rangeId)
{
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    bool withStream = false;
    {
        std::lock_guard<std::mutex> lock(rangeInfoMtx_);
        auto iter = markId2Stream_.find(rangeId);
        if (iter == markId2Stream_.end()) {
            MSPTI_LOGW("Input rangeId[%lu] is invalid.", rangeId);
            return MSPTI_SUCCESS;
        }
        if (iter->second) {
            if (rtProfilerTraceEx(rangeId, static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE),
                                  MARK_TAG_ID, iter->second) != MSPTI_SUCCESS) {
                MSPTI_LOGE("Failed to run range end func.");
                return MSPTI_ERROR_INNER;
            }
            withStream = true;
        }
        markId2Stream_.erase(iter);
    }
    msptiActivityMarker* activity = GetDefaultMarkActivity().Get();
    if (UNLIKELY(activity == nullptr)) {
        MSPTI_LOGE("Get Default MarkActivity is nullptr");
        return MSPTI_ERROR_INNER;
    }
    activity->kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity->flag = withStream ? MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE : MSPTI_ACTIVITY_FLAG_MARKER_END;
    activity->sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity->id = rangeId;
    activity->objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity->objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity->name = "";
    activity->domain = "";
    activity->timestamp = timestamp;
    return Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(activity), sizeof(msptiActivityMarker));
}

void MstxParser::ReportMarkDataToActivity(uint32_t deviceId, const StepTrace* stepTrace)
{
    if (!stepTrace) {
        return;
    }
    msptiActivityMarker mark;
    mark.kind = MSPTI_ACTIVITY_KIND_MARKER;
    mark.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_DEVICE;
    mark.timestamp = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId,
                                                                                         stepTrace->timestamp);
    mark.id = stepTrace->indexId;
    mark.flag = static_cast<msptiActivityFlag>(stepTrace->modelId);
    mark.objectId.ds.deviceId = deviceId;
    mark.objectId.ds.streamId = static_cast<uint32_t>(stepTrace->streamId);
    mark.name = "";
    mark.domain = "";
    if (MstxParser::GetInstance()->IsInnerMarker(mark.id)) {
        // 上报的hccl的计算里面
        HcclReporter::GetInstance()->RecordHcclMarker(&mark);
        return;
    }
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(&mark), sizeof(msptiActivityMarker));
}

bool MstxParser::IsInnerMarker(uint64_t markId)
{
    std::lock_guard<std::mutex> lock(innerMarkerMutex_);
    return innerMarkIds.count(markId);
}

msptiResult MstxParser::InnerDeviceStartA(const char *msg, RtStreamT stream, uint64_t& markId)
{
    markId = gMarkId_++;
    if (stream != nullptr && rtProfilerTraceEx(markId,
                                               static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE),
                                               MARK_TAG_ID, stream) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to run range startA func.");
        return MSPTI_ERROR_INNER;
    }
    {
        std::lock_guard<std::mutex> lock(innerMarkerMutex_);
        innerMarkIds.insert({markId, stream});
    }
    return MSPTI_SUCCESS;
}

msptiResult MstxParser::InnerDeviceEndA(uint64_t rangeId)
{
    {
        std::lock_guard<std::mutex> lock(innerMarkerMutex_);
        auto iter = innerMarkIds.find(rangeId);
        if (iter == innerMarkIds.end()) {
            MSPTI_LOGW("Input rangeId[%lu] is invalid.", rangeId);
            return MSPTI_SUCCESS;
        }
        if (iter->second && rtProfilerTraceEx(rangeId,
                                              static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE),
                                              MARK_TAG_ID, iter->second) != MSPTI_SUCCESS) {
            MSPTI_LOGE("Failed to run range end func.");
            return MSPTI_ERROR_INNER;
        }
    }
    return MSPTI_SUCCESS;
}

} // Parser
} // Mspti
