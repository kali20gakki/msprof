/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_repoter.cpp
 * Description        : 上报记录hccl数据
 * Author             : msprof team
 * Creation Date      : 2024/11/18
 * *****************************************************************************
*/
#include "hccl_reporter.h"
#include "activity/activity_manager.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "hccl_calculator.h"
#include "securec.h"

namespace Mspti {
namespace Parser {
namespace {
msptiActivityHccl CreateDefaultHcclActivityStruct()
{
    msptiActivityHccl activityHccl{};
    activityHccl.kind = MSPTI_ACTIVITY_KIND_HCCL;
    return activityHccl;
}
}

std::unordered_map<std::string, std::string> HcclReporter::commNameCache_;   // 缓存通信域名称，用于延长生命周期
std::unordered_map<uint64_t, std::shared_ptr<HcclOpDesc>> HcclReporter::markId2HcclOp_;

HcclReporter* HcclReporter::GetInstance()
{
    static HcclReporter instance;
    return &instance;
}

msptiResult HcclReporter::RecordHcclMarker(const msptiActivityMarker *markActivity)
{
    if (markActivity == nullptr) {
        MSPTI_LOGE("markActivity is nullptr, record fail");
        return MSPTI_ERROR_INNER;
    }

    {
        std::lock_guard<std::mutex> lock(markMutex_);
        // 如果markId2HcclOp_中没有这个算子, 认为是新增的, 需要补充start,
        if (markActivity->flag == MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE) {
            return RecordStartMarker(markActivity);
        }

        if (markActivity->flag == MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE) {
            return ReportHcclData(markActivity);
        }
    }
    MSPTI_LOGE("mark isn't target kind, markId: %lu, kind: %d", markActivity->id, markActivity->kind);
    return MSPTI_ERROR_INNER;
}

msptiResult HcclReporter::RecordHcclOp(uint32_t markId, std::shared_ptr<HcclOpDesc> opDesc)
{
    std::lock_guard<std::mutex> lock(markMutex_);
    markId2HcclOp_[markId] = std::move(opDesc);
    return MSPTI_SUCCESS;
}

msptiResult HcclReporter::ReportHcclActivity(std::shared_ptr<HcclOpDesc> hcclOpDesc)
{
    static thread_local msptiActivityHccl activityHccl = CreateDefaultHcclActivityStruct();
    activityHccl.name = GetSharedHcclName(hcclOpDesc->opName);
    activityHccl.ds.streamId = hcclOpDesc->streamId;
    activityHccl.ds.deviceId = hcclOpDesc->deviceId;
    activityHccl.bandWidth = hcclOpDesc->bandWidth;
    activityHccl.start = hcclOpDesc->start;
    activityHccl.end = hcclOpDesc->end;
    activityHccl.commName = GetSharedHcclName(hcclOpDesc->commName);
    if (Mspti::Activity::ActivityManager::GetInstance()->Record(
        reinterpret_cast<msptiActivity *>(&activityHccl), sizeof(msptiActivityHccl)) != MSPTI_SUCCESS) {
        MSPTI_LOGE("ReportHcclActivity fail, please check buffer");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult HcclReporter::RecordStartMarker(const msptiActivityMarker *markActivity)
{
    auto it = markId2HcclOp_.find(markActivity->id);
    if (it == markId2HcclOp_.end()) {
        MSPTI_LOGW("The corresponding start mark is not cached, id is %lu", markActivity->id);
        return MSPTI_ERROR_INNER;
    }
    it->second->start = markActivity->timestamp;
    it->second->deviceId = markActivity->objectId.ds.deviceId;
    it->second->streamId = markActivity->objectId.ds.streamId;
    return MSPTI_SUCCESS;
}

msptiResult HcclReporter::ReportHcclData(const msptiActivityMarker *markActivity)
{
    auto it = markId2HcclOp_.find(markActivity->id);
    if (it == markId2HcclOp_.end()) {
        MSPTI_LOGW("The corresponding end mark is not cached, id is %lu", markActivity->id);
        return MSPTI_ERROR_INNER;
    }
    it->second->end = markActivity->timestamp;
    HcclCalculator::CalculateBandWidth(it->second.get());
    ReportHcclActivity(it->second);
    markId2HcclOp_.erase(it);
    return MSPTI_SUCCESS;
}

const char* HcclReporter::GetSharedHcclName(const std::string& hcclName)
{
    std::lock_guard<std::mutex> lock(nameMutex_);
    if (!commNameCache_.count(hcclName)) {
        commNameCache_[hcclName] = hcclName;
    }
    return commNameCache_[hcclName].c_str();
}
}
}
