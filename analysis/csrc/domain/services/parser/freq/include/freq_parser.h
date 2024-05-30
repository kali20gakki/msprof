/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_parser.h
 * Description        : freq_lpm类型二进制数据解析头文件
 * Author             : msprof team
 * Creation Date      : 2024/5/15
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_FREQ_FREQ_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_FREQ_FREQ_PARSER_H

#include <vector>
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/domain/services/parser/parser.h"

namespace Analysis {
namespace Domain {
class FreqParser : public Parser {
private:
    uint32_t ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data);
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
    uint32_t GetNoFileCode() override;
private:
    std::vector<HalFreqData> halUniData_;
    std::vector<std::string> filePrefix_{"lpmFreqConv."};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_FREQ_FREQ_PARSER_H
