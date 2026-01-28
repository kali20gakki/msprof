/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include <string>
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/domain/entities/hccl/include/hccl_task.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/modeling/include/log_modeling.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"

using namespace Analysis;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Utils;

namespace Analysis {
namespace Domain {

const std::string HOST_TASK_TABLE = "HostTask";
const std::string TASK_INFO_TABLE = "TaskInfo";
const std::string HCCL_OP_TABLE = "HCCLOP";
const std::string HCCL_TASK_TABLE = "HCCLTask";
using DeviceId2HostRunTime = std::map<TaskId, std::vector<HostTask>>;
using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
using RuntimeOriDataFormat = std::vector<std::tuple<uint32_t, int32_t, uint16_t, uint32_t, std::string, uint64_t,
                             std::string, int64_t>>;
using HcclTaskOriDataFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, int32_t, uint64_t,
                              double, uint32_t, uint16_t, uint32_t, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t,
                              uint32_t, std::string, double, std::string, std::string, std::string, std::string>>;
using HcclOpOriDataFormat = std::vector<std::tuple<uint16_t, uint64_t, int32_t, uint32_t, std::string, std::string,
                            std::string, uint64_t, uint64_t, std::string, int64_t, int32_t, int32_t, std::string,
                            std::string, uint64_t, std::string>>;
bool CheckPathAndTableExists(const std::string &path, DBRunner& dbRunner, const std::string &tableName)
{
    if (!File::Exist(path)) {
        WARN("There is no %", path);
        return false;
    }
    if (!dbRunner.CheckTableExists(tableName)) {
        WARN("There is no %", tableName);
        return false;
    }
    return true;
}

uint32_t ReadHostGEInfo(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    GEInfoDB geInfoDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "../", "/host", "/sqlite", geInfoDb.GetDBName()});
    DBRunner hostGeInfoDBRunner(hostDbDirectory);
    if (!CheckPathAndTableExists(hostDbDirectory, hostGeInfoDBRunner, TASK_INFO_TABLE)) {
        return ANALYSIS_OK;
    }
    std::string sql{"SELECT stream_id, batch_id, task_id, context_id, block_num, "
                    "mix_block_num FROM TaskInfo where device_id = " + std::to_string(deviceInfo.deviceId)};
    OriDataFormat result;
    bool rc =  hostGeInfoDBRunner.QueryData(sql, result);
    if (!rc) {
        ERROR("Failed to obtain data from the % table.", geInfoDb.GetDBName());
        return ANALYSIS_ERROR;
    }
    int noExistCNt = 0;
    auto deviceTaskMap = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!deviceTaskMap) {
        return ANALYSIS_ERROR;
    }
    for (auto row : result) {
        uint32_t stream_id, batch_id, task_id, context_id, blockNum, mixBlockNum;
        std::tie(stream_id, batch_id, task_id, context_id, blockNum, mixBlockNum) = row;
        TaskId id = {(uint16_t)stream_id, (uint16_t)batch_id, (uint16_t)task_id, context_id};
        auto item = deviceTaskMap->find(id);
        if (item != deviceTaskMap->end()) {
            std::vector<DeviceTask> &deviceTasks = item->second;
            for (auto& deviceTask : deviceTasks) {
                deviceTask.blockNum = (uint16_t)blockNum;
                deviceTask.mixBlockNum = (uint16_t)mixBlockNum;
            }
        } else {
            noExistCNt++;
        }
    }
    INFO("Read host GE info % not in table.", noExistCNt);
    return ANALYSIS_OK;
}

bool ReadHostRuntimeFromDB(const DeviceContext& deviceContext,
    DeviceId2HostRunTime& hostRuntime)
{
    RuntimeDB runtimeDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "../", "/host", "/sqlite", runtimeDb.GetDBName()});
    DBRunner hostRuntimeDBRunner(hostDbDirectory);

    if (!CheckPathAndTableExists(hostDbDirectory, hostRuntimeDBRunner, HOST_TASK_TABLE)) {
        return true; // 如果路径和表不存在，返回true表示没有错误，但hostRuntime为空
    }

    std::string sql{"SELECT stream_id, request_id, batch_id, task_id, context_ids, "
                    "model_id, task_type, connection_id FROM HostTask where device_id = "};
    sql += std::to_string(deviceInfo.deviceId);
    RuntimeOriDataFormat result(0);
    bool rc = hostRuntimeDBRunner.QueryData(sql, result);
    if (!rc) {
        ERROR("Failed to obtain data from the % table.", runtimeDb.GetDBName());
        return false;
    }

    for (const auto& row : result) {
        int32_t request_id;
        uint32_t stream_id, task_id, context_id_u32;
        uint16_t batch_id;
        uint64_t model_id;
        std::string task_type, context_id;
        int64_t connection_id;
        std::tie(stream_id, request_id, batch_id, task_id, context_id, model_id, task_type, connection_id) = row;
        HostTask hostTask;
        hostTask.modelId = model_id;
        hostTask.taskTypeStr = task_type;
        hostTask.streamId = stream_id;
        StrToU32(context_id_u32, context_id);
        TaskId id = {(uint16_t)stream_id, (uint16_t)batch_id, (uint16_t)task_id, context_id_u32};
        hostTask.contextId = context_id_u32;
        hostTask.taskId = task_id;
        hostTask.batchId = batch_id;
        hostTask.streamId = stream_id;
        hostTask.connection_id = connection_id;
        hostTask.requestId = request_id;
        hostRuntime[id].push_back(hostTask);
    }
    return true;
}

uint32_t ReadHostRuntime(DataInventory& dataInventory,
    const DeviceContext& deviceContext)
{
    DeviceId2HostRunTime hostRuntime;
    if (!ReadHostRuntimeFromDB(deviceContext, hostRuntime)) {
        return ANALYSIS_ERROR;
    }

    if (deviceContext.GetChipID() == CHIP_V6_1_0) {
        std::shared_ptr<StreamIdInfo> streamIdInfo;
        MAKE_SHARED_RETURN_VALUE(streamIdInfo, StreamIdInfo, ANALYSIS_ERROR);
        for (auto& task : hostRuntime) {
            streamIdInfo->streamIdMap.emplace(task.first.taskId, task.first.streamId);
        }
        dataInventory.Inject(streamIdInfo);
    }

    std::shared_ptr<DeviceId2HostRunTime> data;
    MAKE_SHARED_RETURN_VALUE(data, DeviceId2HostRunTime, ANALYSIS_ERROR, std::move(hostRuntime));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

uint32_t ReadHcclOp(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    HCCLDB hcclDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "../", "/host", "/sqlite", hcclDb.GetDBName()});
    DBRunner hostHcclDBRunner(hostDbDirectory);
    std::string sql{"SELECT device_id, model_id, index_id, thread_id, op_name, task_type, op_type, begin, "
                    "end - begin, is_dynamic, connection_id, relay, retry, data_type, alg_type, count, "
                    "group_name from HCCLOP where device_id = "};
    sql += std::to_string(deviceInfo.deviceId);
    std::vector<HcclOp> ans;
    std::shared_ptr<std::vector<HcclOp>> data;
    if (!CheckPathAndTableExists(hostDbDirectory, hostHcclDBRunner, HCCL_OP_TABLE)) {
        MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclOp>, ANALYSIS_ERROR, std::move(ans));
        dataInventory.Inject(data);
        return ANALYSIS_OK;
    }
    HcclOpOriDataFormat result(0);
    bool rc =  hostHcclDBRunner.QueryData(sql, result);
    if (!rc) {
        ERROR("Failed to obtain data from the % table.", hcclDb.GetDBName());
        return ANALYSIS_ERROR;
    }
    for (auto row : result) {
        uint16_t deviceId;
        uint64_t modelId, timestamp, duration, count;
        int32_t indexId, relay, retry;
        uint32_t threadId;
        std::string opName, taskType, opType, isDynamic, dataType, algType, groupName;
        int64_t connectionId;
        std::tie(deviceId, modelId, indexId, threadId, opName, taskType, opType, timestamp, duration,
                 isDynamic, connectionId, relay, retry, dataType, algType, count, groupName) = row;
        HcclOp hcclOp{deviceId, modelId, indexId, threadId, opName, taskType, opType, timestamp, duration,
                      isDynamic, connectionId, relay, retry, dataType, algType, count, groupName};
        ans.emplace_back(std::move(hcclOp));
    }
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclOp>, ANALYSIS_ERROR, std::move(ans));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

uint32_t ReadHcclTask(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    HCCLDB hcclDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "../", "/host", "/sqlite", hcclDb.GetDBName()});
    DBRunner hostHcclDBRunner(hostDbDirectory);
    std::string sql{"SELECT model_id, index_id, name, group_name, plane_id, timestamp , duration, stream_id, task_id, "
                    "context_id, batch_id, device_id, is_master, local_rank, remote_rank, thread_id, transport_type, "
                    "size, data_type, link_type, notify_id, rdma_type from HCCLTask where device_id = "};
    sql += std::to_string(deviceInfo.deviceId);
    std::vector<HcclTask> ans;
    std::shared_ptr<std::vector<HcclTask>> data;
    if (!CheckPathAndTableExists(hostDbDirectory, hostHcclDBRunner, HCCL_TASK_TABLE)) {
        MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclTask>, ANALYSIS_ERROR, std::move(ans));
        dataInventory.Inject(data);
        return ANALYSIS_OK;
    }
    HcclTaskOriDataFormat result(0);
    bool rc =  hostHcclDBRunner.QueryData(sql, result);
    if (!rc) {
        ERROR("Failed to obtain data from the % table.", hcclDb.GetDBName());
        return ANALYSIS_ERROR;
    }
    for (const auto& row : result) {
        uint64_t modelId, timestamp;
        int32_t indexId, planeId;
        std::string name, groupName, transportType, dataType, linkType, rdmaType, notifyId;
        double duration, size;
        uint32_t streamId, contextId, localRank, remoteRank, threadId;
        uint16_t taskId, batchId, deviceId, isMaster;
        std::tie(modelId, indexId, name, groupName, planeId, timestamp, duration, streamId, taskId,
                 contextId, batchId, deviceId, isMaster, localRank, remoteRank, threadId, transportType, size, dataType,
                 linkType, notifyId, rdmaType) = row;
        HcclTask hcclTask{modelId, indexId, name, groupName, planeId, timestamp, duration, streamId, taskId,
                          contextId, batchId, deviceId, isMaster, localRank, remoteRank, threadId, transportType, size,
                          dataType, linkType, notifyId, rdmaType};
        ans.emplace_back(std::move(hcclTask));
    }
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclTask>, ANALYSIS_ERROR, std::move(ans));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

uint32_t LoadHostData::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    auto devTaskSummary = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!devTaskSummary) {
        ERROR("LoadHostData get devTaskSummary fail.");
        return ANALYSIS_ERROR;
    }
    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    return ReadHostRuntime(dataInventory, deviceContext) | ReadHostGEInfo(dataInventory, deviceContext)
           | ReadHcclTask(dataInventory, deviceContext) | ReadHcclOp(dataInventory, deviceContext);
}

REGISTER_PROCESS_SEQUENCE(LoadHostData, false, Analysis::Domain::LogModeling, Analysis::Domain::LogModelingV6);
REGISTER_PROCESS_DEPENDENT_DATA(LoadHostData, std::map<TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(LoadHostData, CHIP_ID_ALL);

}
}
