/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_assembler.h
 * Description        : 组合aicore freq层数据
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */
#ifndef ANALYSIS_APPLICATION_AICORE_FREQ_ASSEMBLER_H
#define ANALYSIS_APPLICATION_AICORE_FREQ_ASSEMBLER_H

#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/aicore_freq_data.h"

namespace Analysis {
namespace Application {
class AicoreFreqAssembler : public JsonAssembler {
public:
    AicoreFreqAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    std::unordered_map<uint16_t, uint32_t> GenerateFreqTrace(std::vector<AicoreFreqData> &freqData,
                                                             uint32_t sortIndex, const std::string &profPath);
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};
}
}
#endif // ANALYSIS_APPLICATION_AICORE_FREQ_ASSEMBLER_H
