/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ffts_profile_parser.h
 * Description        : ffts_profile类型二进制数据解析
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PMU_FFTS_PROFILE_PARSER_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PMU_FFTS_PROFILE_PARSER_H

#include <vector>
#include "analysis/csrc/domain/services/parser/parser.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"

namespace Analysis {

namespace Domain {

class FftsProfileParser : public Parser {
private:
    uint32_t ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data);

    std::vector<std::string> GetFilePattern() override;

    uint32_t GetTrunkSize() override;

    uint32_t ParseData(Infra::DataInventory &dataInventory) override;

    uint32_t GetNoFileCode() override;

private:
    std::vector<HalPmuData> halUniData_;
    std::vector<std::string> filePrefix_{"ffts_profile"};
    int cnt_{-1};
};

}
}
#endif // ANALYSIS_DOMAIN_SERVICE_PARSER_PMU_FFTS_PROFILE_PARSER_H
