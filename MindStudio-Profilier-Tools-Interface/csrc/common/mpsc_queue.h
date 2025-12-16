/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_PROJECT_MPSC_QUEUE_H
#define MSPTI_PROJECT_MPSC_QUEUE_H

#include <cstdint>
#include <atomic>
#include <cstddef>
#include <new>
#include "csrc/common/plog_manager.h"

namespace Mspti {
namespace Common {
constexpr size_t DEFAULT_CAPACITY = 2048;

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
        if (count_.load(std::memory_order_relaxed) > DEFAULT_CAPACITY && !isOverFlow_.load(std::memory_order_relaxed)) {
            isOverFlow_.store(true);
            MSPTI_LOGW("MPSC queue length exceeded the maximum threshold! current length: %u", count_.load());
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
