/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : external_correlation_reporter.cpp
 * Description        : 上报记录External Correlation数据
 * Author             : msprof team
 * Creation Date      : 2024/12/2
 * *****************************************************************************
*/

#include "external_correlation_reporter.h"

#include "activity/activity_manager.h"
#include "external/mspti_activity.h"

namespace Mspti {
namespace Reporter {


msptiActivityExternalCorrelation CreateDefaultExternalCorrelationStruct()
{
    msptiActivityExternalCorrelation activityExternalCorrelation{};
    activityExternalCorrelation.kind = MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION;
    return activityExternalCorrelation;
}

ExternalCorrelationReporter* ExternalCorrelationReporter::GetInstance()
{
    static ExternalCorrelationReporter instance;
    return &instance;
}

msptiResult ExternalCorrelationReporter::ReportExternalCorrelationId(uint64_t correlationId)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION)) {
        return MSPTI_SUCCESS;
    }
    std::lock_guard<std::mutex> lock(mapMtx_);
    for (const auto &pair : externalCorrelationMap) {
        static thread_local msptiActivityExternalCorrelation result = CreateDefaultExternalCorrelationStruct();
        result.externalKind = pair.first;
        result.externalId = pair.second.top();
        result.correlationId = correlationId;
        if (Mspti::Activity::ActivityManager::GetInstance()->Record(reinterpret_cast<msptiActivity *>(&result),
            sizeof(msptiActivityExternalCorrelation)) != MSPTI_SUCCESS) {
            return MSPTI_ERROR_INNER;
        }
    }
    return MSPTI_SUCCESS;
}

msptiResult ExternalCorrelationReporter::PushExternalCorrelationId(msptiExternalCorrelationKind kind, uint64_t id)
{
    std::lock_guard<std::mutex> lock(mapMtx_);
    auto iter = externalCorrelationMap.find(kind);
    if (iter == externalCorrelationMap.end()) {
        std::stack<uint64_t> newStack;
        newStack.push(id);
        externalCorrelationMap.insert({kind, newStack});
    } else {
        iter->second.push(id);
    }
    return MSPTI_SUCCESS;
}

msptiResult ExternalCorrelationReporter::PopExternalCorrelationId(msptiExternalCorrelationKind kind, uint64_t *lastId)
{
    if (lastId == nullptr) {
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(mapMtx_);
    auto iter = externalCorrelationMap.find(kind);
    if (iter == externalCorrelationMap.end()) {
        return MSPTI_ERROR_QUEUE_EMPTY;
    } else {
        *lastId = iter->second.top();
        iter->second.pop();
        if (iter->second.empty()) {
            externalCorrelationMap.erase(kind);
        }
    }
    return MSPTI_SUCCESS;
}
} // Reporter
} // Mspti