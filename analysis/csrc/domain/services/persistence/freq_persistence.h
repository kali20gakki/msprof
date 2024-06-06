/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_persistence.h
 * Description        : freq数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/6/4
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_FREQ_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_FREQ_PERSISTENCE_H

#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/services/parser/freq/include/freq_parser.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint32_t>>;
class FreqPersistence : public Process {
private:
    ProcessedDataFormat GenerateFreqData(std::vector<HalFreqLpmData>& freqData);
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_FREQ_PERSISTENCE_H
