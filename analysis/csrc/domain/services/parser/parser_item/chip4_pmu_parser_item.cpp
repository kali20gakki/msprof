/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip4_pmu_parser_item.cpp
 * Description        : context级别PMU数据从二进制转换成抽象结构
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/chip4_pmu_parser_item.h"
#include "securec.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/domain/services/parser/pmu/pmu_accelerator_utils.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

int Chip4PmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(ContextPmu)) {
        ERROR("The TrunkSize of PMU is not equal with the ContextPmu struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *contextPmu = ReinterpretConvert<ContextPmu *>(binaryData);
    auto *pmuData = ReinterpretConvert<HalPmuData *>(halUniData);

    pmuData->hd.taskId.streamId = StarsCommon::GetStreamId(contextPmu->streamId);
    pmuData->hd.taskId.batchId = INVALID_BATCH_ID;
    pmuData->hd.taskId.taskId = StarsCommon::GetTaskId(contextPmu->streamId, contextPmu->taskId);
    pmuData->hd.taskId.contextId = contextPmu->subTaskId;
    pmuData->hd.timestamp = contextPmu->timeList[1];   // 此处使用的是context级别PMU数据的结束时间
    pmuData->type = PMU;
    pmuData->pmu.acceleratorType = GetAcceleratorTypeByType(contextPmu->subTaskType, contextPmu->fftsType);
    pmuData->pmu.ovFlag = contextPmu->ovFlag;
    pmuData->pmu.totalCycle = contextPmu->totalCycle;
    errno_t resPmu = memcpy_s(pmuData->pmu.pmuList, sizeof(pmuData->pmu.pmuList),
                              contextPmu->pmuList, sizeof(contextPmu->pmuList));
    errno_t resTime = memcpy_s(pmuData->pmu.timeList, sizeof(pmuData->pmu.timeList),
                               contextPmu->timeList, sizeof(contextPmu->timeList));
    if (resPmu != EOK || resTime != EOK) {
        ERROR("memcpy blockPmu failed! taskId is %, streamId is %, contextId is", contextPmu->taskId,
              contextPmu->streamId, contextPmu->subTaskId);
    }
    return contextPmu->cnt;
}
}
}