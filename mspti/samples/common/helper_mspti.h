/**
* @file helper_mspti.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef HELPER_MSPTI_H_
#define HELPER_MSPTI_H_

// System headers
#include <vector>
#include <unordered_map>

// Mspti header
#include <mspti.h>
#include "common/util_mspti.h"
#include "securec.h"

msptiSubscriberHandle subscriber;

constexpr size_t DEFAULT_BUFFER_SIZE = 8 * 1024 * 1024;

struct UserData {
    aclrtContext* context;
    aclrtStream* stream;
};

UserData* g_userData;

// mspti 申请内存接口
void UserBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    LOG_PRINT("========== UserBufferRequest ============\n");
    uint8_t *pBuffer = reinterpret_cast<uint8_t *>(malloc(DEFAULT_BUFFER_SIZE + ALIGN_SIZE));
    if (pBuffer == nullptr) {
        LOG_PRINT("malloc buffer failed");
        *buffer = nullptr;
        *size = 0;
        return;
    }
    *buffer = ALIGN_BUFFER(pBuffer, ALIGN_SIZE);
    *size = DEFAULT_BUFFER_SIZE;
    *maxNumRecords = 0;
}

static const char* GetActivityKindString(msptiActivityKind kind)
{
    static const std::unordered_map<msptiActivityKind, const char*> STRING_MAP = {
        {MSPTI_ACTIVITY_KIND_INVALID, "INVALID"},
        {MSPTI_ACTIVITY_KIND_MARKER, "MARKER"},
        {MSPTI_ACTIVITY_KIND_KERNEL, "KERNEL"},
        {MSPTI_ACTIVITY_KIND_API, "API"},
        {MSPTI_ACTIVITY_KIND_HCCL, "HCCL"},
        {MSPTI_ACTIVITY_KIND_MEMORY, "MEMORY"},
        {MSPTI_ACTIVITY_KIND_MEMSET, "MEMSET"},
        {MSPTI_ACTIVITY_KIND_MEMCPY, "MEMCPY"},
        {MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION, "CORRELATION"}
    };
    auto it = STRING_MAP.find(kind);
    return it != STRING_MAP.end() ? it->second : "<unknown>";
}

static const char* GetActivitySourceKindString(msptiActivitySourceKind sourceKind)
{
    static const std::unordered_map<msptiActivitySourceKind, const char*> STRING_MAP = {
        {MSPTI_ACTIVITY_SOURCE_KIND_HOST, "HOST_DATA"},
        {MSPTI_ACTIVITY_SOURCE_KIND_DEVICE, "DEVICE_DATA"}
    };

    auto it = STRING_MAP.find(sourceKind);
    return it != STRING_MAP.end() ? it->second : "<unknown>";
}

static const char* GetActivityMemoryOperationTypeString(msptiActivityMemoryOperationType operationType)
{
    static const std::unordered_map<msptiActivityMemoryOperationType, const char*> STRING_MAP = {
        {MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION, "ALLOCATION"},
        {MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE, "RELEASE"}
    };

    auto it = STRING_MAP.find(operationType);
    return it != STRING_MAP.end() ? it->second : "<unknown>";
}

static const char* GetActivityMemcpyKindString(msptiActivityMemcpyKind memcpyKind)
{
    static const std::unordered_map<msptiActivityMemcpyKind, const char*> STRING_MAP = {
        {MSPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN, "UNKNOWN"},
        {MSPTI_ACTIVITY_MEMCPY_KIND_HTOH, "HTOH"},
        {MSPTI_ACTIVITY_MEMCPY_KIND_HTOD, "HTOD"},
        {MSPTI_ACTIVITY_MEMCPY_KIND_DTOH, "DTOH"},
        {MSPTI_ACTIVITY_MEMCPY_KIND_DTOD, "DTOD"},
        {MSPTI_ACTIVITY_MEMCPY_KIND_DEFAULT, "DEFAULT"}
    };

    auto it = STRING_MAP.find(memcpyKind);
    return it != STRING_MAP.end() ? it->second : "<unknown>";
}

static const char* GetExternalCorrelationKindString(msptiExternalCorrelationKind correlationKind)
{
    static const std::unordered_map<msptiExternalCorrelationKind, const char*> STRING_MAP = {
        {MSPTI_EXTERNAL_CORRELATION_KIND_INVALID, "INVALID"},
        {MSPTI_EXTERNAL_CORRELATION_KIND_UNKNOWN, "UNKNOWN"},
        {MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM0, "CUSTOM0"},
        {MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM1, "CUSTOM1"},
        {MSPTI_EXTERNAL_CORRELATION_KIND_CUSTOM2, "CUSTOM2"}
    };

    auto it = STRING_MAP.find(correlationKind);
    return it != STRING_MAP.end() ? it->second : "<unknown>";
}

static const char* GetActivityMemoryKindString(msptiActivityMemoryKind memoryKind)
{
    static const std::unordered_map<msptiActivityMemoryKind, const char*> STRING_MAP = {
        {MSPTI_ACTIVITY_MEMORY_UNKNOWN, "UNKNOWN"},
        {MSPTI_ACTIVITY_MEMORY_DEVICE, "MEMORY_DEVICE"}
    };

    auto it = STRING_MAP.find(memoryKind);
    return it != STRING_MAP.end() ? it->second : "<unknown>";
}

static const char* GetResultCodeString(msptiResult result)
{
    static const std::unordered_map<msptiResult, const char*> STRING_MAP = {
        {MSPTI_SUCCESS, "SUCCESS"},
        {MSPTI_ERROR_INVALID_PARAMETER, "ERROR_INVALID_PARAMETER"},
        {MSPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED, "MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED"},
        {MSPTI_ERROR_DEVICE_OFFLINE, "DEVICE_OFFLINE"},
        {MSPTI_ERROR_QUEUE_EMPTY, "QUEUE_EMPTY"},
        {MSPTI_ERROR_INNER, "ERROR_INNER"}
    };

    auto it = STRING_MAP.find(result);
    return it != STRING_MAP.end() ? it->second : "<unknown>";
}

static void ShowKernelInfo(msptiActivityKernel* kernel)
{
    if (!kernel) {
        return;
    }
    LOG_PRINT("[%s] type: %s, name: %s, start: %lu, end: %lu, deviceId: %u, streamId: %u, correlationId: %lu\n",
        GetActivityKindString(kernel->kind), kernel->type, kernel->name, kernel->start, kernel->end,
        kernel->ds.deviceId, kernel->ds.streamId, kernel->correlationId);
}

static void ShowApiInfo(msptiActivityApi* api)
{
    if (!api) {
        return;
    }
    LOG_PRINT("[%s] name: %s, start: %lu, end: %lu, processId: %u, threadId: %u, correlationId: %lu\n",
        GetActivityKindString(api->kind), api->name, api->start, api->end, api->pt.processId, api->pt.threadId,
        api->correlationId);
}

static void ShowHcclInfo(msptiActivityHccl* hccl)
{
    if (!hccl) {
        return;
    }
    LOG_PRINT("[%s] start: %lu, end: %lu, deviceId: %u, streamId: %u, bandWidth: %f, name: %s, commName: %s\n",
        GetActivityKindString(hccl->kind), hccl->start, hccl->end, hccl->ds.deviceId, hccl->ds.streamId,
        hccl->bandWidth, hccl->name, hccl->commName);
}

static void ShowMarkerInfo(msptiActivityMarker* marker)
{
    if (!marker) {
        return;
    }
    if (marker->sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_HOST) {
        LOG_PRINT("[%s] flag: %d, sourceKind: %s, timestamp: %lu, processId: %u, threadId: %u, name: %s, domain: %s\n",
            GetActivityKindString(marker->kind), marker->flag, GetActivitySourceKindString(marker->sourceKind),
            marker->timestamp, marker->objectId.pt.processId, marker->objectId.pt.threadId, marker->name,
            marker->domain);
    } else if (marker->sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_DEVICE) {
        LOG_PRINT("[%s] flag: %d, sourceKind: %s, timestamp: %lu, deviceId: %u, streamId: %u, name: %s, domain: %s\n",
            GetActivityKindString(marker->kind), marker->flag, GetActivitySourceKindString(marker->sourceKind),
            marker->timestamp, marker->objectId.ds.deviceId, marker->objectId.ds.streamId, marker->name,
            marker->domain);
    }
}

static void ShowMemCpyInfo(msptiActivityMemcpy* data)
{
    if (!data) {
        return;
    }
    LOG_PRINT("[%s] copyKind: %s, bytes: %lu, start: %lu, end: %lu, deviceId: %u, streamId: %u, correlationId: %lu, "
        "isAsync: %d\n",
        GetActivityKindString(data->kind), GetActivityMemcpyKindString(data->copyKind), data->bytes,
        data->start, data->end, data->deviceId, data->streamId, data->correlationId, data->isAsync);
}

static void ShowMemoryInfo(msptiActivityMemory* memory)
{
    if (!memory) {
        return;
    }
    LOG_PRINT("[%s] operationType: %s, memoryKind: %s, correlationId: %lu, start: %lu, end: %lu, address: %lu, "
              "bytes:%lu, processId: %u, deviceId: %u, streamId: %u\n",
        GetActivityKindString(memory->kind), GetActivityMemoryOperationTypeString(memory->memoryOperationType),
        GetActivityMemoryKindString(memory->memoryKind), memory->correlationId, memory->start, memory->end,
        memory->address, memory->bytes, memory->processId, memory->deviceId, memory->streamId);
}

static void ShowMemSetInfo(msptiActivityMemset* data)
{
    if (!data) {
        return;
    }
    LOG_PRINT("[%s] value: %u, bytes: %lu, start: %lu, end: %lu, deviceId: %u, streamId: %u, correlationId: %lu, "
              "isAsync: %d\n",
        GetActivityKindString(data->kind), data->value, data->bytes, data->start, data->end, data->deviceId,
        data->streamId, data->correlationId, data->isAsync);
}

void PrintActivity(msptiActivity *pRecord)
{
    msptiActivityKind activityKind = pRecord->kind;
    switch (activityKind) {
        case MSPTI_ACTIVITY_KIND_KERNEL:
            ShowKernelInfo(reinterpret_cast<msptiActivityKernel*>(pRecord));
            break;
        case MSPTI_ACTIVITY_KIND_API:
            ShowApiInfo(reinterpret_cast<msptiActivityApi*>(pRecord));
            break;
        case MSPTI_ACTIVITY_KIND_HCCL:
            ShowHcclInfo(reinterpret_cast<msptiActivityHccl*>(pRecord));
            break;
        case MSPTI_ACTIVITY_KIND_MARKER:
            ShowMarkerInfo(reinterpret_cast<msptiActivityMarker*>(pRecord));
            break;
        case MSPTI_ACTIVITY_KIND_MEMCPY:
            ShowMemCpyInfo(reinterpret_cast<msptiActivityMemcpy*>(pRecord));
            break;
        case MSPTI_ACTIVITY_KIND_MEMORY:
            ShowMemoryInfo(reinterpret_cast<msptiActivityMemory*>(pRecord));
            break;
        case MSPTI_ACTIVITY_KIND_MEMSET:
            ShowMemSetInfo(reinterpret_cast<msptiActivityMemset*>(pRecord));
            break;
        default:
            break;
    }
}

// activity数据消费接口
void UserBufferComplete(uint8_t *buffer, size_t size, size_t validSize)
{
    LOG_PRINT("========== UserBufferComplete ============\n");
    if (validSize > 0) {
        msptiActivity *pRecord = nullptr;
        msptiResult status = MSPTI_SUCCESS;
        do {
            status = msptiActivityGetNextRecord(buffer, validSize, &pRecord);
            if (status == MSPTI_SUCCESS) {
                PrintActivity(pRecord);
            } else if (status == MSPTI_ERROR_MAX_LIMIT_REACHED) {
                break;
            } else {
                LOG_PRINT("Consume data fail, error is %s", GetResultCodeString(status));
                break;
            }
        } while (1);
    }
    free(buffer);
}

msptiSubscriberHandle* InitMspti(void *pCallback, void *pUserData)
{
    // 订阅mspti
    g_userData = reinterpret_cast<UserData*>(pUserData);
    msptiSubscribe(&subscriber, (msptiCallbackFunc)pCallback, pUserData);
    msptiActivityRegisterCallbacks(UserBufferRequest, UserBufferComplete);
    return &subscriber;
}

void DeInitMspti(void)
{
    msptiUnsubscribe(subscriber);
    msptiActivityFlushAll(1);
    if (g_userData != nullptr) {
        free(g_userData);
        g_userData = nullptr;
    }
}

#endif // HELPER_MSPTI_H_