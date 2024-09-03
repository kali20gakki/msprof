/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_soc_assembler.h
 * Description        : 组合Stars Soc Info层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_STARS_SOC_ASSEMBLER_H
#define ANALYSIS_APPLICATION_STARS_SOC_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis {
namespace Application {
class StarsSocAssembler : public JsonAssembler {
public:
    StarsSocAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};
}
}
#endif // ANALYSIS_APPLICATION_STARS_SOC_ASSEMBLER_H
