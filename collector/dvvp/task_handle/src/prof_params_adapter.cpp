/**
* @file prof_params_adapter.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "prof_params_adapter.h"
#include "errno/error_code.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "validation/param_validation.h"
#include "prof_acl_api.h"

namespace Analysis {
namespace Dvvp {
namespace Host {
namespace Adapter {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::utils;
ProfParamsAdapter::ProfParamsAdapter() {}

ProfParamsAdapter::~ProfParamsAdapter() {}

int ProfParamsAdapter::Init()
{
    return PROFILING_SUCCESS;
}

void ProfParamsAdapter::ProfStartCfgToParamsCfg(const uint64_t dataTypeConfig,
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    // ts_memcpy
    if (dataTypeConfig & PROF_TASK_TIME_L1_MASK) {
        params->ts_memcpy = MSVP_PROF_ON;
    }
    params->ts_keypoint = MSVP_PROF_ON;
    PlatformType type = ConfigManager::instance()->GetPlatformType();
    if (type == PlatformType::CHIP_V4_1_0 || type == PlatformType::CHIP_V4_2_0) {
        params->stars_acsq_task = MSVP_PROF_ON;
    }
}

void ProfParamsAdapter::UpdateHardwareMemParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams)
{
    if (dstParams->hardware_mem.compare("on") == 0) {
        dstParams->llc_profiling = "on";
        dstParams->msprof_llc_profiling = "on";
        dstParams->llc_profiling_events = srcParams->llc_profiling_events;
        dstParams->llc_interval = dstParams->hardware_mem_sampling_interval;
        dstParams->ddr_profiling = "on";
        dstParams->ddr_profiling_events = srcParams->ddr_profiling_events;
        dstParams->ddr_interval = dstParams->hardware_mem_sampling_interval;
        dstParams->ddr_master_id = srcParams->ddr_master_id;
        if (Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPlatformType() != PlatformType::MINI_TYPE) {
            dstParams->hbmProfiling = "on";
            dstParams->hbm_profiling_events = srcParams->hbm_profiling_events;
            dstParams->hbmInterval = dstParams->hardware_mem_sampling_interval;
        }
    }
}

void ProfParamsAdapter::SetSystemTraceParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams)
{
    if (dstParams == nullptr || srcParams == nullptr) {
        MSPROF_LOGE("params check failed, is nullptr!");
        return;
    }
    MSPROF_LOGI("SetSystemTraceParams, profiling_options: %s", srcParams->profiling_options.c_str());
    UpdateCpuProfiling(dstParams, srcParams);
    if (dstParams->io_profiling.compare("on") == 0) {
        dstParams->nicProfiling = "on";
        dstParams->nicInterval = dstParams->io_sampling_interval;
        if (Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPlatformType() != PlatformType::MINI_TYPE) {
            dstParams->roceProfiling = "on";
            dstParams->roceInterval = dstParams->io_sampling_interval;
        }
    }
    if (dstParams->interconnection_profiling.compare("on") == 0) {
        if (Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPlatformType() != PlatformType::MINI_TYPE) {
            dstParams->pcieProfiling = "on";
            dstParams->pcieInterval = dstParams->interconnection_sampling_interval;
            dstParams->hccsProfiling = "on";
            dstParams->hccsInterval = dstParams->interconnection_sampling_interval;
        }
    }
    dstParams->msprof = srcParams->msprof;
    dstParams->app = srcParams->app;
    UpdateHardwareMemParams(dstParams, srcParams);
    MSPROF_LOGI("SetSystemTraceParams, print updated dstParams");
    if (!dstParams->is_cancel) {
        dstParams->PrintAllFields();
    }
}

void ProfParamsAdapter::UpdateCpuProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> dstParams,
                                           SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> srcParams)
{
    if (dstParams == nullptr || srcParams == nullptr) {
        return;
    }
    if (dstParams->cpu_profiling.compare("on") == 0) {
        dstParams->tsCpuProfiling = "on";
        dstParams->ts_cpu_profiling_events = srcParams->ts_cpu_profiling_events;
        dstParams->aiCtrlCpuProfiling = "on";
        dstParams->ai_ctrl_cpu_profiling_events = srcParams->ai_ctrl_cpu_profiling_events;
    }
}

void ProfParamsAdapter::GenerateLlcEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    MSPROF_LOGI("Begin to GenerateLlcEvents");
    if (params == nullptr || params->msprof.compare("on") == 0) {
        return;
    }
    if (params->llc_profiling.empty()) {
        GenerateLlcDefEvents(params);
        return;
    }
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        if (params->llc_profiling.compare(LLC_PROFILING_CAPACITY) == 0) {
            params->llc_profiling_events = GenerateCapacityEvents();
        } else if (params->llc_profiling.compare(LLC_PROFILING_BANDWIDTH) == 0) {
            params->llc_profiling_events = GenerateBandwidthEvents();
        }
    } else if (ConfigManager::instance()->IsDriverSupportLlc()) {
        if (params->llc_profiling.compare(LLC_PROFILING_READ) == 0) {
            params->llc_profiling_events = LLC_PROFILING_READ;
        } else if (params->llc_profiling.compare(LLC_PROFILING_WRITE) == 0) {
            params->llc_profiling_events = LLC_PROFILING_WRITE;
        }
    }
    if (params->llc_profiling_events.empty()) {
        MSPROF_LOGE("[GenerateLlcEvents]Does not support this llc profiling type : %s", params->llc_profiling.c_str());
    }
}

void ProfParamsAdapter::GenerateLlcDefEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        params->llc_profiling = LLC_PROFILING_CAPACITY;
        params->llc_profiling_events = GenerateCapacityEvents();
    } else if (ConfigManager::instance()->IsDriverSupportLlc()) {
        params->llc_profiling = LLC_PROFILING_READ;
        params->llc_profiling_events = LLC_PROFILING_READ;
    } else {
        MSPROF_LOGW("[GenerateLlcDefEvents]The current platform does not support llc profiling.");
    }
}

std::string ProfParamsAdapter::GenerateCapacityEvents()
{
    const int MAX_LLC_EVENTS = 8; // llc events list size
    std::vector<std::string> llcProfilingEvents;
    for (int i = 0; i < MAX_LLC_EVENTS; i++) {
        std::string tempEvents;
        tempEvents.append("hisi_l3c0_1/dsid");
        tempEvents.append(std::to_string(i));
        tempEvents.append("/");
        llcProfilingEvents.push_back(tempEvents);
    }
    UtilsStringBuilder<std::string> builder;
    return builder.Join(llcProfilingEvents, ",");
}

std::string ProfParamsAdapter::GenerateBandwidthEvents()
{
    MSPROF_LOGI("Begin to GenerateBandwidthEvents");
    std::vector<std::string> llcProfilingEvents;
    llcProfilingEvents.push_back("hisi_l3c0_1/read_allocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/read_hit/");
    llcProfilingEvents.push_back("hisi_l3c0_1/read_noallocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_allocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_hit/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_noallocate/");

    UtilsStringBuilder<std::string> builder;
    return builder.Join(llcProfilingEvents, ",");
}
}   // Adaptor
}   // Host
}   // Dvvp
}   // Analysis
