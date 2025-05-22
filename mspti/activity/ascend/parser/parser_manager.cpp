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
#include "common/thread_local.h"
#include "hccl_reporter.h"
#include "stars_common.h"
#include "mstx_parser.h"

namespace Mspti {
namespace Parser {
namespace {
inline Mspti::Common::ThreadLocal<msptiActivityApi> GetDefaultApiActivity()
{
    static Mspti::Common::ThreadLocal<msptiActivityApi> instance(
        [] () {
            auto* activityApi = new(std::nothrow) msptiActivityApi();
            if (UNLIKELY(activityApi == nullptr)) {
                MSPTI_LOGE("create activityApi failed");
                return activityApi;
            }
            activityApi->kind = MSPTI_ACTIVITY_KIND_API;
            activityApi->pt.processId = Mspti::Common::Utils::GetPid();
            return activityApi;
        });
    return instance;
}
}

std::unordered_map<uint64_t, std::string> ParserManager::hashInfo_map_;
std::mutex ParserManager::hashMutex_;
std::unordered_map<uint16_t, std::unordered_map<uint32_t, std::string>> ParserManager::typeInfo_map_;
std::mutex ParserManager::typeInfoMutex_;

ParserManager *ParserManager::GetInstance()
{
    static ParserManager instance;
    return &instance;
}

uint64_t ParserManager::GenHashId(const std::string &hashInfo)
{
    // DoubleHash耗时和map find的耗时比较
    uint64_t hashId = Common::GetHashIdImple(hashInfo);
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

    msptiActivityApi* api = GetDefaultApiActivity().Get();
    if (UNLIKELY(api == nullptr)) {
        MSPTI_LOGE("Get Default MarkActivity is nullptr");
        return MSPTI_ERROR_INNER;
    }
    api->kind = MSPTI_ACTIVITY_KIND_API;
    api->pt.processId = Mspti::Common::Utils::GetPid();
    api->name = name.data();
    api->pt.threadId = data->threadId;
    api->start = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(data->beginTime);
    api->end = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(data->endTime);
    api->correlationId = Mspti::Common::ContextManager::GetInstance()->GetCorrelationId(data->threadId);
    Mspti::Reporter::ExternalCorrelationReporter::GetInstance()->ReportExternalCorrelationId(api->correlationId);
    if (Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity*>(api), sizeof(msptiActivityApi)) != MSPTI_SUCCESS) {
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult ParserManager::ReportRtTaskTrack(const MsprofRuntimeTrack& track)
{
    uint16_t taskId = static_cast<uint16_t>(track.taskInfo & 0xffff);
    uint16_t streamId = track.streamId;
    uint16_t deviceId = track.deviceId;
    DstType dstKey = std::make_tuple(deviceId, streamId, taskId);
    const static std::map<TsTaskType, std::string> KERNEL_TYPE = {
        {TS_TASK_TYPE_KERNEL_AICORE, "KERNEL_AICORE"},
        {TS_TASK_TYPE_KERNEL_AICPU, "KERNEL_AICPU"},
        {TS_TASK_TYPE_KERNEL_AIVEC, "KERNEL_AIVEC"},
        {TS_TASK_TYPE_KERNEL_MIX_AIC, "KERNEL_MIX_AIC"},
        {TS_TASK_TYPE_KERNEL_MIX_AIV, "KERNEL_MIX_AIV"},
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
            std::lock_guard<std::mutex> lk(kernel_mtx_);
            auto iter = kernel_map_.find(dstKey);
            if (iter == kernel_map_.end()) {
                std::list<std::shared_ptr<msptiActivityKernel>> kernelList{kernel};
                kernel_map_.insert({dstKey, kernelList});
            } else {
                iter->second.emplace_back(std::move(kernel));
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
    uint16_t streamId = StarsCommon::GetStreamId(static_cast<uint16_t>(socLog->streamId),
                                                 static_cast<uint16_t>(socLog->taskId));
    uint16_t taskId = StarsCommon::GetTaskId(static_cast<uint16_t>(socLog->streamId),
                                             static_cast<uint16_t>(socLog->taskId));
    auto dstKey = std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId);
    std::lock_guard<std::mutex> lk(kernel_mtx_);
    auto iter = kernel_map_.find(dstKey);
    if (iter != kernel_map_.end()) {
        auto& kernelList = iter->second;
        if (kernelList.empty()) {
            MSPTI_LOGE("The cache kernel list data is empty.");
            kernel_map_.erase(iter);
            return MSPTI_ERROR_INNER;
        }
        auto& kernel = kernelList.front();
        if (!kernel) {
            MSPTI_LOGE("The cache kernel data is nullptr.");
            return MSPTI_ERROR_INNER;
        }
        if (socLog->funcType == STARS_FUNC_TYPE_BEGIN) {
            kernel->ds.deviceId = deviceId;
            kernel->ds.streamId = streamId;
            kernel->start = static_cast<uint64_t>(socLog->sysCntH) << BIT_OFFSET | socLog->sysCntL;
            kernel->start = Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId, kernel->start);
        } else if (socLog->funcType == STARS_FUNC_TYPE_END) {
            kernel->end = static_cast<uint64_t>(socLog->sysCntH) << BIT_OFFSET | socLog->sysCntL;
            kernel->end = Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId, kernel->end);
            if (Mspti::Activity::ActivityManager::GetInstance()->Record(
                Common::ReinterpretConvert<msptiActivity*>(kernel.get()),
                sizeof(msptiActivityKernel)) != MSPTI_SUCCESS) {
                return MSPTI_ERROR_INNER;
            }
            kernelList.pop_front();
            if (kernelList.empty()) {
                kernel_map_.erase(iter);
            }
        }
    } else {
        MSPTI_LOGW("The kernel is not found for deviceId: %u, streamId: %u, taskId: %u", deviceId, streamId, taskId);
    }
    return MSPTI_SUCCESS;
}

void ParserManager::ReportStepTrace(uint32_t deviceId, const StepTrace* stepTrace)
{
    if (!stepTrace) {
        return;
    }
    switch (stepTrace->tagId) {
        case STEP_TRACE_TAG_MARKEX:
            MstxParser::GetInstance()->ReportMarkDataToActivity(deviceId, stepTrace);
            break;
        default:
            break;
    }
}
}  // Parser
}  // Mspti
