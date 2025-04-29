/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : activity_buffer_pool.cpp
 * Description        : Mspti python adapter buffer pool.
 * Author             : msprof team
 * Creation Date      : 2025/04/12
 * *****************************************************************************
*/

#include "activity_buffer_pool.h"
#include "common/plog_manager.h"
#include "common/utils.h"

namespace Mspti {
namespace Adapter {
bool ActivityBufferPool::SetBufferSize(size_t bufferSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferSize == 0) {
        MSPTI_LOGE("Can not set mspti python adapter buffer pool buffer size with zero");
        return false;
    }
    bufferSize_ = bufferSize;
    return true;
}

bool ActivityBufferPool::SetPoolSize(size_t poolSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (poolSize == 0) {
        MSPTI_LOGE("Can not set mspti python adapter buffer pool size with zero");
        return false;
    }
    poolSize_ = poolSize;
    return true;
}

bool ActivityBufferPool::CheckCanAllocBuffer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return !freeBuffers_.empty() || allocedBuffers_.size() + 1 <= poolSize_;
}

uint8_t* ActivityBufferPool::GetBuffer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferSize_ == 0 || poolSize_ == 0) {
        MSPTI_LOGE("Mspti python adapter buffer pool can not alloc buffer when bufferSize or poolSize is zero");
        return nullptr;
    }
    if (freeBuffers_.empty() && allocedBuffers_.size() + 1 > poolSize_) {
        MSPTI_LOGE("Mspti python adapter buffer pool is full");
        return nullptr;
    }
    if (freeBuffers_.empty()) {
        std::unique_ptr<ActivityBuffer> activitybuffer = nullptr;
        Common::MsptiMakeUniquePtr(activitybuffer, bufferSize_);
        if (activitybuffer == nullptr) {
            return nullptr;
        }
        auto buffer = activitybuffer->Data();
        allocedBuffers_[Common::ReinterpretConvert<uintptr_t>(buffer)] = std::move(activitybuffer);
        MSPTI_LOGI("Mspti python adapter buffer pool alloc new buffer, total buffers: %zu", allocedBuffers_.size());
        return buffer;
    }
    auto freeBufferAddr = *freeBuffers_.begin();
    freeBuffers_.erase(freeBufferAddr);
    auto bufferIter = allocedBuffers_.find(freeBufferAddr);
    if (bufferIter == allocedBuffers_.end()) {
        MSPTI_LOGE("Mspti python adapter buffer pool can not find recycled buffer");
        return nullptr;
    }
    MSPTI_LOGI("Mspti python adapter buffer pool alloc recycled buffer, total buffers: %zu, free buffers: %zu",
               allocedBuffers_.size(), freeBuffers_.size());
    return bufferIter->second->Data();
}

bool ActivityBufferPool::RecycleBuffer(uint8_t* buffer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer == nullptr) {
        MSPTI_LOGE("Mspti python adapter buffer pool recycle with null buffer");
        return false;
    }
    auto bufferAddr = Common::ReinterpretConvert<uintptr_t>(buffer);
    if (allocedBuffers_.find(bufferAddr) != allocedBuffers_.end()) {
        freeBuffers_.insert(bufferAddr);
    } else {
        MSPTI_LOGE("Mspti python adapter buffer pool recycle with unknown buffer");
        return false;
    }
    MSPTI_LOGI("Mspti python adapter buffer pool recycle buffer, total buffers: %zu, free buffers: %zu",
               allocedBuffers_.size(), freeBuffers_.size());
    return true;
}

void ActivityBufferPool::Clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bufferSize_ = 0;
    poolSize_ = 0;
    freeBuffers_.clear();
    allocedBuffers_.clear();
}
} // namespace Adapter
} // namespace Mspti
