/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_adpter.cpp
 * Description        : Mspti python adapter.
 * Author             : msprof team
 * Creation Date      : 2024/11/19
 * *****************************************************************************
*/

#include "mspti_adapter.h"
#include "common/plog_manager.h"
#include "common/utils.h"

namespace {
// mspti adapter buffer alloc and free
constexpr size_t ALIGN_SIZE = 8;
constexpr size_t DEFAULT_BUFFER_SIZE = 8 * 1024 * 1024;
constexpr size_t MAX_BUFFER_SIZE = 256 * 1024 * 1024;
constexpr uint32_t MAX_ALLOC_CNT = MAX_BUFFER_SIZE / DEFAULT_BUFFER_SIZE;

void *MsptiMalloc(size_t size, size_t alignment)
{
    if (alignment > 0) {
        size = (size + alignment - 1) / alignment * alignment;
    }
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
    void *ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = nullptr;
    }
    return ptr;
#else
    return malloc(size);
#endif
}

void MsptiFree(uint8_t *ptr)
{
    if (ptr != nullptr) {
        free(ptr);
    }
}

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
    PyObject *markerData = Py_BuildValue("{sIsIsIsKsKsIsIsIsIssss}",
        KIND, static_cast<uint32_t>(marker->kind),
        FLAG, static_cast<uint32_t>(marker->flag),
        SOURCE_KIND, static_cast<uint32_t>(marker->sourceKind),
        TIMESTAMP, marker->timestamp,
        ID, marker->id,
        PROCESS_ID, marker->objectId.pt.processId,
        THREAD_ID, marker->objectId.pt.threadId,
        DEVICE_ID, marker->objectId.ds.deviceId,
        STREAM_ID, marker->objectId.ds.streamId,
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
std::atomic<uint32_t> MsptiAdapter::allocCnt{0};

void MsptiAdapter::UserBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    if (buffer == nullptr || size == nullptr || maxNumRecords == nullptr) {
        MSPTI_LOGE("Wrong param for mspti adapter alloc buffer");
        return;
    }
    *maxNumRecords = 0;
    if (allocCnt >= MAX_ALLOC_CNT) {
        MSPTI_LOGW("Mspti adapter allocated buffer size exceeds the upper limit");
        *buffer = nullptr;
        *size = 0;
        return;
    }
    uint8_t *pBuffer = Mspti::Common::ReinterpretConvert<uint8_t*>(MsptiMalloc(DEFAULT_BUFFER_SIZE, ALIGN_SIZE));
    if (pBuffer == nullptr) {
        MSPTI_LOGE("Mspti adapter alloc buffer failed");
        *buffer = nullptr;
        *size = 0;
    } else {
        *buffer = pBuffer;
        *size = DEFAULT_BUFFER_SIZE;
        allocCnt++;
    }
}

void MsptiAdapter::UserBufferComplete(uint8_t *buffer, size_t size, size_t validSize)
{
    if (!Py_IsInitialized()) {
        MSPTI_LOGW("Python interpreter is finalized");
        MsptiFree(buffer);
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
            }
        } while (true);
        // Minus allocCnt when buffer is not nullptr
        allocCnt--;
    }
    MsptiFree(buffer);
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
            return msptiActivityRegisterCallbacks(UserBufferRequest, UserBufferComplete);
        }
        return ret;
    }
    return MSPTI_SUCCESS;
}

msptiResult MsptiAdapter::Stop()
{
    refCnt_--;
    if (refCnt_ == 0) {
        return msptiUnsubscribe(subscriber_);
    }
    return MSPTI_SUCCESS;
}

msptiResult MsptiAdapter::FlushAll()
{
    std::lock_guard<std::mutex> lk(mtx_);
    return msptiActivityFlushAll(1);
}

msptiResult MsptiAdapter::FlushPeriod(uint32_t time)
{
    std::lock_guard<std::mutex> lk(mtx_);
    return msptiActivityFlushPeriod(time);
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
} // Adapter
} // Mspti