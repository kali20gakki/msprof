/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_adapter.cpp
 * Description        : Mspti python adapter.
 * Author             : msprof team
 * Creation Date      : 2024/11/19
 * *****************************************************************************
*/

#include "mspti_adapter.h"
#include "common/plog_manager.h"
#include "common/utils.h"

namespace {
// python data keyword
const char *KIND            = "kind";
const char *START           = "start";
const char *END             = "end";
const char *DEVICE_ID       = "deviceId";
const char *STREAM_ID       = "streamId";
const char *CORRELATION_ID  = "correlationId";
const char *TYPE            = "type";
const char *NAME            = "name";
const char *FLAG            = "flag";
const char *SOURCE_KIND     = "sourceKind";
const char *TIMESTAMP       = "timestamp";
const char *ID              = "id";
const char *PROCESS_ID      = "processId";
const char *THREAD_ID       = "threadId";
const char *DOMAIN_NAME     = "domain";
const char *BANDWIDTH       = "bandWidth";
const char *COMMNAME        = "commName";

// default value
const int64_t INVALID_PROCESSID = -1L;
const int64_t INVALID_THREADID = -1L;
const int64_t INVALID_DEVICEID = -1L;
const int64_t INVALID_STERAMID = -1L;

void CallKernelCallback(PyObject *kernelCallback, const msptiActivityKernel *kernel)
{
    if (kernelCallback == nullptr) {
        MSPTI_LOGW("Kernel callback is nullptr");
        return;
    }
    if (kernel == nullptr) {
        MSPTI_LOGE("Kernel data is nullptr");
        return;
    }
    PyObject *kernelData = Py_BuildValue("{sIsKsKsIsIsKssss}",
        KIND, static_cast<uint32_t>(kernel->kind),
        START, kernel->start,
        END, kernel->end,
        DEVICE_ID, kernel->ds.deviceId,
        STREAM_ID, kernel->ds.streamId,
        CORRELATION_ID, kernel->correlationId,
        TYPE, kernel->type,
        NAME, kernel->name);
    if (kernelData == nullptr) {
        MSPTI_LOGE("Build python kernel data failed");
        return;
    }
    /* make sure callback doesn't go away */
    Py_INCREF(kernelCallback);
    auto ret = PyObject_CallFunction(kernelCallback, "O", kernelData);
    if (ret == nullptr) {
        MSPTI_LOGE("Call kernel callback failed");
    } else {
        Py_DECREF(ret);
    }
    Py_DECREF(kernelCallback);
    Py_XDECREF(kernelData);
}

void CallMstxCallback(PyObject *mstxCallback, const msptiActivityMarker *marker)
{
    if (mstxCallback == nullptr) {
        MSPTI_LOGW("Mstx callback is nullptr");
        return;
    }
    if (marker == nullptr) {
        MSPTI_LOGE("Marker data is nullptr");
        return;
    }
    auto sourceKind = marker->sourceKind;
    PyObject *markerData = Py_BuildValue("{sIsIsIsKsKsLsLsLsLssss}",
        KIND, static_cast<uint32_t>(marker->kind),
        FLAG, static_cast<uint32_t>(marker->flag),
        SOURCE_KIND, static_cast<uint32_t>(marker->sourceKind),
        TIMESTAMP, marker->timestamp,
        ID, marker->id,
        PROCESS_ID, sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_HOST ? marker->objectId.pt.processId : INVALID_PROCESSID,
        THREAD_ID, sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_HOST ? marker->objectId.pt.threadId : INVALID_THREADID,
        DEVICE_ID, sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_DEVICE ? marker->objectId.ds.deviceId : INVALID_DEVICEID,
        STREAM_ID, sourceKind == MSPTI_ACTIVITY_SOURCE_KIND_DEVICE ? marker->objectId.ds.streamId : INVALID_STERAMID,
        NAME, marker->name,
        DOMAIN_NAME, marker->domain);

    if (markerData == nullptr) {
        MSPTI_LOGE("Build python marker data failed");
        return;
    }
    /* make sure callback doesn't go away */
    Py_INCREF(mstxCallback);
    auto ret = PyObject_CallFunction(mstxCallback, "O", markerData);
    if (ret == nullptr) {
        MSPTI_LOGE("Call mstx callback failed");
    } else {
        Py_DECREF(ret);
    }
    Py_DECREF(mstxCallback);
    Py_XDECREF(markerData);
}

void CallHcclCallback(PyObject *hcclCallback, const msptiActivityHccl* hccl)
{
    if (hcclCallback == nullptr) {
        MSPTI_LOGW("Hccl callback is nullptr");
        return;
    }
    if (hccl == nullptr) {
        MSPTI_LOGE("Hccl data is nullptr");
        return;
    }
    PyObject *hcclData = Py_BuildValue("{sIsKsKsIsIsdssss}",
        KIND, static_cast<uint32_t>(hccl->kind),
        START, hccl->start,
        END, hccl->end,
        DEVICE_ID, hccl->ds.deviceId,
        STREAM_ID, hccl->ds.streamId,
        BANDWIDTH, hccl->bandWidth,
        NAME, hccl->name,
        COMMNAME, hccl->commName);
    if (hcclData == nullptr) {
        MSPTI_LOGE("Build python hccl data failed");
        return;
    }
    /* make sure callback doesn't go away */
    Py_INCREF(hcclCallback);
    auto ret = PyObject_CallFunction(hcclCallback, "O", hcclData);
    if (ret == nullptr) {
        MSPTI_LOGE("Call hccl callback failed");
    } else {
        Py_DECREF(ret);
    }
    Py_DECREF(hcclCallback);
    Py_XDECREF(hcclData);
}

struct PyGILGuard {
    PyGILGuard() : gilState(PyGILState_Ensure()) {}
    ~PyGILGuard()
    {
        PyGILState_Release(gilState);
    }

    PyGILState_STATE gilState;
};
}

namespace Mspti {
namespace Adapter {
void MsptiAdapter::UserBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    if (buffer == nullptr || size == nullptr || maxNumRecords == nullptr) {
        MSPTI_LOGE("Wrong param for mspti adapter alloc buffer");
        return;
    }
    *maxNumRecords = 0;
    auto instance = GetInstance();
    if (!instance->bufferPool.CheckCanAllocBuffer()) {
        MSPTI_LOGW("Mspti adapter allocated buffer size exceeds the upper limit");
        *buffer = nullptr;
        *size = 0;
        return;
    }
    auto *pBuffer = instance->bufferPool.GetBuffer();
    if (pBuffer == nullptr) {
        MSPTI_LOGE("Mspti adapter alloc buffer failed");
        *buffer = nullptr;
        *size = 0;
    } else {
        *buffer = pBuffer;
        *size = instance->bufferSize;
    }
}

void MsptiAdapter::UserBufferComplete(uint8_t *buffer, size_t size, size_t validSize)
{
    if (!Py_IsInitialized()) {
        MSPTI_LOGW("Python interpreter is finalized");
        return;
    }
    MSPTI_LOGI("Mspti adapter buffer complete, size: %zu, validSize: %zu", size, validSize);
    PyGILGuard gilGuard;
    if (validSize > 0 && buffer != nullptr) {
        msptiActivity *record = nullptr;
        msptiResult status = MSPTI_SUCCESS;
        do {
            status = msptiActivityGetNextRecord(buffer, validSize, &record);
            if (status == MSPTI_SUCCESS) {
                UserBufferConsume(record);
            } else if (status == MSPTI_ERROR_MAX_LIMIT_REACHED) {
                break;
            } else {
                MSPTI_LOGE("Mspti adapter consume buffer failed, status: %d", status);
                break;
            }
        } while (true);
    }
    if (buffer != nullptr) {
        if (!GetInstance()->bufferPool.RecycleBuffer(buffer)) {
            MSPTI_LOGE("Mspti adapter recycle buffer failed");
        }
    }
}

void MsptiAdapter::UserBufferConsume(msptiActivity *record)
{
    if (record == nullptr) {
        MSPTI_LOGE("Mspti consume buffer failed");
        return;
    }
    if (record->kind == MSPTI_ACTIVITY_KIND_KERNEL) {
        msptiActivityKernel *kernel = Mspti::Common::ReinterpretConvert<msptiActivityKernel*>(record);
        CallKernelCallback(GetInstance()->GetKernelCallback(), kernel);
    } else if (record->kind == MSPTI_ACTIVITY_KIND_MARKER) {
        msptiActivityMarker* marker = Mspti::Common::ReinterpretConvert<msptiActivityMarker*>(record);
        CallMstxCallback(GetInstance()->GetMstxCallback(), marker);
    } else if (record->kind == MSPTI_ACTIVITY_KIND_HCCL) {
        msptiActivityHccl* hccl = Mspti::Common::ReinterpretConvert<msptiActivityHccl *>(record);
        CallHcclCallback(GetInstance()->GetHcclCallback(), hccl);
    } else {
        MSPTI_LOGW("Not supported mspti activity kind");
    }
}

msptiResult MsptiAdapter::Start()
{
    auto originCnt = refCnt_.fetch_add(1);
    if (originCnt == 0) {
        auto ret = msptiSubscribe(&subscriber_, nullptr, nullptr);
        if (ret == MSPTI_SUCCESS) {
            bufferPool.Clear();
            if (!(bufferPool.SetBufferSize(bufferSize) && bufferPool.SetPoolSize(MAX_BUFFER_SIZE / bufferSize))) {
                MSPTI_LOGE("Mspti adapter init buffer pool failed");
                msptiUnsubscribe(subscriber_);
                return MSPTI_ERROR_INNER;
            }
            return msptiActivityRegisterCallbacks(UserBufferRequest, UserBufferComplete);
        }
        return ret;
    }
    return MSPTI_SUCCESS;
}

msptiResult MsptiAdapter::Stop()
{
    if (refCnt_ >= 1) {
        refCnt_--;
    }
    return (refCnt_ == 0) ? msptiUnsubscribe(subscriber_) : MSPTI_SUCCESS;
}

msptiResult MsptiAdapter::FlushAll()
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto ret = msptiActivityFlushAll(1);
    if (refCnt_ == 0) {
        bufferPool.Clear();
    }
    return ret;
}

msptiResult MsptiAdapter::FlushPeriod(uint32_t time)
{
    std::lock_guard<std::mutex> lk(mtx_);
    return msptiActivityFlushPeriod(time);
}

msptiResult MsptiAdapter::SetBufferSize(size_t size)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (size * MB > MAX_BUFFER_SIZE) {
        MSPTI_LOGW("Set buffer size exceeds the upper limit");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    if (refCnt_ > 0) {
        MSPTI_LOGW("Change buffer size when mspti is running, will not take effect until next start");
    }
    bufferSize = size * MB;
    return MSPTI_SUCCESS;
}

msptiResult MsptiAdapter::RegisterMstxCallback(PyObject *mstxCallback)
{
    std::lock_guard<std::mutex> lk(mtx_);
    Py_XINCREF(mstxCallback);
    mstxCallback_ = mstxCallback;
    return msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER);
}

msptiResult MsptiAdapter::UnregisterMstxCallback()
{
    std::lock_guard<std::mutex> lk(mtx_);
    Py_XDECREF(mstxCallback_);
    mstxCallback_ = nullptr;
    return msptiActivityDisable(MSPTI_ACTIVITY_KIND_MARKER);
}

PyObject* MsptiAdapter::GetMstxCallback() const
{
    return mstxCallback_;
}

msptiResult MsptiAdapter::RegisterKernelCallback(PyObject *kernelCallback)
{
    std::lock_guard<std::mutex> lk(mtx_);
    Py_XINCREF(kernelCallback);
    kernelCallback_ = kernelCallback;
    return msptiActivityEnable(MSPTI_ACTIVITY_KIND_KERNEL);
}

PyObject* MsptiAdapter::GetKernelCallback() const
{
    return kernelCallback_;
}

msptiResult MsptiAdapter::UnregisterKernelCallback()
{
    std::lock_guard<std::mutex> lk(mtx_);
    Py_XDECREF(kernelCallback_);
    kernelCallback_ = nullptr;
    return msptiActivityDisable(MSPTI_ACTIVITY_KIND_KERNEL);
}

msptiResult MsptiAdapter::RegisterHcclCallback(PyObject *hcclCallback)
{
    std::lock_guard<std::mutex> lk(mtx_);
    Py_XINCREF(hcclCallback);
    hcclCallback_ = hcclCallback;
    return msptiActivityEnable(MSPTI_ACTIVITY_KIND_HCCL);
}

PyObject* MsptiAdapter::GetHcclCallback() const
{
    return hcclCallback_;
}

msptiResult MsptiAdapter::UnregisterHcclCallback()
{
    std::lock_guard<std::mutex> lk(mtx_);
    Py_XDECREF(hcclCallback_);
    hcclCallback_ = nullptr;
    return MSPTI_SUCCESS;
}

msptiResult MsptiAdapter::EnableDomain(const char* domain)
{
    return msptiActivityEnableMarkerDomain(domain);
}

msptiResult MsptiAdapter::DisableDomain(const char* domain)
{
    return msptiActivityDisableMarkerDomain(domain);
}
} // Adapter
} // Mspti
