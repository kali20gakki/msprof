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
const int32_t INVALID_PLANE = -1;
const int32_t PLAIN_OFFSET = 2;
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

HcclAssembler::HcclAssembler() : JsonAssembler(PROCESS_HCCL, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

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
                                          std::unordered_map<uint16_t, uint32_t>& pidMap, const LayerInfo &layerInfo)
{
    uint32_t formatPid;
    int tid;
    std::string transport;
    std::string dataType;
    std::string linkType;
    for (auto &data : task) {
        // L0场景不需要呈现小算子数据
        if (data.planeId == INVALID_PLANE) {
            return;
        }
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layerInfo.sortIndex);
        auto it = groupIndex_.find(data.groupName);
        if (it == groupIndex_.end()) {
            auto tmpId = groupIndex_.size() * this->maxPlainId_;
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
                                data.duration_estimated, data.bandwidth, data.notifyId, transport,
                                data.taskType, dataType, linkType);
        res_.push_back(event);
    }
}

void HcclAssembler::GenerateHcclOpTrace(const std::vector<CommunicationOpData>& opData, const std::string& profPath,
                                        std::unordered_map<uint16_t, uint32_t>& pidMap, const LayerInfo &layerInfo)
{
    uint32_t formatPid;
    int tid;
    std::string dataType;
    for (auto &data : opData) {
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layerInfo.sortIndex);
        auto it = groupIndex_.find(data.groupName);
        if (it == groupIndex_.end()) {
            tid = static_cast<int>(groupIndex_.size() * this->maxPlainId_);
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

void HcclAssembler::GenerateConnectionTrace(const CommunicationOpData& data, uint32_t formatPid, int tid)
{
    auto connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
    auto traceName = HOST_TO_DEVICE + connId;
    std::shared_ptr<FlowEvent> flow;
    MAKE_SHARED_RETURN_VOID(flow, FlowEvent, formatPid, tid, std::to_string(data.start / NS_TO_US), HOST_TO_DEVICE,
                            connId, traceName, FLOW_END, FLOW_BP);
    res_.push_back(flow);
}

void HcclAssembler::GenerateMetaDataEvent(std::unordered_map<uint16_t, uint32_t>& pidMap, const LayerInfo &layerInfo)
{
    for (const auto &it : pidMap) {
        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                layerInfo.component);
        res_.push_back(processName);
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                layerInfo.label);
        res_.push_back(processLabel);
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                layerInfo.sortIndex);
        res_.push_back(processIndex);
    }
    int planeId;
    std::string traceName;
    for (const auto &it : pidTidSet_) {
        planeId = it.second % this->maxPlainId_;
        traceName = {"Plane " + std::to_string(planeId)};
        if (planeId == 0) {
            traceName = {"Group " + std::to_string(it.second / this->maxPlainId_) + " Communication"};
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
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layerInfo = GetLayerInfo(PROCESS_HCCL);
    if (taskData != nullptr) {
        auto maxTask = std::max_element(taskData->begin(), taskData->end(), [](const CommunicationTaskData &ld,
                const CommunicationTaskData &rd) {
            return ld.planeId < rd.planeId;
        });
        if (maxTask != taskData->end() && maxTask->planeId != INVALID_PLANE) {
            this->maxPlainId_ = maxTask->planeId + PLAIN_OFFSET;  // plainId自身加1，group分组再加1
        }
        GenerateHcclTaskTrace(*taskData, profPath, devicePid, layerInfo);
    }
    if (opData != nullptr) {
        GenerateHcclOpTrace(*opData, profPath, devicePid, layerInfo);
    }
    GenerateMetaDataEvent(devicePid, layerInfo);
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
