/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : bound_queue.h
 * Description        : Bound Queue.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_BOUND_QUEUE_H
#define MSPTI_COMMON_BOUND_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

namespace Mspti {
namespace Common {

static const uint32_t QUEUE_CAPACITY_SIZE = 100;

template<class T>
class QueueBase {
public:
    explicit QueueBase(size_t capacity) : capacity_(capacity) {}
    virtual ~QueueBase() {}

protected:
    bool IsQueueFull() const
    {
        return  queue_.size() == capacity_;
    }

    bool IsQueueEmpty() const
    {
        return queue_.empty();
    }

protected:
    size_t capacity_;
    std::queue<T> queue_;
};

template<class T>
class BoundQueue : public QueueBase<T> {
public:
    explicit BoundQueue(size_t capacity)
        : QueueBase<T>(capacity),
          quit_(false),
          hisMaxCnt_(0)
    {
    }

    virtual ~BoundQueue() = default;

    bool TryPush(const T &data)
    {
        std::lock_guard<std::mutex> lk(mtx_);

        if (this->IsQueueFull()) {
            return false;
        }

        this->queue_.push(data);
        cvPush_.notify_all();
        return true;
    }

    bool Push(const T &data)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        size_t size = this->queue_.size();
        hisMaxCnt_ = (size > hisMaxCnt_) ? size : hisMaxCnt_;
        // if queue is full, wait until notify from pop, then check again
        cvPop_.wait(lk, [this] { return !this->IsQueueFull() || quit_; });
        if (!quit_) {
            this->queue_.push(data);
            cvPush_.notify_all();
            return true;
        }

        return false;
    }

    bool TryPop(T &data)
    {
        std::lock_guard<std::mutex> lk(mtx_);

        if (this->IsQueueEmpty()) {
            return false;
        }

        data = this->queue_.front();
        this->queue_.pop();
        cvPop_.notify_all();

        return true;
    }

    bool TryBatchPop(int batchCount, std::vector<T> &data)
    {
        std::lock_guard<std::mutex> lk(mtx_);

        if (this->IsQueueEmpty()) {
            return false;
        }

        int count = 0;
        while (!this->IsQueueEmpty() && (count < batchCount)) {
            data.push_back(this->queue_.front());
            this->queue_.pop();
            count++;
        }

        cvPop_.notify_all();

        return true;
    }

    bool Pop(T &data)
    {
        std::unique_lock<std::mutex> lk(mtx_);

        // if queue is empty, wait until notify from Push
        cvPush_.wait(lk, [this] { return !this->IsQueueEmpty() || quit_; });
        if (!this->IsQueueEmpty()) {
            data = this->queue_.front();
            this->queue_.pop();
            cvPop_.notify_all();
            return true;
        }

        return false;
    }

    void Quit()
    {
        std::lock_guard<std::mutex> lk(mtx_);

        if (quit_) {
            return;
        }
        quit_ = true;
        cvPush_.notify_all();
        cvPop_.notify_all();
    }

    void SetQuit()
    {
        std::lock_guard<std::mutex> lk(mtx_);

        if (!quit_) {
            quit_ = true;
            cvPush_.notify_all();
            cvPop_.notify_all();
        }
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return this->queue_.size();
    }

private:
    volatile bool quit_;
    size_t hisMaxCnt_;
    std::mutex mtx_;
    std::condition_variable cvPush_;
    std::condition_variable cvPop_;
};
}  // Common
}  // Mspti

#endif
