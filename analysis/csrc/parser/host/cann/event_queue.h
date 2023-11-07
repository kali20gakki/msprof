/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event_queue.h
 * Description        : EventQueue模块：使用vector模拟队列
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_QUEUE_H
#define ANALYSIS_PARSER_HOST_CANN_QUEUE_H

#include <cstdint>
#include <vector>
#include "event.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {

// Event队列, 为了性能随使用vector模拟EventQueue, 队列中每个元素为Event的指针
// 每一个threadId对应一个EventQueue
class EventQueue {
public:
    EventQueue(uint32_t threadId, uint64_t initSize);
    // 队列排序
    void Lock();
    // 向队尾插入一个Event
    void Push(const std::shared_ptr<Event> &event);
    // 模拟弹出队头的Event
    std::shared_ptr<Event> Pop();
    // 获取队头的Event
    std::shared_ptr<Event> Top();
    // 判断队列是否为空
    bool Empty() const;
    // 获取所有Event中最晚的时间点
    uint64_t GetBound() const;

private:
    uint32_t threadId_ = 0; // 该队列中的Event所在的线程ID
    uint64_t size_ = 0;     // Tail ptr
    uint64_t index_ = 0;    // Head ptr
    uint64_t bound_ = 0;    // 该EventQueue最晚的时间点
    bool lock_ = false;     // 是否已经排序完成标志位, true为排序完成
    std::vector<std::shared_ptr<Event>> data_;
};

} // namespace Cann
} // namespace Host
} // namespace Parser
} // namespace Analysis
#endif // ANALYSIS_PARSER_HOST_CANN_EVENT_QUEUE_H