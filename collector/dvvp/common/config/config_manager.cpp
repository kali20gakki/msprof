/* *
 * @file config_manager.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "config_manager.h"
#include <string>
#include "message/prof_params.h"
#include "singleton/singleton.h"
#include "utils/utils.h"
#include "errno/error_code.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "driver_plugin.h"
#include "ai_drv_dev_api.h"
namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Config {
static const std::string TYPE_CONFIG = "type";
static const std::string FRQ_CONFIG = "frq";
static const std::string AIC_CONFIG = "aicFrq";
static const std::string INOTIFY_CFG_PATH_STR = "/var/log/npu/profiling/";

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;

ConfigManager::ConfigManager()
    : isInit_(false)
{
    Init();
}
ConfigManager::~ConfigManager()
{
    Uninit();
}

int ConfigManager::Init()
{
    if (isInit_) {
        MSPROF_LOGD("ConfigManager has been inited");
        return PROFILING_SUCCESS;
    }
    int64_t versionInfo = 0;
    uint32_t chipId = 0;
    int ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(0, MODULE_TYPE_SYSTEM, INFO_TYPE_VERSION, &versionInfo);
    if (ret != DRV_ERROR_NONE) {
        if (ret == MSPROF_HELPER_HOST) {
            chipId = static_cast<uint64_t>(PlatformType::HELPER_DEVICE_TYPE);
        } else {
            MSPROF_LOGE("halGetDeviceInfo get device type version failed , ret:%d", ret);
            MSPROF_CALL_ERROR("EK9999", "halGetDeviceInfo get device type version failed , ret:%d", ret);
            return PROFILING_FAILED;
        }
    } else {
        chipId = ((static_cast<uint64_t>(versionInfo) >> 8) & 0xff); // 8:shift 8 bits, get the low 8 bits(0xff)
        if (chipId >= static_cast<uint32_t>(PlatformType::END_TYPE)) {
            MSPROF_LOGE("halGetDeviceInfo get chip invalid, versionInfo:0x%llx, chipId:%u", versionInfo, chipId);
            return PROFILING_FAILED;
        }
    }
    configMap_[TYPE_CONFIG] = std::to_string(chipId);
    InitFrequency();
    isInit_ = true;
    return PROFILING_SUCCESS;
}

void ConfigManager::Uninit()
{
    if (isInit_) {
        isInit_ = false;
    }
}

int ConfigManager::GetAicoreEvents(const std::string &aicoreMetricsType, std::string &aicoreEvents) const
{
    if (aicoreMetricsType.empty()) {
        return PROFILING_FAILED;
    }
    auto iter = AICORE_METRICS_LIST.find(aicoreMetricsType);
    if (iter != AICORE_METRICS_LIST.end()) {
        aicoreEvents = iter->second;
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGE("Invalid metrics type %s", aicoreMetricsType.c_str());
    std::string errReason = "aic_metrics type should be in [";
    for (auto aiCoreType : AICORE_METRICS_LIST) {
        errReason += aiCoreType.first;
        errReason += "|";
    }
    if (errReason[errReason.size() - 1] == '|') {
        errReason.replace(errReason.size() - 1, 1, "]");
    }
    MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
        std::vector<std::string>({"aic_metrics", aicoreMetricsType, errReason}));
    return PROFILING_FAILED;
}

int ConfigManager::GetL2cacheEvents(std::string &l2CacheEvents)
{
    PlatformType platformType = GetPlatformType();
    auto iter_platform_metric = L2_CACHE_PLATFORM_METRICS_MAP.find(platformType);
    if (iter_platform_metric != L2_CACHE_PLATFORM_METRICS_MAP.end()) {
        std::string l2CacheMetrics = iter_platform_metric->second;
        auto iter_metric_events = L2_CACHE_METRICS_EVENTS_MAP.find(l2CacheMetrics);
        if (iter_metric_events != L2_CACHE_METRICS_EVENTS_MAP.end()) {
            l2CacheEvents = iter_metric_events->second;
            return PROFILING_SUCCESS;
        }
        return PROFILING_FAILED;
    }
    MSPROF_LOGE("Invalid platform type %d for l2 cache", static_cast<int>(platformType));
    return PROFILING_FAILED;
}

void ConfigManager::MsprofL2CacheAdapter(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params->l2CacheTaskProfiling != MSVP_PROF_ON) {
        params->l2CacheTaskProfiling = "off";
        return;
    }
    int ret = ConfigManager::instance()->GetL2cacheEvents(params->l2CacheTaskProfilingEvents);
    if (ret != PROFILING_SUCCESS) {
        params->l2CacheTaskProfiling = "off";
        MSPROF_LOGW("The platform is invalid for l2 cache, l2 cache data will not be collected");
    } else {
        params->l2CacheTaskProfiling = MSVP_PROF_ON;
    }
}

std::string ConfigManager::GetFrequency() const
{
    std::string frequency;
    auto iter = configMap_.find(FRQ_CONFIG);
    if (iter != configMap_.end()) {
        frequency = iter->second;
    }
    return frequency;
}

std::string ConfigManager::GetAicDefFrequency() const
{
    std::string frequency;
    auto iter = configMap_.find(AIC_CONFIG);
    if (iter != configMap_.end()) {
        frequency = iter->second;
    }
    return frequency;
}

void ConfigManager::InitFrequency()
{
    std::string frequency;
    std::string aicFrq;
    std::string type = configMap_[TYPE_CONFIG];
    auto platType  = static_cast<PlatformType>(std::stoi(type));
    auto iterator = FREQUENCY_TYPE.find(platType);
    if (iterator != FREQUENCY_TYPE.end()) {
        frequency = iterator->second;
    }
    auto iter = AIC_TYPE.find(platType);
    if (iter != AIC_TYPE.end()) {
        aicFrq = iter->second;
    }
    configMap_[AIC_CONFIG] = aicFrq;
    configMap_[FRQ_CONFIG] = frequency;
}

std::string ConfigManager::GetChipIdStr()
{
    auto iter =  configMap_.find(TYPE_CONFIG);
    if (iter != configMap_.end()) {
        return iter->second;
    }
    return std::to_string(static_cast<uint32_t>(PlatformType::MINI_TYPE));
}

PlatformType ConfigManager::GetPlatformType() const
{
    auto iter =  configMap_.find(TYPE_CONFIG);
    if (iter != configMap_.end()) {
        auto type = (PlatformType)(std::stoi(iter->second));
        return type;
    }
    return PlatformType::MINI_TYPE;
}

bool ConfigManager::IsDriverSupportLlc()
{
    PlatformType type = GetPlatformType();
    if (type == PlatformType::CLOUD_TYPE || type == PlatformType::DC_TYPE ||
        type == PlatformType::MDC_TYPE || type == PlatformType::CHIP_V4_1_0) {
        return true;
    }
    return false;
}

std::string ConfigManager::GetPerfDataDir(const int devId) const
{
    std::string perfDataDir = INOTIFY_CFG_PATH_STR + std::to_string(devId);
    return perfDataDir;
}

std::string ConfigManager::GetDefaultWorkDir() const
{
    return std::string(INOTIFY_CFG_PATH_STR);
}
}
}
}
}
