/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_usage_processor.h
 * Description        : 处理CpuUsage,MemUsage,DiskUsage,NetWorkUsage表相关数据
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
using OriTimeUsage = std::vector<std::tuple<uint64_t, double>>;

class HostUsageProcessor : public DataProcessor {
public:
    HostUsageProcessor() = default;
    explicit HostUsageProcessor(const std::string &profPath_, DBInfo &&dbInfo);
    virtual ~HostUsageProcessor() = default;
protected:
    DBInfo usageDb_;
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
private:
    std::string processorName_;
};

class HostMemUsageProcessor : public HostUsageProcessor {
public:
    HostMemUsageProcessor() = default;
    explicit HostMemUsageProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
private:
    std::string processorName_;
};

class HostDiskUsageProcessor : public HostUsageProcessor {
public:
    HostDiskUsageProcessor() = default;
    explicit HostDiskUsageProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
private:
    std::string processorName_;
};

class HostNetworkUsageProcessor : public HostUsageProcessor {
public:
    HostNetworkUsageProcessor() = default;
    explicit HostNetworkUsageProcessor(const std::string &profPath_);
private:
    bool ProcessData(DataInventory& dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
private:
    std::string processorName_;
};
}
}

#endif // ANALYSIS_DOMAIN_HOST_USAGE_PROCESSOR_H
