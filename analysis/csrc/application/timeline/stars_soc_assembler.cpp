/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_soc_assembler.cpp
 * Description        : 组合Stars Soc层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/stars_soc_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
namespace {
const std::string L2_BUFFER_BW_LEVEL = "L2 Buffer Bw Level";
const std::string MATA_BW_LEVEL = "Mata Bw Level";
}

StarsSocAssembler::StarsSocAssembler() : JsonAssembler(PROCESS_STARS_SOC, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateStarsSocTrace(std::vector<SocBandwidthData> &socBandwidthData,
    const std::unordered_map<uint16_t, uint32_t> &pidMap, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    for (const auto &data : socBandwidthData) {
        time = std::to_string(data.timestamp / NS_TO_US);
        pid = pidMap.at(data.deviceId);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, L2_BUFFER_BW_LEVEL);
        event->SetSeriesValue(L2_BUFFER_BW_LEVEL, static_cast<double>(data.l2_buffer_bw_level));
        res.push_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, MATA_BW_LEVEL);
        event->SetSeriesValue(MATA_BW_LEVEL, static_cast<double>(data.mata_bw_level));
        res.push_back(event);
    }
}

uint8_t StarsSocAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto socBandwidthData = dataInventory.GetPtr<std::vector<SocBandwidthData>>();
    if (socBandwidthData == nullptr) {
        WARN("Can't get socBandwidthData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_STARS_SOC);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateStarsSocTrace(*socBandwidthData, pidMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any Stars Soc process data");
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
