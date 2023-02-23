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
#include "task_relationship_mgr.h"
#include "message/codec.h"
#include "uploader_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::TaskHandle;

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
        : hostMonotonicStart_(0),
          hostCntvctStart_(0),
          hostCntvctDiff_(0),
          devMonotonic_(0),
          devCntvct_(0) {}
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

    void GetHostAndDeviceTime(int devIndexId)
    {
        static const int MULTIPLE = 4;
        static const int GET_TIME_COUNT = 3;
        std::vector<std::pair<unsigned long long, unsigned long long>> hostCntvcts;
        std::vector<unsigned long long> hostMonotonics;
        std::vector<std::pair<unsigned long long, unsigned long long>> devTime;
        
        unsigned long long deviceCntvct;
        unsigned long long devicemonotonic;
        unsigned long long hostCntvctStart;
        unsigned long long hostCntvctStop;
        unsigned long long hostMonotonicStart;
        for (int i = 0; i < GET_TIME_COUNT; i++) {
            hostMonotonicStart = Utils::GetClockMonotonicRaw();
            hostCntvctStart = Utils::GetCPUCycleCounter();
            analysis::dvvp::driver::DrvGetDeviceTime(devIndexId, devicemonotonic, deviceCntvct);
            hostCntvctStop = Utils::GetCPUCycleCounter();

            hostMonotonics.push_back(hostMonotonicStart);
            hostCntvcts.push_back(std::make_pair(hostCntvctStart, hostCntvctStop));
            devTime.push_back(std::make_pair(devicemonotonic, deviceCntvct));
        }
        int hostCntvctAddr = 0;
        std::map<unsigned long long, int> hostCntvctDiffList;
        for (auto iter : hostCntvcts) {
            hostCntvctDiffList.insert(std::make_pair(iter.second - iter.first, hostCntvctAddr++));
        }
        hostCntvctDiff_ = hostCntvctDiffList.begin()->first / MULTIPLE;
        hostCntvctStart_ = hostCntvcts[hostCntvctDiffList.begin()->second].first;
        hostMonotonicStart_ = hostMonotonics[hostCntvctDiffList.begin()->second];
        devCntvct_ = devTime[hostCntvctDiffList.begin()->second].second;
        devMonotonic_ = devTime[hostCntvctDiffList.begin()->second].first;
    }

    std::string GenerateDevStartTime()
    {
        std::stringstream devStartData;
        devStartData << CLOCK_MONOTONIC_RAW_KEY << ": " << devMonotonic_ << std::endl;
        devStartData << CLOCK_CNTVCT_KEY << ": " << devCntvct_ << std::endl;
        return devStartData.str();
    }
    
    std::string GenerateHostStartTime()
    {
        std::stringstream devStartData;
        devStartData << CLOCK_MONOTONIC_RAW_KEY << ": " << hostMonotonicStart_ << std::endl;
        devStartData << CLOCK_CNTVCT_KEY << ": " << hostCntvctStart_ << std::endl;
        devStartData << CLOCK_CNTVCT_KEY_DIFF << ": " << hostCntvctDiff_ << std::endl;
        return devStartData.str();
    }

    int StoreTime(const std::string &fileName, const std::string &startTime)
    {
        SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
        MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
        jobCtx->dev_id = std::to_string(devIndexId_);
        jobCtx->job_id = jobId_;
        analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
            analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);
        MSPROF_LOGI("[%s]storeTime.id: %s,fileName: %s", jobDeviceType_.c_str(), jobId_.c_str(), fileName.c_str());
        int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadFileData(jobId_,
            startTime, fileDataParams, jobCtx);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("[%s]Failed to upload data for %s", jobDeviceType_.c_str(), fileName.c_str());
        }
        return ret;
    }

    void GetAndStoreStartTime(int hostProfiling, std::string jobId, int devIndexId, std::string jobDeviceType)
    {
        if (hostProfiling) {
            return;
        }
        jobId_ = jobId;
        devIndexId_ = devIndexId;
        jobDeviceType_ = jobDeviceType;
        GetHostAndDeviceTime(devIndexId_);
        MSPROF_LOGI("devId:%d, hostMonotonicStart=%llu ns, hostCntvctStart=%llu ns, hostCntvctDiff=%llu, "
                    "devCntvct=%llu ns", hostMonotonicStart_, hostCntvctStart_, hostCntvctDiff_, devCntvct_);
        std::string deviceId = std::to_string(
            TaskRelationshipMgr::instance()->GetFlushSuffixDevId(jobId_, devIndexId_));
        std::stringstream timeData;
        timeData << "[" << std::string(DEVICE_TAG_KEY) << deviceId << "]" << std::endl;
        std::string startTime = GenerateHostStartTime();
        timeData << startTime;
        std::string fileName = "host_start.log." + deviceId;
        int ret = StoreTime(fileName, timeData.str());
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("[%s]Failed to upload data for %s", jobDeviceType_.c_str(), fileName.c_str());
            return;
        }
        fileName = "dev_start.log." + deviceId;
        startTime = GenerateDevStartTime();
        ret = StoreTime(fileName, startTime);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("[%s]Failed to upload data for %s", jobDeviceType_.c_str(), fileName.c_str());
            return;
        }
    }

protected:
    unsigned long long hostMonotonicStart_;
    unsigned long long hostCntvctStart_;
    unsigned long long hostCntvctDiff_;
    unsigned long long devMonotonic_;
    unsigned long long devCntvct_;
    int devIndexId_;
    std::string jobDeviceType_;
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
private:
    std::string jobId_;
};
}}}

#endif
