/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_common.h
 * Description        : 通过streamId标记位计算taskId
 * Author             : msprof team
 * Creation Date      : 2024/5/28
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STARS_COMMON_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STARS_COMMON_H

#include <cstdint>

/*
 * 在2023年11月 task id底层存在规则变动，取用了stream id中某位作为标记位，基于标记位进行新的task id计算，具体计算规则为：
 * 1、判断stream id的第13位是否为0，为0则直接返回task id，否则
 * 2、取task id低13位，与stream id高3位，组成一个完整taskId
 */

namespace Analysis {
namespace Domain {
const uint16_t STREAM_LOW_OPERATOR = 1u << 11;
const uint16_t STREAM_JUDGE_OPERATOR = 0x1000;
const uint16_t TASK_LOW_OPERATOR = 0x1FFF;
const uint16_t STREAM_HIGH_OPERATOR = 0xE000;
class StarsCommon {
public:
static uint16_t GetStreamId(uint16_t originId)
{
    auto res = originId & STREAM_LOW_OPERATOR;
    return res;
}

static uint16_t GetTaskId(uint16_t streamId, uint16_t taskId)
{
    if ((streamId & STREAM_JUDGE_OPERATOR) != 0) {
        taskId = taskId & TASK_LOW_OPERATOR;
        taskId |= (streamId & STREAM_HIGH_OPERATOR);
    }
    return taskId;
}
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_PARSER_ITEM_STARS_COMMON_H
