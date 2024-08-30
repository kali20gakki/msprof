/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_assembler.h
 * Description        : 组合HCCL层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/27
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/hccl_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/application/timeline/connection_id_pool.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Viewer::Database;
namespace {
const uint32_t SORT_INDEX = 15;
const std::string HCCL_ASSEMBLER = "HCCL";
const std::string HCCL_LABEL = "NPU";
const int32_t INVALID_PLANE = -1;
const int INDEX_LENGTH = 9;  // plane id 最多 8 个 + group 栏的预留位
}

std::string TransEnumToType(uint64_t key, const std::unordered_map<std::string, uint16_t > &enumTable)
{
    for (const auto &node : enumTable) {
        if (node.second == key) {
            return node.first;
        }
    }
    return "";
}

HcclAssembler::HcclAssembler() : JsonAssembler(HCCL_ASSEMBLER, {{"msprof", FileCategory::MSPROF}}) {}

void HcclOpTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["connection_id"] << connectionId_;
    ostream["model id"] << modelId_;
    ostream["data_type"] << dataType_;
    ostream["alg_type"] << algType_;
    ostream["count"] << count_;
}

void HcclTaskTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["notify_id"] << notifyId_;
    ostream["duration estimated(us)"] << esDur_;
    ostream["stream id"] << streamId_;
    ostream["task id"] << taskId_;
    ostream["context id"] << contextId_;
    ostream["task type"] << taskType;
    ostream["src rank"] << srcRank_;
    ostream["dst rank"] << dstRank_;
    ostream["transport type"] << transportType_;
    ostream["size(Byte)"] << size_;
    ostream["data type"] << dataType_;
    ostream["link type"] << linkType_;
    ostream["bandwidth(GB/s)"] << bandwidth_;
    ostream["model id"] << modelId_;
}

void HcclAssembler::GenerateHcclTaskTrace(const std::vector<CommunicationTaskData>& task, const std::string &profPath,
                                          std::unordered_map<uint16_t, int>& pidMap)
{
    int formatPid;
    int tid;
    std::string transport;
    std::string dataType;
    std::string linkType;
    for (auto &data : task) {
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, SORT_INDEX);
        auto it = groupIndex_.find(data.groupName);
        if (it == groupIndex_.end()) {
            auto tmpId = groupIndex_.size() * INDEX_LENGTH;
            groupIndex_[data.groupName] = static_cast<int>(tmpId);
            tid = static_cast<int>(tmpId + data.planeId + 1);
        } else {
            tid = it->second + static_cast<int>(data.planeId) + 1;
        }
        pidTidSet_.insert({formatPid, tid});
        transport = TransEnumToType(data.transportType, HCCL_TRANSPORT_TYPE_TABLE);
        dataType = TransEnumToType(data.dataType, HCCL_DATA_TYPE_TABLE);
        linkType = TransEnumToType(data.linkType, HCCL_LINK_TYPE_TABLE);
        std::shared_ptr<HcclTaskTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, HcclTaskTraceEvent, formatPid, tid, data.duration / NS_TO_US,
                                std::to_string(data.start / NS_TO_US), data.taskType, data.srcRank, data.dstRank,
                                data.streamId, data.taskId, data.contextId, data.modelId, data.size,
                                data.duration_estimated / NS_TO_US, data.bandwidth, data.notifyId, transport,
                                data.taskType, dataType, linkType);
        res_.push_back(event);
    }
}

void HcclAssembler::GenerateHcclOpTrace(const std::vector<CommunicationOpData>& opData, const std::string& profPath,
                                        std::unordered_map<uint16_t, int>& pidMap)
{
    int formatPid;
    int tid;
    std::string dataType;
    for (auto &data : opData) {
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, SORT_INDEX);
        auto it = groupIndex_.find(data.groupName);
        if (it == groupIndex_.end()) {
            tid = static_cast<int>(groupIndex_.size() * INDEX_LENGTH);
            groupIndex_[data.groupName] = tid;
        } else {
            tid = it->second;
        }
        pidTidSet_.insert({formatPid, tid});
        dataType = TransEnumToType(data.dataType, HCCL_DATA_TYPE_TABLE);
        std::shared_ptr<HcclOpTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, HcclOpTraceEvent, formatPid, tid, (data.end - data.start) / NS_TO_US,
                                std::to_string(data.start / NS_TO_US), data.opName, data.modelId, data.count,
                                data.connectionId, dataType, data.algType);
        res_.push_back(event);
        GenerateConnectionTrace(data, formatPid, tid);
    }
}

void HcclAssembler::GenerateConnectionTrace(const CommunicationOpData& data, int formatPid, int tid)
{
    auto connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
    auto traceName = HOST_TO_DEVICE + connId;
    std::shared_ptr<FlowEvent> flow;
    MAKE_SHARED_RETURN_VOID(flow, FlowEvent, formatPid, tid, std::to_string(data.start / NS_TO_US), HOST_TO_DEVICE,
                            connId, traceName, FLOW_END, FLOW_BP);
    res_.push_back(flow);
}

void HcclAssembler::GenerateMetaDataEvent(std::unordered_map<uint16_t, int>& pidMap)
{
    for (const auto &it : pidMap) {
        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                HCCL_ASSEMBLER);
        res_.push_back(processName);
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                HCCL_LABEL);
        res_.push_back(processLabel);
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                SORT_INDEX);
        res_.push_back(processIndex);
    }
    int planeId;
    std::string traceName;
    for (const auto &it : pidTidSet_) {
        planeId = it.second % INDEX_LENGTH;
        traceName = {"Plane " + std::to_string(planeId)};
        if (planeId == 0) {
            traceName = {"Group " + std::to_string(it.second / INDEX_LENGTH) + " Communication"};
        }
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, it.first, it.second, META_DATA_THREAD_NAME, traceName);
        res_.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.first, it.second, META_DATA_THREAD_INDEX,
                                it.second);
        res_.push_back(threadIndex);
    }
}

uint8_t HcclAssembler::AssembleData(DataInventory& dataInventory, JsonWriter& ostream, const std::string& profPath)
{
    auto taskData = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
    auto opData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    if (taskData == nullptr && opData == nullptr) {
        WARN("Can't get hccl task data and hccl op data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, int> devicePid;
    if (taskData != nullptr) {
        GenerateHcclTaskTrace(*taskData, profPath, devicePid);
    }
    if (opData != nullptr) {
        GenerateHcclOpTrace(*opData, profPath, devicePid);
    }
    GenerateMetaDataEvent(devicePid);
    if (res_.empty()) {
        ERROR("Can't Generate any Ascend process data");
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
