/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event_grouper.cpp
 * Description        : EventGrouper模块：多线程解析bin, 生成Event插入到对应类型EventQueue
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */


#include "analysis/csrc/parser/host/cann/event_grouper.h"

using namespace Analysis::Utils;

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {

CANNWarehouses &EventGrouper::GetGroupEvents()
{
    return cannWarehouses_;
}

std::set<uint32_t> &EventGrouper::GetThreadIdSet()
{
    return threadIds_;
}

std::vector<std::shared_ptr<MsprofApi>> &EventGrouper::GetApiTraces()
{
    return apiTraces_;
}

std::vector<std::shared_ptr<Adapter::FlipTask>> &EventGrouper::GetFlipTasks()
{
    return flipTasks_;
}

bool EventGrouper::Group()
{
    Utils::TimeLogger t{"Group all events"};
    const uint32_t poolSize = 8;
    ThreadPool pool(poolSize);
    pool.Start();

    // 每个Parser一个线程
    pool.AddTask([this]() {
        GroupEvents<ApiEventParser, MsprofApi,
                    &CANNWarehouse::kernelEvents>("Kernel", EventType::EVENT_TYPE_API);
    });
    pool.AddTask([this]() {
        GroupEvents<GraphIdParser, MsprofAdditionalInfo,
                    &CANNWarehouse::graphIdMapEvents>("GraphIdMap", EventType::EVENT_TYPE_GRAPH_ID_MAP);
    });
    pool.AddTask([this]() {
        GroupEvents<FusionOpInfoParser, MsprofAdditionalInfo,
                    &CANNWarehouse::fusionOpInfoEvents>("FusionOpInfo", EventType::EVENT_TYPE_FUSION_OP_INFO);
    });
    pool.AddTask([this]() {
        GroupEvents<NodeBasicInfoParser, MsprofCompactInfo,
                    &CANNWarehouse::nodeBasicInfoEvents>("NodeBasicInfo", EventType::EVENT_TYPE_NODE_BASIC_INFO);
    });
    pool.AddTask([this]() {
        GroupEvents<TensorInfoParser, ConcatTensorInfo,
                    &CANNWarehouse::tensorInfoEvents>("TensorInfo", EventType::EVENT_TYPE_TENSOR_INFO);
    });
    pool.AddTask([this]() {
        GroupEvents<CtxIdParser, MsprofAdditionalInfo,
                    &CANNWarehouse::contextIdEvents>("ContextId", EventType::EVENT_TYPE_CONTEXT_ID);
    });
    pool.AddTask([this]() {
        GroupEvents<HcclInfoParser, MsprofAdditionalInfo,
                    &CANNWarehouse::hcclInfoEvents>("HcclInfo", EventType::EVENT_TYPE_HCCL_INFO);
    });
    pool.AddTask([this]() {
        GroupEvents<TaskTrackParser, MsprofCompactInfo,
                    &CANNWarehouse::taskTrackEvents>("TaskTrack", EventType::EVENT_TYPE_TASK_TRACK);
    });

    pool.WaitAllTasks();
    pool.Stop();
    return true;
}

bool EventGrouper::isKernelApiEvent(const std::shared_ptr<MsprofApi> &trace) const
{
    // 先排除异常条件下上报的非法api
    if (trace->beginTime <= 0 || trace->beginTime == trace->endTime) {
        ERROR("Invaild api event, threadId = %, level = %, begin = %, end = %",
              trace->threadId, trace->level, trace->beginTime, trace->endTime);
        return false;
    }
    // 再检查是否是Model Node HCCL 这三层的trace
    if (trace->level == MSPROF_REPORT_MODEL_LEVEL ||
        trace->level == MSPROF_REPORT_NODE_LEVEL ||
        trace->level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
        return true;
    }
    return false;
}

// 对于KernelEvents特化
template<>
void EventGrouper::GroupEvents<ApiEventParser, MsprofApi, &CANNWarehouse::kernelEvents>(
    const std::string &typeName, EventType eventType)
{
    // 1. 解析bin
    Utils::TimeLogger t{"Group " + typeName};
    auto parser = MakeShared<ApiEventParser>(hostPath_);
    if (!parser) {
        ERROR("ApiEventParser make shared ptr failed");
        return;
    }
    auto traces = parser->ParseData<MsprofApi>();
    apiTraces_ = traces; // 保存一份全量api

    // 统计各个threadId元素个数，用于EventQueue分配精确的内存大小，避免大量内存浪费
    std::unordered_map<uint32_t, uint64_t> threadIdNum;
    std::set<uint32_t> threadIds;
    for (const auto &trace: traces) {
        threadIds.insert(trace->threadId);
        if (isKernelApiEvent(trace)) {
            threadIdNum[trace->threadId] += 1;
        }
    }

    // 使用临时CANNWarehouses用于暂存kernel类型的数据，减小多线程竞争，提升性能
    CANNWarehouses kernelWarehouses;
    // 2. 转换生成Event，并将其添加到 CANNWarehouses
    for (const auto &trace: traces) {
        // 只处理符合条件的trace，提升性能
        if (!isKernelApiEvent(trace)) {
            continue;
        }
        EventInfo info{eventType, trace->level,
                       trace->beginTime, trace->endTime};
        auto event = MakeShared<Event>(trace, info);
        if (!event) {
            ERROR("ApiEvent make shared ptr failed");
            break;
        }
        // 新建
        if (!kernelWarehouses.FindAndInsertIfNotExist(trace->threadId)) {
            kernelWarehouses[trace->threadId].kernelEvents =
                MakeShared<EventQueue>(trace->threadId, threadIdNum[trace->threadId]);
        }
        // 插入
        kernelWarehouses[trace->threadId].kernelEvents->Push(event);
    }

    // 将暂存的kernelWarehouses更新到cannWarehouses_
    for (auto kv: threadIdNum) {
        cannWarehouses_[kv.first].kernelEvents = kernelWarehouses[kv.first].kernelEvents;
    }

    std::lock_guard<std::mutex> lock(tidLock_);
    threadIds_.insert(threadIds.begin(), threadIds.end());
}

// 对于taskTrackEvents特化
template<>
void EventGrouper::GroupEvents<TaskTrackParser, MsprofCompactInfo, &CANNWarehouse::taskTrackEvents>(
    const std::string &typeName, EventType eventType)
{
    // 1. 解析bin
    Utils::TimeLogger t{"Group " + typeName};
    auto parser = MakeShared<TaskTrackParser>(hostPath_);
    if (!parser) {
        ERROR("TaskTrackEvent make shared ptr failed");
        return;
    }
    auto traces = parser->ParseData<MsprofCompactInfo>();
    flipTasks_ = parser->ParseData<Adapter::FlipTask>();

    // 统计各个threadId元素个数，用于EventQueue分配精确的内存大小，避免大量内存浪费
    std::unordered_map<uint32_t, uint64_t> threadIdNum;
    std::set<uint32_t> threadIds;
    for (const auto &trace: traces) {
        threadIds.insert(trace->threadId);
        threadIdNum[trace->threadId] += 1;
    }

    for (const auto &trace: traces) {
        EventInfo info{eventType, trace->level, trace->timeStamp, trace->timeStamp};
        auto event = MakeShared<Event>(trace, info);
        if (!event) {
            ERROR("TaskTrackEvent make shared ptr failed");
            break;
        }
        // 新建
        if (!cannWarehouses_.FindAndInsertIfNotExist(trace->threadId) ||
            !(cannWarehouses_[trace->threadId].taskTrackEvents)) {
            cannWarehouses_[trace->threadId].taskTrackEvents =
                MakeShared<EventQueue>(trace->threadId, threadIdNum[trace->threadId]);
        }

        // 插入
        cannWarehouses_[trace->threadId].taskTrackEvents->Push(event);
    }
    std::lock_guard<std::mutex> lock(tidLock_);
    threadIds_.insert(threadIds.begin(), threadIds.end());
}

} // namespace Cann
} // namespace Host
} // namespace Parser
} // namespace Analysis