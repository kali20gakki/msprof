/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compute_task_info_processor.h
 * Description        : compute_task_info_processor，处理TaskInfo表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/compute_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/association/credential/id_pool.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;

struct ComputeTaskInfoData {
    int32_t deviceId;
    uint32_t modelId;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t contextId;
    uint32_t batchId;
    uint32_t blockDim;
    uint32_t maxBlockDim;
    uint64_t globalTaskId;
    uint64_t opName;
    uint64_t taskType;
    uint64_t opType;
    uint64_t inputFormats;
    uint64_t inputDataTypes;
    uint64_t inputShapes;
    uint64_t outputFormats;
    uint64_t outputDataTypes;
    uint64_t outputShapes;
    ComputeTaskInfoData() : deviceId(INT32_MAX), modelId(UINT32_MAX), streamId(UINT32_MAX), taskId(UINT32_MAX),
        contextId(UINT32_MAX), batchId(UINT32_MAX), blockDim(UINT32_MAX), maxBlockDim(UINT32_MAX),
        globalTaskId(UINT64_MAX), opName(UINT64_MAX), taskType(UINT64_MAX), opType(UINT64_MAX),
        inputFormats(UINT64_MAX), inputDataTypes(UINT64_MAX), inputShapes(UINT64_MAX), outputFormats(UINT64_MAX),
        outputDataTypes(UINT64_MAX), outputShapes(UINT64_MAX)
    {}
};

ComputeTaskInfoProcessor::ComputeTaskInfoProcessor(const std::string &msprofDBPath,
                                                   const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool ComputeTaskInfoProcessor::Run()
{
    INFO("ComputeTaskInfoProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_COMPUTE_TASK_INFO);
    return flag;
}

ComputeTaskInfoProcessor::OriDataFormat ComputeTaskInfoProcessor::GetData(const DBInfo &geInfoDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT model_id, op_name, stream_id, task_id, block_dim, mix_block_dim, task_type, op_type, "
                    "timestamp, batch_id, IFNULL(input_formats, 'NULL'), IFNULL(input_data_types, 'NULL'), "
                    "IFNULL(input_shapes, 'NULL'), IFNULL(output_formats, 'NULL'), IFNULL(output_data_types, 'NULL'), "
                    "IFNULL(output_shapes, 'NULL'), device_id, context_id FROM " + geInfoDB.tableName};
    if (!geInfoDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", geInfoDB.tableName);
    }
    return oriData;
}

ComputeTaskInfoProcessor::ProcessedDataFormat ComputeTaskInfoProcessor::FormatData(const OriDataFormat &oriData)
{
    ProcessedDataFormat processedData;
    ComputeTaskInfoData data;
    std::string oriOpName;
    std::string oriTaskType;
    std::string oriOpType;
    std::string oriInputFormats;
    std::string oriInputDataTypes;
    std::string oriInputShapes;
    std::string oriOutputFormats;
    std::string oriOutputDataTypes;
    std::string oriOutputShapes;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for compute task data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.modelId, oriOpName, data.streamId, data.taskId, data.blockDim,
                 data.maxBlockDim, oriTaskType, oriOpType, std::ignore, data.batchId, oriInputFormats,
                 oriInputDataTypes, oriInputShapes, oriOutputFormats, oriOutputDataTypes,
                 oriOutputShapes, data.deviceId, data.contextId) = row;
        data.opName = IdPool::GetInstance().GetUint64Id(oriOpName);
        data.taskType = IdPool::GetInstance().GetUint64Id(oriTaskType);
        data.opType = IdPool::GetInstance().GetUint64Id(oriOpType);
        data.globalTaskId = IdPool::GetInstance().GetId(
            std::make_tuple(data.deviceId, data.streamId, data.taskId, data.contextId, data.batchId));
        data.inputFormats = IdPool::GetInstance().GetUint64Id(oriInputFormats);
        data.inputDataTypes = IdPool::GetInstance().GetUint64Id(oriInputDataTypes);
        data.inputShapes = IdPool::GetInstance().GetUint64Id(oriInputShapes);
        data.outputFormats = IdPool::GetInstance().GetUint64Id(oriOutputFormats);
        data.outputDataTypes = IdPool::GetInstance().GetUint64Id(oriOutputDataTypes);
        data.outputShapes = IdPool::GetInstance().GetUint64Id(oriOutputShapes);
        processedData.emplace_back(data.opName, data.globalTaskId, data.blockDim, data.maxBlockDim, data.taskType,
                                   data.opType, data.inputFormats, data.inputDataTypes, data.inputShapes,
                                   data.outputFormats, data.outputDataTypes, data.outputShapes);
    }
    return processedData;
}

bool ComputeTaskInfoProcessor::Process(const std::string &fileDir)
{
    DBInfo geInfoDB("ge_info.db", "TaskInfo");
    MAKE_SHARED0_NO_OPERATION(geInfoDB.database, GEInfoDB);
    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, geInfoDB.dbName});
    // 并不是所有场景都有ge info数据
    auto status = CheckPath(dbPath);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    MAKE_SHARED_RETURN_VALUE(geInfoDB.dbRunner, DBRunner, false, dbPath);
    auto oriData = GetData(geInfoDB);
    if (oriData.empty()) {
        ERROR("Get % data failed in %.", geInfoDB.tableName, dbPath);
        return false;
    }
    auto processedData = FormatData(oriData);
    if (!SaveData(processedData, TABLE_NAME_COMPUTE_TASK_INFO)) {
        ERROR("Save data failed.");
        return false;
    }
    return true;
}

}
}
}