/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_module_mem_summary.cpp
 * Description        : npu 内存数据
 * Author             : msprof team
 * Creation Date      : 2025/05/09
 * *****************************************************************************
 */
#include "npu_module_mem_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"

namespace Analysis {
namespace Application {

using namespace Analysis::Domain;
using namespace Analysis::Utils;

// index
const std::vector<std::string> NPU_MODULE_MEM_HEADERS = {
    "Device_id", "Component", "Timestamp(us)", "Total Reserved(KB)", "Device"};

NpuModuleMemAssembler::NpuModuleMemAssembler(const std::string &name, const std::string &profPath)
    :SummaryAssembler(name, profPath)
{
    // reserve the module map
    for (const auto& pair : MODULE_NAME_TABLE) {
        moduleMap_[pair.second] = pair.first;
    }
}

uint8_t NpuModuleMemAssembler::AssembleData(DataInventory &dataInventory)
{
    auto npuModuleMemData = dataInventory.GetPtr<std::vector<NpuModuleMemData>>();
    // set headers
    headers_ = NPU_MODULE_MEM_HEADERS;
    if (npuModuleMemData == nullptr) {
        WARN("No data to export npu module memory summary");
        return DATA_NOT_EXIST;
    }
    // sort
    std::sort(npuModuleMemData->begin(), npuModuleMemData->end(),
        [](const NpuModuleMemData &a, const NpuModuleMemData &b) {
            if (a.moduleId == b.moduleId) {
                return a.timestamp < b.timestamp;
            }
            return a.moduleId < b.moduleId;
        });
    for (auto& item :*npuModuleMemData) {
        // push data to res
        res_.emplace_back(std::vector<std::string>{std::to_string(item.deviceId),
            moduleMap_.find(item.moduleId) != moduleMap_.end() ? moduleMap_.at(item.moduleId) : UNKNOWN,
            DivideByPowersOfTenWithPrecision(item.timestamp) + "\t",
            std::to_string(item.totalReserved / KILOBYTE),
            item.deviceType});
    }
    if (res_.empty()) {
        ERROR("Can't match any task data, failed to generate npu_module_mem_*.csv");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, OUTPUT_PATH, NPU_MODULE_MEMORY_NAME + "_" + GetTimeStampStr()}),
                {});
    return ASSEMBLE_SUCCESS;
}
}
}