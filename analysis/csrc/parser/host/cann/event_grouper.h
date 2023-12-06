/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event_grouper.h
 * Description        : EventGrouper模块：多线程解析bin, 生成Event插入到对应类型EventQueue
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_EVENT_GROUPER_H
#define ANALYSIS_PARSER_HOST_CANN_EVENT_GROUPER_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <set>
#include "event.h"
#include "event_queue.h"
#include "cann_warehouse.h"
#include "safe_unordered_map.h"
#include "thread_pool.h"
#include "prof_common.h"
#include "addition_info_parser.h"
#include "api_event_parser.h"
#include "compact_info_parser.h"
#include "log.h"
#include "utils.h"
#include "time_logger.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {

using ThreadPool = Analysis::Utils::ThreadPool;
using Event = Analysis::Entities::Event;
using EventType = Analysis::Entities::EventType;
using EventInfo = Analysis::Entities::EventInfo;
using EventQueue = Analysis::Entities::EventQueue;

using CANNWarehouses = Analysis::Utils::SafeUnorderedMap<uint32_t, CANNWarehouse>;

class EventGrouper {
public:
    // hostPath为采集侧落盘host二进制数据路径
    explicit EventGrouper(const std::string &hostPath) : hostPath_(hostPath)
    {}
    // 多线程解析bin,生成Event插入到对应类型EventQueue
    bool Group();

    // 获取解析分类完成的Event, Key为threadId
    CANNWarehouses &GetGroupEvents();
    // 获取所有ThreadID的集合
    std::set<uint32_t> &GetThreadIdSet();
    // 获取原始全量Api类型的数据
    std::vector<std::shared_ptr<MsprofApi>> &GetApiTraces();
    // 获取FlipTask数据
    std::vector<std::shared_ptr<Adapter::FlipTask>> &GetFlipTasks();
private:
    bool isKernelApiEvent(const std::shared_ptr<MsprofApi> &trace) const;

    template<typename P, typename M, std::shared_ptr<EventQueue> CANNWarehouse::*element>
    void GroupEvents(const std::string &typeName, EventType eventType)
    {
        // 1. 解析bin
        Utils::TimeLogger t{"Group " + typeName};
        auto parser = Utils::MakeShared<P>(hostPath_);
        if (!parser) {
            ERROR("EventParser make shared ptr failed");
            return;
        }
        auto traces = parser->template ParseData<M>();

        // 统计各个threadId元素个数，用于EventQueue分配精确的内存大小，避免大量内存浪费
        std::unordered_map<uint32_t, uint64_t> threadIdNum;
        std::set<uint32_t> threadIds;
        for (const auto &trace: traces) {
            threadIds.insert(trace->threadId);
            threadIdNum[trace->threadId] += 1;
        }

        for (const auto &trace: traces) {
            EventInfo info{eventType, trace->level, trace->timeStamp, trace->timeStamp};
            auto event = Utils::MakeShared<Event>(trace, typeName, info);
            if (!parser) {
                ERROR("Event make shared ptr failed");
                return;
            }
            // 新建
            if (!cannWarehouses_.FindAndInsertIfNotExist(trace->threadId) ||
                !(cannWarehouses_[trace->threadId].*element)) {
                cannWarehouses_[trace->threadId].*element =
                    Utils::MakeShared<EventQueue>(trace->threadId, threadIdNum[trace->threadId]);
            }

            // 插入
            (cannWarehouses_[trace->threadId].*element)->Push(event);
        }
        std::lock_guard<std::mutex> lock(tidLock_);
        threadIds_.insert(threadIds.begin(), threadIds.end());
    }

private:
    std::mutex tidLock_;
    std::vector<std::shared_ptr<MsprofApi>> apiTraces_;
    std::vector<std::shared_ptr<Adapter::FlipTask>> flipTasks_;
    std::set<uint32_t> threadIds_;
    std::string hostPath_;
    CANNWarehouses cannWarehouses_; // 所有threadId的数据
};

// 特化模板需要声明在类外
template<>
void EventGrouper::GroupEvents<ApiEventParser, MsprofApi, &CANNWarehouse::kernelEvents>(
    const std::string &typeName,
    EventType eventType);

template<>
void EventGrouper::GroupEvents<TaskTrackParser, MsprofCompactInfo, &CANNWarehouse::taskTrackEvents>(
    const std::string &typeName,
    EventType eventType);

} // namespace Cann
} // namespace Host
} // namespace Parser
} // namespace Analysis
#endif // ANALYSIS_PARSER_HOST_CANN_EVENT_GROUPER_H