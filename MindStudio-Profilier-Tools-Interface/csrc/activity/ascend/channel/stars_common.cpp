/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_common.cpp
 * Description        : 通过streamId标记位计算taskId
 * Author             : msprof team
 * Creation Date      : 2025/3/7
 * *****************************************************************************
 */

#include "stars_common.h"

namespace Mspti {
namespace Convert {

bool StarsCommon::isExpand = false;

void StarsCommon::SetStreamExpandStatus(uint8_t expandStatus)
{
    isExpand = (expandStatus == 1);
}

uint16_t StarsCommon::GetStreamId(uint16_t streamId, uint16_t taskId)
{
    if (isExpand) {
        if ((streamId & STREAM_JUDGE_BIT15_OPERATOR) != 0) {
            return taskId & EXPANDING_LOW_OPERATOR;
        } else {
            return streamId & EXPANDING_LOW_OPERATOR;
        }
    } else {
        if ((streamId & STREAM_JUDGE_BIT12_OPERATOR) != 0) {
            return streamId % STREAM_LOW_OPERATOR;
        }
        if ((streamId & STREAM_JUDGE_BIT13_OPERATOR) != 0) {
            streamId = taskId & COMMON_LOW_OPERATOR;
        }
        return streamId % STREAM_LOW_OPERATOR;
    }
}

uint16_t StarsCommon::GetTaskId(uint16_t streamId, uint16_t taskId)
{
    if (isExpand) {
        if ((streamId & STREAM_JUDGE_BIT15_OPERATOR) != 0) {
            return (taskId & STREAM_JUDGE_BIT15_OPERATOR) | (streamId & EXPANDING_LOW_OPERATOR);
        } else {
            return taskId;
        }
    } else {
        if ((streamId & STREAM_JUDGE_BIT12_OPERATOR) != 0) {
            taskId = taskId & TASK_LOW_OPERATOR;
            taskId |= (streamId & STREAM_HIGH_OPERATOR);
        } else if ((streamId & STREAM_JUDGE_BIT13_OPERATOR) != 0) {
            taskId = (streamId & COMMON_LOW_OPERATOR) | (taskId & COMMON_HIGH_OPERATOR);
        }
        return taskId;
    }
}
} // namespace Parser
} // namespace Mspti