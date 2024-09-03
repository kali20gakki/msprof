/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip_trans_assembler.cpp
 * Description        : 组合CANN层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/chip_trans_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
namespace {
const std::vector<std::string> PA_LINK_COUNTERS {"PA Link Rx", "PA Link Tx"};
const std::vector<std::string> PCIE_COUNTERS {"PCIE Read Bandwidth", "PCIE Write Bandwidth"};
const std::string PA_ID = "PA Link ID";
const std::string PCIE_ID = "PCIE ID";
}

ChipTransAssembler::ChipTransAssembler() : JsonAssembler(PROCESS_STARS_CHIP_TRANS, {
    {"msprof", FileCategory::MSPROF}}) {}

void GeneratePaLinkInfoTrace(std::vector<PaLinkInfoData> &paLinkInfoData,
    const std::unordered_map<uint16_t, uint32_t> &pidMap, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string id;
    double paLinkId;
    double value;
    uint32_t pid;
    std::unordered_map<std::string, std::shared_ptr<CounterEvent>> eventMap;
    for (const auto &data : paLinkInfoData) {
        time = std::to_string(data.local_time / NS_TO_US);
        pid = pidMap.at(data.deviceId);
        id = std::to_string(data.pa_link_id);
        paLinkId = static_cast<double>(data.pa_link_id);
        std::vector<std::string> level {data.pa_link_traffic_monit_rx, data.pa_link_traffic_monit_tx};
        for (size_t i = 0; i < level.size(); ++i) {
            std::string tmpKey = PA_LINK_COUNTERS[i] + time + id;
            if (StrToDouble(value, level[i]) != ANALYSIS_OK) {
                ERROR("paLinkInfoData is invalid");
                continue;
            }
            if (eventMap.find(tmpKey) != eventMap.end()) {
                eventMap[tmpKey]->SetSeriesValue(PA_ID, paLinkId);
                eventMap[tmpKey]->SetSeriesValue(PA_LINK_COUNTERS[i], value);
            } else {
                MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, PA_LINK_COUNTERS[i]);
                event->SetSeriesValue(PA_ID, paLinkId);
                event->SetSeriesValue(PA_LINK_COUNTERS[i], value);
                eventMap[tmpKey] = event;
            }
        }
    }
    for (auto &counterEvent: eventMap) {
        res.push_back(counterEvent.second);
    }
}

void GeneratePcieInfoTrace(std::vector<PcieInfoData> &pcieInfoData,
    const std::unordered_map<uint16_t, uint32_t> &pidMap, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    std::string id;
    double paLinkId;
    uint32_t pid;
    std::unordered_map<std::string, std::shared_ptr<CounterEvent>> eventMap;
    for (const auto &data : pcieInfoData) {
        time = std::to_string(data.local_time / NS_TO_US);
        pid = pidMap.at(data.deviceId);
        id = std::to_string(data.pcie_id);
        paLinkId = static_cast<double>(data.pcie_id);
        std::vector<uint64_t> level {data.pcie_read_bandwidth, data.pcie_write_bandwidth};
        for (size_t i = 0; i < level.size(); i++) {
            std::string tmpKey = PCIE_COUNTERS[i] + time + id;
            if (eventMap.find(tmpKey) != eventMap.end()) {
                eventMap[tmpKey]->SetSeriesValue(PA_ID, paLinkId);
                eventMap[tmpKey]->SetSeriesValue(PCIE_COUNTERS[i], data.pcie_read_bandwidth);
            } else {
                MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, PCIE_COUNTERS[i]);
                event->SetSeriesValue(PA_ID, paLinkId);
                event->SetSeriesValue(PCIE_COUNTERS[i], data.pcie_write_bandwidth);
                eventMap[tmpKey] = event;
            }
        }
    }
    for (auto &counterEvent: eventMap) {
        res.push_back(counterEvent.second);
    }
}

uint8_t ChipTransAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto paLinkInfoData = dataInventory.GetPtr<std::vector<PaLinkInfoData>>();
    if (paLinkInfoData == nullptr) {
        WARN("Can't get paLinkInfoData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_STARS_CHIP_TRANS);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    auto pcieInfoData = dataInventory.GetPtr<std::vector<PcieInfoData>>();
    if (pcieInfoData == nullptr) {
        WARN("Can't get pcieInfoData from dataInventory"); // 很多时候这个表并不存在
    } else {
        GeneratePcieInfoTrace(*pcieInfoData, pidMap, res_);
    }
    GeneratePaLinkInfoTrace(*paLinkInfoData, pidMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any Stars Chip Trans data");
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
