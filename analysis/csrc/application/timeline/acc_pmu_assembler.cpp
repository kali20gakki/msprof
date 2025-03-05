/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_assembler.cpp
 * Description        : 组合Acc PMU层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/acc_pmu_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace {
const std::vector<std::string> COUNTERS {"read_bandwidth", "read_ost", "write_bandwidth", "write_ost"};
const std::string ACC_ID = "acc_id";
const std::string VALUE = "value";
}

AccPmuAssembler::AccPmuAssembler() : JsonAssembler(PROCESS_ACC_PMU, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateAccPmuTrace(std::vector<AccPmuData> &accPmuData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                         std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string accId;
    uint32_t pid;
    for (const auto &data : accPmuData) {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        std::vector<uint32_t> level {data.readBwLevel, data.readOstLevel, data.writeBwLevel, data.writeOstLevel};
        for (size_t i = 0; i < level.size(); i++) {
            MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, COUNTERS[i]);
            event->SetSeriesIValue(ACC_ID, data.accId);
            event->SetSeriesIValue(VALUE, level[i]);
            res.push_back(event);
        }
    }
}

uint8_t AccPmuAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto accPmuData = dataInventory.GetPtr<std::vector<AccPmuData>>();
    if (accPmuData == nullptr) {
        WARN("Can't get accPmuData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_ACC_PMU);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateAccPmuTrace(*accPmuData, pidMap, res_);
    GenerateHWMetaData(pidMap, layerInfo, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any Acc PMU process data");
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
