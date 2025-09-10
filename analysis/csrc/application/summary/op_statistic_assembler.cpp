/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : op_statistic_assembler.cpp
 * Description        : 组合op_statistic数据
 * Author             : msprof team
 * Creation Date      : 2025/5/30
 * *****************************************************************************
 */

#include "analysis/csrc/application/summary/op_statistic_assembler.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Utils;
using Context = Analysis::Domain::Environment::Context;

OpStatisticAssembler::OpStatisticAssembler(const std::string& name, const std::string& profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "OP Type", "Core Type", "Count", "Total Time(us)",
        "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"};
}
void OpStatisticAssembler::GenerateOpStatistic(std::vector<OpStatisticData>& opStatisticData)
{
    // 按totalTime降序排序
    std::sort(opStatisticData.begin(), opStatisticData.end(),
              [](const OpStatisticData& a, const OpStatisticData& b) {
                  return a.totalTime > b.totalTime; // 降序排序
              });
    for (const auto& data : opStatisticData) {
        std::vector<std::string> row = {
            std::to_string(data.deviceId),
            data.opType,
            data.coreType,
            data.count,
            DoubleToStr(data.totalTime),
            DoubleToStr(data.min),
            DoubleToStr(data.avg),
            DoubleToStr(data.max),
            DoubleToStr(data.ratio)
        };
        res_.emplace_back(row);
    }
}

uint8_t OpStatisticAssembler::AssembleData(DataInventory& dataInventory)
{
    auto commStatisticData = dataInventory.GetPtr<std::vector<OpStatisticData>>();
    auto isLevel0 = Context::GetInstance().IsLevel0(profPath_);
    if (commStatisticData == nullptr || isLevel0) {
        WARN("Op Statistic data not exist.");
        return DATA_NOT_EXIST;
    }
    GenerateOpStatistic(*commStatisticData);
    if (res_.empty()) {
        ERROR("Can't Generate any Op statistic process data");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, OUTPUT_PATH, OP_STATISTIC_NAME}), {});
    return ASSEMBLE_SUCCESS;
}
}
}