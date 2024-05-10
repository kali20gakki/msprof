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

namespace Analysis {
namespace Domain {
extern thread_local std::string g_deviceFilePath;

constexpr uint32_t MIN_SUB_DIR_NBAME_LEN = 6;
enum class AicMetricsEventsType {
    AIC_ARITHMETIC_UTILIZATION = 0,
    AIC_PIPE_UTILIZATION,
    AIC_MEMORY,
    AIC_MEMORY_L0,
    AIC_RESOURCE_CONFLICT_RATIO,
    AIC_MEMORY_UB,
    AIC_L2_CACHE,
    AIC_PIPELINE_EXECUTE_UTILIZATION,
    AIC_METRICS_UNKNOWN
};
const std::unordered_map<std::string, AicMetricsEventsType> aicMetricsMap = {
    {"ArithmeticUtilization", AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION},
    {"PipeUtilization", AicMetricsEventsType::AIC_PIPE_UTILIZATION},
    {"Memory", AicMetricsEventsType::AIC_MEMORY},
    {"MemoryL0", AicMetricsEventsType::AIC_MEMORY_L0},
    {"ResourceConflictRatio", AicMetricsEventsType::AIC_RESOURCE_CONFLICT_RATIO},
    {"MemoryUB", AicMetricsEventsType::AIC_MEMORY_UB},
    {"L2Cache", AicMetricsEventsType::AIC_L2_CACHE},
    {"PipelineExecuteUtilization", AicMetricsEventsType::AIC_PIPELINE_EXECUTE_UTILIZATION}
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
    {"PipelineExecuteUtilization", AivMetricsEventsType::AIV_PIPELINE_EXECUTE_UTILIZATION}
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
};

struct SampleInfo {
    // aic sample info
    bool aiCoreProfiling;
    AicMetricsEventsType aiCoreMetrics;
    std::string aiCoreProfilingEventsStr;
    uint32_t aiCoreProfilingEvents[8];
    uint32_t aiCoreProfilingCnt;
    ProfilingMode aiCoreProfilingMode;
    uint32_t aiCoreSamplingInterval;

    // aiv sample info
    bool aivProfiling;
    AivMetricsEventsType aivMetrics;
    std::string aivProfilingEventsStr;
    uint32_t aivProfilingEvents[8];
    ProfilingMode aivProfilingMode;
    uint32_t aivSamplingInterval;

    // 910B芯片MIX算子是否开启block模式
    bool taskBlock;
};

struct DfxInfo {
    std::string stopAt;
};

struct DeviceContextInfo {
    std::string deviceFilePath;
    DeviceInfo deviceInfo;
    SampleInfo sampleInfo;
    DfxInfo dfxInfo;
};

class DeviceContext : public Infra::Context {
public:
    static DeviceContext& Instance();

    // getters
    void Getter(std::string &deviceFilePath) const;

    void Getter(DeviceInfo &deviceInfo) const;

    void Getter(SampleInfo &sampleInfo) const;

    void Getter(DfxInfo &dfxInfo) const;

    uint32_t GetChipID() const override { return deviceContextInfo.deviceInfo.chipID; }

    std::string GetDeviceFilePath() const { return this->deviceContextInfo.deviceFilePath; }

    const std::string &GetDfxStopAtName() const override { return deviceContextInfo.dfxInfo.stopAt; }
private:
    DeviceContextInfo deviceContextInfo;
    bool isInitialized_; // 标记是否已初始化
    DeviceContext() : isInitialized_(false) {};
    ~DeviceContext() = default;
    bool GetInfoJson();
    bool GetSampleJson();
};

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_DEVICE_CONTEXT_H
