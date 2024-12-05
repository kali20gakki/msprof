/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_tx_assembler.cpp
 * Description        : 组合device_tx层数据
 * Author             : msprof team
 * Creation Date      : 2024/11/18
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/device_tx_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/application/timeline/connection_id_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

void DeviceTxTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
}

DeviceTxAssembler::DeviceTxAssembler()
    : JsonAssembler(PROCESS_MSPROFTX, {{MSPROF_JSON_FILE, FileCategory::MSPROF},
                    {MSPROF_TX_FILE, FileCategory::MSPROF_TX}})
{}

void DeviceTxAssembler::GenerateDeviceTxTrace(const std::vector<MsprofTxDeviceData> &txData,
                                              const std::string &profPath, const LayerInfo &layer,
                                              std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    std::string traceName = NA;
    for (const auto &data : txData) {
        auto it = h2dMessage_.find(data.connectionId);
        if (it != h2dMessage_.end()) {
            traceName = it->second;
        }
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(data.streamId);
        // 存储pid，tid组合的最小集
        dPidTidSet_.insert({formatPid, tid});
        std::shared_ptr<DeviceTxTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, DeviceTxTraceEvent, formatPid, tid, data.duration / NS_TO_US,
                                DivideByPowersOfTenWithPrecision(data.start), traceName, data.streamId, data.taskId);
        res_.push_back(event);
        GenerateTxConnectionTrace(data, formatPid);
    }
}

void DeviceTxAssembler::GenerateTxConnectionTrace(const MsprofTxDeviceData &data, uint32_t formatPid)
{
    auto connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::MSPROF_TX);
    auto name = MS_TX;
    name.append("_").append(connId);
    int tid = static_cast<int>(data.streamId);
    std::shared_ptr<FlowEvent> end;
    MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid, DivideByPowersOfTenWithPrecision(data.start),
                            MS_TX, connId, name, FLOW_END, FLOW_BP);
    res_.push_back(end);
}

uint8_t DeviceTxAssembler::AssembleData(DataInventory& dataInventory, JsonWriter& ostream, const std::string& profPath)
{
    auto deviceTxData = dataInventory.GetPtr<std::vector<MsprofTxDeviceData>>();
    if (deviceTxData == nullptr) {
        WARN("Can't get device_tx data from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto txData = dataInventory.GetPtr<std::vector<MsprofTxHostData>>();
    if (txData != nullptr) {
        for (const auto &data : *txData) {
            if (data.connectionId != DEFAULT_CONNECTION_ID_MSTX) {  // tx ex数据才会有connectionId
                h2dMessage_.emplace(data.connectionId, data.message);
            }
        }
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto deviceLayer = GetLayerInfo(PROCESS_TASK);
    GenerateDeviceTxTrace(*deviceTxData, profPath, deviceLayer, devicePid);
    GenerateTaskMetaData(devicePid, deviceLayer, res_, dPidTidSet_);
    if (res_.empty()) {
        ERROR("Can't Generate any device tx process data");
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