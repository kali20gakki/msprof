/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mpsc_queue.h
 * Description        : 多线程生产单线程消费队列
 * Author             : msprof team
 * Creation Date      : 2025/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_PROJECT_MPSC_QUEUE_H
#define MSPTI_PROJECT_MPSC_QUEUE_H

#include <cstdint>
#include <atomic>
#include <cstddef>
#include <new>
#include "common/plog_manager.h"

namespace Mspti {
namespace Common {
constexpr size_t DEFAULT_CAPCAITY = 2048;

template<typename T>
class MPSCQueue {
public:
    MPSCQueue() : head_(new(std::nothrow) NodeT), tail_(head_.load(std::memory_order_relaxed))
    {
        NodeT *front = head_.load(std::memory_order_relaxed);
        if (front == nullptr) {
            MSPTI_LOGE("MPSC head_ new buffer failed.");
            return;
        }
        front->next.store(nullptr, std::memory_order_relaxed);
    }

    ~MPSCQueue()
    {
        T output;
        while (this->Pop(output)) {}
        NodeT *front = head_.load(std::memory_order_relaxed);
        delete front;
    }

    void CheckOverflow()
    {
        if (count_.load(std::memory_order_relaxed) > DEFAULT_CAPCAITY && !isOverFlow_.load(std::memory_order_relaxed)) {
            isOverFlow_.store(true);
            MSPTI_LOGW("MPSC queue length exceeded the maximum threshold! current lenght: %u", count_.load());
        }
    }

    void Push(const T &input)
    {
        NodeT *node = new(std::nothrow) NodeT;
        if (node == nullptr) {
            MSPTI_LOGE("MPSC queue new buffer failed.");
            return;
        }
        node->data = input;
        node->next.store(nullptr, std::memory_order_relaxed);

        NodeT *prevhead_ = head_.exchange(node, std::memory_order_acq_rel);
        prevhead_->next.store(node, std::memory_order_release);
        count_.fetch_add(1, std::memory_order_relaxed);
        CheckOverflow();
    }

    void Push(T &&input)
    {
        NodeT *node = new(std::nothrow) NodeT;
        if (node == nullptr) {
            MSPTI_LOGE("MPSC queue new buffer failed.");
            return;
        }
        node->data = std::move(input);
        node->next.store(nullptr, std::memory_order_relaxed);

        NodeT *prevhead = head_.exchange(node, std::memory_order_acq_rel);
        prevhead->next.store(node, std::memory_order_release);
        count_.fetch_add(1, std::memory_order_relaxed);
        CheckOverflow();
    }

    bool Pop(T &output)
    {
        NodeT *tail = tail_.load(std::memory_order_relaxed);
        if (tail == nullptr) {
            MSPTI_LOGE("MPSC queue tail is nullptr.");
            return false;
        }
        NodeT *next = tail->next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return false;
        }

        output = std::move(next->data);
        tail_.store(next, std::memory_order_release);
        count_.fetch_sub(1, std::memory_order_relaxed);
        delete tail;
        return true;
    }

    bool Peek(T &output) const
    {
        NodeT *tail = tail_.load(std::memory_order_acquire);
        if (tail == nullptr) {
            MSPTI_LOGE("MPSC queue tail is nullptr.");
            return false;
        }
        NodeT *next = tail->next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return false;
        }

        output = next->data;
        return true;
    }

    bool IsEmpty() const
    {
        NodeT *tail = tail_.load(std::memory_order_acquire);
        if (tail == nullptr) {
            MSPTI_LOGE("MPSC queue tail is nullptr.");
            return true;
        }
        NodeT *next = tail->next.load(std::memory_order_acquire);
        return (next == nullptr);
    }

private:
    struct NodeT {
        T data;
        std::atomic<NodeT *> next;
    };
    alignas(64) std::atomic<bool> isOverFlow_{false};
    alignas(64) std::atomic<size_t> count_{0};  // count不能实时反应链表实际长度，不要用count判断IsEmpty
    alignas(64) std::atomic<NodeT *> head_;
    alignas(64) std::atomic<NodeT *> tail_;
};
}
}

#endif // MSPTI_PROJECT_MPSC_QUEUE_H
