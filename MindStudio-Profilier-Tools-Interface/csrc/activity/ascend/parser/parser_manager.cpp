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

#include "csrc/activity/ascend/parser/parser_manager.h"

#include "csrc/activity/activity_manager.h"
#include "csrc/activity/ascend/reporter/external_correlation_reporter.h"
#include "csrc/common/thread_local.h"
#include "hccl_reporter.h"
#include "stars_common.h"
#include "mstx_parser.h"
#include "cann_hash_cache.h"

#include "communication_calculator.h"

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

ParserManager *ParserManager::GetInstance()
{
    static ParserManager instance;
    return &instance;
}

msptiResult ParserManager::ReportApi(const MsprofApi* const data)
{
    if (!data) {
        return MSPTI_ERROR_INNER;
    }
    const auto& name = CannHashCache::GetInstance().GetHashInfo(data->itemId);
    if (name.empty()) {
        MSPTI_LOGW("Get HashInfo failed. HashId: %lu", data->itemId);
        return MSPTI_SUCCESS;
    }

    if (!Mspti::Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_API)) {
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

msptiResult ParserManager::StartAnalysisTask(msptiActivityKind kind)
{
    return GetAnalysisTask(kind)->StartTask();
}

msptiResult ParserManager::StartAnalysisTasks(const std::array<std::atomic<bool>, MSPTI_ACTIVITY_KIND_COUNT> &kinds)
{
    for (int kindIndex = 0; kindIndex < MSPTI_ACTIVITY_KIND_COUNT; kindIndex++) {
        if (!kinds[kindIndex]) {
            continue;
        }
        auto kind = static_cast<msptiActivityKind>(kindIndex);
        if (StartAnalysisTask(kind) != MSPTI_SUCCESS) {
            return MSPTI_ERROR_INNER;
        }
    }
    return MSPTI_SUCCESS;
}

msptiResult ParserManager::StopAnalysisTask(msptiActivityKind kind)
{
    return GetAnalysisTask(kind)->StopTask();
}

msptiResult ParserManager::StopAnalysisTasks(const std::array<std::atomic<bool>, MSPTI_ACTIVITY_KIND_COUNT> &kinds)
{
    for (int kindIndex = 0; kindIndex < MSPTI_ACTIVITY_KIND_COUNT; kindIndex++) {
        if (!kinds[kindIndex]) {
            continue;
        }
        auto kind = static_cast<msptiActivityKind>(kindIndex);
        if (StopAnalysisTask(kind) != MSPTI_SUCCESS) {
            return MSPTI_ERROR_INNER;
        }
    }
    return MSPTI_SUCCESS;
}

Mspti::Parser::ProfTask* ParserManager::GetAnalysisTask(msptiActivityKind kind)
{
    switch (kind) {
        case MSPTI_ACTIVITY_KIND_COMMUNICATION:
            return &CannTrackCache::GetInstance();
        default:
            return &Mspti::Parser::NullProfTask::GetInstance();
    }
}
}  // Parser
}  // Mspti
