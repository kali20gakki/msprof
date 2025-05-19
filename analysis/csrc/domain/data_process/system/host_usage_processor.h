/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_usage_processor.h
 * Description        : 处理CpuUsage,MemUsage,DiskUsage,NetWorkUsage, Syscall表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_HOST_USAGE_PROCESSOR_H
#define ANALYSIS_DOMAIN_HOST_USAGE_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// start_time, cpu_no, usage
using OriCpuUsage = std::vector<std::tuple<uint64_t, std::string, double>>;

// start_time, usage
using OriMemUsage = std::vector<std::tuple<uint64_t, double>>;

// start_time, usage, disk_read, disk_write
using OriDiskUsage = std::vector<std::tuple<uint64_t, double, double, double>>;

// start_time, usage, speed
using OriNetworkUsage = std::vector<std::tuple<uint64_t, double, double>>;

// runtime_api_name, runtime_pid, runtime_tid, runtime_trans_start, runtime_trans_end
using OriSysCall = std::vector<std::tuple<std::string, uint64_t, uint64_t, uint64_t, uint64_t>>;

class HostUsageProcessor : public DataProcessor {
public:
    HostUsageProcessor() = default;
    explicit HostUsageProcessor(const std::string &profPath_, const std::string &processorName, DBInfo &&dbInfo);
    virtual ~HostUsageProcessor() = default;
protected:
    DBInfo usageDb_;
    std::string processorName_;
private:
    bool Process(DataInventory& dataInventory) override;
    virtual bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) = 0;
};

class HostCpuUsageProcessor : public HostUsageProcessor {
public:
    HostCpuUsageProcessor() = default;
    explicit HostCpuUsageProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class HostMemUsageProcessor : public HostUsageProcessor {
public:
    HostMemUsageProcessor() = default;
    explicit HostMemUsageProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class HostDiskUsageProcessor : public HostUsageProcessor {
public:
    HostDiskUsageProcessor() = default;
    explicit HostDiskUsageProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class HostNetworkUsageProcessor : public HostUsageProcessor {
public:
    HostNetworkUsageProcessor() = default;
    explicit HostNetworkUsageProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class OSRuntimeApiProcessor : public HostUsageProcessor {
public:
    OSRuntimeApiProcessor() = default;
    explicit OSRuntimeApiProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};
}
}

#endif // ANALYSIS_DOMAIN_HOST_USAGE_PROCESSOR_H
