/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_db_dumper.h
 * Description        : cann_db_dumper用于将analyzer中的数据落盘
 * Author             : msprof team
 * Creation Date      : 2023-11-03
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/drafts/cann_trace_db_dumper.h"

#include "analysis/csrc/parser/host/cann/hash_data.h"
#include "analysis/csrc/parser/host/cann/type_data.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/utils/time_logger.h"
#include "analysis/csrc/viewer/database/drafts/number_mapping.h"
#include "collector/inc/toolchain/prof_common.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using ThreadPool = Analysis::Utils::ThreadPool;
using HashData = Analysis::Parser::Host::Cann::HashData;
using TypeData = Analysis::Parser::Host::Cann::TypeData;

using HCCLOpsDumpData = std::vector<std::tuple<uint32_t, uint32_t, int32_t, uint32_t, std::string, std::string,
                                               std::string, uint64_t, uint64_t, std::string, int64_t>>;

using HostTasksDumpData =
    std::vector<std::tuple<uint32_t, int64_t, uint32_t, uint32_t, std::string,
                           uint32_t, std::string, uint32_t, std::string, int64_t>>;

using HcclTasksDumpData = std::vector<
    std::tuple<uint32_t, int64_t, std::string, std::string, int64_t, std::string,
               double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
               std::string, double, std::string, std::string, std::string, std::string>>;

const uint32_t UNDEFINED_INT_VALUE = 4294967295;
const int32_t INVALID_VALUE = -1;
const std::string NA = "N/A";
const uint32_t INPUT_FORMAT_INDEX = 0;
const uint32_t OUTPUT_FORMAT_INDEX = 1;
CANNTraceDBDumper::CANNTraceDBDumper(std::string hostFilePath) : hostFilePath_(std::move(hostFilePath)),
                                                                 result_(true)
{}

bool CANNTraceDBDumper::DumpData(TreeAnalyzer analyzer)
{
    Utils::TimeLogger t{"Dump CANN data"};
    ThreadPool pool(poolSize_);
    pool.Start();
    pool.AddTask([this, &analyzer]() {
        HCCLBigOpDescs hcclBigOps = analyzer.GetHcclBigOps();
        HostTasks hcclTasks = analyzer.GetHCCLTasks();
        DumpHcclOps(hcclBigOps);
        DumpHcclTasks(hcclTasks);
    });
    pool.AddTask([this, &analyzer]() {
        HostTasks hostTasks = analyzer.GetTasks();
        DumpHostTasks(hostTasks);
    });
    pool.AddTask([this, &analyzer]() {
        HostTasks computeTasks = analyzer.GetComputeTasks();
        DumpOpDesc(computeTasks);
    });
    pool.WaitAllTasks();
    pool.Stop();
    if (!result_) {
        ERROR("Dump CANN data failed!");
        return false;
    }
    return true;
}

void CANNTraceDBDumper::DumpHcclOps(const HCCLBigOpDescs &hcclOps)
{
    if (hcclOps.empty()) {
        INFO("Empty hccl OPs");
        return;
    }
    Utils::TimeLogger t{"Dump hccl OPs"};
    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", hcclDB.GetDBName()});
    DBRunner hcclOpDBRunner(hcclTaskDBPath);
    if (!hcclOpDBRunner.CreateTable("HCCLOP", hcclDB.GetTableCols("HCCLOP"))) {
        ERROR("DumpHcclOps: create table HCCLOP failed");
        result_ = false;
        return;
    }
    HCCLOpsDumpData data;
    if (!Utils::Reserve(data, hcclOps.size())) {
        result_ = false;
        return;
    }
    for (const auto &op: hcclOps) {
        if (op == nullptr || op->hcclBigOpDesc == nullptr) {
            ERROR("DumpHcclOps: Empty op or desc");
            continue;
        }
        auto deviceId = op->hcclBigOpDesc->deviceId;
        auto desc = op->hcclBigOpDesc;
        uint64_t modelId = desc->modelId;
        int32_t indexId = desc->indexId;
        std::string isDynamic = NA;
        int64_t connection_id = desc->connectionId;
        auto msprofCompactInfo = desc->nodeDesc;
        uint32_t thread_id = desc->thread_id;
        data.emplace_back(deviceId, modelId, indexId,
                          thread_id,
                          HashData::GetInstance().Get(op->name),
                          msprofCompactInfo == nullptr ? "HCCL" : NumberMapping::Get(
                              NumberMapping::MappingType::GE_TASK_TYPE,
                              msprofCompactInfo->data.nodeBasicInfo.taskType),
                          msprofCompactInfo == nullptr ? NA : HashData::GetInstance().Get(
                              msprofCompactInfo->data.nodeBasicInfo.opType),
                          desc->beginTime, desc->endTime,
                          msprofCompactInfo == nullptr ? isDynamic :
                          std::to_string(msprofCompactInfo->data.nodeBasicInfo.opState),
                          connection_id);
    }
    if (!hcclOpDBRunner.InsertData("HCCLOP", data)) {
        ERROR("DumpHcclOps: Insert into db failed");
        result_ = false;
    }
}

void CANNTraceDBDumper::DumpHostTasks(const HostTasks &hostTasks)
{
    if (hostTasks.empty()) {
        INFO("Empty host tasks");
        return;
    }
    Utils::TimeLogger t{"Dump host tasks"};
    RuntimeDB runtimeDB;
    std::string hostTaskDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", runtimeDB.GetDBName()});
    DBRunner hostTaskDBRunner(hostTaskDBPath);
    if (!hostTaskDBRunner.CreateTable("HostTask", runtimeDB.GetTableCols("HostTask"))) {
        result_ = false;
        ERROR("DumpHostTasks: create table HostTask failed");
        return;
    }
    HostTasksDumpData data;
    if (!Utils::Reserve(data, hostTasks.size())) {
        result_ = false;
        return;
    }

    for (const auto &task: hostTasks) {
        auto taskType = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, task->taskType);
        data.emplace_back(task->modelId, task->requestId, task->streamId, task->taskId,
                          std::to_string(task->contextId),
                          task->batchId, taskType, task->deviceId,
                          std::to_string(task->timeStamp),
                          task->connection_id);
    }
    if (!hostTaskDBRunner.InsertData("HostTask", data)) {
        result_ = false;
        ERROR("DumpHostTasks: Insert into db failed");
    }
}

void CANNTraceDBDumper::DumpOpDesc(const HostTasks &computeTasks)
{
    if (computeTasks.empty()) {
        INFO("Empty kernel tasks");
        return;
    }
    Utils::TimeLogger t{"DumpOpDesc Start"};
    GEInfoDB geInfoDb;
    std::string opDescDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", geInfoDb.GetDBName()});
    DBRunner opDescDBRunner(opDescDBPath);
    if (!opDescDBRunner.CreateTable("TaskInfo", geInfoDb.GetTableCols("TaskInfo"))) {
        result_ = false;
        ERROR("DumpOpDesc: Create table TaskInfo failed");
        return;
    }
    TaskInfoData data;
    if (!Utils::Reserve(data, computeTasks.size())) {
        result_ = false;
        return;
    }
    for (const auto &task: computeTasks) {
        AddTaskInfo(task, data);
    }
    if (!opDescDBRunner.InsertData("TaskInfo", data)) {
        result_ = false;
        ERROR("DumpOpDesc: Insert into db failed");
    }
}

void CANNTraceDBDumper::AddTensorShapeInfo(const std::shared_ptr<ConcatTensorInfo> &tensorDesc,
                                           MsprofNodeBasicInfo nodeBasicInfo, TaskInfoData &data,
                                           const std::shared_ptr<HostTask> &task)
{
    auto tensorNum = tensorDesc->tensorNum;
    std::vector<std::string> inputFormat;
    std::vector<std::string> inputDataType;
    std::vector<std::string> inputShape;
    std::vector<std::string> outputFormat;
    std::vector<std::string> outputDataType;
    std::vector<std::string> outputShape;
    for (uint32_t i = 0; i < tensorNum; i++) {
        std::vector<std::string> shapes;
        auto tenosrData = tensorDesc->tensorData[i];
        for (const auto shape: tenosrData.shape) {
            if (shape == 0) {
                break;
            }
            shapes.emplace_back(std::to_string(shape));
        }
        if (tenosrData.tensorType == INPUT_FORMAT_INDEX) {
            inputFormat.emplace_back(GetFormat(tenosrData.format));
            inputDataType.emplace_back(
                NumberMapping::Get(NumberMapping::MappingType::GE_DATA_TYPE, tenosrData.dataType));
            inputShape.emplace_back(Utils::Join(shapes, ","));
        } else if (tenosrData.tensorType == OUTPUT_FORMAT_INDEX) {
            outputFormat.emplace_back(GetFormat(tenosrData.format));
            outputDataType.emplace_back(
                NumberMapping::Get(NumberMapping::MappingType::GE_DATA_TYPE, tenosrData.dataType)
            );
            outputShape.emplace_back(Utils::Join(shapes, ","));
        }
    }
    uint32_t blockDim = nodeBasicInfo.blockDim & 0xffff;
    auto mixBlockDim = blockDim * (nodeBasicInfo.blockDim >> 16);
    auto opFlag = nodeBasicInfo.opFlag ? "YES" : "NO";
    auto opState = std::to_string(nodeBasicInfo.opState);
    data.emplace_back(task->modelId, HashData::GetInstance().Get(nodeBasicInfo.opName), task->streamId,
                      task->taskId,
                      blockDim, mixBlockDim, opState,
                      NumberMapping::Get(
                          NumberMapping::MappingType::GE_TASK_TYPE, nodeBasicInfo.taskType),
                      HashData::GetInstance().Get(nodeBasicInfo.opType), task->requestId, task->thread_id,
                      task->timeStamp, task->batchId, tensorNum, Utils::Join(inputFormat, ";"),
                      Utils::Join(inputDataType, ";"), Utils::AddQuotation(Utils::Join(inputShape, ";")),
                      Utils::Join(outputFormat, ";"), Utils::Join(outputDataType, ";"),
                      Utils::AddQuotation(Utils::Join(outputShape, ";")), task->deviceId, task->contextId, opFlag);
}

std::string CANNTraceDBDumper::GetFormat(uint32_t oriFormat)
{
    auto format = oriFormat & 0xff;
    auto subFormat = (oriFormat & 0xffff00) >> 8;
    std::string enumFormat = NumberMapping::Get(NumberMapping::MappingType::GE_FORMAT, format);
    std::string enumSubFormat = std::to_string(subFormat);
    std::vector<std::string> vec{enumFormat, enumSubFormat};
    if (subFormat > 0) {
        enumFormat = Utils::Join(vec, ":");
    }
    return enumFormat;
}

void CANNTraceDBDumper::AddTaskInfo(const std::shared_ptr<HostTask> &task, TaskInfoData &data)
{
    if (!task || !task->op) {
        ERROR("DumpOpDesc: Empty op or desc");
        return;
    }
    auto desc = task->op->opDesc;
    if (!desc or !desc->nodeDesc) {
        // L0
        auto name = HashData::GetInstance().Get(task->op->name);
        data.emplace_back(task->modelId, name, task->streamId, task->taskId, 0, 0, NA, NA, NA,
                          task->requestId, task->thread_id, task->timeStamp, task->batchId,
                          0, "", "", "", "", "", "",
                          task->deviceId, task->contextId, NA);
        return;
    }
    auto node = desc->nodeDesc;
    auto nodeBasicInfo = node->data.nodeBasicInfo;
    auto blockDim = nodeBasicInfo.blockDim & 0xffff;
    auto mixBlockDim = blockDim * (nodeBasicInfo.blockDim >> 16);
    auto tensorDesc = desc->tensorDesc;
    auto opFlag = nodeBasicInfo.opFlag ? "YES" : "NO";
    auto opState = std::to_string(nodeBasicInfo.opState);
    if (!tensorDesc) {
        data.emplace_back(task->modelId, HashData::GetInstance().Get(nodeBasicInfo.opName), task->streamId,
                          task->taskId, blockDim,
                          mixBlockDim, opState, NumberMapping::Get(NumberMapping::MappingType::GE_TASK_TYPE,
                                                                   nodeBasicInfo.taskType),
                          HashData::GetInstance().Get(nodeBasicInfo.opType),
                          task->requestId, task->thread_id, task->timeStamp, task->batchId,
                          0, NA, NA, NA, NA, NA, NA, task->deviceId, task->contextId, opFlag);
        return;
    }
    AddTensorShapeInfo(tensorDesc, nodeBasicInfo, data, task);
}

void CANNTraceDBDumper::DumpHcclTasks(const HostTasks &hcclTasks)
{
    if (hcclTasks.empty()) {
        INFO("Empty hccl tasks");
        return;
    }
    Utils::TimeLogger t{"DumpHcclTasks start"};
    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", hcclDB.GetDBName()});
    DBRunner hcclTaskDBRunner(hcclTaskDBPath);
    if (!hcclTaskDBRunner.CreateTable("HCCLTask", hcclDB.GetTableCols("HCCLTask"))) {
        result_ = false;
        ERROR("DumpHcclTasks: create table HCCLTask failed");
        return;
    }
    HcclTasksDumpData data;
    if (!Utils::Reserve(data, hcclTasks.size())) {
        result_ = false;
        return;
    }
    for (const auto &task: hcclTasks) {
        if (!task || !task->op || !task->op->hcclSmallOpDesc) {
            ERROR("Empty op or desc");
            continue;
        }
        auto desc = task->op->hcclSmallOpDesc;
        if (!desc->hcclInfo || !desc->hcclInfo->data) {
            data.emplace_back(task->modelId, task->requestId, NA, NA, INVALID_VALUE, std::to_string(task->timeStamp),
                              INVALID_VALUE, task->streamId, task->taskId, task->contextId, task->batchId,
                              task->deviceId, desc->isMaster, INVALID_VALUE, INVALID_VALUE, NA, INVALID_VALUE,
                              NA, NA, NA, NA);
            continue;
        }
        auto hcclTrace = Utils::ReinterpretConvert<MsprofHcclInfo *>(desc->hcclInfo->data);
        data.emplace_back(task->modelId, task->requestId, HashData::GetInstance().Get(hcclTrace->itemId),
                          std::to_string(hcclTrace->groupName), hcclTrace->planeID, std::to_string(task->timeStamp),
                          hcclTrace->durationEstimated, task->streamId, task->taskId, task->contextId, task->batchId,
                          task->deviceId, desc->isMaster, hcclTrace->localRank, hcclTrace->remoteRank,
                          NumberMapping::Get(NumberMapping::MappingType::HCCL_TRANSPORT_TYPE, hcclTrace->transportType),
                          static_cast<double>(hcclTrace->dataSize),
                          NumberMapping::Get(NumberMapping::MappingType::HCCL_DATA_TYPE, hcclTrace->dataType),
                          NumberMapping::Get(NumberMapping::MappingType::HCCL_LINK_TYPE, hcclTrace->linkType),
                          std::to_string(hcclTrace->notifyID),
                          NumberMapping::Get(NumberMapping::MappingType::HCCL_RDMA_TYPE, hcclTrace->rdmaType));
    }
    if (!hcclTaskDBRunner.InsertData("HCCLTask", data)) {
        result_ = false;
        ERROR("DumpHcclTasks: Insert into db failed");
    }
}
} // Drafts
} // Database
} // Viewer
} // Analysis