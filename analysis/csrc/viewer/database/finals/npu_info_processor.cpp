/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : target_info_npu_processor.cpp
 * Description        : 落盘不同device对应芯片型号数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/npu_info_processor.h"

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;
namespace {
    const std::unordered_map<uint16_t, std::string> CHIP_TABLE = {
        {0, "Ascend310"},
        {1, "Ascend910A"},
        {4, "Ascend310P"},
        {5, "Ascend910B"},
        {7, "Ascend310B"},
    };
}

NpuInfoProcessor::NpuInfoProcessor(const std::string &reportDBPath,
                                               const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool NpuInfoProcessor::Run()
{
    INFO("NpuInfoProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, TABLE_NAME_NPU_INFO);
    return flag;
}

bool NpuInfoProcessor::Process(const std::string &fileDir)
{
    INFO("NpuInfoProcessor Process, dir is %", fileDir);
    auto deviceDirs = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    NpuInfoDataFormat npuInfoData;
    for (const auto& deviceDir : deviceDirs) {
        UpdateNpuData(fileDir, deviceDir, npuInfoData);
    }
    return SaveData(npuInfoData, TABLE_NAME_NPU_INFO);
}

void NpuInfoProcessor::UpdateNpuData(const std::string &fileDir, const std::string &deviceDir,
                                           NpuInfoDataFormat &npuInfoData)
{
    INFO("NpuInfoProcessor UpdateNpuData, dir is %", fileDir);
    uint16_t deviceId = Utils::GetDeviceIdByDevicePath(deviceDir);
    uint16_t chip = Context::GetInstance().GetPlatformVersion(deviceId, fileDir);
    std::string chipName;
    auto it = CHIP_TABLE.find(chip);
    if (it == CHIP_TABLE.end()) {
        ERROR("Unknown chip type key: % in %", chip, deviceDir);
        chipName = "UNKNOWN";
    } else {
        chipName = it->second;
    }
    npuInfoData.emplace_back(deviceId, chipName);
}

} // Database
} // Viewer
} // Analysis