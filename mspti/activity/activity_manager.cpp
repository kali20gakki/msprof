/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : activity_manager.cpp
 * Description        : Manager Activity.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "activity/activity_manager.h"

#include <cstring>
#include <functional>
#include "securec.h"

#include "activity/ascend/dev_task_manager.h"
#include "activity/ascend/reporter/external_correlation_reporter.h"
#include "common/plog_manager.h"
#include "common/utils.h"

namespace Mspti {
namespace Activity {

void ActivityBuffer::Init(msptiBuffersCallbackRequestFunc func)
{
    if (func == nullptr) {
        MSPTI_LOGE("The request callback is nullptr.");
        return;
    }
    func(&buf_, &buf_size_, &records_num_);
    const static uint64_t MIN_ACTIVITY_BUFFER_SIZE = 2 * 1024 * 1024;
    if (buf_size_ < MIN_ACTIVITY_BUFFER_SIZE) {
        MSPTI_LOGW("Please malloc the Activity Buffer more than 2MB. Current is %lu Bytes.", buf_size_);
    }
}

void ActivityBuffer::UnInit(msptiBuffersCallbackCompleteFunc func)
{
    if (func == nullptr) {
        MSPTI_LOGE("The complete callback is nullptr.");
        return;
    }
    func(buf_, buf_size_, valid_size_);
}

msptiResult ActivityBuffer::Record(msptiActivity *activity, size_t size)
{
    if (activity == nullptr) {
        MSPTI_LOGE("The activity is nullptr, failed to record.");
        return MSPTI_ERROR_INNER;
    }
    if (buf_ == nullptr) {
        MSPTI_LOGE("The ActivityBuffer is nullptr, failed to record activity.");
        return MSPTI_ERROR_INNER;
    }
    if (size > buf_size_ - valid_size_) {
        MSPTI_LOGW("Record is dropped due to insufficient space of Activity Buffer.");
        return MSPTI_ERROR_INNER;
    }
    if (memcpy_s(buf_ + valid_size_, buf_size_ - valid_size_, activity, size) != EOK) {
        return MSPTI_ERROR_INNER;
    }
    valid_size_ += size;
    records_num_++;
    return MSPTI_SUCCESS;
}

size_t ActivityBuffer::BufSize()
{
    return buf_size_;
}

size_t ActivityBuffer::ValidSize()
{
    return valid_size_;
}

const std::set<msptiActivityKind> ActivityManager::supportActivityKinds_ = {
    MSPTI_ACTIVITY_KIND_MARKER, MSPTI_ACTIVITY_KIND_KERNEL, MSPTI_ACTIVITY_KIND_API, MSPTI_ACTIVITY_KIND_HCCL,
    MSPTI_ACTIVITY_KIND_MEMORY, MSPTI_ACTIVITY_KIND_MEMSET, MSPTI_ACTIVITY_KIND_MEMCPY,
    MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION
};

ActivityManager *ActivityManager::GetInstance()
{
    static ActivityManager instance;
    return &instance;
}

ActivityManager::~ActivityManager()
{
    if (thread_run_.load() || t_.joinable()) {
        thread_run_.store(false);
        {
            std::unique_lock<std::mutex> lck(cv_mtx_);
            try {
                cv_.notify_one();
            } catch(...) {
                // Exception occurred during destruction of ActivityManager
            }
        }
        try {
            t_.join();
        } catch(...) {
            // Exception occurred during destruction of ActivityManager
        }
    }
    JoinWorkThreads();
    activity_set_.clear();
    devices_.clear();
    MSPTI_LOGI("Total activity record: %lu. Total activity drop: %lu",
        total_record_num_.load(), total_drop_num_.load());
}

void ActivityManager::JoinWorkThreads()
{
    for (auto &thread : work_thread_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

msptiResult ActivityManager::RegisterCallbacks(
    msptiBuffersCallbackRequestFunc funcBufferRequested,
    msptiBuffersCallbackCompleteFunc funcBufferCompleted)
{
    if (funcBufferRequested == nullptr || funcBufferCompleted == nullptr) {
        MSPTI_LOGE("Call msptiActivityRegisterCallbacks failed while request or complete callback is nullptr.");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    bufferRequested_handle_ = funcBufferRequested;
    bufferCompleted_handle_ = funcBufferCompleted;
    if (!t_.joinable()) {
        t_ = std::thread(std::bind(&ActivityManager::Run, this));
        thread_run_.store(true);
    }
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::Register(msptiActivityKind kind)
{
    if (supportActivityKinds_.find(kind) == supportActivityKinds_.end()) {
        MSPTI_LOGE("The ActivityKind: %d was not support.", static_cast<int>(kind));
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    {
        std::lock_guard<std::mutex> lk(activity_mtx_);
        if (activity_set_.find(kind) != activity_set_.end()) {
            return MSPTI_SUCCESS;
        }
        activity_set_.insert(kind);
        append_only_activity_set_.insert(kind);
        MSPTI_LOGI("Register Activity kind: %d", static_cast<int>(kind));
    }
    std::lock_guard<std::mutex> lk(devices_mtx_);
    for (auto device : devices_) {
        Mspti::Ascend::DevTaskManager::GetInstance()->StartDevProfTask(device, {kind});
    }
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::UnRegister(msptiActivityKind kind)
{
    if (supportActivityKinds_.find(kind) == supportActivityKinds_.end()) {
        MSPTI_LOGE("The ActivityKind: %d was not support.", static_cast<int>(kind));
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    {
        std::lock_guard<std::mutex> lk(activity_mtx_);
        if (activity_set_.find(kind) == activity_set_.end()) {
            return MSPTI_SUCCESS;
        }
        activity_set_.erase(kind);
        MSPTI_LOGI("UnRegister Activity kind: %d", static_cast<int>(kind));
    }
    return MSPTI_SUCCESS;
}

bool ActivityManager::IsActivityKindEnable(msptiActivityKind kind)
{
    std::lock_guard<std::mutex> lk(activity_mtx_);
    return activity_set_.find(kind) != activity_set_.end();
}

msptiResult ActivityManager::GetNextRecord(uint8_t *buffer, size_t validBufferSizeBytes, msptiActivity **record)
{
    if (buffer == nullptr) {
        MSPTI_LOGE("The address of Activity Buffer is nullptr.");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    static thread_local size_t pos = 0;
    if (pos >= validBufferSizeBytes) {
        pos = 0;
        return MSPTI_ERROR_MAX_LIMIT_REACHED;
    }

    static const std::unordered_map<msptiActivityKind, size_t> activityKindDataSize = {
        {MSPTI_ACTIVITY_KIND_MARKER,                sizeof(msptiActivityMarker)},
        {MSPTI_ACTIVITY_KIND_KERNEL,                sizeof(msptiActivityKernel)},
        {MSPTI_ACTIVITY_KIND_API,                   sizeof(msptiActivityApi)},
        {MSPTI_ACTIVITY_KIND_HCCL,                  sizeof(msptiActivityHccl)},
        {MSPTI_ACTIVITY_KIND_MEMORY,                sizeof(msptiActivityMemory)},
        {MSPTI_ACTIVITY_KIND_MEMSET,                sizeof(msptiActivityMemset)},
        {MSPTI_ACTIVITY_KIND_MEMCPY,                sizeof(msptiActivityMemcpy)},
        {MSPTI_ACTIVITY_KIND_EXTERNAL_CORRELATION,  sizeof(msptiActivityExternalCorrelation)}
    };

    msptiActivityKind *pKind = Common::ReinterpretConvert<msptiActivityKind*>(buffer + pos);
    auto iter = activityKindDataSize.find(*pKind);
    if (iter == activityKindDataSize.end()) {
        MSPTI_LOGE("GetNextRecord failed, invalid kind: %d", *pKind);
        return MSPTI_ERROR_INNER;
    }
    *record = Common::ReinterpretConvert<msptiActivity*>(buffer + pos);
    pos += iter->second;
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::FlushAll()
{
    {
        std::lock_guard<std::mutex> lk(buf_mtx_);
        if (cur_buf_) {
            auto consumeBuf = std::move(cur_buf_);
            consumeBuf->UnInit(this->bufferCompleted_handle_);
        }
    }
    JoinWorkThreads();
    MSPTI_LOGI("Flush all activity buffer.");
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::FlushPeriod(uint32_t time)
{
    std::unique_lock<std::mutex> lck(cv_mtx_);
    if (time == 0) {
        flush_period_time_ = DEFAULT_PERIOD_FLUSH_TIME;
        flush_period_ = false;
    } else {
        flush_period_time_ = time;
        flush_period_ = true;
        cv_.notify_one();
    }
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::Record(msptiActivity *activity, size_t size)
{
    if (activity == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    {
        std::lock_guard<std::mutex> lk(activity_mtx_);
        if (activity_set_.find(activity->kind) == activity_set_.end()) {
            return MSPTI_SUCCESS;
        }
    }
    static const float ACTIVITY_BUFFER_THRESHOLD = 0.8;
    std::lock_guard<std::mutex> lk(buf_mtx_);
    if (!cur_buf_) {
        Mspti::Common::MsptiMakeUniquePtr(cur_buf_);
        if (!cur_buf_) {
            MSPTI_LOGE("Failed to init Activity Buffer.");
            return MSPTI_ERROR_INNER;
        }
        cur_buf_->Init(bufferRequested_handle_);
    } else if (cur_buf_->ValidSize() >= ACTIVITY_BUFFER_THRESHOLD * cur_buf_->BufSize()) {
        {
            std::unique_lock<std::mutex> lck(cv_mtx_);
            buf_full_ = true;
            co_activity_buffers_.emplace_back(std::move(cur_buf_));
            cv_.notify_one();
        }
        Mspti::Common::MsptiMakeUniquePtr(cur_buf_);
        if (!cur_buf_) {
            MSPTI_LOGE("Failed to init Activity Buffer.");
            return MSPTI_ERROR_INNER;
        }
        cur_buf_->Init(bufferRequested_handle_);
    }
    if (cur_buf_->Record(activity, size) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to record activity.");
        total_drop_num_++;
        return MSPTI_ERROR_INNER;
    }
    total_record_num_++;
    return MSPTI_SUCCESS;
}

void ActivityManager::Run()
{
    pthread_setname_np(pthread_self(), "ActivityManager");
    while (true) {
        {
            std::unique_lock<std::mutex> lk(cv_mtx_);
            bool serveForWaitFor = true;
            cv_.wait_for(lk, std::chrono::milliseconds(flush_period_time_), [&]() {
                serveForWaitFor = !serveForWaitFor;
                return (serveForWaitFor && flush_period_) || buf_full_ || !thread_run_.load();
            });
            if (!thread_run_.load()) {
                break;
            }
            {
                for (auto& activity_buffer : co_activity_buffers_) {
                    work_thread_.emplace_back(std::thread([this] (std::unique_ptr<ActivityBuffer> activity_buffer) {
                        activity_buffer->UnInit(this->bufferCompleted_handle_);
                    }, std::move(activity_buffer)));
                }
                co_activity_buffers_.clear();
                buf_full_ = false;
            }
        }
    }
    JoinWorkThreads();
}

msptiResult ActivityManager::SetDevice(uint32_t deviceId)
{
    MSPTI_LOGI("Set device: %u", deviceId);
    {
        std::lock_guard<std::mutex> lk(devices_mtx_);
        if (devices_.find(deviceId) != devices_.end()) {
            MSPTI_LOGW("Device: %u is already set.", deviceId);
            return MSPTI_SUCCESS;
        }
        devices_.insert(deviceId);
    }
    std::lock_guard<std::mutex> lk(activity_mtx_);
    if (activity_set_.empty()) {
        return MSPTI_SUCCESS;
    }
    return Mspti::Ascend::DevTaskManager::GetInstance()->StartDevProfTask(deviceId, activity_set_);
}

msptiResult ActivityManager::ResetAllDevice()
{
    auto ret = MSPTI_SUCCESS;
    std::lock_guard<std::mutex> lk(devices_mtx_);
    for (const auto& device : devices_) {
        MSPTI_LOGI("Reset device: %u", device);
        auto temp = Mspti::Ascend::DevTaskManager::GetInstance()->StopDevProfTask(device, append_only_activity_set_);
        if (temp != MSPTI_SUCCESS) {
            ret = temp;
        }
    }
    return ret;
}

}  // Activity
}  // Mspti

msptiResult msptiActivityRegisterCallbacks(
    msptiBuffersCallbackRequestFunc funcBufferRequested, msptiBuffersCallbackCompleteFunc funcBufferCompleted)
{
    return Mspti::Activity::ActivityManager::GetInstance()->RegisterCallbacks(funcBufferRequested, funcBufferCompleted);
}

msptiResult msptiActivityEnable(msptiActivityKind kind)
{
    return Mspti::Activity::ActivityManager::GetInstance()->Register(kind);
}

msptiResult msptiActivityDisable(msptiActivityKind kind)
{
    return Mspti::Activity::ActivityManager::GetInstance()->UnRegister(kind);
}

msptiResult msptiActivityGetNextRecord(uint8_t *buffer, size_t validBufferSizeBytes, msptiActivity **record)
{
    return Mspti::Activity::ActivityManager::GetInstance()->GetNextRecord(buffer, validBufferSizeBytes, record);
}

msptiResult msptiActivityFlushAll(uint32_t flag)
{
    return Mspti::Activity::ActivityManager::GetInstance()->FlushAll();
}

msptiResult msptiActivityFlushPeriod(uint32_t time)
{
    return Mspti::Activity::ActivityManager::GetInstance()->FlushPeriod(time);
}

msptiResult msptiActivityPushExternalCorrelationId(msptiExternalCorrelationKind kind, uint64_t id)
{
    return Mspti::Reporter::ExternalCorrelationReporter::GetInstance()->PushExternalCorrelationId(kind, id);
}

msptiResult msptiActivityPopExternalCorrelationId(msptiExternalCorrelationKind kind, uint64_t *lastId)
{
    return Mspti::Reporter::ExternalCorrelationReporter::GetInstance()->PopExternalCorrelationId(kind, lastId);
}