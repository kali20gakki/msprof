/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ffts_log_parser_item.h
 * Description        : 解析ffts log数据
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/ffts_plus_log_parser_item.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

int FftsPlusLogParseItem(uint8_t *binaryData, uint32_t binaryDataSize,
                         uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(FftsPlusLog)) {
        ERROR("binaryDataSize is not equal to the size of binaryData");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *log = ReinterpretConvert<FftsPlusLog *>(binaryData);

    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);

    unionData->hd.taskId.streamId = StarsCommon::GetStreamId(log->streamId);
    unionData->hd.taskId.batchId = 0;
    unionData->hd.taskId.taskId = StarsCommon::GetTaskId(log->streamId, log->taskId);
    unionData->hd.taskId.contextId = log->subTaskId;

    unionData->type = FFTS_LOG;

    if (log->funcType == PARSER_ITEM_FFTS_PLUS_LOG_START) {
        unionData->ffts.isEndTimestamp = false;
        unionData->hd.timestamp = log->timestamp;
    } else {
        unionData->ffts.isEndTimestamp = true;
        unionData->hd.timestamp = INVALID_TIMESTAMP;
    }
    unionData->ffts.timestamp = log->timestamp;

    unionData->ffts.fftsType = log->fftsType;
    unionData->ffts.subTaskType = log->subTaskType;
    unionData->ffts.threadId = log->threadId;
    return log->cnt;
}

REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_FFTS_PLUS_LOG_START, FftsPlusLogParseItem);
REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_FFTS_PLUS_LOG_END, FftsPlusLogParseItem);
}
}