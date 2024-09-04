/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : qos_assembler.cpp
 * Description        : 组合QoS层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/29
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/qos_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/qos_data.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
namespace {
const std::string QOS = "QoS";
const std::string SERIES = "value";
}

QosAssembler::QosAssembler() : JsonAssembler(PROCESS_QOS, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void GenerateQosTrace(std::vector<QosData> &qosData, const std::unordered_map<uint16_t, uint32_t> &pidMap,
    const std::unordered_map<uint16_t, std::vector<std::string>> &qosEventsMap,
    std::vector<std::shared_ptr<TraceEvent>> &res)
{
    std::shared_ptr<CounterEvent> event;
    std::string time;
    uint32_t pid;
    std::string counter;
    for (const auto &data : qosData) {
        auto it = qosEventsMap.find(data.deviceId);
        if (it == qosEventsMap.end()) {
            continue;
        }
        time = std::to_string(data.localTime / NS_TO_US);
        pid = pidMap.at(data.deviceId);
        std::vector<uint32_t> bandwidth {data.bw1, data.bw2, data.bw3, data.bw4, data.bw5, data.bw6, data.bw7,
            data.bw8, data.bw9, data.bw10};
        for (size_t i = 0; i < it->second.size(); i++) {
            counter.clear();
            counter.append(QOS).append(" ").append(it->second[i]);
            MAKE_SHARED_RETURN_VOID(event, CounterEvent, pid, DEFAULT_TID, time, counter);
            event->SetSeriesValue(SERIES, bandwidth[i]);
            res.push_back(event);
        }
    }
}

uint8_t QosAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    auto qosData = dataInventory.GetPtr<std::vector<QosData>>();
    if (qosData == nullptr) {
        WARN("Can't get qosData from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> pidMap;
    std::unordered_map<uint16_t, std::vector<std::string>> qosEventsMap;
    auto layerInfo = GetLayerInfo(PROCESS_QOS);
    auto deviceList = File::GetFilesWithPrefix(profPath, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        auto pid = Context::GetInstance().GetPidFromInfoJson(deviceId, profPath);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap[deviceId] = formatPid;
        qosEventsMap[deviceId] = Context::GetInstance().GetQosEvents(deviceId, profPath);
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    GenerateQosTrace(*qosData, pidMap, qosEventsMap, res_);
    if (res_.empty()) {
        ERROR("Can't Generate any QoS process data");
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
