// Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
/* ******************************************************************************
 * File Name          : metric_processor.cpp
 * Description        : metric_processor处理metric pmu相关数据
 * Author             : msprof team
 * Creation Date      : 2024/11/9
 * *****************************************************************************
 */
#include "analysis/csrc/domain/data_process/ai_task/metric_processor.h"
#include <algorithm>
#include <unordered_set>

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/metric_summary.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"


namespace Analysis {
namespace Domain {

namespace {
    const uint64_t INVALID_DB_SIZE = 0;
    const std::unordered_set<std::string> INVALID_COLUMN_NAMES = {
        "job_id", "host_id", "device_id", "task_id", "stream_id", "index_id",
        "model_id", "overflow", "overflowed_cycles", "device_id", "batch_id",
        "task_type", "core_type", "subtask_id", "start_time", "end_time", "ffts_type"};
    const std::unordered_set<std::string> UNSUPPORTED_HEADER = {
        "aic_vec_ratio", "aic_vec_time", "aiv_mac_ratio", "aiv_mac_time", "aiv_mte1_ratio",
        "aic_ub_read_bw", "aiv_mte1_time",
        "aic_ub_write_bw", "aiv_l1_read_bw", "aiv_l1_write_bw",
        "aic_l0c_read_bw", "aic_l0c_write_bw", "aiv_l0a_read_bw",
        "aiv_l0a_write_bw", "aiv_l0b_read_bw", "aiv_l0b_write_bw",
        "aiv_l0c_read_bw_cube", "aiv_l0c_write_bw_cube",
        "aic_ub_read_bw_mte", "aic_ub_write_bw_mte", "aic_ub_read_bw_vector",
        "aic_ub_write_bw_vector", "aiv_mac_fp16_ratio", "aiv_mac_int8_ratio",
        "aic_vec_fp32_ratio", "aic_vec_fp16_ratio", "aic_vec_int32_ratio",
        "aic_vec_misc_ratio", "aic_vec_fp16_128lane_ratio", "aic_vec_fp16_64lane_ratio",
        "aic_vec_bankgroup_cflt_ratio", "aic_vec_bank_cflt_ratio", "aic_vec_resc_cflt_ratio"};
    const std::unordered_set<std::string> NOT_SUPPORT_PMU_FOR_CHIP_V1 = {
        "vec_ratio", "vec_time",
        "ub_read_bw", "ub_read_bw(GB/s)",
        "ub_write_bw", "ub_write_bw(GB/s)",
        "l0c_write_bw", "l0c_write_bw(GB/s)",
        "l2_write_bw", "l2_write_bw(GB/s)",
        "vec_fp32_ratio",
        "vec_fp16_ratio",
        "vec_int32_ratio",
        "vec_misc_ratio",
        "vec_bankgroup_cflt_ratio",
        "vec_bank_cflt_ratio",
        "vec_resc_cflt_ratio"
    };
    const std::string TASK_BASED = "task-based";
}

MetricProcessor::MetricProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool MetricProcessor::Process(DataInventory &dataInventory)
{
    std::string metricMode;
    if (!Parser::Environment::Context::GetInstance().GetMetricMode(metricMode, profPath_)) {
        ERROR("Get Metric Mode device info failed.");
        return false;
    }

    if (metricMode == TASK_BASED) {
        return TaskBasedProcess(dataInventory);
    }

    WARN("Metric mode is not task_based, the metric summary will not processed.");
    return true;
}

bool MetricProcessor::TaskBasedProcess(DataInventory& dataInventory)
{
    INFO("TaskBasedProcess, dir is %", profPath_);
    bool flag = true;
    // get file list of which prefix is device_
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    // dbPath and deviceID
    std::unordered_map<std::string, uint16_t> dbPathAndDeviceID;
    DBInfo metricDB("metric_summary.db", "MetricSummary");
    // check DB
    for (const auto& devicePath : deviceList) {
        auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (deviceId == Parser::Environment::INVALID_DEVICE_ID) {
            ERROR("the invalid deviceId cannot to be identified.");
            return false;
        }
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, metricDB.dbName});
        if (!metricDB.ConstructDBRunner(dbPath)) {
            ERROR("construct DB Runner failed.");
            flag = false;
            continue;
        }
        auto status = CheckPathAndTable(dbPath, metricDB);
        if (status == CHECK_SUCCESS) {
            dbPathAndDeviceID.insert({dbPath, deviceId});
        } else if (status == CHECK_FAILED) {
            return false;
        }
    }
    if (dbPathAndDeviceID.empty()) {
        INFO("There is no db to export, prof path is %.", profPath_);
        return true;
    }
    auto headers = GetAndCheckTableColumns(dbPathAndDeviceID, metricDB);
    if (headers.empty()) {
        ERROR("GetAndCheckTableColumns failed, please check tableColumns are consistent.");
        return false;
    }

    if (!ProcessData(dbPathAndDeviceID, metricDB, headers, dataInventory)) {
        ERROR("Process data failed.");
        return false;
    }

    return flag;
}

std::vector<std::string> MetricProcessor::GetAndCheckTableColumns(
    const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceID, Analysis::Infra::DBInfo &metricDB)
{
    INFO("GetAndCheckTableColumns.");
    TableColumns tableColumns;
    std::vector<std::string> headers;
    for (const auto& pair : dbPathAndDeviceID) {
        MAKE_SHARED_RETURN_VALUE(metricDB.dbRunner, DBRunner, headers, pair.first);
        if (metricDB.dbRunner == nullptr) {
            ERROR("Create % connection failed.", pair.first);
            continue;
        }
        if (tableColumns.empty()) {
            tableColumns = metricDB.dbRunner->GetTableColumns(metricDB.tableName);
        } else if (tableColumns != metricDB.dbRunner->GetTableColumns(metricDB.tableName)) {
            ERROR("%'s tableColumns are different from others.", pair.first);
            // 若发现在各db中表字段不同,则返回空表头
            headers.clear();
            return headers;
        }
    }
    auto res = RemoveNeedlessColumns(tableColumns);
    for (auto tableColumn : res) {
        headers.emplace_back(tableColumn.name);
    }
    return headers;
}

std::vector<TableColumn> MetricProcessor::RemoveNeedlessColumns(std::vector<TableColumn> &tableColumns)
{
    std::vector<TableColumn> usedColumns;
    for (auto &tableColumn: tableColumns) {
        if (INVALID_COLUMN_NAMES.find(tableColumn.name) == INVALID_COLUMN_NAMES.end() &&
            UNSUPPORTED_HEADER.find(tableColumn.name) == UNSUPPORTED_HEADER.end()) {
            usedColumns.emplace_back(tableColumn);
        }
    }
    if (usedColumns.empty()) {
        ERROR("No used columns find.");
        return {};
    }
    return usedColumns;
}

bool MetricProcessor::ProcessData(const std::unordered_map<std::string, uint16_t> &dbPathAndDeviceID,
                                  Analysis::Infra::DBInfo &metricDB, std::vector<std::string> &headers,
                                  DataInventory &dataInventory)
{
    bool flag = true;
    std::map<TaskId, std::vector<std::vector<std::string>>> processedData;
    for (const auto& pair : dbPathAndDeviceID) {
        if (!TaskBasedProcessByDevice(pair, headers, metricDB, processedData)) {
            ERROR("TaskBasedProcess: process data failed, dbPath is %.",  pair.first);
            flag = false;
        }
    }
    // add memory data, 由于cube_utilization需要task_duration字段, 故放在assembler中计算
    if (AddMemoryBound(processedData, headers)) {
        headers.push_back("memory_bound");
    }

    auto metricSummaryHeaders = ModifySummaryHeaders(headers);
    if (metricSummaryHeaders.empty()) {
        ERROR("Processing data: get modified metric Summary failed, no metric data will be reserved.");
        return false;
    }

    MetricSummary processedMetric = MetricSummary(metricSummaryHeaders, processedData);
    std::shared_ptr<MetricSummary> data;
    MAKE_SHARED_RETURN_VALUE(data, MetricSummary, ANALYSIS_ERROR, processedMetric);
    if (!dataInventory.Inject(data)) {
        ERROR("Inject metric data to data inventory failed.");
        return false;
    }
    return flag;
}

bool MetricProcessor::TaskBasedProcessByDevice(const std::pair<const std::string, uint16_t> &dbPathAndDeviceID,
                                               const std::vector<std::string> &headers,
                                               Analysis::Infra::DBInfo &metricDB,
                                               std::map<TaskId, std::vector<std::vector<std::string>>> &processedData)
{
    std::vector<std::vector<std::string>> oriData = GetTaskBasedData(dbPathAndDeviceID.first, headers, metricDB);

    if (!FormatTaskBasedData(oriData, processedData, dbPathAndDeviceID.second)) {
        ERROR("FormatData failed, dbPath is %, deviceID is %.", dbPathAndDeviceID.first, dbPathAndDeviceID.second);
        return false;
    }

    return true;
}

std::vector<std::vector<std::string>> MetricProcessor::GetTaskBasedData(const std::string &dbPath,
    const std::vector<std::string> &headers, DBInfo &metricDB)
{
    std::vector<std::vector<std::string>> oriData;
    std::vector<std::tuple<std::string>> lineData;
    auto version = Parser::Environment::Context::GetInstance().GetPlatformVersion(
        Parser::Environment::DEFAULT_DEVICE_ID, profPath_);
    const bool IS_CHIP_V1 = Parser::Environment::Context::GetInstance().IsChipV1(version);

    MAKE_SHARED_RETURN_VALUE(metricDB.dbRunner, DBRunner, oriData, dbPath);
    if (metricDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    // 遍历tableColumns, 按列请求每个tableColumnKey的数据,再直接把vector<string> data列按表头顺序拼起来, 形成metric表
    for (auto &header : headers) {
        std::string sql;
        if (header.find("cycle") != std::string::npos) {
            sql = "SELECT " + header + " FROM " + metricDB.tableName;
        } else {
            sql = "SELECT ROUND(" + header + ", 3) FROM " + metricDB.tableName;
        }
        if (!metricDB.dbRunner->QueryData(sql, lineData)) {
            ERROR("Query % data failed, db path is %.", header, dbPath);
            continue;
        }
        // 如果chip类型为chipV1, 对于chipV1不支持的表头, 直接把该表头对应的列数据置为 "N/A"
        if (IS_CHIP_V1) {
            if (NOT_SUPPORT_PMU_FOR_CHIP_V1.find(header) != NOT_SUPPORT_PMU_FOR_CHIP_V1.end()) {
                INFO("For V1 chips, % data is not support, filling with N/A.", header);
                std::vector<std::string> tempVectorData (lineData.size(), NA);
                oriData.emplace_back(tempVectorData);
                continue;
            }
        }
        InsertLineDataToOriData(oriData, lineData);
    }

    std::vector<std::string> taskIdColumns = {"stream_id", "task_id", "batch_id"};
    std::string subtask_id_format =  "subtask_id";
    if (!Parser::Environment::Context::GetInstance().IsChipV4(version)) {
        subtask_id_format = "(case when subtask_id=" + std::to_string(UINT32_MAX) + " then 'N/A' else subtask_id end)";
    }
    taskIdColumns.insert(taskIdColumns.begin(), subtask_id_format);
    // 获得taskIdColumns的数据, 插入oriData表
    for (auto &taskIdColumn : taskIdColumns) {
        std::string sql = "SELECT " + taskIdColumn + " FROM " + metricDB.tableName;
        if (!metricDB.dbRunner->QueryData(sql, lineData)) {
            ERROR("Query taskId data failed, db path is %.", dbPath);
            continue;
        }
        InsertLineDataToOriData(oriData, lineData);
    }
    return oriData;
}

void MetricProcessor::InsertLineDataToOriData(std::vector<std::vector<std::string>> &oriData,
                                              std::vector<std::tuple<std::string>> &lineData)
{
    std::vector<std::string> tempVectorData;
    tempVectorData.reserve(lineData.size());
    for (auto &data : lineData) {
        tempVectorData.emplace_back(std::get<0>(data));
    }
    oriData.emplace_back(tempVectorData);
    lineData.clear();
}

bool MetricProcessor::FormatTaskBasedData(const std::vector<std::vector<std::string>> &oriData,
                                          std::map<TaskId, std::vector<std::vector<std::string>>> &processedData,
                                          const uint16_t &deviceId)
{
    INFO("FormatTaskBasedData.");
    if (oriData.empty() || oriData[0].empty()) {
        ERROR("Task-based original data is empty.");
        return false;
    }
    const int DATA_OFFSET = 4;
    const int KEY_OFFSET = 3;
    // oriDatum.at(-3) = stream_id -2-task_id -1-batch_id -4-subtask_id==context_id
    // subtask_id = N/A时不作为匹配依据, 以stream_id, task_id, batch_id 作为key进行group, 此时一个key可能对应多行数据
    const auto colSize = oriData.size();
    if (colSize < DATA_OFFSET) {
        ERROR("Processed metric data is not complete, please check fetching data process.");
        return false;
    }
    for (size_t row = 0; row < oriData[0].size(); ++row) {
        std::vector<std::string> processedDatum;
        for (size_t col = 0; col < colSize - DATA_OFFSET; ++col) {
            processedDatum.emplace_back(oriData[col][row]);
        }
        std::vector<uint16_t> tmpId;
        for (size_t col = colSize - KEY_OFFSET; col < colSize; ++col) {
            uint16_t temp;
            if (Utils::StrToU16(temp, oriData[col][row])) {
                WARN("Convert taskId info failed.");
                continue;
            }
            tmpId.emplace_back(temp);
        }
        TaskId tempTaskId;
        if (oriData[colSize - DATA_OFFSET][row] == "N/A") {
            tempTaskId = {tmpId[0], tmpId[2], tmpId[1], UINT32_MAX, deviceId};
        } else {
            uint32_t tempContextId;
            if (Utils::StrToU32(tempContextId, oriData[colSize - DATA_OFFSET][row])) {
                WARN("Convert context id failed, the row data will be discarded.");
                continue;
            }
            tempTaskId = {tmpId[0], tmpId[2], tmpId[1], tempContextId, deviceId};
        }
        if (processedData.find(tempTaskId) != processedData.end()) {
            processedData[tempTaskId].emplace_back(processedDatum);
            continue;
        }
        processedData[tempTaskId] = std::vector<std::vector<std::string>>({processedDatum});
    }

    if (processedData.empty()) {
        ERROR("FormatTaskBasedData data processing error.");
        return false;
    }
    return true;
}

bool MetricProcessor::AddMemoryBound(std::map<TaskId, std::vector<std::vector<std::string>>> &processedData,
                                     std::vector<std::string> &headers)
{
    INFO("Calculating memory bound...");
    std::vector<int> needDataIndex;
    if (!CheckAndGetMemoryBoundRelatedDataIndex(headers, needDataIndex)) {
        INFO("can't find memory bound needed column, no memory bound data added.");
        return false;
    }
    // 若字段存在, 则进行下一步计算, 遍历processedData, lineData用string存储,
    // needDataIndex[0] = mac_ratio_index, [1]=vec_ratio_index, [2]mte2_ratio_index
    const int mte2_ratio_index = 2;
    for (auto &processedDatum : processedData) {
        for (auto &lineData : processedDatum.second) {
            double mac_ratio_double, vec_ratio_double, mte2_ratio_double;
            if (Utils::StrToDouble(mac_ratio_double, lineData[needDataIndex[0]]) ||
                Utils::StrToDouble(vec_ratio_double, lineData[needDataIndex[1]]) ||
                Utils::StrToDouble(mte2_ratio_double, lineData[needDataIndex[mte2_ratio_index]])) {
                WARN("failed to convert memory bound data to double, set memory bound data N/A");
                lineData.push_back("N/A");
                continue;
            }
            lineData.push_back(std::to_string(mte2_ratio_double / std::max(mac_ratio_double, vec_ratio_double)));
        }
    }
    return true;
}
bool MetricProcessor::CheckAndGetMemoryBoundRelatedDataIndex(const std::vector<std::string> &headers,
                                                             std::vector<int> &neededDataIndex)
{
    // 先检查计算memory_bound所需的字段在不在....
    std::string mac_ratio = "mac_ratio";
    std::string vec_ratio = "vec_ratio";
    std::string mte2_ratio = "mte2_ratio";
    std::string mac_exe_ratio = "mac_exe_ratio";
    std::string vec_exe_ratio = "vec_exe_ratio";
    std::string mte2_exe_ratio = "mte2_exe_ratio";
    // 计算memory_bound需要的data包括以上六种, 需要前三种或者后三种, 同名_exe_字段仅headers名不同, 计算时字段意义一致
    std::vector<std::string> neededData = {
        mac_ratio, vec_ratio, mte2_ratio,
        mac_exe_ratio, vec_exe_ratio, mte2_exe_ratio};
    int count = 0;
    neededDataIndex = {0, 0, 0};
    const int neededDataSize = 3;
    for (unsigned index = 0; index < neededData.size(); ++index) {
        for (unsigned headersIndex = 0; headersIndex < headers.size(); ++headersIndex) {
            if (neededData[index] == headers[headersIndex]) {
                neededDataIndex[index % neededDataSize] = headersIndex;
                count++;
            }
        }
    }
    if (count == neededDataSize) {
        return true;
    }
    return false;
}

std::vector<std::string> MetricProcessor::ModifySummaryHeaders(const std::vector<std::string> &oriHeaders)
{
    std::vector<std::string> metricSummaryHeaders;
    metricSummaryHeaders.reserve(oriHeaders.size());
    for (const auto& oriHeader : oriHeaders) {
        if (oriHeader.find("_extra") != std::string::npos) {
            const int SUFFIX_SIZE = 6;
            metricSummaryHeaders.emplace_back(oriHeader.substr(0, oriHeader.size() - SUFFIX_SIZE));
            continue;
        }
        if (oriHeader == "total_time" || oriHeader == "aic_total_time") {
            metricSummaryHeaders.emplace_back("aicore_time(us)");
            continue;
        }
        if (oriHeader == "aiv_total_time") {
            metricSummaryHeaders.emplace_back("aiv_time(us)");
            continue;
        }
        if (oriHeader.find("time") != std::string::npos) {
            metricSummaryHeaders.emplace_back(oriHeader + "(us)");
            continue;
        }
        if (oriHeader.find("bw") != std::string::npos && oriHeader.find("(GB/s)") == std::string::npos) {
            metricSummaryHeaders.emplace_back(oriHeader + "(GB/s)");
            continue;
        }
        if (oriHeader.find("datas") != std::string::npos && oriHeader.find("(KB)") == std::string::npos) {
            metricSummaryHeaders.emplace_back(oriHeader + "(KB)");
            continue;
        }
        metricSummaryHeaders.emplace_back(oriHeader);
    }
    return metricSummaryHeaders;
}

} // Domain
} // Analysis
