/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memory_repoter.cpp
 * Description        : 上报记录Device Memory数据
 * Author             : msprof team
 * Creation Date      : 2024/12/2
 * *****************************************************************************
*/
#include "memory_reporter.h"
#include "activity/activity_manager.h"
#include "common/context_manager.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "common/runtime_utils.h"
#include "securec.h"

namespace Mspti {
namespace Reporter {
namespace {
msptiActivityMemcpyKind GetMsptiMemcpyKind(RtMemcpyKindT rtMemcpykind)
{
    static const std::unordered_map<RtMemcpyKindT, msptiActivityMemcpyKind> memoryKindMap = {
        {RT_MEMCPY_HOST_TO_HOST, MSPTI_ACTIVITY_MEMCPY_KIND_HTOH},
        {RT_MEMCPY_HOST_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_HTOD},
        {RT_MEMCPY_DEVICE_TO_HOST, MSPTI_ACTIVITY_MEMCPY_KIND_DTOH},
        {RT_MEMCPY_DEVICE_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_DTOD},
        {RT_MEMCPY_MANAGED, MSPTI_ACTIVITY_MEMCPY_KIND_HTOH},
        {RT_MEMCPY_ADDR_DEVICE_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_DTOD},
        {RT_MEMCPY_HOST_TO_DEVICE_EX, MSPTI_ACTIVITY_MEMCPY_KIND_HTOD},
        {RT_MEMCPY_DEVICE_TO_HOST_EX, MSPTI_ACTIVITY_MEMCPY_KIND_DTOH},
        {RT_MEMCPY_DEFAULT, MSPTI_ACTIVITY_MEMCPY_KIND_DEFAULT},
        {RT_MEMCPY_RESERVED, MSPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN}
    };
    auto iter = memoryKindMap.find(rtMemcpykind);
    return (iter == memoryKindMap.end() ? MSPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN : iter->second);
}
}

MemoryRecord::MemoryRecord(VOID_PTR_PTR devPtr, uint64_t size, msptiActivityMemoryKind memKind,
                           msptiActivityMemoryOperationType opType)
    : devPtr(devPtr), size(size), memKind(memKind), opType(opType),
      start(Common::ContextManager::GetInstance()->GetHostTimeStampNs()) {}

MemoryRecord::~MemoryRecord()
{
    if (devPtr != nullptr &&
        Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MEMORY)) {
        end = Common::ContextManager::GetInstance()->GetHostTimeStampNs();
        MemoryReporter::GetInstance()->ReportMemoryActivity(*this);
    }
}

MemsetRecord::MemsetRecord(uint32_t value, uint64_t bytes, RtStreamT stream, uint8_t isAsync)
    : value(value), bytes(bytes), stream(stream), isAsync(isAsync),
      start(Common::ContextManager::GetInstance()->GetHostTimeStampNs()) {}

MemsetRecord::~MemsetRecord()
{
    if (Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MEMSET)) {
        end = Common::ContextManager::GetInstance()->GetHostTimeStampNs();
        MemoryReporter::GetInstance()->ReportMemsetActivity(*this);
    }
}

MemcpyRecord::MemcpyRecord(RtMemcpyKindT copyKind, uint64_t bytes, RtStreamT stream, uint8_t isAsync)
    : copyKind(copyKind), bytes(bytes), stream(stream), isAsync(isAsync),
      start(Common::ContextManager::GetInstance()->GetHostTimeStampNs()) {}

MemcpyRecord::~MemcpyRecord()
{
    if (Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MEMCPY)) {
        end = Common::ContextManager::GetInstance()->GetHostTimeStampNs();
        MemoryReporter::GetInstance()->ReportMemcpyActivity(*this);
    }
}

MemoryReporter* MemoryReporter::GetInstance()
{
    static MemoryReporter instance;
    return &instance;
}

msptiResult MemoryReporter::ReportMemoryActivity(const MemoryRecord &record)
{
    uintptr_t address = (record.devPtr != nullptr ? Common::ReinterpretConvert<uintptr_t>(*record.devPtr) : 0);
    auto size = record.size;
    {
        std::lock_guard<std::mutex> lock(addrMtx_);
        auto iter = addrBytesInfo_.find(address);
        if (iter == addrBytesInfo_.end()) {
            if (record.opType == MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE) {
                MSPTI_LOGW("Address %llu release, but not have allocation record.", address);
            } else {
                addrBytesInfo_.insert({address, size});
            }
        } else {
            if (record.opType == MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE) {
                size = iter->second;
                addrBytesInfo_.erase(iter);
            } else {
                MSPTI_LOGW("Address %llu more than one allocation record.", address);
                iter->second = size;
            }
        }
    }
    msptiActivityMemory activityMemory{};
    activityMemory.kind = MSPTI_ACTIVITY_KIND_MEMORY;
    activityMemory.processId = Common::Utils::GetPid();
    activityMemory.streamId = MSPTI_INVALID_STREAM_ID;
    activityMemory.memoryKind = record.memKind;
    activityMemory.memoryOperationType = record.opType;
    activityMemory.correlationId = Common::ContextManager::GetInstance()->GetCorrelationId();
    activityMemory.start = record.start;
    activityMemory.end = record.end;
    activityMemory.address = address;
    activityMemory.bytes = size;
    activityMemory.deviceId = Common::GetDeviceId();
    if (Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(&activityMemory), sizeof(msptiActivityMemory)) != MSPTI_SUCCESS) {
        MSPTI_LOGE("ReportMemoryActivity fail, please check buffer");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult MemoryReporter::ReportMemsetActivity(const MemsetRecord &record)
{
    msptiActivityMemset activityMemset{};
    activityMemset.kind = MSPTI_ACTIVITY_KIND_MEMSET;
    activityMemset.value = record.value;
    activityMemset.bytes = record.bytes;
    activityMemset.start = record.start;
    activityMemset.end = record.end;
    activityMemset.deviceId = Common::GetDeviceId();
    activityMemset.streamId =
        (record.stream == nullptr ? MSPTI_INVALID_STREAM_ID : Common::GetStreamId(record.stream));
    activityMemset.correlationId = Common::ContextManager::GetInstance()->GetCorrelationId();
    activityMemset.isAsync = record.isAsync;
    if (Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(&activityMemset), sizeof(msptiActivityMemset)) != MSPTI_SUCCESS) {
        MSPTI_LOGE("ReportMemsetActivity fail, please check buffer");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult MemoryReporter::ReportMemcpyActivity(const MemcpyRecord &record)
{
    msptiActivityMemcpy activityMemcpy{};
    activityMemcpy.kind = MSPTI_ACTIVITY_KIND_MEMCPY;
    activityMemcpy.copyKind = GetMsptiMemcpyKind(record.copyKind);
    activityMemcpy.bytes = record.bytes;
    activityMemcpy.start = record.start;
    activityMemcpy.end = record.end;
    activityMemcpy.deviceId = Common::GetDeviceId();
    activityMemcpy.streamId =
        (record.stream == nullptr ? MSPTI_INVALID_STREAM_ID : Common::GetStreamId(record.stream));
    activityMemcpy.correlationId = Common::ContextManager::GetInstance()->GetCorrelationId();
    activityMemcpy.isAsync = record.isAsync;
    if (Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(&activityMemcpy), sizeof(msptiActivityMemcpy)) != MSPTI_SUCCESS) {
        MSPTI_LOGE("ReportMemcpyActivity fail, please check buffer");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}
} // Reporter
} // Mspti
