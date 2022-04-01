/**
 * @file bound_queue.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_COMMON_UTILS_BOUND_QUEUE_H
#define ADX_COMMON_UTILS_BOUND_QUEUE_H
#include <condition_variable>
#include <queue>
#include <mutex>
namespace Adx {
template<typename T>
class BoundQueue {
public:
    explicit BoundQueue (uint32_t capacity)
        : quit_(false), capacity_(capacity) {}
    virtual ~BoundQueue() {}
    bool TryPush(T &value)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (this->IsFull()) {
            return false;
        }

        dataQueue_.push(value);
        cvPush_.notify_all();
        return true;
    }

    bool Push(T &value)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cvPop_.wait(lk, [=] { return !this->IsFull() || quit_;});
        dataQueue_.push(value);
        cvPush_.notify_all();
        return true;
    }

    bool TryPop(T &value)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (dataQueue_.empty()) {
            return false;
        }

        value = dataQueue_.front();
        dataQueue_.pop();
        cvPop_.notify_all();
        return true;
    }

    bool Pop(T &value)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cvPush_.wait(lk, [=] { return !this->IsEmpty() || quit_; });
        if (!this->IsEmpty()) {
            value = this->dataQueue_.front();
            this->dataQueue_.pop();
            cvPop_.notify_all();
            return true;
        }

        return false;
    }

    bool IsEmpty() const
    {
        return dataQueue_.empty();
    }

    bool IsFull()
    {
        return dataQueue_.size() == capacity_;
    }

    void Quit()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!quit_) {
            quit_ = true;
            cvPush_.notify_all();
            cvPop_.notify_all();
        }
    }

    uint32_t Size()
    {
        return dataQueue_.size();
    }
private:
    mutable bool quit_;
    mutable std::mutex mtx_;
    std::queue<T> dataQueue_;
    std::condition_variable cvPop_;
    std::condition_variable cvPush_;
    uint32_t capacity_;
};
}
#endif
