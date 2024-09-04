/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ddr_assembler.cpp
 * Description        : 组合DDR层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/ddr_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
namespace {
const std::string READ_COUNTER = "DDR/Read";
const std::string WRITE_COUNTER = "DDR/Write";
const std::string READ_SERIES = "Read(MB/s)";
const std::string WRITE_SERIES = "Write(MB/s)";
}

DDRAssembler::DDRAssembler() : JsonAssembler(PROCESS_DDR, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateDDRTrace(std::vector<DDRData> &ddrData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
                      std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    for (const auto &data : ddrData) {
        time = std::to_string(data.timestamp / NS_TO_US);
        pid = pidMap.at(data.deviceId);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, READ_COUNTER);
        event->SetSeriesValue(READ_SERIES, data.fluxRead);
        res.push_back(event);
        MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, WRITE_COUNTER);
        event->SetSeriesValue(WRITE_SERIES, data.fluxWrite);
        res.push_back(event);
    }
}

uint8_t DDRAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto ddrData = dataInventory.GetPtr<std::vector<DDRData>>();
    if (ddrData == nullptr) {
        WARN("Can't get ddrData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    auto layerInfo = GetLayerInfo(PROCESS_DDR);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateDDRTrace(*ddrData, pidMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any DDR process data");
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
