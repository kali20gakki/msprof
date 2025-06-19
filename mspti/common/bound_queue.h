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

template <typename T>
class MPSCQueue {
public:
    explicit MPSCQueue()
    {
        Node* dummy = new Node();
        head_.store(dummy);
        tail_ = dummy;
    }

    ~MPSCQueue()
    {
        while (!IsEmpty()) {
            T ans;
            Pop(ans);
        }
        delete tail_;
    }

    bool Push(const T& value)
    {
        Node* node = new Node(value);
        Node* prev = head_.exchange(node, std::memory_order_acq_rel);
        prev->next.store(node, std::memory_order_release);
        return true;
    }

    // 仅限单线程调用，非线程安全
    bool Pop(T& result)
    {
        Node* next = tail_->next.load(std::memory_order_acquire);
        if (!next) return false;

        result = std::move(*next->data);
        delete tail_;
        tail_ = next;
        return true;
    }

    // 仅限单线程调用，非线程安全
    bool Peek(T& result)
    {
        Node* next = tail_->next.load(std::memory_order_acquire);
        if (!next) return false;

        result = *next->data;
        return true;
    }

    // 仅单线程调用
    bool IsEmpty() const
    {
        return tail_->next.load(std::memory_order_acquire) == nullptr;
    }

    // 仅单线程调用
    template<typename Predicate>
    bool PopIf(T& result, Predicate pred)
    {
        Node* prev = tail_;
        Node* curr = tail_->next.load(std::memory_order_acquire);
        
        while (curr != nullptr) {
            if (pred(*curr->data)) {
                // 找到满足条件的节点
                Node* next = curr->next.load(std::memory_order_acquire);
                prev->next.store(next, std::memory_order_release);

                if (curr == head_.load(std::memory_order_acquire)) {
                    head_.compare_exchange_strong(curr, prev);
                }

                result = std::move(*curr->data);
                delete curr;
                tail_ = prev;
                
                return true;
            }
            
            prev = curr;
            curr = curr->next.load(std::memory_order_acquire);
        }
        return false;
    }

private:
    struct Node {
        std::unique_ptr<T> data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}
        explicit Node(T value) : data(std::make_unique<T>(std::move(value))), next(nullptr) {}
    };

    std::atomic<Node*> head_{};
    Node* tail_;  // only used by consumer thread
};

}  // Common
}  // Mspti

#endif
