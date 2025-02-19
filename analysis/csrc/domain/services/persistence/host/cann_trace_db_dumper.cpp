/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_db_dumper.h
 * Description        : cann_db_dumper用于将analyzer中的数据落盘
 * Author             : msprof team
 * Creation Date      : 2023-11-03
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/host/cann_trace_db_dumper.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"

namespace Analysis {
namespace Domain {
using ThreadPool = Analysis::Utils::ThreadPool;
using HashData = Analysis::Domain::Host::Cann::HashData;
using TypeData = Analysis::Domain::Host::Cann::TypeData;
using namespace Infra;

using HCCLOpsDumpData = std::vector<std::tuple<uint32_t, uint64_t, int32_t, uint32_t, std::string, std::string,
                                               std::string, uint64_t, uint64_t, std::string, int64_t, int64_t,
                                               int32_t, int32_t, std::string, std::string, uint64_t, std::string>>;

using HostTasksDumpData =
    std::vector<std::tuple<uint32_t, int64_t, uint32_t, uint32_t, std::string,
                           uint32_t, std::string, uint32_t, std::string, int64_t, uint32_t>>;

using HcclTasksDumpData = std::vector<
    std::tuple<uint32_t, int64_t, std::string, std::string, int64_t, std::string,
               double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
               std::string, double, std::string, std::string, std::string, std::string, uint32_t>>;

using GeFusionOpsDumpData = std::vector<std::tuple<uint64_t, std::string, uint32_t, std::string, std::string,
                                        std::string, std::string, std::string, std::string>>;

namespace {
const int32_t INVALID_VALUE = -1;
const std::string NA = "N/A";
const uint32_t INPUT_FORMAT_INDEX = 0;
const uint32_t OUTPUT_FORMAT_INDEX = 1;

void AddHcclOpDumpData(HCCLOpsDumpData& data, const std::shared_ptr<Analysis::Domain::Operator> &op)
{
    auto bigOpDesc = op->hcclBigOpDesc;
    // several attributes can not get currently, use default value
    uint64_t modelId = bigOpDesc->modelId;
    int32_t indexId = bigOpDesc->indexId;
    std::string isDynamic = NA;
    int64_t connectionId = bigOpDesc->connectionId;
    int64_t kfcConnectionId = bigOpDesc->kfcConnectionId;
    uint32_t thread_id = bigOpDesc->thread_id;
    std::string taskType = "COMMUNICATION";
    std::string opType = NA;
    auto nodeDesc = bigOpDesc->nodeDesc;
    if (nodeDesc != nullptr) {
        opType = HashData::GetInstance().Get(nodeDesc->data.nodeBasicInfo.opType);
        isDynamic = std::to_string(nodeDesc->data.nodeBasicInfo.opState);
    }
    int32_t relay = -1;
    int32_t retry = -1;
    std::string dataType = NA;
    std::string algType = NA;
    uint64_t count = UINT64_MAX;
    std::string groupName = NA;
    auto opInfoDesc = bigOpDesc->opInfoDesc;
    if (opInfoDesc != nullptr) {
        relay = opInfoDesc->data.hcclopInfo.relay;
        retry = opInfoDesc->data.hcclopInfo.retry;
        dataType = NumberMapping::Get(NumberMapping::MappingType::HCCL_DATA_TYPE,
                                      opInfoDesc->data.hcclopInfo.dataType);
        algType = HashData::GetInstance().Get(opInfoDesc->data.hcclopInfo.algType);
        count = opInfoDesc->data.hcclopInfo.count;
        groupName = std::to_string(opInfoDesc->data.hcclopInfo.groupName);
    }
    data.emplace_back(bigOpDesc->deviceId, modelId, indexId, thread_id,
                      HashData::GetInstance().Get(op->name),
                      taskType, opType, bigOpDesc->beginTime, bigOpDesc->endTime, isDynamic, connectionId,
                      kfcConnectionId, relay, retry, dataType, algType, count, groupName);
}
}
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
    pool.AddTask([this, &analyzer]() {
        GeFusionOpInfos geFusionOps = analyzer.GetGeFusionOpInfos();
        DumpGeFusionOps(geFusionOps);
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
        AddHcclOpDumpData(data, op);
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
                          task->connection_id, task->thread_id);
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
                NumberMapping::Get(NumberMapping::MappingType::GE_DATA_TYPE, tenosrData.dataType));
            outputShape.emplace_back(Utils::Join(shapes, ","));
        }
    }
    auto desc = task->op->opDesc;
    auto attr = desc->nodeAttr;
    auto hashId = attr ? std::to_string(attr->data.nodeAttrInfo.hashId) : NA;
    uint32_t blockDim = nodeBasicInfo.blockDim & 0xffff;
    auto mixBlockDim = blockDim * (nodeBasicInfo.blockDim >> 16);
    auto opFlag = nodeBasicInfo.opFlag ? "YES" : "NO";
    auto opState = std::to_string(nodeBasicInfo.opState);
    auto inputFormatStr = inputFormat.empty() ? NA : Utils::Join(inputFormat, ";");
    auto inputDataTypeStr = inputDataType.empty() ? NA : Utils::Join(inputDataType, ";");
    auto inputShapeStr = inputShape.empty() ? NA : Utils::AddQuotation(Utils::Join(inputShape, ";"));
    auto outputFormatStr = outputFormat.empty() ? NA : Utils::Join(outputFormat, ";");
    auto outputDataTypeStr = outputDataType.empty() ? NA : Utils::Join(outputDataType, ";");
    auto outputShapeStr = outputShape.empty() ? NA : Utils::AddQuotation(Utils::Join(outputShape, ";"));
    data.emplace_back(task->modelId, HashData::GetInstance().Get(nodeBasicInfo.opName), task->streamId,
                      task->taskId, blockDim, mixBlockDim, opState,
                      NumberMapping::Get(NumberMapping::MappingType::GE_TASK_TYPE, nodeBasicInfo.taskType),
                      HashData::GetInstance().Get(nodeBasicInfo.opType), task->requestId, task->thread_id,
                      task->timeStamp, task->batchId, tensorNum, inputFormatStr, inputDataTypeStr, inputShapeStr,
                      outputFormatStr, outputDataTypeStr, outputShapeStr,
                      task->deviceId, task->contextId, opFlag, hashId);
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
                          0, NA, NA, NA, NA, NA, NA,
                          task->deviceId, task->contextId, NA, NA);
        return;
    }
    auto node = desc->nodeDesc;
    auto attr = desc->nodeAttr;
    auto nodeBasicInfo = node->data.nodeBasicInfo;
    auto hashId = attr ? std::to_string(attr->data.nodeAttrInfo.hashId) : NA;
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
                          0, NA, NA, NA, NA, NA, NA, task->deviceId, task->contextId, opFlag, hashId);
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
                              NA, NA, NA, NA, task->thread_id);
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
                          NumberMapping::Get(NumberMapping::MappingType::HCCL_RDMA_TYPE, hcclTrace->rdmaType),
                          task->thread_id);
    }
    if (!hcclTaskDBRunner.InsertData("HCCLTask", data)) {
        result_ = false;
        ERROR("DumpHcclTasks: Insert into db failed");
    }
}


void CANNTraceDBDumper::DumpGeFusionOps(const GeFusionOpInfos &geFusionOps)
{
    if (geFusionOps.empty()) {
        INFO("Empty geFusionOps");
        return;
    }
    Utils::TimeLogger t{"DumpGeFusionOps start"};
    GeModelInfoDB geModelInfoDb;
    std::string geModelInfoDbPath = Utils::File::PathJoin({hostFilePath_, "sqlite", geModelInfoDb.GetDBName()});
    DBRunner geFusionOpDBRunner(geModelInfoDbPath);
    if (!geFusionOpDBRunner.CreateTable("GeFusionOpInfo", geModelInfoDb.GetTableCols("GeFusionOpInfo"))) {
        result_ = false;
        ERROR("DumpGeFusionOps: create table GeFusionOpInfo failed");
        return;
    }
    GeFusionOpsDumpData data;
    if (!Utils::Reserve(data, geFusionOps.size())) {
        result_ = false;
        return;
    }
    for (const auto &geFusionOp: geFusionOps) {
        auto fusionStruct = geFusionOp->fusionOpInfo;
        std::vector<std::string> fusionOpNames;
        if (!Utils::Reserve(fusionOpNames, fusionStruct->fusionOpNum)) {
            break;
        }
        for (uint32_t i = 0; i < fusionStruct->fusionOpNum; i++) {
            fusionOpNames.emplace_back(HashData::GetInstance().Get(fusionStruct->fusionOpId[i]));
        }
        data.emplace_back(
            geFusionOp->modelId,
            HashData::GetInstance().Get(fusionStruct->opName),
            fusionStruct->fusionOpNum,
            Utils::Join(fusionOpNames, ","),
            std::to_string(fusionStruct->inputMemsize),
            std::to_string(fusionStruct->outputMemsize),
            std::to_string(fusionStruct->weightMemSize),
            std::to_string(fusionStruct->workspaceMemSize),
            std::to_string(fusionStruct->totalMemSize)
        );
    }
    if (!geFusionOpDBRunner.InsertData("GeFusionOpInfo", data)) {
        result_ = false;
        ERROR("DumpGeFusionOps: Insert into db failed");
    }
}
} // Domain
} // Analysis
