/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_statistic_assembler.cpp
 * Description        : 组合communication_statistic数据
 * Author             : msprof team
 * Creation Date      : 2025/5/24
 * *****************************************************************************
 */

#include "analysis/csrc/application/summary/comm_statistic_assembler.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Utils;

CommStatisticAssembler::CommStatisticAssembler(const std::string& name, const std::string& profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "OP Type", "Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)",
        "Max Time(us)", "Ratio(%)"};
}

void CommStatisticAssembler::GenerateCommStatistic(std::vector<HcclStatisticData>& commStatisticData)
{
    // 按totalTime降序排序
    std::sort(commStatisticData.begin(), commStatisticData.end(),
              [](const HcclStatisticData& a, const HcclStatisticData& b) {
                  return a.totalTime > b.totalTime; // 降序排序
              });
    for (const auto& data : commStatisticData) {
        std::vector<std::string> row = {
            std::to_string(data.deviceId),
            data.opType,
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

uint8_t CommStatisticAssembler::AssembleData(DataInventory& dataInventory)
{
    auto commStatisticData = dataInventory.GetPtr<std::vector<HcclStatisticData>>();
    if (commStatisticData == nullptr) {
        WARN("Communication Statistic data not exist.");
        return DATA_NOT_EXIST;
    }
    GenerateCommStatistic(*commStatisticData);
    if (res_.empty()) {
        ERROR("Can't Generate any communication statistic process data");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, OUTPUT_PATH, COMM_STATISTIC_NAME}), {});
    return ASSEMBLE_SUCCESS;
}
}
}