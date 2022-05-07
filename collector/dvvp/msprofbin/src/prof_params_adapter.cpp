/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msprof bin params adapter
 * Author: ly
 * Create: 2020-12-10
 */

#include "prof_params_adapter.h"
#include <google/protobuf/util/json_util.h>
#include "errno/error_code.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "config/config.h"
#include "validation/param_validation.h"
#include "config/config_manager.h"
namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::utils;

ProfParamsAdapter::ProfParamsAdapter()
{
}

ProfParamsAdapter::~ProfParamsAdapter()
{
}

int ProfParamsAdapter::Init()
{
    return PROFILING_SUCCESS;
}

void ProfParamsAdapter::GenerateLlcEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params == nullptr || (params->hardware_mem.compare("on") != 0)) {
        return;
    }
    if (params->llc_profiling.empty()) {
        GenerateLlcDefEvents(params);
        return;
    }
    if (Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        if (params->llc_profiling.compare(LLC_PROFILING_CAPACITY) == 0) {
            params->llc_profiling_events = GenerateCapacityEvents();
        } else if (params->llc_profiling.compare(LLC_PROFILING_BANDWIDTH) == 0) {
            params->llc_profiling_events = GenerateBandwidthEvents();
        }
    } else if (Analysis::Dvvp::Common::Config::ConfigManager::instance()->IsDriverSupportLlc()) {
        if (params->llc_profiling.compare(LLC_PROFILING_READ) == 0) {
            params->llc_profiling_events = LLC_PROFILING_READ;
        } else if (params->llc_profiling.compare(LLC_PROFILING_WRITE) == 0) {
            params->llc_profiling_events = LLC_PROFILING_WRITE;
        }
    }
    if (params->llc_profiling_events.empty()) {
        MSPROF_LOGE("Does not support this llc profiling type : %s", Utils::BaseName(params->llc_profiling).c_str());
    }
}

void ProfParamsAdapter::GenerateLlcDefEvents(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        params->llc_profiling = LLC_PROFILING_CAPACITY;
        params->llc_profiling_events = GenerateCapacityEvents();
    } else if (Analysis::Dvvp::Common::Config::ConfigManager::instance()->IsDriverSupportLlc()) {
        params->llc_profiling = LLC_PROFILING_READ;
        params->llc_profiling_events = LLC_PROFILING_READ;
    } else {
        MSPROF_LOGW("The current platform does not support llc profiling.");
    }
}

std::string ProfParamsAdapter::GenerateCapacityEvents()
{
    std::vector<std::string> llcProfilingEvents;
    const int MAX_LLC_EVENTS = 8; // llc events list size
    for (int i = 0; i < MAX_LLC_EVENTS; i++) {
        std::string tempEvents;
        tempEvents.append("hisi_l3c0_1/dsid");
        tempEvents.append(std::to_string(i));
        tempEvents.append("/");
        llcProfilingEvents.push_back(tempEvents);
    }
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;
    return builder.Join(llcProfilingEvents, ",");
}

std::string ProfParamsAdapter::GenerateBandwidthEvents()
{
    std::vector<std::string> llcProfilingEvents;
    llcProfilingEvents.push_back("hisi_l3c0_1/read_allocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/read_hit/");
    llcProfilingEvents.push_back("hisi_l3c0_1/read_noallocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_allocate/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_hit/");
    llcProfilingEvents.push_back("hisi_l3c0_1/write_noallocate/");
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;
    return builder.Join(llcProfilingEvents, ",");
}

int ProfParamsAdapter::UpdateParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params == nullptr) {
        return PROFILING_FAILED;
    }
    if (params->io_profiling.compare("on") == 0) {
        params->nicProfiling = "on";
        params->nicInterval = params->io_sampling_interval;
        params->roceProfiling = "on";
        params->roceInterval = params->io_sampling_interval;
    }
    if (params->interconnection_profiling.compare("on") == 0) {
        params->pcieProfiling = "on";
        params->pcieInterval = params->interconnection_sampling_interval;
        params->hccsProfiling = "on";
        params->hccsInterval = params->interconnection_sampling_interval;
    }
    if (params->hardware_mem.compare("on") == 0) {
        params->msprof_llc_profiling = "on";
        params->llc_interval = params->hardware_mem_sampling_interval;
        params->ddr_profiling = "on";
        params->ddr_interval = params->hardware_mem_sampling_interval;
        params->hbmProfiling = "on";
        params->ddr_profiling_events = "read,write";
        params->hbm_profiling_events = "read,write";
        params->hbmInterval = params->hardware_mem_sampling_interval;
    }

    if (params->cpu_profiling.compare("on") == 0) {
        params->tsCpuProfiling = "on";
        params->aiCtrlCpuProfiling = "on";
        params->ai_ctrl_cpu_profiling_events = "0x8,0x11";
        params->ts_cpu_profiling_events = "0x8,0x11";
    }
    params->exportIterationId = DEFAULT_INTERATION_ID;
    params->exportModelId = DEFAULT_MODEL_ID;

    return PROFILING_SUCCESS;
}
}
}
}