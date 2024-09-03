/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_usage_assembler.h
 * Description        : 组合host usage相关数据
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_HOST_USAGE_ASSEMBLER_H
#define ANALYSIS_APPLICATION_HOST_USAGE_ASSEMBLER_H

#include <vector>

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"

namespace Analysis {
namespace Application {
class HostUsageAssembler : public JsonAssembler {
public:
    HostUsageAssembler(const std::string &assembleName);
protected:
    std::vector<std::shared_ptr<TraceEvent>> res_;
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    virtual uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) = 0;
};

// 处理host cpu usage 数据
class CpuUsageAssembler : public HostUsageAssembler {
public:
    CpuUsageAssembler();
private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理host memory usage 数据
class MemUsageAssembler : public HostUsageAssembler {
public:
    MemUsageAssembler();
private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理host disk usage 数据
class DiskUsageAssembler : public HostUsageAssembler {
public:
    DiskUsageAssembler();
private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

// 处理host network usage 数据
class NetworkUsageAssembler : public HostUsageAssembler {
public:
    NetworkUsageAssembler();
private:
    uint8_t GenerateDataTrace(DataInventory &dataInventory, uint32_t pidMap) override;
};

}
}
#endif // ANALYSIS_APPLICATION_HOST_USAGE_ASSEMBLER_H
