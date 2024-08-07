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
#include "common/plog_manager.h"
#include "common/utils.h"

namespace Mspti {
namespace Activity {

void ActivityBuffer::Init(msptiBuffersCallbackRequestFunc func)
{
    if (func == nullptr) {
        return;
    }
    func(&buf_, &buf_size_, &records_num_);
    const static uint64_t MIN_ACTIVITY_BUFFER_SIZE = 2 * 1024 * 1024;
    if (buf_size_ < MIN_ACTIVITY_BUFFER_SIZE) {
        PRINT_LOGW("Please malloc the Activity Buffer more than 2MB. Current is %lu Bytes.", buf_size_);
    }
}

void ActivityBuffer::UnInit(msptiBuffersCallbackCompleteFunc func)
{
    if (func == nullptr) {
        return;
    }
    func(buf_, buf_size_, valid_size_);
}

msptiResult ActivityBuffer::Record(msptiActivity *activity, size_t size)
{
    if (activity == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    if (size > buf_size_ - valid_size_) {
        PRINT_LOGW("Record is dropped due to insufficient space of Activity Buffer.");
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
        t_.join();
    }
    JoinWorkThreads();
    FlushAll();
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
    {
        std::lock_guard<std::mutex> lk(activity_mtx_);
        if (activity_set_.find(kind) != activity_set_.end()) {
            return MSPTI_SUCCESS;
        }
        activity_set_.insert(kind);
    }
    std::unordered_set<uint32_t> devices;
    {
        std::lock_guard<std::mutex> lk(devices_mtx_);
        devices.insert(devices_.begin(), devices_.end());
    }
    for (auto device : devices) {
        Mspti::Ascend::DevTaskManager::GetInstance()->StartDevProfTask(device, kind);
    }
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::UnRegister(msptiActivityKind kind)
{
    {
        std::lock_guard<std::mutex> lk(activity_mtx_);
        if (activity_set_.find(kind) == activity_set_.end()) {
            return MSPTI_SUCCESS;
        }
        activity_set_.erase(kind);
    }
    std::unordered_set<uint32_t> devices;
    {
        std::lock_guard<std::mutex> lk(devices_mtx_);
        devices.insert(devices_.begin(), devices_.end());
    }
    for (auto device : devices) {
        Mspti::Ascend::DevTaskManager::GetInstance()->StopDevProfTask(device, kind);
    }
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::GetNextRecord(uint8_t *buffer, size_t validBufferSizeBytes, msptiActivity **record)
{
    if (buffer == nullptr) {
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    static thread_local size_t pos = 0;
    if (pos >= validBufferSizeBytes) {
        pos = 0;
        return MSPTI_ERROR_MAX_LIMIT_REACHED;
    }
    msptiActivityKind *pKind = reinterpret_cast<msptiActivityKind*>(buffer + pos);
    if (*pKind == MSPTI_ACTIVITY_KIND_MARKER) {
        *record = reinterpret_cast<msptiActivity*>(buffer + pos);
        pos += sizeof(msptiActivityMark);
    }
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::FlushAll()
{
    if (cur_buf_) {
        co_activity_buffers_.emplace_back(std::move(cur_buf_));
    }
    for (const auto& activity_buffer : co_activity_buffers_) {
        activity_buffer->UnInit(bufferCompleted_handle_);
    }
    co_activity_buffers_.clear();
    return MSPTI_SUCCESS;
}

msptiResult ActivityManager::Record(msptiActivity *activity, size_t size)
{
    if (activity == nullptr) {
        return MSPTI_ERROR_INNER;
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
    } else if (cur_buf_->ValidSize() > ACTIVITY_BUFFER_THRESHOLD * cur_buf_->BufSize()) {
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
    while (true) {
        {
            std::unique_lock<std::mutex> lk(cv_mtx_);
            cv_.wait(lk, [&] () {return buf_full_ || !thread_run_.load();});
            if (!thread_run_.load()) {
                break;
            }
            for (auto& activity_buffer : co_activity_buffers_) {
                work_thread_.emplace_back(std::thread([this] (std::unique_ptr<ActivityBuffer> activity_buffer) {
                    activity_buffer->UnInit(this->bufferCompleted_handle_);
                }, std::move(activity_buffer)));
            }
            co_activity_buffers_.clear();
            buf_full_ = false;
        }
    }
    JoinWorkThreads();
}

void ActivityManager::SetDevice(uint32_t deviceId)
{
    {
        std::lock_guard<std::mutex> lk(devices_mtx_);
        if (devices_.find(deviceId) != devices_.end()) {
            return;
        }
        devices_.insert(deviceId);
    }
    std::unordered_set<msptiActivityKind> activity_set;
    {
        std::lock_guard<std::mutex> lk(activity_mtx_);
        activity_set.insert(activity_set_.begin(), activity_set_.end());
    }
    for (auto activity : activity_set) {
        Mspti::Ascend::DevTaskManager::GetInstance()->StartDevProfTask(deviceId, activity);
    }
}

void ActivityManager::DeviceReset(uint32_t deviceId)
{
    {
        std::lock_guard<std::mutex> lk(devices_mtx_);
        if (devices_.find(deviceId) == devices_.end()) {
            return;
        }
        devices_.erase(deviceId);
    }
    std::unordered_set<msptiActivityKind> activity_set;
    {
        std::lock_guard<std::mutex> lk(activity_mtx_);
        activity_set.insert(activity_set_.begin(), activity_set_.end());
    }
    for (auto activity : activity_set) {
        Mspti::Ascend::DevTaskManager::GetInstance()->StopDevProfTask(deviceId, activity);
    }
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
