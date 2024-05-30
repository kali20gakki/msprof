/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_parser_item.cpp
 * Description        : step_trace类型二进制数据解析
 * Author             : msprof team
 * Creation Date      : 2024/5/8
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/step_trace_parser_item.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

namespace Analysis {
namespace Domain {
using namespace Utils;

int StepTraceParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(StepTrace)) {
        ERROR("StepTraceParseItem failure, struct is StepTrace");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<StepTrace *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);

    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;

    uni->type = STEP_TRACE;

    uni->stepTrace.timestamp = bin->timestamp;
    uni->stepTrace.indexId = bin->indexId;
    uni->stepTrace.modelId = bin->modelId;
    uni->stepTrace.tagId = bin->tagId;
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(TRACK_PARSER, PARSER_ITEM_STEP_TRACE, StepTraceParseItem);
}
}