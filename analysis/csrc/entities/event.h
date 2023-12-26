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
#include <vector>
#include "prof_common.h"

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
    EVENT_TYPE_DUMMY, // 虚拟类型，用于建树时标志虚拟节点
    EVENT_TYPE_INVALID
};

// Event关键信息
struct EventInfo {
    EventInfo(EventType type, uint16_t level, uint64_t start, uint64_t end)
        : type(type), level(level), start(start), end(end)
    {}
    EventType type = EventType::EVENT_TYPE_INVALID; // 类型
    uint16_t level = 0;                             // 层级
    uint64_t start = 0;                             // 开始时间
    uint64_t end = 0;                               // 结束时间 对于addtional Event start == end
};

// Event为本工程对软硬件上报的各类信息(Trace)的抽象, 表示一个时间点或时间片发生的事件
struct Event {
    Event(std::shared_ptr<MsprofApi> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<MsprofAdditionalInfo> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<MsprofCompactInfo> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<ConcatTensorInfo> eventPtr, const EventInfo &eventInfo);
    union {
        std::shared_ptr<MsprofApi> apiPtr;
        std::shared_ptr<MsprofAdditionalInfo> additionPtr;
        std::shared_ptr<MsprofCompactInfo> compactPtr;
        std::shared_ptr<ConcatTensorInfo> tensorPtr;
    };
    EventInfo info;
    uint32_t id = 0;    // 全局唯一ID

    ~Event()
    {
        if (apiPtr) {
            apiPtr.~shared_ptr();
        } else if (additionPtr) {
            additionPtr.~shared_ptr();
        } else if (compactPtr) {
            compactPtr.~shared_ptr();
        } else if (tensorPtr) {
            tensorPtr.~shared_ptr();
        }
    }
};

} // namespace Entities
} // namespace Analysis

#endif // ANALYSIS_ENTITIES_EVENT_H