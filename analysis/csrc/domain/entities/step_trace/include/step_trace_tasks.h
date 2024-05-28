/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step.h
 * Description        : 有限状态机相关数据结构
 * Author             : msprof team
 * Creation Date      : 2024/5/8
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_STEP_TRACE_TASKS_H
#define MSPROF_ANALYSIS_STEP_TRACE_TASKS_H

#include <cstdint>
#include <vector>
#include <map>

namespace Analysis {
namespace Domain {
struct TimePair {
    uint64_t start = 0;
    uint64_t end = 0;
};

// 记录一个完整迭代内事件信息
struct StepTraceTasks {
    uint32_t indexId = 0;
    uint32_t iterId = 0;
    TimePair stepTrace;
    std::map<uint16_t, std::vector<TimePair>> allReduceTable;  // key 为 streamId
    std::map<uint16_t, std::vector<TimePair>> getNextTable; // key 为 streamId
    std::vector<TimePair> trainingTrace;
};

}
}

#endif // MSPROF_ANALYSIS_STEP_TRACE_TASKS_H
