/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : activity_buffer_pool.h
 * Description        : Mspti python adapter buffer pool.
 * Author             : msprof team
 * Creation Date      : 2025/04/12
 * *****************************************************************************
*/

#ifndef MSPTI_ADAPTER_BUFFER_POOL_H_
#define MSPTI_ADAPTER_BUFFER_POOL_H_

#include <mutex>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Mspti {
namespace Adapter {
class ActivityBuffer {
public:
    explicit ActivityBuffer(size_t size)
    {
        buf_.reserve(size);
    }

    ActivityBuffer() = delete;
    ActivityBuffer(const ActivityBuffer&) = delete;
    ActivityBuffer& operator=(const ActivityBuffer&) = delete;
    ActivityBuffer(ActivityBuffer&&) = default;
    ActivityBuffer& operator=(ActivityBuffer&&) = default;

    uint8_t* Data()
    {
        return buf_.data();
    }

private:
    std::vector<uint8_t> buf_;
};

class ActivityBufferPool {
public:
    ActivityBufferPool() = default;
    bool SetBufferSize(size_t bufferSize);
    bool SetPoolSize(size_t poolSize);
    bool CheckCanAllocBuffer();
    uint8_t* GetBuffer();
    bool RecycleBuffer(uint8_t* buffer);
    void Clear();

private:
    std::mutex mutex_;
    size_t bufferSize_{0};
    size_t poolSize_{0};
    std::unordered_map<uintptr_t, std::unique_ptr<ActivityBuffer>> allocedBuffers_;
    std::unordered_set<uintptr_t> freeBuffers_;
};
} // namespace Adapter
} // namespace Mspti

#endif // MSPTI_ADAPTER_BUFFER_POOL_H_
