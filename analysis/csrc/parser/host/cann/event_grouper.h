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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <set>

#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/entities/event_queue.h"
#include "analysis/csrc/parser/host/cann/addition_info_parser.h"
#include "analysis/csrc/parser/host/cann/api_event_parser.h"
#include "analysis/csrc/parser/host/cann/cann_warehouse.h"
#include "analysis/csrc/parser/host/cann/compact_info_parser.h"
#include "collector/inc/toolchain/prof_common.h"
#include "analysis/csrc/utils/safe_unordered_map.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/utils/time_logger.h"
#include "analysis/csrc/utils/utils.h"

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
    std::vector<std::shared_ptr<Event>> &GetApiTraces();
    // 获取FlipTask数据
    std::vector<std::shared_ptr<Adapter::FlipTask>> &GetFlipTasks();
private:
    bool isKernelApiEvent(const std::shared_ptr<MsprofApi> &trace);
    void InitLastKernelTimes(const std::set<uint32_t> &threadIds);
    void RecordCANNWareHouses();

    template<typename P, typename M, std::shared_ptr<EventQueue> CANNWarehouse::*element>
    void GroupEvents(const std::string &typeName, EventType eventType)
    {
        // 1. 解析bin
        Utils::TimeLogger t{"Group " + typeName};
        std::shared_ptr<P> parser;
        MAKE_SHARED_RETURN_VOID(parser, P, hostPath_);
        auto traces = parser->template ParseData<M>();
        SortByTimeAndLevel(traces);

        // 统计各个threadId元素个数，用于EventQueue分配精确的内存大小，避免大量内存浪费
        std::unordered_map<uint32_t, uint64_t> threadIdNum;
        std::set<uint32_t> threadIds;
        for (const auto &trace: traces) {
            threadIds.insert(trace->threadId);
            threadIdNum[trace->threadId] += 1;
        }

        for (const auto &trace: traces) {
            EventInfo info{eventType, trace->level, trace->timeStamp, trace->timeStamp};
            std::shared_ptr<Event> event;
            MAKE_SHARED_BREAK(event, Event, trace, info);
            // 新建
            if (!cannWarehouses_.FindAndInsertIfNotExist(trace->threadId) ||
                !(cannWarehouses_[trace->threadId].*element)) {
                std::shared_ptr<EventQueue> que;
                MAKE_SHARED_BREAK(que, EventQueue, trace->threadId, threadIdNum[trace->threadId]);
                cannWarehouses_[trace->threadId].*element = que;
            }

            // 插入
            (cannWarehouses_[trace->threadId].*element)->Push(event);
        }
        std::lock_guard<std::mutex> lock(tidLock_);
        threadIds_.insert(threadIds.begin(), threadIds.end());
    }

    template<typename T>
    bool SortByTimeAndLevel(std::vector<std::shared_ptr<T>> &traces)
    {
        auto comp =
            [](std::shared_ptr<T> &api1, std::shared_ptr<T> &api2) {
                return api1->timeStamp < api2->timeStamp ||
                    (api1->timeStamp == api2->timeStamp && api1->level > api2->level);
            };
        std::sort(traces.begin(), traces.end(), comp);
    }

private:
    std::mutex tidLock_;
    std::vector<std::shared_ptr<Event>> apiTraces_;
    std::vector<std::shared_ptr<Adapter::FlipTask>> flipTasks_;
    std::set<uint32_t> threadIds_;
    std::string hostPath_;
    CANNWarehouses cannWarehouses_; // 所有threadId的数据
    // 记录已经处理好的kernelEvents的最晚时间（threadId, level, time）
    std::unordered_map<uint32_t, std::unordered_map<uint16_t, std::pair<uint64_t, uint64_t>>> lastKernelTimes_;
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

template<>
bool EventGrouper::SortByTimeAndLevel<MsprofApi>(std::vector<std::shared_ptr<MsprofApi>> &traces);

} // namespace Cann
} // namespace Host
} // namespace Parser
} // namespace Analysis
#endif // ANALYSIS_PARSER_HOST_CANN_EVENT_GROUPER_H