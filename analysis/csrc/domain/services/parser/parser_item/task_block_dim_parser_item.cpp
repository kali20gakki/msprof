/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_block_dim_parser_item.cpp
 * Description        : Task BlockDim类型二进制数据解析
 * Author             : msprof team
 * Creation Date      : 2024/8/3
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/task_block_dim_parser_item.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

namespace Analysis {
namespace Domain {
using namespace Utils;

int BlockDimParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(BlockDim)) {
        ERROR("BlockDimParseItem failure, struct is BlockDim");
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto *bin = ReinterpretConvert<BlockDim *>(binaryData);
    auto *uni = ReinterpretConvert<HalTrackData *>(halUniData);
    if (bin == nullptr || uni == nullptr) {
        ERROR("Struct of block dim reinterpret convert failed.");
        return DEFAULT_CNT;
    }

    uni->hd.taskId.streamId = bin->streamId;
    uni->hd.taskId.batchId = INVALID_BATCH_ID;
    uni->hd.taskId.taskId = bin->taskId;
    uni->hd.taskId.contextId = INVALID_CONTEXT_ID;
    uni->hd.timestamp = bin->timestamp;

    uni->type = BLOCK_DIM;

    uni->blockDim.timestamp = bin->timestamp;
    uni->blockDim.blockDim = bin->blockDim;
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(TRACK_PARSER, PARSER_ITEM_BLOCK_DIM, BlockDimParseItem);
}
}