/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : llc_assembler.cpp
 * Description        : 组合LLC层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/llc_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
namespace {
const std::string HIT_RATE_SERIES = "Hit Rate(%)";
const std::string THROUGHPUT_SERIES = "Throughput(MB/s)";
struct CounterName {
    std::string hitRate;
    std::string throughput;
};
const std::unordered_map<std::string, CounterName> CounterNameMap {
    {"read", {"Read/Hit Rate", "Read/Throughput"}},
    {"write", {"Write/Hit Rate", "Write/Throughput"}}
};
}

LLcAssembler::LLcAssembler() : JsonAssembler(PROCESS_LLC, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateLLcTrace(const std::vector<LLcData> &llcData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                      std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string llcId;
    std::string counterName;
    uint32_t pid;
    for (const auto &data : llcData) {
        time = std::to_string(data.localTime / NS_TO_US);
        llcId = std::to_string(data.llcID);
        pid = pidMap.at(data.deviceId); // 业务可以保证此处一定可以找到
        // hitRate
        counterName.clear();
        counterName.append("LLC ").append(llcId).append(" ").append(CounterNameMap.at(data.mode).hitRate);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, counterName);
        event->SetSeriesValue(HIT_RATE_SERIES, data.hitRate);
        res.push_back(event);
        // throughput
        counterName.clear();
        counterName.append("LLC ").append(llcId).append(" ").append(CounterNameMap.at(data.mode).throughput);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, counterName);
        event->SetSeriesValue(THROUGHPUT_SERIES, data.throughput);
        res.push_back(event);
    }
}

uint8_t LLcAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto LlcData = dataInventory.GetPtr<std::vector<LLcData>>();
    if (LlcData == nullptr) {
        WARN("Can't get LLcData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_LLC);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateLLcTrace(*LlcData, pidMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any LLc process data");
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
