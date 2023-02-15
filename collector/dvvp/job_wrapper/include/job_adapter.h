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
#include <map>
#include "message/prof_params.h"
#include "config/config.h"
#include "utils/utils.h"
#include "errno/error_code.h"
#include "ai_drv_dev_api.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::config;

enum class HostTimeType {
    START_REALTIME_BEGIN = 0,
    START_REALTIME_END,
    START_MONO_BEGIN,
    START_MONO_END,
    CNTVCT_BEGIN,
    CNTVCT_END
};

enum class DevTimeType {
    DEVICE_START_MONO = 0,
    DEVICE_CNTVCT
};

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
    JobAdapter() {}
    virtual ~JobAdapter()
    {
        hostTimeList_.clear();
        devTimeList_.clear();
    }

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

    void GetHostAndDeviceTime(int devIndexId)
    {
        std::map<unsigned long long,
                 std::pair<std::map<HostTimeType, unsigned long long>,
                           std::map<DevTimeType, unsigned long long>>> timeMap;
        const int getTimeCount = 3;
        for (int i = 0; i < getTimeCount; i++) {
            unsigned long long startMonoBegin;
            unsigned long long startRealtimebegin;
            unsigned long long cntvctbegin;
            unsigned long long startMonoEnd;
            unsigned long long startRealtimeEnd;
            unsigned long long cntvctEnd;
            unsigned long long deviceStartMono;
            unsigned long long deviceCntvct;
            Utils::GetTime(startRealtimebegin, startMonoBegin, cntvctbegin);
            analysis::dvvp::driver::DrvGetDeviceTime(devIndexId, deviceStartMono, deviceCntvct);
            Utils::GetTime(startRealtimeEnd, startMonoEnd, cntvctEnd);

            std::map<HostTimeType, unsigned long long> hostTimeList = {
                {HostTimeType::START_REALTIME_BEGIN, startRealtimebegin},
                {HostTimeType::START_REALTIME_END, startRealtimeEnd},
                {HostTimeType::START_MONO_BEGIN, startMonoBegin},
                {HostTimeType::START_MONO_END, startMonoEnd},
                {HostTimeType::CNTVCT_BEGIN, cntvctbegin},
                {HostTimeType::CNTVCT_END, cntvctEnd}
            };
            std::map<DevTimeType, unsigned long long> deviceTimeList = {
                {DevTimeType::DEVICE_START_MONO, deviceStartMono},
                {DevTimeType::DEVICE_CNTVCT, deviceCntvct}
            };
            timeMap.insert(std::make_pair(startRealtimeEnd - startRealtimebegin,
                std::make_pair(hostTimeList, deviceTimeList)));
        }
        hostTimeList_ = timeMap.begin()->second.first;
        devTimeList_ = timeMap.begin()->second.second;
    }

    std::string GenerateDevStartTime()
    {
        std::stringstream devStartData;
        devStartData << CLOCK_REALTIME_KEY << ": " << hostTimeList_.at(HostTimeType::START_REALTIME_BEGIN) << std::endl;
        devStartData << CLOCK_MONOTONIC_RAW_KEY << ": " << devTimeList_.at(DevTimeType::DEVICE_START_MONO) << std::endl;
        devStartData << CLOCK_CNTVCT_KEY << ": " << devTimeList_.at(DevTimeType::DEVICE_CNTVCT) << std::endl;
        return devStartData.str();
    }
    
    std::string GenerateHostStartTime()
    {
        std::stringstream devStartData;
        devStartData << CLOCK_REALTIME_KEY_START << ": " << hostTimeList_.at(HostTimeType::START_REALTIME_BEGIN) <<
                        std::endl;
        devStartData << CLOCK_MONOTONIC_RAW_KEY_START << ": " << hostTimeList_.at(HostTimeType::START_MONO_BEGIN) <<
                        std::endl;
        devStartData << CLOCK_CNTVCT_KEY_START << ": " << hostTimeList_.at(HostTimeType::CNTVCT_BEGIN) << std::endl;
        devStartData << CLOCK_REALTIME_KEY_END << ": " << hostTimeList_.at(HostTimeType::START_REALTIME_END) <<
                        std::endl;
        devStartData << CLOCK_MONOTONIC_RAW_KEY_END << ": " << hostTimeList_.at(HostTimeType::START_MONO_END) <<
                        std::endl;
        devStartData << CLOCK_CNTVCT_KEY_END << ": " << hostTimeList_.at(HostTimeType::CNTVCT_END) << std::endl;
        return devStartData.str();
    }

protected:
    std::map<HostTimeType, unsigned long long> hostTimeList_;
    std::map<DevTimeType, unsigned long long> devTimeList_;
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
