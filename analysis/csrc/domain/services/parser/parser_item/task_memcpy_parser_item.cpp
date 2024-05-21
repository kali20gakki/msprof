/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_flip_parser_item.cpp
 * Description        : taskFlip解析数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/28
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/task_memcpy_parser_item.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"

namespace Analysis {
namespace Domain {
using namespace Utils;

const int DEFAULT_CNT = -1;

int TaskMemcpyParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(TaskMemcpy)) {
        ERROR("TaskMemcpyParseItem failure, struct is TaskFlip");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *bin = ReinterpretConvert<TaskMemcpy *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);

    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;

    uni->type = TS_MEMCPY;

    uni->taskMemcpy.taskStatus = bin->taskStatus;
    return DEFAULT_CNT;
}
}
}
