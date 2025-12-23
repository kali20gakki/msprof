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
#include "memory_reporter.h"
#include "csrc/activity/activity_manager.h"
#include "csrc/common/context_manager.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/utils.h"
#include "csrc/common/runtime_utils.h"
#include "csrc/common/thread_local.h"
#include "securec.h"

namespace Mspti {
namespace Reporter {
namespace {
msptiActivityMemcpyKind GetMsptiMemcpyKind(AclrtMemcpyKind rtMemcpykind)
{
    static const std::unordered_map<AclrtMemcpyKind, msptiActivityMemcpyKind> memoryKindMap = {
        {ACL_MEMCPY_HOST_TO_HOST, MSPTI_ACTIVITY_MEMCPY_KIND_HTOH},
        {ACL_MEMCPY_HOST_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_HTOD},
        {ACL_MEMCPY_DEVICE_TO_HOST, MSPTI_ACTIVITY_MEMCPY_KIND_DTOH},
        {ACL_MEMCPY_DEVICE_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_DTOD},
        {ACL_MEMCPY_HOST_TO_BUF_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_HTOD},
        {ACL_MEMCPY_DEFAULT, MSPTI_ACTIVITY_MEMCPY_KIND_DEFAULT},
        // 未知的memcpy类型
        {ACL_MEMCPY_INNER_DEVICE_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN},
        {ACL_MEMCPY_INTER_DEVICE_TO_DEVICE, MSPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN},
    };
    auto iter = memoryKindMap.find(rtMemcpykind);
    return (iter == memoryKindMap.end() ? MSPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN : iter->second);
}

inline Mspti::Common::ThreadLocal<msptiActivityMemory>& GetDefaultMemoryActivity()
{
    static Mspti::Common::ThreadLocal<msptiActivityMemory> instance(
        [] () {
            auto* activityMemory = new(std::nothrow) msptiActivityMemory();
            if (UNLIKELY(activityMemory == nullptr)) {
                MSPTI_LOGE("create default activityMemory failed");
                return activityMemory;
            }
            activityMemory->kind = MSPTI_ACTIVITY_KIND_MEMORY;
            activityMemory->processId = Common::Utils::GetPid();
            activityMemory->streamId = MSPTI_INVALID_STREAM_ID;
            return activityMemory;
        });
    return instance;
}

inline Mspti::Common::ThreadLocal<msptiActivityMemcpy>& GetDefaultMemcpyActivity()
{
    static Mspti::Common::ThreadLocal<msptiActivityMemcpy> instance(
        [] () {
            auto* activityMemcpy = new(std::nothrow) msptiActivityMemcpy();
            if (UNLIKELY(activityMemcpy == nullptr)) {
                MSPTI_LOGE("create default activityMemcpy failed");
                return activityMemcpy;
            }
            activityMemcpy->kind = MSPTI_ACTIVITY_KIND_MEMCPY;
            return activityMemcpy;
        });
    return instance;
}

inline Mspti::Common::ThreadLocal<msptiActivityMemset>& GetDefaultMemsetActivity()
{
    static Mspti::Common::ThreadLocal<msptiActivityMemset> instance(
        [] () {
            auto* activityMemset = new(std::nothrow) msptiActivityMemset();
            if (UNLIKELY(activityMemset == nullptr)) {
                MSPTI_LOGE("create default activityMemory failed");
                return activityMemset;
            }
            activityMemset->kind = MSPTI_ACTIVITY_KIND_MEMSET;
            return activityMemset;
        });
    return instance;
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

MemsetRecord::MemsetRecord(uint32_t value, uint64_t bytes, AclrtStream stream, uint8_t isAsync)
    : value(value), bytes(bytes), stream(stream), isAsync(isAsync),
      start(Common::ContextManager::GetInstance()->GetHostTimeStampNs()) {}

MemsetRecord::~MemsetRecord()
{
    if (Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MEMSET)) {
        end = Common::ContextManager::GetInstance()->GetHostTimeStampNs();
        MemoryReporter::GetInstance()->ReportMemsetActivity(*this);
    }
}

MemcpyRecord::MemcpyRecord(AclrtMemcpyKind copyKind, uint64_t bytes, AclrtStream stream, uint8_t isAsync)
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
    msptiActivityMemory* activityMemory = GetDefaultMemoryActivity().Get();
    if (UNLIKELY(activityMemory == nullptr)) {
        MSPTI_LOGE("Get Default MemoryActivity is nullptr");
        return MSPTI_ERROR_INNER;
    }
    activityMemory->memoryKind = record.memKind;
    activityMemory->memoryOperationType = record.opType;
    activityMemory->correlationId = Common::ContextManager::GetInstance()->GetCorrelationId();
    activityMemory->start = record.start;
    activityMemory->end = record.end;
    activityMemory->address = address;
    activityMemory->bytes = size;
    activityMemory->deviceId = Common::GetDeviceId();
    if (Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(activityMemory), sizeof(msptiActivityMemory)) != MSPTI_SUCCESS) {
        MSPTI_LOGE("ReportMemoryActivity fail, please check buffer");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult MemoryReporter::ReportMemsetActivity(const MemsetRecord &record)
{
    msptiActivityMemset* activityMemset = GetDefaultMemsetActivity().Get();
    if (UNLIKELY(activityMemset == nullptr)) {
        MSPTI_LOGE("Get Default MarkActivity is nullptr");
        return MSPTI_ERROR_INNER;
    }
    activityMemset->value = record.value;
    activityMemset->bytes = record.bytes;
    activityMemset->start = record.start;
    activityMemset->end = record.end;
    activityMemset->deviceId = Common::GetDeviceId();
    activityMemset->streamId =
        (record.stream == nullptr ? MSPTI_INVALID_STREAM_ID : Common::GetStreamId(record.stream));
    activityMemset->correlationId = Common::ContextManager::GetInstance()->GetCorrelationId();
    activityMemset->isAsync = record.isAsync;
    if (Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(activityMemset), sizeof(msptiActivityMemset)) != MSPTI_SUCCESS) {
        MSPTI_LOGE("ReportMemsetActivity fail, please check buffer");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult MemoryReporter::ReportMemcpyActivity(const MemcpyRecord &record)
{
    msptiActivityMemcpy* activityMemcpy = GetDefaultMemcpyActivity().Get();
    if (UNLIKELY(activityMemcpy == nullptr)) {
        MSPTI_LOGE("Get Default MarkActivity is nullptr");
        return MSPTI_ERROR_INNER;
    }
    activityMemcpy->copyKind = GetMsptiMemcpyKind(record.copyKind);
    activityMemcpy->bytes = record.bytes;
    activityMemcpy->start = record.start;
    activityMemcpy->end = record.end;
    activityMemcpy->deviceId = Common::GetDeviceId();
    activityMemcpy->streamId =
        (record.stream == nullptr ? MSPTI_INVALID_STREAM_ID : Common::GetStreamId(record.stream));
    activityMemcpy->correlationId = Common::ContextManager::GetInstance()->GetCorrelationId();
    activityMemcpy->isAsync = record.isAsync;
    if (Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(activityMemcpy), sizeof(msptiActivityMemcpy)) != MSPTI_SUCCESS) {
        MSPTI_LOGE("ReportMemcpyActivity fail, please check buffer");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}
} // Reporter
} // Mspti
