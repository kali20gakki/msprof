/* *
 * @file config_manager.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ANALYSIS_DVVP_COMMON_CONFIG_MANAGER_H
#define ANALYSIS_DVVP_COMMON_CONFIG_MANAGER_H

#include <string>
#include <map>
#include "mmpa_api.h"
#include "singleton/singleton.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "message/prof_params.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Config {

#define HELPER_DEVICE_TYPE MDC_TYPE
enum class PlatformType {
    MINI_TYPE = 0,
    CLOUD_TYPE,
    MDC_TYPE,
    LHISI_TYPE,
    DC_TYPE,
    CHIP_V4_1_0,
    END_TYPE
};
const std::map<PlatformType, std::string> FREQUENCY_TYPE = {
    {PlatformType::MINI_TYPE, "19.2"},
    {PlatformType::CLOUD_TYPE, "100"},
    {PlatformType::LHISI_TYPE, "24"},
    {PlatformType::DC_TYPE, "38.4"},
    {PlatformType::MDC_TYPE, "38.4"},
    {PlatformType::CHIP_V4_1_0, "100"}
};

const std::map<PlatformType, std::string> AIC_TYPE = {
    {PlatformType::MINI_TYPE, "680"},
    {PlatformType::CLOUD_TYPE, "800"},
    {PlatformType::LHISI_TYPE, "300"},
    {PlatformType::DC_TYPE, "1150"},
    {PlatformType::MDC_TYPE, "960"},
    {PlatformType::CHIP_V4_1_0, "800"}
};

const std::map<std::string, std::string> AICORE_METRICS_LIST = {
    {"ArithmeticUtilization", "0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f"},
    {"PipeUtilization", "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55"},
    {"Memory", "0x15,0x16,0x31,0x32,0xf,0x10,0x12,0x13"},
    {"MemoryL0", "0x1b,0x1c,0x21,0x22,0x27,0x28,0x29,0x2a"},
    {"ResourceConflictRatio", "0x64,0x65,0x66"},
    {"MemoryUB", "0x10,0x13,0x37,0x38,0x3d,0x3e,0x43,0x44"}
};
const std::map<std::string, std::string> AI_VECTOR_CORE_METRICS_LIST = {
    {"ArithmeticUtilization", "0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f"},
    {"PipeUtilization", "0x8,0xa,0x9,0xb,0xc,0xd,0x54,0x55"},
    {"Memory", "0x15,0x16,0x31,0x32,0xf,0x10,0x12,0x13"},
    {"MemoryL0", "0x1b,0x1c,0x21,0x22,0x27,0x28,0x29,0x2a"},
    {"ResourceConflictRatio", "0x64,0x65,0x66"},
    {"MemoryUB", "0x10,0x13,0x37,0x38,0x3d,0x3e,0x43,0x44"}
};

const std::map<PlatformType, std::string> L2_CACHE_PLATFORM_METRICS_MAP = {
    {PlatformType::CLOUD_TYPE, "HitRateAndVictimRateCloud"},
    {PlatformType::DC_TYPE, "HitRateAndVictimRate"},
    {PlatformType::MDC_TYPE, "HitRateAndVictimRate"}
};

const std::map<std::string, std::string> L2_CACHE_METRICS_EVENTS_MAP = {
    {"HitRateAndVictimRate", "0x78,0x79,0x77,0x71,0x6a,0x6c,0x74,0x62"},
    {"HitRateAndVictimRateCloud", "0x5b,0x59,0x5c,0x7d,0x7e,0x71,0x79,0x7c"}
};

class ConfigManager : public analysis::dvvp::common::singleton::Singleton<ConfigManager> {
public:
    ConfigManager();
    virtual ~ConfigManager();
    int Init();
    void Uninit();
    std::string GetFrequency() const;
    std::string GetChipIdStr() const;
    PlatformType GetPlatformType() const;
    std::string GetAicDefFrequency() const;
    bool IsDriverSupportLlc() const;
    std::string GetPerfDataDir(const int devId = 0) const;
    std::string GetDefaultWorkDir() const;
    int GetAicoreEvents(const std::string &aicoreMetricsType, std::string &aicoreEvents) const;
    int GetL2cacheEvents(std::string &l2CacheEvents) const;
    void MsprofL2CacheAdapter(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

private:
    void InitFrequency();

private:
    bool isInit_;
    std::map<std::string, std::string> configMap_;
};
}
}
}
}
#endif
