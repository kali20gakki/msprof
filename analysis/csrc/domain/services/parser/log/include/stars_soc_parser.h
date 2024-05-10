/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_soc_parser.h
 * Description        : 解析star_soc.data数据
 * Author             : msprof team
 * Creation Date      : 2024/4/25
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_LOG_STARS_SOC_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_LOG_STARS_SOC_PARSER_H

#include <vector>
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/parser/parser.h"

namespace Analysis {
namespace Domain {
class StarsSocParser : public Parser {
private:
    uint32_t ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data);
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory &dataInventory) override;
    uint32_t GetNoFileCode() override;
private:
    std::vector<HalLogData> halUniData_;
    std::vector<std::string> filePrefix_{"stars_soc."};
    int cnt_{-1};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_LOG_STARS_SOC_PARSER_H
