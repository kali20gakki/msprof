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

#include "kernel_parser.h"

#include <tuple>
#include <list>
#include <unordered_map>
#include <mutex>

#include "csrc/common/utils.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/context_manager.h"
#include "csrc/activity/activity_manager.h"
#include "cann_hash_cache.h"
#include "stars_common.h"

#include "device_task_calculator.h"

namespace Mspti {
namespace Parser {
namespace {
inline const std::string &GetValidKernelTypeName(TsTaskType taskType)
{
    const static std::string nullInfo = "";
    const static std::unordered_map<TsTaskType, std::string> KERNEL_TYPE = {
        { TS_TASK_TYPE_KERNEL_AICORE, "KERNEL_AICORE" },   { TS_TASK_TYPE_KERNEL_AICPU, "KERNEL_AICPU" },
        { TS_TASK_TYPE_KERNEL_AIVEC, "KERNEL_AIVEC" },     { TS_TASK_TYPE_KERNEL_MIX_AIC, "KERNEL_MIX_AIC" },
        { TS_TASK_TYPE_KERNEL_MIX_AIV, "KERNEL_MIX_AIV" },
    };
    auto it = KERNEL_TYPE.find(taskType);
    return it == KERNEL_TYPE.end() ? nullInfo : it->second;
}
}

class KernelParser::KernelParserImpl {
    // <deviceId, streamId, taskId>
    using DstType = std::tuple<uint16_t, uint16_t, uint16_t>;

public:
    msptiResult ReportRtTaskTrack(uint32_t agingFlag, const MsprofCompactInfo *data);

private:
    msptiResult DealUnAgingRtTaskTrack(std::shared_ptr<DeviceTask> tasks);
    msptiResult DealAgingRtTaskTrack(std::shared_ptr<DeviceTask> tasks);
    msptiResult DealDeviceTask(std::shared_ptr<DeviceTask> tasks);
    bool IsGraphTask(DstType dstType);

private:
    std::unordered_map<DstType, std::list<std::unique_ptr<msptiActivityKernel>>, Common::TupleHash> kernel_map_;
    std::mutex kernel_mtx_;

    std::unordered_map<DstType, std::shared_ptr<msptiActivityKernel>, Common::TupleHash> unaging_kernel_map_;
    std::mutex unaging_kernel_mtx_;
};

msptiResult KernelParser::KernelParserImpl::ReportRtTaskTrack(uint32_t agingFlag, const MsprofCompactInfo *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_KERNEL)) {
        return MSPTI_SUCCESS;
    }
    auto &track = data->data.runtimeTrack;
    auto taskId = static_cast<uint16_t>(track.taskInfo & 0xffff);
    uint16_t streamId = track.streamId;
    uint16_t deviceId = track.deviceId;
    DstType dstKey = std::make_tuple(deviceId, streamId, taskId);
    const std::string &kernelName = GetValidKernelTypeName(static_cast<TsTaskType>(track.taskType));
    if (kernelName.empty()) {
        return MSPTI_SUCCESS;
    }

    std::unique_ptr<msptiActivityKernel> kernel{ nullptr };
    Mspti::Common::MsptiMakeUniquePtr(kernel);
    if (UNLIKELY(!kernel)) {
        MSPTI_LOGE("Malloc kernel memory failed.");
        return MSPTI_ERROR_INNER;
    }
    kernel->kind = MSPTI_ACTIVITY_KIND_KERNEL;
    kernel->name = CannHashCache::GetInstance().GetHashInfo(track.kernelName).c_str();
    kernel->type = kernelName.c_str();
    Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId(data->threadId);
    if (agingFlag) {
        kernel->correlationId = Mspti::Common::ContextManager::GetInstance()->GetCorrelationId();
        std::lock_guard<std::mutex> lk(kernel_mtx_);
        kernel_map_[dstKey].push_back(std::move(kernel));
    } else {
        std::lock_guard<std::mutex> lk(unaging_kernel_mtx_);
        auto iter = unaging_kernel_map_.find(dstKey);
        if (iter == unaging_kernel_map_.end()) {
            // 仅在第一次下发的时候更新correlationId
            kernel->correlationId = Mspti::Common::ContextManager::GetInstance()->GetCorrelationId();
            unaging_kernel_map_.emplace(dstKey, std::move(kernel));
        }
    }
    std::shared_ptr<DeviceTask> task;
    Common::MsptiMakeSharedPtr(task, 0, 0, streamId, taskId, deviceId, false, agingFlag);
    if (UNLIKELY(!task)) {
        MSPTI_LOGE("Malloc task memory failed.");
        return MSPTI_ERROR_INNER;
    }
    DeviceTaskCalculator::GetInstance().RegisterCallBack({ task },
        std::bind(&KernelParser::KernelParserImpl::DealDeviceTask, this, std::placeholders::_1));
    return MSPTI_SUCCESS;
}

msptiResult KernelParser::KernelParserImpl::DealAgingRtTaskTrack(std::shared_ptr<DeviceTask> task)
{
    auto dstKey = std::make_tuple(task->deviceId, task->streamId, task->taskId);
    std::unique_ptr<msptiActivityKernel> kernel;
    {
        std::lock_guard<std::mutex> lk(kernel_mtx_);
        auto it = kernel_map_.find(dstKey);
        if (it == kernel_map_.end()) {
            return MSPTI_SUCCESS;
        }
        auto &kernelList = it->second;
        if (kernelList.empty()) {
            MSPTI_LOGE("The cache kernel list data is empty.");
            kernel_map_.erase(it);
            return MSPTI_ERROR_INNER;
        }
        kernel = std::move(kernelList.front());
        kernelList.pop_front();
        if (kernelList.empty()) {
            kernel_map_.erase(it);
        }
    }
    kernel->ds.deviceId = task->deviceId;
    kernel->ds.streamId = task->streamId;
    kernel->start = task->start;
    kernel->end = task->end;
    return Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(kernel.get()), sizeof(msptiActivityKernel));
}

msptiResult KernelParser::KernelParserImpl::DealUnAgingRtTaskTrack(std::shared_ptr<DeviceTask> task)
{
    auto dstKey = std::make_tuple(task->deviceId, task->streamId, task->taskId);
    std::shared_ptr<msptiActivityKernel> kernel;
    {
        std::lock_guard<std::mutex> lk(unaging_kernel_mtx_);
        auto it = unaging_kernel_map_.find(dstKey);
        if (it == unaging_kernel_map_.end()) {
            return MSPTI_SUCCESS;
        }
        kernel = it->second;
    }
    kernel->ds.deviceId = task->deviceId;
    kernel->ds.streamId = task->streamId;
    kernel->start = task->start;
    kernel->end = task->end;
    return Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(kernel.get()), sizeof(msptiActivityKernel));
}

msptiResult KernelParser::KernelParserImpl::DealDeviceTask(std::shared_ptr<DeviceTask> task)
{
    if (UNLIKELY(!task)) {
        MSPTI_LOGE("task from calculator is nullptr");
        return MSPTI_ERROR_INNER;
    }
    auto dstKey = std::make_tuple(task->deviceId, task->streamId, task->taskId);
    if (IsGraphTask(dstKey)) {
        return DealUnAgingRtTaskTrack(task);
    } else {
        return DealAgingRtTaskTrack(task);
    }
}

inline bool KernelParser::KernelParserImpl::IsGraphTask(DstType dstType)
{
    std::lock_guard<std::mutex> lk(unaging_kernel_mtx_);
    return unaging_kernel_map_.count(dstType);
}

// KernelParser
KernelParser &KernelParser::GetInstance()
{
    static KernelParser instance;
    return instance;
}

KernelParser::KernelParser() : pImpl(std::make_unique<KernelParserImpl>()){};

KernelParser::~KernelParser() = default;

msptiResult KernelParser::ReportRtTaskTrack(uint32_t agingFlag, const MsprofCompactInfo *data)
{
    return pImpl->ReportRtTaskTrack(agingFlag, data);
}
}
}