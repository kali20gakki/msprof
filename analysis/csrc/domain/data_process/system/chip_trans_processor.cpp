/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip_trans_processor.cpp
 * Description        : chip_trans_processor，处理PaLinkInfo,PcieInfo表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/08/26
 * *****************************************************************************
 */

#include "chip_trans_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"


namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
using namespace Parser::Environment;

ChipTransProcessor::ChipTransProcessor(const std::string& profPaths)
    : DataProcessor(profPaths)
{}

bool ChipTransProcessor::Process(DataInventory& dataInventory)
{
    ChipTransData chipTransData;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    if (!Context::GetInstance().GetProfTimeRecordInfo(chipTransData.timeRecord, profPath_)) {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    bool flag = true;
    for (const auto& devicePath : deviceList) {
        if (!ProcessOneDevice(devicePath, chipTransData)) {
            flag = false;
        }
    }
    if (!SaveToDataInventory<PaLinkInfoData>(std::move(chipTransData.resPaData), dataInventory,
                                             TABLE_NAME_PA_LINK_INFO)) {
        ERROR("Save data failed, %.", TABLE_NAME_PA_LINK_INFO);
        return false;
    }
    if (!SaveToDataInventory<PcieInfoData>(std::move(chipTransData.resPcieData), dataInventory,
                                           TABLE_NAME_PCIE_INFO)) {
        ERROR("Save data failed, %.", TABLE_NAME_PCIE_INFO);
        return false;
    }
    return flag;
}


bool ChipTransProcessor::ProcessOneDevice(const std::string& devicePath, ChipTransData& chipTransData)
{
    auto deviceId = GetDeviceIdByDevicePath(devicePath);
    DBInfo paLinkInfo("chip_trans.db", "PaLinkInfo");
    DBInfo pcieInfo("chip_trans.db", "PcieInfo");
    std::string paLinkDBPath = Utils::File::PathJoin({devicePath, SQLITE, paLinkInfo.dbName});
    std::string pcieDBPath = Utils::File::PathJoin({devicePath, SQLITE, pcieInfo.dbName});
    if (!paLinkInfo.ConstructDBRunner(paLinkDBPath) || !pcieInfo.ConstructDBRunner(pcieDBPath)) {
        return false;
    }
    auto status = CheckPathAndTable(paLinkDBPath, paLinkInfo);
    if (status != CHECK_SUCCESS) {
        return status != CHECK_FAILED;
    }
    status = CheckPathAndTable(pcieDBPath, pcieInfo);
    if (status != CHECK_SUCCESS) {
        return status != CHECK_FAILED;
    }
    chipTransData.oriPaData = LoadPaData(paLinkInfo);
    chipTransData.oriPcieData = LoadPcieData(pcieInfo);
    std::vector<PaLinkInfoData> paData;
    std::vector<PcieInfoData> pcieData;
    if (!FormatData(paData, pcieData, chipTransData, deviceId)) {
        ERROR("Format data failed, %.", PROCESSOR_NAME_CHIP_TRAINS);
        return false;
    }
    chipTransData.resPaData.insert(chipTransData.resPaData.end(), paData.begin(), paData.end());
    chipTransData.resPcieData.insert(chipTransData.resPcieData.end(), pcieData.begin(), pcieData.end());
    return true;
}

OriPaFormat ChipTransProcessor::LoadPaData(const DBInfo& paLinkInfo)
{
    OriPaFormat oriPaData;
    std::string sql{"SELECT pa_link_id, pa_link_traffic_monit_rx, pa_link_traffic_monit_tx, sys_time "
                    "FROM " + paLinkInfo.tableName};
    if (!paLinkInfo.dbRunner->QueryData(sql, oriPaData)) {
        ERROR("Failed to obtain data from the % table.", paLinkInfo.tableName);
    }
    return oriPaData;
}

OriPcieFormat ChipTransProcessor::LoadPcieData(const DBInfo& pcieInfo)
{
    OriPcieFormat oriPcieData;
    std::string sql{"SELECT pcie_id, pcie_write_bandwidth, pcie_read_bandwidth, sys_time "
                    "FROM " + pcieInfo.tableName};
    if (!pcieInfo.dbRunner->QueryData(sql, oriPcieData)) {
        ERROR("Failed to obtain data from the % table.", pcieInfo.tableName);
    }
    return oriPcieData;
}

bool ChipTransProcessor::FormatData(std::vector<PaLinkInfoData>& paFormatData,
    std::vector<PcieInfoData>& pcieFormatData, ChipTransData& chipTransData, const uint16_t &deviceId)
{
    PaLinkInfoData paLinkInfoData;
    PcieInfoData pcieInfoData;
    paLinkInfoData.deviceId = deviceId;
    paLinkInfoData.deviceId = deviceId;
    pcieInfoData.deviceId = deviceId;
    if (!Reserve(paFormatData, chipTransData.resPaData.size()) ||
        !Reserve(pcieFormatData, chipTransData.resPcieData.size())) {
        ERROR("Reserve for Chip trains data failed.");
        return false;
    }
    for (auto& row : chipTransData.oriPaData) {
        uint64_t sysTime;
        std::tie(paLinkInfoData.pa_link_id, paLinkInfoData.pa_link_traffic_monit_rx,
                 paLinkInfoData.pa_link_traffic_monit_tx, sysTime) = row;
        HPFloat timestamp(sysTime);
        paLinkInfoData.local_time = GetLocalTime(timestamp, chipTransData.timeRecord).Uint64();
        paFormatData.push_back(paLinkInfoData);
    }
    for (auto& row : chipTransData.oriPcieData) {
        uint64_t sysTime;
        std::tie(pcieInfoData.pcie_id, pcieInfoData.pcie_write_bandwidth,
                 pcieInfoData.pcie_read_bandwidth, sysTime) = row;
        HPFloat timestamp(sysTime);
        pcieInfoData.local_time = GetLocalTime(timestamp, chipTransData.timeRecord).Uint64();
        pcieFormatData.push_back(pcieInfoData);
    }
    return true;
}

}
}