/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_info_processor.cpp
 * Description        : communication_info_processor，处理表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/communication_info_processor.h"
#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/data_process/ai_task/communication_info_processor.h"
#include "analysis/csrc/domain/data_process/ai_task/kfc_comm_processor.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Application::Credential;
using namespace Domain::Environment;
using namespace Analysis::Utils;
namespace {
const std::string INDEX_NAME = "CommunicationTaskIndex";
const std::vector<std::string> COMMUNICATION_TASK_INDEX_COL_NAMES = {"globalTaskId"};
}

CommunicationInfoProcessor::CommunicationInfoProcessor(const std::string &msprofDBPath,
                                                       const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool CommunicationInfoProcessor::Run()
{
    INFO("CommunicationInfoProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_COMMUNICATION);
    return flag;
}

bool CommunicationInfoProcessor::Process(const std::string &fileDir)
{
    bool flag = true;
    GeHashMap hashMap;
    if (!GetGeHashMap(hashMap, fileDir)) {
        ERROR("Can't get hash data.");
        flag = false;
    }
    Infra::DataInventory dataInventory;
    std::shared_ptr<GeHashMap> hashMapPtr;
    MAKE_SHARED_RETURN_VALUE(hashMapPtr, GeHashMap, false, std::move(hashMap));
    dataInventory.Inject(hashMapPtr);
    Domain::CommunicationInfoProcessor communicationInfoProcessor(fileDir);
    communicationInfoProcessor.Run(dataInventory, TABLE_NAME_COMMUNICATION_TASK_INFO);
    Domain::KfcCommProcessor kfcProcess(fileDir);
    kfcProcess.Run(dataInventory, TABLE_NAME_COMMUNICATION_TASK_INFO);
    auto commTask = dataInventory.GetPtr<std::vector<Domain::CommunicationTaskData>>();
    auto commOp = dataInventory.GetPtr<std::vector<Domain::CommunicationOpData>>();
    auto kfcTask = dataInventory.GetPtr<std::vector<Domain::KfcTaskData>>();
    auto kfcOp = dataInventory.GetPtr<std::vector<Domain::KfcOpData>>();

    CommunicationTaskDataFormat taskData;
    CommunicationOpDataFormat opData;
    if (commTask) {
        ConvertTaskData(taskData, *commTask);
    }
    if (kfcTask) {
        ConvertTaskData(taskData, *kfcTask);
    }
    if (commOp) {
        ConvertOpData(opData, *commOp, fileDir);
    }
    if (kfcOp) {
        ConvertOpData(opData, *kfcOp, fileDir);
    }

    if (!taskData.empty() && !SaveData(taskData, TABLE_NAME_COMMUNICATION_TASK_INFO)) {
        ERROR("Save % data failed.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        flag = false;
    }
    if (!taskData.empty() &&
        !CreateTableIndex(TABLE_NAME_COMMUNICATION_TASK_INFO, INDEX_NAME, COMMUNICATION_TASK_INDEX_COL_NAMES)) {
        ERROR("Create table index failed.");
        flag = false;
    }
    if (!opData.empty() && !SaveData(opData, TABLE_NAME_COMMUNICATION_OP)) {
        ERROR("Save % data failed.", TABLE_NAME_COMMUNICATION_OP);
        flag = false;
    }
    return flag;
}

template<typename T>
void CommunicationInfoProcessor::ConvertTaskData(CommunicationTaskDataFormat &taskData, const std::vector<T> &commTask)
{
    uint64_t notifyId;
    for (const T &data: commTask) {
        auto opName = IdPool::GetInstance().GetUint64Id(data.opName);
        auto globalTaskId = IdPool::GetInstance().GetId(
            std::make_tuple(data.deviceId, data.streamId, data.taskId, data.contextId, data.batchId));
        auto taskType = IdPool::GetInstance().GetUint64Id(data.taskType);
        auto groupName = IdPool::GetInstance().GetUint64Id(data.groupName);
        auto opId = IdPool::GetInstance().GetUint32Id(data.opKey);
        if (!IsNumber(data.notifyId) || StrToU64(notifyId, data.notifyId) != ANALYSIS_OK) {
            notifyId = UINT64_MAX; // UINT64_MAX在db的INTEGER字段中为 -1
        }
        taskData.emplace_back(opName, globalTaskId, taskType, data.planeId, groupName, notifyId, data.rdmaType,
                              data.srcRank, data.dstRank, data.transportType, data.size, data.dataType,
                              data.linkType, opId, data.isMaster);
    }
}

template<typename T>
void CommunicationInfoProcessor::ConvertOpData(CommunicationOpDataFormat &opData, const std::vector<T> &commOp,
                                               const std::string &fileDir)
{
    auto profId = IdPool::GetInstance().GetUint32Id(fileDir);
    for (const T &data: commOp) {
        auto opName = IdPool::GetInstance().GetUint64Id(data.opName);
        auto connectionId = Utils::Contact(profId, data.connectionId);
        auto groupName = IdPool::GetInstance().GetUint64Id(data.groupName);
        auto opId = IdPool::GetInstance().GetUint32Id(data.opKey);
        auto algType = IdPool::GetInstance().GetUint64Id(data.algType);
        auto opType = IdPool::GetInstance().GetUint64Id(data.opType);
        opData.emplace_back(opName, data.timestamp, data.end, connectionId, groupName, opId, data.relay, data.retry,
                            data.dataType, algType, data.count, opType);
    }
}
}
}
}
