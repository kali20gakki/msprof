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

#include "csrc/activity/ascend/parser/mstx_parser.h"

#include "csrc/activity/activity_manager.h"
#include "csrc/include/mspti_activity.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/context_manager.h"
#include "csrc/common/config.h"
#include "csrc/common/utils.h"
#include "csrc/common/runtime_utils.h"
#include "csrc/common/thread_local.h"
#include "csrc/activity/ascend/parser/hccl_reporter.h"

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
std::unordered_map<uint64_t, AclrtStream> MstxParser::innerMarkIds;

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

msptiResult MstxParser::ReportMark(const char* msg, AclrtStream stream, const char* domain)
{
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    auto msgPtr = TryCacheMarkMsg(msg);
    if (msgPtr == nullptr) {
        MSPTI_LOGE("Try Cache Mark msg failed.");
        return MSPTI_ERROR_INNER;
    }
    uint64_t markId = ++gMarkId_;
    if (stream != nullptr && Common::ProfTrace(markId,
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

msptiResult MstxParser::ReportRangeStartA(const char* msg, AclrtStream stream, uint64_t& markId, const char* domain)
{
    RangeStartContext mstxContext{};
    uint64_t timestamp = 0;
    if (Common::ContextManager::GetInstance()->GetHostTimeInfo(mstxContext.devTimeInfo)) {
        timestamp = Common::ContextManager::CalculateRealTime(Mspti::Common::Utils::GetHostSysCnt(),
                                                              mstxContext.devTimeInfo);
    } else {
        timestamp = Mspti::Common::Utils::GetClockRealTimeNs();
    }
    mstxContext.stream = stream;
    auto msgPtr = TryCacheMarkMsg(msg);
    if (msgPtr == nullptr) {
        MSPTI_LOGE("Try Cache Mark msg failed.");
        return MSPTI_ERROR_INNER;
    }
    markId = ++gMarkId_;
    if (stream != nullptr && Common::ProfTrace(markId,
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
        markId2Context_.emplace(markId, mstxContext);
    }
    return ret;
}

msptiResult MstxParser::ReportRangeEnd(uint64_t rangeId)
{
    uint64_t timestamp;
    bool withStream = false;
    {
        std::lock_guard<std::mutex> lock(rangeInfoMtx_);
        auto iter = markId2Context_.find(rangeId);
        if (iter == markId2Context_.end()) {
            MSPTI_LOGW("Input rangeId[%lu] is invalid.", rangeId);
            return MSPTI_SUCCESS;
        }
        timestamp = Common::ContextManager::CalculateRealTime(Mspti::Common::Utils::GetHostSysCnt(),
                                                              iter->second.devTimeInfo);
        if (iter->second.stream) {
            if (Common::ProfTrace(rangeId, static_cast<uint64_t>(MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE),
                                  MARK_TAG_ID, iter->second.stream) != MSPTI_SUCCESS) {
                MSPTI_LOGE("Failed to run range end func.");
                return MSPTI_ERROR_INNER;
            }
            withStream = true;
        }
        markId2Context_.erase(iter);
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
    Common::DevTimeInfo devTimeInfo;
    uint64_t deviceMarkId = stepTrace->indexId;
    {
        std::lock_guard<std::mutex> lk(deviceRangeInfoMtx_);
        auto it = deviceMarkId2Context_.find(deviceMarkId);
        if (it != deviceMarkId2Context_.end()) {
            devTimeInfo = it->second;
            deviceMarkId2Context_.erase(it);
        } else if (Common::ContextManager::GetInstance()->GetCurDevTimeInfo(deviceId, devTimeInfo)) {
            if (static_cast<uint32_t>(stepTrace->modelId) != MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE) {
                deviceMarkId2Context_.emplace(deviceMarkId, devTimeInfo);
            }
        } else {
            MSPTI_LOGW("GetCurDevTimeInfo fail! deviceId is %u, rangeId is %lu", deviceId, stepTrace->indexId);
        }
    }
    msptiActivityMarker mark;
    mark.kind = MSPTI_ACTIVITY_KIND_MARKER;
    mark.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_DEVICE;
    mark.timestamp = Common::ContextManager::CalculateRealTime(stepTrace->timestamp, devTimeInfo);
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

msptiResult MstxParser::InnerDeviceStartA(AclrtStream stream, uint64_t& markId)
{
    markId = ++gMarkId_;
    if (stream != nullptr && Common::ProfTrace(markId,
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
        if (iter->second && Common::ProfTrace(rangeId,
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
