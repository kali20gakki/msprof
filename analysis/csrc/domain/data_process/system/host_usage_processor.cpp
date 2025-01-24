/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_usage_processor.cpp
 * Description        : 处理CpuUsage,MemUsage,DiskUsage,NetWorkUsage表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/system/host_usage_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
HostUsageProcessor::HostUsageProcessor(const std::string &profPath_, DBInfo &&dbInfo) : usageDb_(dbInfo),
    DataProcessor(profPath_) {}

bool HostUsageProcessor::Process(DataInventory &dataInventory)
{
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_)) {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, usageDb_.dbName});
    if (!usageDb_.ConstructDBRunner(dbPath) || usageDb_.dbRunner == nullptr) {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, usageDb_);
    if (status == CHECK_FAILED) {
        return false;
    } else if (status == NOT_EXIST) {
        return true;
    }
    return ProcessData(dataInventory, record, dbPath);
}

HostCpuUsageProcessor::HostCpuUsageProcessor(const std::string &profPath_) : processorName_(PROCESSOR_NAME_CPU_USAGE),
    HostUsageProcessor(profPath_, DBInfo("host_cpu_usage.db", "CpuUsage")) {}

bool HostCpuUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                        const std::string &dbPath)
{
    OriCpuUsage oriData;
    std::vector<CpuUsageData> res;
    std::string sql{"SELECT start_time, cpu_no, usage FROM " + usageDb_.tableName +
                    " ORDER BY CAST(cpu_no AS DECIMAL)"};
    if (!usageDb_.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query cpu usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty()) {
        ERROR("cpu usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size())) {
        ERROR("Reserved for cpu usage data failed.");
        return false;
    }
    CpuUsageData data;
    for (const auto &row : oriData) {
        std::tie(data.start, data.cpuNo, data.usage) = row;
        HPFloat startTimestamp{data.start};
        data.start = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    if (!SaveToDataInventory<CpuUsageData>(std::move(res), dataInventory, processorName_)) {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

HostMemUsageProcessor::HostMemUsageProcessor(const std::string &profPath_) : processorName_(PROCESSOR_NAME_MEM_USAGE),
    HostUsageProcessor(profPath_, DBInfo("host_mem_usage.db", "MemUsage")) {}

bool HostMemUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                        const std::string &dbPath)
{
    OriTimeUsage oriData;
    std::vector<MemUsageData> res;
    std::string sql{"SELECT start_time, usage FROM " + usageDb_.tableName};
    if (!usageDb_.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query mem usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty()) {
        ERROR("mem usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size())) {
        ERROR("Reserved for mem usage data failed.");
        return false;
    }
    MemUsageData data;
    for (const auto &row : oriData) {
        std::tie(data.start, data.usage) = row;
        HPFloat startTimestamp{data.start};
        data.start = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    if (!SaveToDataInventory<MemUsageData>(std::move(res), dataInventory, processorName_)) {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

HostDiskUsageProcessor::HostDiskUsageProcessor(const std::string &profPath_)
    : processorName_(PROCESSOR_NAME_DISK_USAGE),
      HostUsageProcessor(profPath_, DBInfo("host_disk_usage.db", "DiskUsage")) {}

bool HostDiskUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                         const std::string &dbPath)
{
    OriTimeUsage oriData;
    std::vector<DiskUsageData> res;
    std::string sql{"SELECT start_time, usage FROM " + usageDb_.tableName};
    if (!usageDb_.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query disk usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty()) {
        ERROR("disk usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size())) {
        ERROR("Reserved for disk usage data failed.");
        return false;
    }
    DiskUsageData data;
    for (const auto &row : oriData) {
        std::tie(data.start, data.usage) = row;
        HPFloat startTimestamp{data.start};
        data.start = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    if (!SaveToDataInventory<DiskUsageData>(std::move(res), dataInventory, processorName_)) {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}

HostNetworkUsageProcessor::HostNetworkUsageProcessor(const std::string &profPath_)
    : processorName_(PROCESSOR_NAME_NETWORK_USAGE),
      HostUsageProcessor(profPath_, DBInfo("host_network_usage.db", "NetworkUsage")) {}

bool HostNetworkUsageProcessor::ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record,
                                            const std::string &dbPath)
{
    OriTimeUsage oriData;
    std::vector<NetWorkUsageData> res;
    std::string sql{"SELECT start_time, usage FROM " + usageDb_.tableName};
    if (!usageDb_.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query network usage data failed, db path is %.", dbPath);
    }
    if (oriData.empty()) {
        ERROR("network usage original data is empty. db path is %", dbPath);
        return false;
    }
    if (!Reserve(res, oriData.size())) {
        ERROR("Reserved for network usage data failed.");
        return false;
    }
    NetWorkUsageData data;
    for (const auto &row : oriData) {
        std::tie(data.start, data.usage) = row;
        HPFloat startTimestamp{data.start};
        data.start = GetLocalTime(startTimestamp, record).Uint64();
        res.push_back(data);
    }
    if (!SaveToDataInventory<NetWorkUsageData>(std::move(res), dataInventory, processorName_)) {
        ERROR("Save data failed, %.", processorName_);
        return false;
    }
    return true;
}
}
}