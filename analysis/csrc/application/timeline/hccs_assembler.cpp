/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccs_assembler.cpp
 * Description        : 组合HCCS层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/hccs_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
namespace {
const std::string RX_COUNTER = "Rx";
const std::string TX_COUNTER = "Tx";
const std::string RX_SERIES = "Rx(MB/s)";
const std::string TX_SERIES = "Tx(MB/s)";
}

HCCSAssembler::HCCSAssembler() : JsonAssembler(PROCESS_HCCS, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateHccsTrace(std::vector<HccsData> &hccsData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                       std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    for (const auto &data : hccsData) {
        time = DivideByPowersOfTenWithPrecision(data.timestamp);
        pid = pidMap.at(data.deviceId);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, RX_COUNTER);
        event->SetSeriesDValue(RX_SERIES, data.rxThroughput);
        res.push_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, TX_COUNTER);
        event->SetSeriesDValue(TX_SERIES, data.txThroughput);
        res.push_back(event);
    }
}

uint8_t HCCSAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto hccsData = dataInventory.GetPtr<std::vector<HccsData>>();
    if (hccsData == nullptr) {
        WARN("Can't get hccsData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_HCCS);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateHccsTrace(*hccsData, pidMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any HCCS process data");
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
