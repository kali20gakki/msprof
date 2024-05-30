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

#include "analysis/csrc/domain/services/parser/parser_item/task_flip_parser_item.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

namespace Analysis {
namespace Domain {
using namespace Utils;

int TaskFlipParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(TaskFlip)) {
        ERROR("TaskFlipParseItem failure, struct is TaskFlip");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<TaskFlip *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);

    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;

    uni->type = TS_TASK_FLIP;

    uni->flip.timestamp = bin->timestamp;
    uni->flip.flipNum = bin->flipNum;

    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(TRACK_PARSER, PARSER_ITEM_TASK_FLIP, TaskFlipParseItem);
}
}