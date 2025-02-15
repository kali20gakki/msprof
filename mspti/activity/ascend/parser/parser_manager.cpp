/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser_manager.cpp
 * Description        : Manager Parser.
 * Author             : msprof team
 * Creation Date      : 2024/08/14
 * *****************************************************************************
*/

#include "activity/ascend/parser/parser_manager.h"

#include "activity/activity_manager.h"
#include "activity/ascend/reporter/external_correlation_reporter.h"
#include "common/config.h"
#include "common/context_manager.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "hccl_reporter.h"

namespace Mspti {
namespace Parser {
namespace {
msptiActivityApi CreateDefaultApiActivityStruct()
{
    msptiActivityApi activityApi{};
    activityApi.kind = MSPTI_ACTIVITY_KIND_API;
    activityApi.pt.processId = Mspti::Common::Utils::GetPid();
    return activityApi;
}
}

std::unordered_map<uint64_t, std::string> ParserManager::hashInfo_map_;
std::mutex ParserManager::hashMutex_;
std::unordered_map<uint16_t, std::unordered_map<uint32_t, std::string>> ParserManager::typeInfo_map_;
std::mutex ParserManager::typeInfoMutex_;
std::map<uint64_t, std::shared_ptr<std::string>> ParserManager::markMsg_;
std::mutex ParserManager::markMsgMtx_;

// InnerDeviceMarker
std::mutex ParserManager::innerMarkerMutex_;
std::unordered_map<uint64_t, RtStreamT> ParserManager::innerMarkIds;

ParserManager *ParserManager::GetInstance()
{
    static ParserManager instance;
    return &instance;
}

static uint64_t GetHashIdImple(const std::string &hashInfo)
{
    static const uint32_t UINT32_BITS = 32;
    uint32_t prime[2] = {29, 131};
    uint32_t hash[2] = {0};
    for (char d : hashInfo) {
        hash[0] = hash[0] * prime[0] + static_cast<uint32_t>(d);
        hash[1] = hash[1] * prime[1] + static_cast<uint32_t>(d);
    }
    return (((static_cast<uint64_t>(hash[0])) << UINT32_BITS) | hash[1]);
}

uint64_t ParserManager::GenHashId(const std::string &hashInfo)
{
    // DoubleHash耗时和map find的耗时比较
    uint64_t hashId = GetHashIdImple(hashInfo);
    {
        std::lock_guard<std::mutex> lock(hashMutex_);
        const auto iter = hashInfo_map_.find(hashId);
        if (iter == hashInfo_map_.end()) {
            hashInfo_map_.insert({hashId, hashInfo});
        }
    }
    return hashId;
}

std::string& ParserManager::GetHashInfo(uint64_t hashId)
{
    static std::string nullInfo = "";
    std::lock_guard<std::mutex> lock(hashMutex_);
    const auto iter = hashInfo_map_.find(hashId);
    if (iter != hashInfo_map_.end()) {
        return iter->second;
    }
    return nullInfo;
}

void ParserManager::RegReportTypeInfo(uint16_t level, uint32_t typeId, const std::string& typeName)
{
    std::lock_guard<std::mutex> lock(typeInfoMutex_);
    auto level_iter = typeInfo_map_.find(level);
    if (level_iter == typeInfo_map_.end()) {
        std::unordered_map<uint32_t, std::string> type_entry {{typeId, typeName}};
        typeInfo_map_.insert({level, type_entry});
        return;
    }
    auto type_iter = level_iter->second.find(typeId);
    if (type_iter == level_iter->second.end()) {
        level_iter->second.insert({typeId, typeName});
        return;
    }
}

std::string& ParserManager::GetTypeName(uint16_t level, uint32_t typeId)
{
    static std::string nullInfo = "";
    std::lock_guard<std::mutex> lock(typeInfoMutex_);
    auto level_iter = typeInfo_map_.find(level);
    if (level_iter != typeInfo_map_.end()) {
        auto type_iter = level_iter->second.find(typeId);
        if (type_iter != level_iter->second.end()) {
            return type_iter->second;
        }
    }
    return nullInfo;
}

msptiResult ParserManager::ReportApi(const MsprofApi* const data)
{
    if (!data) {
        return MSPTI_ERROR_INNER;
    }
    const auto& name = GetHashInfo(data->itemId);
    if (name.empty()) {
        MSPTI_LOGW("Get HashInfo failed. HashId: %lu", data->itemId);
        return MSPTI_SUCCESS;
    }

    static thread_local msptiActivityApi api = CreateDefaultApiActivityStruct();
    api.name = name.data();
    api.pt.threadId = data->threadId;
    api.start = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(data->beginTime);
    api.end = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(data->endTime);
    api.correlationId = Mspti::Common::ContextManager::GetInstance()->GetCorrelationId(data->threadId);
    Mspti::Reporter::ExternalCorrelationReporter::GetInstance()->ReportExternalCorrelationId(api.correlationId);
    if (Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(&api), sizeof(msptiActivityApi)) != MSPTI_SUCCESS) {
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult ParserManager::ReportRtTaskTrack(const MsprofRuntimeTrack& track)
{
    uint16_t flipId = static_cast<uint16_t>(track.taskInfo >> 16);
    uint16_t taskId = static_cast<uint16_t>(track.taskInfo & 0xffff);
    uint16_t streamId = track.streamId;
    uint16_t deviceId = track.deviceId;
    DstType dstKey = std::make_tuple(deviceId, streamId, taskId);
    DsfType dsfKey = std::make_tuple(deviceId, streamId, flipId);
    {
        std::lock_guard<std::mutex> lk(flip_map_mtx_);
        auto iter = flip_dst_map_.find(dsfKey);
        if (iter == flip_dst_map_.end()) {
            flip_dst_map_.insert({dsfKey, {}});
        }
    }
    const static std::map<TsTaskType, std::string> KERNEL_TYPE = {
        {TS_TASK_TYPE_KERNEL_AICORE, "KERNEL_AICORE"},
        {TS_TASK_TYPE_KERNEL_AICPU, "KERNEL_AICPU"},
        {TS_TASK_TYPE_KERNEL_AIVEC, "KERNEL_AIVEC"},
    };
    auto typeIter = KERNEL_TYPE.find(static_cast<TsTaskType>(track.taskType));
    if (typeIter != KERNEL_TYPE.end()) {
        std::shared_ptr<msptiActivityKernel> kernel{nullptr};
        Mspti::Common::MsptiMakeSharedPtr(kernel);
        if (!kernel) {
            MSPTI_LOGE("Mallod memory failed.");
            return MSPTI_ERROR_INNER;
        }
        kernel->kind = MSPTI_ACTIVITY_KIND_KERNEL;
        kernel->correlationId = Mspti::Common::ContextManager::GetInstance()->GetCorrelationId();
        kernel->name = GetHashInfo(track.kernelName).c_str();
        kernel->type = typeIter->second.c_str();
        {
            std::lock_guard<std::mutex> lk(flip_map_mtx_);
            auto iter = flip_dst_map_.find(dsfKey);
            if (iter != flip_dst_map_.end()) {
                iter->second.emplace_back(dstKey);
            }
        }
        {
            std::lock_guard<std::mutex> lk(kernel_mtx_);
            auto iter = kernel_map_.find(dstKey);
            if (iter == kernel_map_.end()) {
                kernel_map_.insert({dstKey, kernel});
            }
        }
    }
    return MSPTI_SUCCESS;
}

msptiResult ParserManager::ReportStarsSocLog(uint32_t deviceId, const StarsSocLog* socLog)
{
    if (!socLog) {
        return MSPTI_ERROR_INNER;
    }
    constexpr int32_t BIT_OFFSET = 32;
    auto dstKey = std::make_tuple(static_cast<uint16_t>(deviceId), static_cast<uint16_t>(socLog->streamId),
        static_cast<uint16_t>(socLog->taskId));
    std::lock_guard<std::mutex> lk(kernel_mtx_);
    auto iter = kernel_map_.find(dstKey);
    if (iter != kernel_map_.end()) {
        auto& kernel = iter->second;
        if (!kernel) {
            MSPTI_LOGE("The cache kernel data is nullptr.");
            return MSPTI_ERROR_INNER;
        }
        if (socLog->funcType == STARS_FUNC_TYPE_BEGIN) {
            kernel->ds.deviceId = deviceId;
            kernel->ds.streamId = socLog->streamId;
            kernel->start = static_cast<uint64_t>(socLog->sysCntH) << BIT_OFFSET | socLog->sysCntL;
            kernel->start = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId,
                kernel->start);
        } else if (socLog->funcType == STARS_FUNC_TYPE_END) {
            kernel->end = static_cast<uint64_t>(socLog->sysCntH) << BIT_OFFSET | socLog->sysCntL;
            kernel->end = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId, kernel->end);
            if (Mspti::Activity::ActivityManager::GetInstance()->Record(
                Common::ReinterpretConvert<msptiActivity*>(kernel.get()),
                sizeof(msptiActivityKernel)) != MSPTI_SUCCESS) {
                return MSPTI_ERROR_INNER;
            }
        }
    }
    return MSPTI_SUCCESS;
}

static void ReportMarkDataToActivity(uint32_t deviceId, const StepTrace* stepTrace)
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
    if (ParserManager::GetInstance()->isInnerMarker(mark.id)) {
        // 上报的hccl的计算里面
        HcclReporter::GetInstance()->RecordHcclMarker(&mark);
        return;
    }
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(&mark), sizeof(msptiActivityMarker));
}

void ParserManager::ReportStepTrace(uint32_t deviceId, const StepTrace* stepTrace)
{
    if (!stepTrace) {
        return;
    }
    switch (stepTrace->tagId) {
        case STEP_TRACE_TAG_MARKEX:
            ReportMarkDataToActivity(deviceId, stepTrace);
            break;
        default:
            break;
    }
}

void ParserManager::ReportFlipInfo(uint32_t deviceId, const TaskFlipInfo* flipInfo)
{
    if (!flipInfo) {
        return;
    }
    auto dsfKey = std::make_tuple(static_cast<uint16_t>(deviceId), flipInfo->streamId, flipInfo->flipId);
    std::lock_guard<std::mutex> lk(flip_map_mtx_);
    auto iter = flip_dst_map_.find(dsfKey);
    if (iter != flip_dst_map_.end()) {
        const auto& dstKeys = iter->second;
        std::lock_guard<std::mutex> lk(kernel_mtx_);
        for (const auto dstKey : dstKeys) {
            kernel_map_.erase(dstKey);
        }
    }
    flip_dst_map_.erase(dsfKey);
}

std::shared_ptr<std::string> ParserManager::TryCacheMarkMsg(const char* msg)
{
    // msg字符串已在对外接口进行判空和长度判断操作
    std::lock_guard<std::mutex> lk(markMsgMtx_);
    if (markMsg_.size() > MARK_MAX_CACHE_NUM) {
        MSPTI_LOGE("Cache mark msg failed, current size: %u, limit size: %u", markMsg_.size(), MARK_MAX_CACHE_NUM);
        return nullptr;
    }
    std::shared_ptr<std::string> msgPtr{nullptr};
    Mspti::Common::MsptiMakeSharedPtr(msgPtr, msg);
    if (msgPtr == nullptr) {
        MSPTI_LOGE("Failed to malloc memory for mark msg.");
        return nullptr;
    }
    uint64_t hashId = GetHashIdImple(*msgPtr);
    auto iter = markMsg_.find(hashId);
    if (iter == markMsg_.end()) {
        iter = markMsg_.insert({hashId, msgPtr}).first;
    }
    return iter->second;
}

msptiResult ParserManager::ReportMark(const char* msg, RtStreamT stream, const char* domain)
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
    msptiActivityMarker activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = (stream != nullptr) ? MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE :
        MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.id = markId;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity.name = msgPtr->c_str();
    activity.domain = domain;
    activity.timestamp = timestamp;
    return Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(&activity), sizeof(msptiActivityMarker));
}

msptiResult ParserManager::ReportRangeStartA(const char* msg, RtStreamT stream, uint64_t& markId, const char* domain)
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
    msptiActivityMarker activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = (stream != nullptr) ? MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE :
        MSPTI_ACTIVITY_FLAG_MARKER_START;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.id = markId;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity.name = msgPtr->c_str();
    activity.domain = domain;
    activity.timestamp = timestamp;
    auto ret = Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(&activity), sizeof(msptiActivityMarker));
    {
        std::lock_guard<std::mutex> lock(rangeInfoMtx_);
        rangeInfo_.insert({markId, stream});
    }
    return ret;
}

msptiResult ParserManager::ReportRangeEnd(uint64_t rangeId)
{
    uint64_t timestamp = Mspti::Common::ContextManager::GetInstance()->GetHostTimeStampNs();
    bool withStream = false;
    {
        std::lock_guard<std::mutex> lock(rangeInfoMtx_);
        auto iter = rangeInfo_.find(rangeId);
        if (iter == rangeInfo_.end()) {
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
        rangeInfo_.erase(iter);
    }
    msptiActivityMarker activity;
    activity.kind = MSPTI_ACTIVITY_KIND_MARKER;
    activity.flag = withStream ? MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE : MSPTI_ACTIVITY_FLAG_MARKER_END;
    activity.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    activity.id = rangeId;
    activity.objectId.pt.processId = Mspti::Common::Utils::GetPid();
    activity.objectId.pt.threadId = Mspti::Common::Utils::GetTid();
    activity.name = "";
    activity.domain = "";
    activity.timestamp = timestamp;
    return Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(&activity), sizeof(msptiActivityMarker));
}

bool ParserManager::isInnerMarker(uint64_t markId)
{
    std::lock_guard<std::mutex> lock(innerMarkerMutex_);
    return innerMarkIds.count(markId);
}

msptiResult ParserManager::InnerDeviceStartA(const char *msg, RtStreamT stream, uint64_t& markId)
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

msptiResult ParserManager::InnerDeviceEndA(uint64_t rangeId)
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
}  // Parser
}  // Mspti
