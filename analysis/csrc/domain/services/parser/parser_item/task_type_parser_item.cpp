/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_type_parser_item.cpp
 * Description        : taskType数据类型解析
 * Author             : msprof team
 * Creation Date      : 24-4-29 下午3:49
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/task_type_parser_item.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"

namespace Analysis {
namespace Domain {
using namespace Utils;

const int DEFAULT_CNT = -1;

int TaskTypeParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(TaskType)) {
        ERROR("TaskTypeParseItem failure, struct is TaskFlip");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<TaskType *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);

    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;

    uni->type = TS_TASK_TYPE;

    uni->taskType.taskType = bin->taskType;
    uni->taskType.taskStatus = bin->taskStatus;
    return DEFAULT_CNT;
}
}
}