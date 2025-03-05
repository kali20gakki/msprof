/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_assembler.cpp
 * Description        : 组合NpuMem层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/npu_mem_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace {
struct CounterName {
    std::string hbm;
    std::string ddr;
    std::string memory;
};
const std::unordered_map<std::string, CounterName> CounterNameMap {
    {"0", {"APP/HBM", "APP/DDR", "APP/Memory"}},
    {"1", {"Device/HBM", "Device/DDR", "Device/Memory"}}
};
}

NpuMemAssembler::NpuMemAssembler() : JsonAssembler(PROCESS_NPU_MEM, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateNpuMemTrace(std::vector<NpuMemData> &npuMemData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                         std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    for (const auto &data : npuMemData) {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        double hbmValue = static_cast<double>(data.hbm) / B_TO_KB;
        double ddrValue = static_cast<double>(data.ddr) / B_TO_KB;
        double memoryValue = static_cast<double>(data.memory) / B_TO_KB;
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, CounterNameMap.at(data.event).ddr);
        event->SetSeriesDValue("KB", ddrValue);
        res.push_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, CounterNameMap.at(data.event).hbm);
        event->SetSeriesDValue("KB", hbmValue);
        res.push_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, CounterNameMap.at(data.event).memory);
        event->SetSeriesDValue("KB", memoryValue);
        res.push_back(event);
    }
}

uint8_t NpuMemAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto npuMemData = dataInventory.GetPtr<std::vector<NpuMemData>>();
    if (npuMemData == nullptr) {
        WARN("Can't get npuMemData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_NPU_MEM);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateNpuMemTrace(*npuMemData, pidMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any NpuMem process data");
        return ASSEMBLE_FAILED;
    }
    for (const auto &node : res_) {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}
}
