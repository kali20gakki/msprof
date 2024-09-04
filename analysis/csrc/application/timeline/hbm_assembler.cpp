/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hbm_assembler.cpp
 * Description        : 组合HBM层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/hbm_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
namespace {
const std::unordered_map<std::string, std::string> CounterNameMap {
    {"read", "Read"},
    {"write", "Write"}
};
}

HBMAssembler::HBMAssembler() : JsonAssembler(PROCESS_HBM, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateHbmTrace(std::vector<HbmData> &hbmData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                      std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string counterName;
    std::string series;
    for (const auto &data : hbmData) {
        counterName.clear();
        counterName.append("HBM ").append(std::to_string(data.hbmId)).append("/").append(CounterNameMap.at(
            data.eventType));
        series.clear();
        series.append(CounterNameMap.at(data.eventType)).append("(MB/s)");
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pidMap.at(data.deviceId), DEFAULT_TID,
            std::to_string(data.localTime / NS_TO_US), counterName);
        event->SetSeriesValue(series, data.bandWidth);
        res.push_back(event);
    }
}

uint8_t HBMAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto hbmData = dataInventory.GetPtr<std::vector<HbmData>>();
    if (hbmData == nullptr) {
        WARN("Can't get hbmData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_HBM);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateHbmTrace(*hbmData, pidMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any HBM process data");
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
