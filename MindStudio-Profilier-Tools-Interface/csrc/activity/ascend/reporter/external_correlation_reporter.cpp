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

#include "csrc/activity/ascend/reporter/external_correlation_reporter.h"

#include "csrc/activity/activity_manager.h"
#include "csrc/include/mspti_activity.h"
#include "csrc/common/thread_local.h"
#include "csrc/common/utils.h"
#include "csrc/common/plog_manager.h"

namespace Mspti {
namespace Reporter {
namespace {
inline Mspti::Common::ThreadLocal<msptiActivityExternalCorrelation> GetDefaultExternalCorrelationActivity()
{
    static Mspti::Common::ThreadLocal<msptiActivityExternalCorrelation> instance(
            [] () {
                auto* activityExternalCorrelation = new(std::nothrow) msptiActivityExternalCorrelation();
                if (UNLIKELY(activityExternalCorrelation == nullptr)) {
                    MSPTI_LOGE("create default activityExternalCorrelation failed");
                    return activityExternalCorrelation;
                }
                activityExternalCorrelation->kind = MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION;
                return activityExternalCorrelation;
            });
    return instance;
}
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
        msptiActivityExternalCorrelation* result = GetDefaultExternalCorrelationActivity().Get();
        if (UNLIKELY(result == nullptr)) {
            MSPTI_LOGE("Get Default ExternalCorrelationActivity is nullptr");
            return MSPTI_ERROR_INNER;
        }
        result->kind = MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION;
        result->externalKind = pair.first;
        result->externalId = pair.second.top();
        result->correlationId = correlationId;
        if (Mspti::Activity::ActivityManager::GetInstance()->Record(reinterpret_cast<msptiActivity *>(result),
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