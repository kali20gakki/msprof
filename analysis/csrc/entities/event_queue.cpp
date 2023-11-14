/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event_queue.cpp
 * Description        : EventQueue模块：使用vector模拟队列
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include <algorithm>
#include "event_queue.h"

namespace Analysis {
namespace Entities {

EventQueue::EventQueue(uint32_t threadId, uint64_t initSize) : threadId_(threadId), initSize_(initSize)
{
    data_.reserve(initSize); // initSize = Api size + Tasktrack size
}

void EventQueue::Sort()
{
    auto comp = [](const std::shared_ptr<Event> &event1,
                   const std::shared_ptr<Event> &event2) {
        return event1->info.start < event2->info.start
            || (event1->info.start == event2->info.start && event1->info.level > event2->info.level);
    };
    std::sort(data_.begin(), data_.begin() + tail_, comp);
}

void EventQueue::Push(const std::shared_ptr<Event> &event)
{
    if (!event) {
        return; // 补充log
    }

    if (tail_ == initSize_) {
        return; // 补充log
    }
    data_.emplace_back(event);
    tail_ += 1;
    bound_ = std::max(event->info.end, bound_);
}

std::shared_ptr<Event> EventQueue::Pop()
{
    if (Empty()) {
        return nullptr; // 补充log
    }
    auto event = data_[head_];
    head_ += 1;
    return event;
}

std::shared_ptr<Event> EventQueue::Top()
{
    if (Empty()) {
        return nullptr; // 补充log
    }
    return data_[head_];
}

bool EventQueue::Empty() const
{
    return head_ >= tail_;
}

uint64_t EventQueue::GetBound() const
{
    return bound_;
}

uint64_t EventQueue::GetSize() const
{
    if (Empty()) {
        return 0; // 补充log
    }
    return tail_ - head_;
}

} // namespace Entities
} // namespace Analysis