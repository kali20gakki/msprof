/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_context.h
 * Description        : device侧上下文基类
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_DEVICE_CONTEXT_H
#define ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_DEVICE_CONTEXT_H
#include <cstdint>
#include <unordered_map>
#include "analysis/csrc/infrastructure/context/include/context.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
constexpr int DEFAULT_PMU_LENGTH = 8;

enum class AicMetricsEventsType {
    AIC_ARITHMETIC_UTILIZATION = 0,
    AIC_PIPE_UTILIZATION,
    AIC_PIPE_UTILIZATION_EXCT,
    AIC_MEMORY,
    AIC_MEMORY_L0,
    AIC_RESOURCE_CONFLICT_RATIO,
    AIC_MEMORY_UB,
    AIC_L2_CACHE,
    AIC_PIPELINE_EXECUTE_UTILIZATION,
    AIC_MEMORY_ACCESS,
    AIC_METRICS_UNKNOWN
};
const std::unordered_map<std::string, AicMetricsEventsType> aicMetricsMap = {
    {"ArithmeticUtilization", AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION},
    {"PipeUtilization", AicMetricsEventsType::AIC_PIPE_UTILIZATION},
    {"PipeUtilizationExct", AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT},
    {"Memory", AicMetricsEventsType::AIC_MEMORY},
    {"MemoryL0", AicMetricsEventsType::AIC_MEMORY_L0},
    {"ResourceConflictRatio", AicMetricsEventsType::AIC_RESOURCE_CONFLICT_RATIO},
    {"MemoryUB", AicMetricsEventsType::AIC_MEMORY_UB},
    {"L2Cache", AicMetricsEventsType::AIC_L2_CACHE},
    {"PipelineExecuteUtilization", AicMetricsEventsType::AIC_PIPELINE_EXECUTE_UTILIZATION},
    {"MemoryAccess", AicMetricsEventsType::AIC_MEMORY_ACCESS}
};

enum class AivMetricsEventsType {
    AIV_ARITHMETIC_UTILIZATION = 0,
    AIV_PIPE_UTILIZATION,
    AIV_MEMORY,
    AIV_MEMORY_L0,
    AIV_RESOURCE_CONFLICT_RATIO,
    AIV_MEMORY_UB,
    AIV_L2_CACHE,
    AIV_PIPELINE_EXECUTE_UTILIZATION,
    AIV_MEMORY_ACCESS,
    AIV_METRICS_UNKNOWN
};
const std::unordered_map<std::string, AivMetricsEventsType> aivMetricsMap = {
    {"ArithmeticUtilization", AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION},
    {"PipeUtilization", AivMetricsEventsType::AIV_PIPE_UTILIZATION},
    {"Memory", AivMetricsEventsType::AIV_MEMORY},
    {"MemoryL0", AivMetricsEventsType::AIV_MEMORY_L0},
    {"ResourceConflictRatio", AivMetricsEventsType::AIV_RESOURCE_CONFLICT_RATIO},
    {"MemoryUB", AivMetricsEventsType::AIV_MEMORY_UB},
    {"L2Cache", AivMetricsEventsType::AIV_L2_CACHE},
    {"PipelineExecuteUtilization", AivMetricsEventsType::AIV_PIPELINE_EXECUTE_UTILIZATION},
    {"MemoryAccess", AivMetricsEventsType::AIV_MEMORY_ACCESS}
};

enum class ProfilingMode {
    PROFILING_MODE_SAMPLE_BASED = 0, // "sample-based"
    PROFILING_MODE_TASK_BASED,  // "task-based"
    PROFILING_UNKOWN
};
const std::unordered_map<std::string, ProfilingMode> proflingMap = {
    {"sample-based", ProfilingMode::PROFILING_MODE_SAMPLE_BASED},
    {"task-based",   ProfilingMode::PROFILING_MODE_TASK_BASED}
};

struct DeviceInfo {
    uint32_t chipID;
    uint32_t deviceId;
    uint32_t aicFrequency;
    uint32_t aiCoreNum;
    uint32_t aivFrequency;
    uint32_t aivNum;
    double hwtsFrequency;
};

struct CpuInfo {
    double frequency{1000};
};

struct SampleInfo {
    // aic sample info
    bool aiCoreProfiling = false;
    AicMetricsEventsType aiCoreMetrics = AicMetricsEventsType::AIC_METRICS_UNKNOWN;
    std::string aiCoreProfilingEventsStr;
    std::vector<uint32_t> aiCoreProfilingEvents;
    uint32_t aiCoreProfilingCnt = 0;
    ProfilingMode aiCoreProfilingMode = ProfilingMode::PROFILING_UNKOWN;
    uint32_t aiCoreSamplingInterval = 0;

    // aiv sample info
    bool aivProfiling = false;
    AivMetricsEventsType aivMetrics = AivMetricsEventsType::AIV_METRICS_UNKNOWN;
    std::string aivProfilingEventsStr;
    std::vector<uint32_t> aivProfilingEvents;
    ProfilingMode aivProfilingMode = ProfilingMode::PROFILING_UNKOWN;
    uint32_t aivSamplingInterval = 0;

    // dynamic
    bool dynamic = false;

    SampleInfo() : aiCoreProfilingEvents(DEFAULT_PMU_LENGTH), aivProfilingEvents(DEFAULT_PMU_LENGTH) {}
};

struct DfxInfo {
    std::string stopAt;
};

struct HostStartLog {
    uint64_t clockMonotonicRaw{0};
    uint64_t cntVct{0};
    uint64_t cntVctDiff{0};
};

struct DeviceStartLog {
    uint64_t clockMonotonicRaw{0};
    uint64_t cntVct{0};
    uint64_t cntVctDiff{0};  // device.start.log没有该字段，填充值
};

struct DeviceContextInfo {
    std::string deviceFilePath;
    DeviceInfo deviceInfo;
    SampleInfo sampleInfo;
    DfxInfo dfxInfo;
    HostStartLog hostStartLog;
    DeviceStartLog deviceStart;
    CpuInfo cpuInfo;
};

class DeviceContext : public Infra::Context {
public:
    static DeviceContext& Instance();

    // getters
    void Getter(std::string &deviceFilePath) const { deviceFilePath = this->deviceContextInfo.deviceFilePath; };

    void Getter(DeviceInfo &deviceInfo) const { deviceInfo = this->deviceContextInfo.deviceInfo; };

    void Getter(SampleInfo &sampleInfo) const { sampleInfo = this->deviceContextInfo.sampleInfo; };

    void Getter(DfxInfo &dfxInfo) const { dfxInfo = this->deviceContextInfo.dfxInfo; };

    void Getter(HostStartLog &hostStartLog) const { hostStartLog = this->deviceContextInfo.hostStartLog; };

    void Getter(DeviceStartLog &deviceStart) const { deviceStart = this->deviceContextInfo.deviceStart; };
    void Getter(CpuInfo &cpuInfo) const { cpuInfo = this->deviceContextInfo.cpuInfo; };

    uint32_t GetChipID() const override { return deviceContextInfo.deviceInfo.chipID; }

    std::string GetDeviceFilePath() const { return this->deviceContextInfo.deviceFilePath; }

    const std::string &GetDfxStopAtName() const override { return deviceContextInfo.dfxInfo.stopAt; }

    bool Init(const std::string &devicePath);

    void SetStopAt(const std::string & stopAt) { deviceContextInfo.dfxInfo.stopAt = stopAt; }
private:
    DeviceContextInfo deviceContextInfo;
    bool isInitialized_; // 标记是否已初始化
    DeviceContext() : isInitialized_(false) { deviceContextInfo.deviceInfo = {0, 0, 0, 0, 0, 0, 0}; };
    ~DeviceContext() = default;
    bool GetInfoJson();
    bool GetSampleJson();
    bool GetDeviceStart();
    bool GetHostStart();
    bool GetCpuInfo();
};
std::vector<DataInventory> DeviceContextEntry(const char *targetDir, const char *stopAt);
std::vector<std::string> GetDeviceDirectories(const std::string &path);
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_DEVICE_CONTEXT_H