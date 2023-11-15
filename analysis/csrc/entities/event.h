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

#ifndef ANALYSIS_ENTITIES_EVENT_H
#define ANALYSIS_ENTITIES_EVENT_H

#include <string>
#include <memory>
#include <atomic>

namespace Analysis {
namespace Entities {

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

const uint16_t EVENT_LEVEL_ACL = 20000;
const uint16_t EVENT_LEVEL_MODEL = 15000;
const uint16_t EVENT_LEVEL_NODE = 10000;
const uint16_t EVENT_LEVEL_HCCL = 5500;
const uint16_t EVENT_LEVEL_RUNTIME = 5000;

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

// Event关键信息
struct EventInfo {
    EventType type = EventType::EVENT_TYPE_INVALID; // 类型
    uint16_t level = 0;                             // 层级
    uint64_t start = 0;                             // 开始时间
    uint64_t end = 0;                               // 结束时间 对于addtional Event start == end
};

// Event为本工程对软硬件上报的各类信息(Trace)的抽象, 表示一个时间点或时间片发生的事件
struct Event {
    Event(std::shared_ptr<char> eventPtr, std::string eventDesc, EventInfo &eventInfo);
    
    std::shared_ptr<char> event;
    std::string desc;
    EventInfo info;
    uint32_t id = 0;    // 全局唯一ID
};

} // namespace Entities
} // namespace Analysis

#endif // ANALYSIS_ENTITIES_EVENT_H