/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_parser_item.h
 * Description        : 解析acc pmu数据
 * Author             : msprof team
 * Creation Date      : 2024/6/3
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/parser_item/acc_pmu_parser_item.h"
#include "analysis/csrc/dfx/log.h"
#include "securec.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

int AccPmuParserItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData)
{
    if (binaryDataSize != sizeof(AccPmu)) {
        ERROR("The trunkSize of accPmu is not equal with the AccPmu struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *accPmu = ReinterpretConvert<AccPmu *>(binaryData);
    auto *unionData = ReinterpretConvert<HalLogData *>(halUniData);
    unionData->type = ACC_PMU;
    unionData->accPmu.timestamp = accPmu->timestamp;
    unionData->accPmu.accId = accPmu->accId;
    unionData->hd.timestamp = accPmu->timestamp;
    errno_t resBandwidth = memcpy_s(unionData->accPmu.bandwidth, sizeof(unionData->accPmu.bandwidth),
                                    accPmu->bandwidth, sizeof(accPmu->bandwidth));
    errno_t resOst = memcpy_s(unionData->accPmu.ost, sizeof(unionData->accPmu.ost),
                              accPmu->ost, sizeof(accPmu->ost));
    if (resBandwidth != EOK || resOst != EOK) {
        ERROR("memcpy accPmu failed! accId is %", accPmu->accId);
    }
    return accPmu->cnt;
}

REGISTER_PARSER_ITEM(LOG_PARSER, PARSER_ITEM_ACC_PMU, AccPmuParserItem);
}
}
