/* ******************************************************************************
    版权所有 (c) 华为技术有限公司 2025-2025
    Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : object_pool.h
 * Description        : 简易对象池
 * Author             : msprof team
 * Creation Date      : 2025/06/03
 * *****************************************************************************
 */

#ifndef MSPTI_COMMON_OBJECT_POOL_H
#define MSPTI_COMMON_OBJECT_POOL_H

#include <iostream>
#include <atomic>
#include <mutex>
#include <stack>
#include <memory>

#include "common/plog_manager.h"

namespace Mspti {
namespace Common {
const size_t OBJECT_POOL_DEFAULT_SIZE = 1024;

template <typename T, size_t N> class ObjectPool {
public:
    using Ptr = std::shared_ptr<T>;

    ObjectPool() : top_(N)
    {
        for (size_t i = 0; i < N; ++i) {
            auto ptr = new(std::nothrow) T();
            if (ptr == nullptr) {
                MSPTI_LOGE("Object pool init fail, new Object fail");
                continue;
            }
            buffer_[i].reset(ptr);
        }
    }

    Ptr Get()
    {
        size_t currentTop = top_.load(std::memory_order_acquire);
        while (currentTop > 0) {
            if (top_.compare_exchange_weak(currentTop, currentTop - 1, std::memory_order_acq_rel)) {
                T *rawPtr = buffer_[currentTop - 1].release();
                return Ptr(rawPtr, [this](T *ptr) { this->Release(ptr); });
            }
        }

        std::lock_guard<std::mutex> lock(overFlowMutex_);
        if (!overFlowPool_.empty()) {
            T *obj = overFlowPool_.top().release();
            overFlowPool_.pop();
            return Ptr(obj, [this](T *ptr) { this->Release(ptr); });
        }

        // 溢出池空，new一个
        T *obj = new (std::nothrow) T();
        return Ptr(obj, [this](T *ptr) { this->Release(ptr); });
    }

private:
    void Release(T *obj)
    {
        size_t currentTop = top_.load(std::memory_order_acquire);
        while (currentTop < N) {
            if (top_.compare_exchange_weak(currentTop, currentTop + 1, std::memory_order_acq_rel)) {
                buffer_[currentTop].reset(obj);
                return;
            }
        }

        std::lock_guard<std::mutex> lock(overFlowMutex_);
        overFlowPool_.emplace(obj);
    }

    std::unique_ptr<T> buffer_[N];
    std::atomic<size_t> top_;

    std::stack<std::unique_ptr<T>> overFlowPool_;
    std::mutex overFlowMutex_;
};
}
}

#endif