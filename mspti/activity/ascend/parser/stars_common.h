/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_common.h
 * Description        : 通过streamId标记位计算taskId
 * Author             : msprof team
 * Creation Date      : 2025/3/7
 * *****************************************************************************
 */

#ifndef MSPTI_PARSER_STARS_COMMON_H
#define MSPTI_PARSER_STARS_COMMON_H

#include <cstdint>

/*
 * 在2023年11月 task id底层存在规则变动，取用了stream id中某位作为标记位，基于标记位进行新的task id计算，具体计算规则为：
 * 1、判断stream id的第13位是否为0，为0则直接返回task id，否则
 * 2、取task id低13位，与stream id高3位，组成一个完整taskId
 *
 * 2024年7月 task id规则变动，新增stream id标记位，扩展新的task id计算规则，变更后如下：
 * 1、判断stream id的第13位是否为0
 * 2、不为0，则取task id低13位，与stream id高3位，组成一个完整taskId
 * 3、为0，则继续判断stream id第14位是否为0，是则直接返回task id；不为0 则交换stream id和task id的低12位
 */

namespace Mspti {
namespace Parser {
const uint16_t STREAM_LOW_OPERATOR = 1u << 11;
const uint16_t STREAM_JUDGE_BIT12_OPERATOR = 0x1000;
const uint16_t STREAM_JUDGE_BIT13_OPERATOR = 0x2000;
const uint16_t TASK_LOW_OPERATOR = 0x1FFF;
const uint16_t STREAM_HIGH_OPERATOR = 0xE000;
const uint16_t COMMON_LOW_OPERATOR = 0x0FFF;
const uint16_t COMMON_HIGH_OPERATOR = 0xF000;
class StarsCommon {
public:
static uint16_t GetStreamId(uint16_t streamId, uint16_t taskId)
{
    if ((streamId & STREAM_JUDGE_BIT12_OPERATOR) != 0) {
        return streamId % STREAM_LOW_OPERATOR;
    }
    if ((streamId & STREAM_JUDGE_BIT13_OPERATOR) != 0) {
        streamId = taskId & COMMON_LOW_OPERATOR;
    }
    auto res = streamId % STREAM_LOW_OPERATOR;
    return res;
}

static uint16_t GetTaskId(uint16_t streamId, uint16_t taskId)
{
    if ((streamId & STREAM_JUDGE_BIT12_OPERATOR) != 0) {
        taskId = taskId & TASK_LOW_OPERATOR;
        taskId |= (streamId & STREAM_HIGH_OPERATOR);
    } else if ((streamId & STREAM_JUDGE_BIT13_OPERATOR) != 0) {
        taskId = (streamId & COMMON_LOW_OPERATOR) | (taskId & COMMON_HIGH_OPERATOR);
    }
    return taskId;
}
};
}
}
#endif
