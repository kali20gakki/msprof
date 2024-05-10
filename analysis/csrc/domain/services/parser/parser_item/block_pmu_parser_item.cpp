/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : block_pmu_parser_item.cpp
 * Description        : block级别PMU数据从二进制转换成抽象结构
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/block_pmu_parser_item.h"
#include <iostream>
#include "securec.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/domain/services/parser/pmu/pmu_accelerator_utils.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

int BlockPmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(BlockPmu)) {
        ERROR("The TrunkSize of PMU is not equal with the ContextPmu struct");
        return ERROR_SIZE_MISMATCH;
    }

    auto *blockPmu = ReinterpretConvert<BlockPmu *>(binaryData);
    auto *pmuData = ReinterpretConvert<HalPmuData *>(halUniData);

    pmuData->hd.taskId.streamId = blockPmu->streamId;
    pmuData->hd.taskId.batchId = INVALID_BATCH_ID;
    pmuData->hd.taskId.taskId = blockPmu->taskId;
    pmuData->hd.taskId.contextId = blockPmu->subTaskId;
    pmuData->hd.timestamp = blockPmu->timeList[0];

    pmuData->type = BLOCK_PMU;
    pmuData->pmu.acceleratorType = GetAcceleratorTypeByType(blockPmu->subTaskType, blockPmu->fftsType);
    pmuData->pmu.subBlockId = blockPmu->subBlockId;
    pmuData->pmu.blockId = blockPmu->blockId;
    pmuData->pmu.totalCycle = blockPmu->totalCycle;
    pmuData->pmu.coreType = blockPmu->coreType;
    pmuData->pmu.coreId = blockPmu->coreId;
    errno_t resPmu =  memcpy_s(pmuData->pmu.pmuList, sizeof(pmuData->pmu.pmuList),
                               blockPmu->pmuList, sizeof(blockPmu->pmuList));
    errno_t resTime = memcpy_s(pmuData->pmu.timeList, sizeof(pmuData->pmu.timeList),
                               blockPmu->timeList, sizeof(blockPmu->timeList));
    if (resPmu != EOK || resTime != EOK) {
        ERROR("memcpy blockPmu failed! taskId is %, streamId is %, contextId is", blockPmu->taskId,
              blockPmu->streamId, blockPmu->subTaskId);
    }
    return blockPmu->cnt;
}
}
}