/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_summary.cpp
 * Description        : npu 内存数据
 * Author             : msprof team
 * Creation Date      : 2025/05/09
 * *****************************************************************************
 */
#include "npu_memory_assembler.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

NpuMemoryAssembler::NpuMemoryAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "event", "ddr(KB)", "hbm(KB)", "memory(KB)", "timestamp(us)"};
}

uint8_t NpuMemoryAssembler::AssembleData(Analysis::Infra::DataInventory &dataInventory)
{
    auto npuMemData = dataInventory.GetPtr<std::vector<NpuMemData>>();
    if (npuMemData == nullptr) {
        WARN("No data to export npu memory summary");
        return DATA_NOT_EXIST;
    }
    for (const auto& item : *npuMemData) {
        // push data to res
        res_.emplace_back(std::vector<std::string>{
            std::to_string(item.deviceId),
            EVENT_MAP.find(item.event) != EVENT_MAP.end() ? EVENT_MAP.at(item.event) : UNKNOWN,
            std::to_string(item.ddr / KILOBYTE),
            std::to_string(item.hbm / KILOBYTE),
            std::to_string(item.memory / KILOBYTE),
            DivideByPowersOfTenWithPrecision(item.timestamp) + "\t"
        });
    }
    if (res_.empty()) {
        ERROR("Can't match any task data, failed to generate npu_mem_*.csv");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, OUTPUT_PATH, NPU_MEMORY_NAME + "_" + GetTimeStampStr()}),
                {});
    return ASSEMBLE_SUCCESS;
}
}
}