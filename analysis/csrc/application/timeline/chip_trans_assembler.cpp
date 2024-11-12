/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip_trans_assembler.cpp
 * Description        : 组合Stars Chip Trans层数据
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
const int STARS_PA = 111111;  // 这两个值按照二进制的func_type对数据类型做区分，Python使用的是字符串，实测类型无影响
const int STARS_PCIE = 100010;
}

ChipTransAssembler::ChipTransAssembler() : JsonAssembler(PROCESS_STARS_CHIP_TRANS, {
    {MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GeneratePaLinkInfoTrace(std::vector<PaLinkInfoData> &paLinkInfoData,
    const std::unordered_map<uint16_t, uint32_t> &pidMap, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint64_t value;
    uint32_t pid;
    for (const auto &data : paLinkInfoData) {
        time = std::to_string(data.local_time / NS_TO_US);
        pid = pidMap.at(data.deviceId);
        std::vector<std::string> level {data.pa_link_traffic_monit_rx, data.pa_link_traffic_monit_tx};
        for (size_t i = 0; i < level.size(); ++i) {
            if (StrToU64(value, level[i]) != ANALYSIS_OK) {
                ERROR("paLinkInfoData is invalid");
                continue;
            }
            MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, STARS_PA, time, PA_LINK_COUNTERS[i]);
            event->SetSeriesIValue(PA_ID, data.pa_link_id);
            event->SetSeriesIValue(PA_LINK_COUNTERS[i], value);
            res.push_back(event);
        }
    }
}

void GeneratePcieInfoTrace(std::vector<PcieInfoData> &pcieInfoData,
    const std::unordered_map<uint16_t, uint32_t> &pidMap, std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    for (const auto &data : pcieInfoData) {
        time = std::to_string(data.local_time / NS_TO_US);
        pid = pidMap.at(data.deviceId);
        std::vector<uint64_t> level {data.pcie_read_bandwidth, data.pcie_write_bandwidth};
        for (size_t i = 0; i < level.size(); i++) {
            MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, STARS_PCIE, time, PCIE_COUNTERS[i]);
            event->SetSeriesIValue(PCIE_ID, data.pcie_id);
            event->SetSeriesIValue(PCIE_COUNTERS[i], level[i]);
            res.push_back(event);
        }
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
