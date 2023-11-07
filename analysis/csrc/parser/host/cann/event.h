/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event.h
 * Description        : Event模块：定义Event数据结构
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_EVENT_H
#define ANALYSIS_PARSER_HOST_CANN_EVENT_H

#include <cstdint>
#include <string>
#include <memory>
#include <utility>
#include <atomic>

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {

/*
 Profiling在CANN软件栈分为ACL、Model、Node、Hccl、Runtime 5层
------------------------- Pytorch -------------------------------
|-
-------------------------- PTA ----------------------------------
|-
------------------------- CANN ---------------------------------
|- ACL Level
|- Model Level [graph_id_map, fusion_op_info]
|- Node Level [node_basic_info, tensor_info, context_id]
|- HCCL Level [hccl_info, context_id]
|- Runtime Level [task_track, mem_cpy]
-------------------------- NPU ---------------------------------

 Event类型：api, event, node_basic_info, tensor_info, hccl_info,
context_id, graph_id_map, fusion_op_info, task_track, mem_cpy

 */

enum class EventType {
    EVENT_TYPE_API = 0,
    EVENT_TYPE_EVENT,  // 两个EVENT_TYPE_EVENT可以拼出一个EVENT_TYPE_API
    EVENT_TYPE_NODE_BASIC_INFO,
    EVENT_TYPE_TENSOR_INFO,
    EVENT_TYPE_HCCL_INFO,
    EVENT_TYPE_CONTEXT_ID,
    EVENT_TYPE_GRAPH_ID_MAP,
    EVENT_TYPE_FUSION_OP_INFO,
    EVENT_TYPE_TASK_TRACK,
    EVENT_TYPE_MEM_CPY,
    EVENT_TYPE_INVALID
};

// 单例模式实现EventID生成，保证ID唯一
class EventIDGenerator {
public:
    static EventIDGenerator &GetInstance()
    {
        static EventIDGenerator instance;
        return instance;
    }

    uint32_t GetID()
    {
        return ++idCnt;
    }

private:
    EventIDGenerator() = default;
    ~EventIDGenerator() = default;
    EventIDGenerator(const EventIDGenerator &) = delete;
    EventIDGenerator &operator=(const EventIDGenerator &) = delete;
    std::atomic<uint32_t> idCnt{0};
};

// Event关键信息
struct EventInfo {
    EventType type = EventType::EVENT_TYPE_INVALID; // 类型
    uint16_t level = 0;                             // 层级
    uint64_t start = 0;                             // 开始时间
    uint64_t end = 0;                               // 结束时间 对于addtional Event start == end
};

// Event为本工程对硬件上报的各类信息(Trace)的抽象, 表示一个时间点或时间片发生的事件
class Event {
public:
    Event(std::shared_ptr<char> eventPtr, const std::string &eventDesc, EventInfo &eventInfo)
        : event_(eventPtr), desc_(eventDesc), info_(eventInfo), id_(EventIDGenerator::GetInstance().GetID())
    {}

private:
    uint32_t id_ = 0;    // 全局唯一ID
    EventInfo info_;
    std::string desc_;
    std::shared_ptr<char> event_;
};

} // namespace Cann
} // namespace Host
} // namespace Parser
} // namespace Analysis

#endif // ANALYSIS_PARSER_HOST_CANN_EVENT_H
