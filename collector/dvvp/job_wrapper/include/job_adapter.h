/**
* @file job_adapter.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_JOB_ADAPTER_H
#define ANALYSIS_DVVP_JOB_ADAPTER_H

#include <vector>
#include "message/prof_params.h"
#include "utils/utils.h"
#include "errno/error_code.h"
#include "ai_drv_dev_api.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
struct PMUEventsConfig {
    std::vector<std::string> ctrlCPUEvents;
    std::vector<std::string> tsCPUEvents;
    std::vector<std::string> aiCoreEvents;
    std::vector<int> aiCoreEventsCoreIds;
    std::vector<std::string> llcEvents;
    std::vector<std::string> ddrEvents;
    std::vector<std::string> aivEvents;
    std::vector<std::string> hbmEvents;
    std::vector<int> aivEventsCoreIds;
};

class JobAdapter {
public:
    JobAdapter()
        : startMono_(0),
          startRealtime_(0),
          cntvct_(0),
          deviceStartMono_(0),
          deviceCntvct_(0) {}
    virtual ~JobAdapter() {}

public:
    virtual int StartProf(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params) = 0;
    virtual int StopProf(void) = 0;

public:
    virtual const analysis::dvvp::message::StatusInfo& GetLastStatus()
    {
        return status_;
    }
    SHARED_PTR_ALIA<PMUEventsConfig> CreatePmuEventConfig(
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params, int devId)
    {
        SHARED_PTR_ALIA<PMUEventsConfig> pmuEventsConfig = nullptr;
        MSVP_MAKE_SHARED0_RET(pmuEventsConfig, PMUEventsConfig, nullptr);
        pmuEventsConfig->ctrlCPUEvents = Utils::Split(params->ai_ctrl_cpu_profiling_events, false, "", ",");
        pmuEventsConfig->tsCPUEvents = Utils::Split(params->ts_cpu_profiling_events, false, "", ",");
        pmuEventsConfig->hbmEvents = Utils::Split(params->hbm_profiling_events, false, "", ",");
        pmuEventsConfig->aiCoreEvents = Utils::Split(params->ai_core_profiling_events, false, "", ",");
        if (!params->host_profiling) {
            int ret = RepackAicore(pmuEventsConfig->aiCoreEventsCoreIds, devId);
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("RepackAicore failed , jobId:%s, devId:%d", params->job_id.c_str(), devId);
            }
        }
        pmuEventsConfig->llcEvents = Utils::Split(params->llc_profiling_events, false, "", ",");
        pmuEventsConfig->ddrEvents = Utils::Split(params->ddr_profiling_events, false, "", ",");
        pmuEventsConfig->aivEvents = Utils::Split(params->aiv_profiling_events, false, "", ",");
        if (!params->host_profiling) {
            int ret = RepackAiv(pmuEventsConfig->aivEventsCoreIds, devId);
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("RepackAiv failed , jobId:%s, devId:%d", params->job_id.c_str(), devId);
            }
        }
        MSPROF_LOGI("CreatePmuEventConfig, aiCoreEventSize:%d, devId:%d", pmuEventsConfig->aiCoreEvents.size(), devId);
        return pmuEventsConfig;
    }
protected:
    unsigned long long startMono_;
    unsigned long long startRealtime_;
    unsigned long long cntvct_;
    unsigned long long deviceStartMono_;
    unsigned long long deviceCntvct_;
    analysis::dvvp::message::StatusInfo status_;

private:
    int RepackAicore(std::vector<int> &aiCores, int devId)
    {
        int64_t aiCoreNum = 0;
        if (analysis::dvvp::driver::DrvGetAiCoreNum(devId, aiCoreNum) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        RepackCoreId(aiCoreNum, aiCores);
        return PROFILING_SUCCESS;
    }

    int RepackAiv(std::vector<int> &aivCores, int devId)
    {
        int64_t aivNum = 0;
        if (analysis::dvvp::driver::DrvGetAivNum(devId, aivNum) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
        RepackCoreId(aivNum, aivCores);
        return PROFILING_SUCCESS;
    }

    void RepackCoreId(const int coreNum, std::vector<int> &aiCores)
    {
        int startCoreIndex = 0;
        for (int kk = 0; kk < coreNum; ++kk) {
            if (startCoreIndex >= coreNum) {
                break;
            }
            aiCores.push_back(startCoreIndex);
            startCoreIndex++;
        }
    }
};
}}}

#endif


