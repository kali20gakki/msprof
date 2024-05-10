/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_soc_parser.h
 * Description        : 解析acsq task数据
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/acsq_log_parser_item.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

int AcsqLogParseItem(uint8_t *binaryData, uint32_t binaryDataSize,
                     uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(AcsqLog)) {
        ERROR("binaryDataSize is not equal to the size of binaryData");
        return ERROR_SIZE_MISMATCH;
    }
    auto *log = ReinterpretConvert<AcsqLog *>(binaryData);

    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);

    unionData->hd.taskId.streamId = log->streamId;
    unionData->hd.taskId.batchId = 0;
    unionData->hd.taskId.taskId = log->taskId;
    unionData->hd.taskId.contextId = INVALID_CONTEXT_ID;

    unionData->type = ACSQ_LOG;

    if (log->funcType == PARSER_ITEM_ACSQ_LOG_START) {
        unionData->acsq.isEndTimestamp = false;
        unionData->hd.timestamp = log->timestamp;
    } else {
        unionData->acsq.isEndTimestamp = true;
        unionData->hd.timestamp = INVALID_TIMESTAMP;
    }
    unionData->acsq.taskType = log->taskType;
    unionData->acsq.timestamp = log->timestamp;
    return log->cnt;
}

}
}