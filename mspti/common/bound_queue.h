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
#include <atomic>

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

constexpr size_t DEFAULT_CAPCAITY = 1024;

template<typename T, size_t Capacity>
class MPSCQueue {
public:
    MPSCQueue()
    {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    bool Push(const T& item)
    {
        size_t pos = writeIndex_.fetch_add(1, std::memory_order_acq_rel);
        Slot& slot = buffer_[pos & mask_];

        size_t expected_seq = pos;
        if (slot.sequence.load(std::memory_order_acquire) != expected_seq) {
            return false;
        }

        slot.data = item;
        slot.sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool Pop(T& result)
    {
        Slot& slot = buffer_[readIndex_ & mask_];
        size_t expected_seq = readIndex_ + 1;

        if (slot.sequence.load(std::memory_order_acquire) != expected_seq) {
            return false; // 无数据
        }

        result = std::move(slot.data);
        slot.sequence.store(readIndex_ + Capacity, std::memory_order_release);
        ++readIndex_;
        return true;
    }

    bool IsEmpty() const
    {
        return writeIndex_.load(std::memory_order_acquire) == readIndex_;
    }

    bool Peek(T& result) const
    {
        const Slot& slot = buffer_[readIndex_ & mask_];
        size_t expected_seq = readIndex_ + 1;

        if (slot.sequence.load(std::memory_order_acquire) != expected_seq) {
            return false; // 当前槽位还未写入
        }

        result = slot.data; // 复制数据，不移动游标
        return true;
    }

    template<typename Predicate>
    bool PopIf(T& result, Predicate pred)
    {
        while (true) {
            Slot& slot = buffer_[readIndex_ & mask_];
            size_t expected_seq = readIndex_ + 1;

            if (slot.sequence.load(std::memory_order_acquire) != expected_seq) {
                return false;
            }

            if (pred(slot.data)) {
                result = std::move(slot.data);
                slot.sequence.store(readIndex_ + Capacity, std::memory_order_release);
                ++readIndex_;
                return true;
            } else {
                slot.sequence.store(readIndex_ + Capacity, std::memory_order_release);
                ++readIndex_;
                continue;
            }
        }
    }

private:
    struct Slot {
        T data;
        std::atomic<size_t> sequence;
    };

    static constexpr size_t mask_ = Capacity - 1;
    Slot buffer_[Capacity];
    std::atomic<size_t> writeIndex_{0};
    size_t readIndex_{0}; // only consumer touches
};

}  // Common
}  // Mspti

#endif
