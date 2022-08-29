/**
* @file collection_register.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_JOB_WRAPPER_PROF_JOB_H
#define ANALYSIS_DVVP_JOB_WRAPPER_PROF_JOB_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "ai_drv_dev_api.h"
#include "ai_drv_prof_api.h"
#include "app/application.h"
#include "config/config.h"
#include "collection_register.h"
#include "collection_register.h"
#include "prof_timer.h"
#include "transport/prof_channel.h"
#include "transport/transport.h"
#include "transport/uploader.h"

using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::config;
namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
#define CHECK_JOB_EVENT_PARAM_RET(cfg, ret)  do {                                      \
    if ((cfg) == nullptr || (cfg)->comParams == nullptr ||                             \
        (cfg)->jobParams.events == nullptr || (cfg)->jobParams.events->size() == 0) {  \
        MSPROF_LOGI("Job check event param not pass");                                   \
        return ret;                                                                    \
    }                                                                                  \
} while (0)

#define CHECK_JOB_CONTEXT_PARAM_RET(cfg, ret)  do {                                        \
    if ((cfg) == nullptr || (cfg)->comParams == nullptr ||                                 \
        (cfg)->comParams->jobCtx == nullptr || (cfg)->comParams->params == nullptr) {      \
        MSPROF_LOGI("Job check context param failed");                                     \
        return ret;                                                                        \
    }                                                                                      \
} while (0)

#define CHECK_JOB_COMMON_PARAM_RET(cfg, ret) do {                                          \
    if ((cfg) == nullptr || (cfg)->comParams == nullptr) {                                 \
        MSPROF_LOGI("Job check comm param failed");                                        \
        return ret;                                                                        \
    }                                                                                      \
} while (0)

#define CHECK_JOB_CONFIG_UNSIGNED_SIZE_RET(size, ret) do {                             \
    if ((size) == 0 || (size) > 0x7FFFFFFF) {                                      \
        MSPROF_LOGE("Profiling Config Size Out Range");                               \
        return ret;                                                                    \
    }                                                                                  \
} while (0)

constexpr int DEFAULT_PERIOD_TIME = 10;

class PerfExtraTask : public analysis::dvvp::common::thread::Thread {
public:
    PerfExtraTask(unsigned int bufSize, const std::string &retDir,
                  SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                  SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param);
    ~PerfExtraTask() override;
    void SetJobCtx(SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx);
    int Init();
    int UnInit();

private:
    void Run(const struct error_message::Context &errorContext) override;
    void PerfScriptTask();
    void ResolvePerfRecordData(const std::string &fileName);
    void StoreData(const std::string &fileName);

private:
    volatile bool isInited_;
    long long dataSize_;
    std::string retDir_;
    analysis::dvvp::common::memory::Chunk buf_;
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param_;
};

class ProfDrvJob : public ICollectionJob {
public:
    ProfDrvJob();
    ~ProfDrvJob() override;

protected:
    void AddReader(const std::string &key, int devId, analysis::dvvp::driver::AI_DRV_CHANNEL channelId,
        const std::string &filePath);
    void RemoveReader(const std::string &key, int devId, analysis::dvvp::driver::AI_DRV_CHANNEL channelId);
    std::string GetEventsStr(const std::vector<std::string> &events, const std::string &separator = ",");
    unsigned int GetEventSize(const std::vector<std::string> &events);
    std::string BindFileWithChannel(const std::string &fileName) const;
    std::string GenerateFileName(const std::string &fileName, int devId);

protected:
    SHARED_PTR_ALIA<CollectionJobCfg> collectionJobCfg_;
};

class ProfCtrlcpuJob : public ICollectionJob {
public:
    ProfCtrlcpuJob();
    ~ProfCtrlcpuJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
    bool IsGlobalJobLevel() override
    {
        return true;
    }
private:
    int GetCollectCtrlCpuEventCmd(const std::vector<std::string> &events, std::string &profCtrlcpuCmd);
    int PrepareDataDir(std::string &file);

private:
    MmProcess ctrlcpuProcess_;
    SHARED_PTR_ALIA<CollectionJobCfg> collectionJobCfg_;
    SHARED_PTR_ALIA<PerfExtraTask> perfExtraTask_;
};

class ProfSysInfoBase : public ICollectionJob {
public:
    ProfSysInfoBase()
        : sampleIntervalNs_(0)
    {
    }
    ~ProfSysInfoBase() override {}

protected:
    SHARED_PTR_ALIA<CollectionJobCfg> collectionJobCfg_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader_;
    unsigned long long sampleIntervalNs_;
};

class ProfSysMemJob : public ProfSysInfoBase {
public:
    ProfSysMemJob();
    ~ProfSysMemJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
    bool IsGlobalJobLevel() override
    {
        return true;
    }
};

class ProfAllPidsJob : public ProfSysInfoBase {
public:
    ProfAllPidsJob();
    ~ProfAllPidsJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
    bool IsGlobalJobLevel() override
    {
        return true;
    }
};

class ProfSysStatJob : public ProfSysInfoBase {
public:
    ProfSysStatJob();
    ~ProfSysStatJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
    bool IsGlobalJobLevel() override
    {
        return true;
    }
};

class ProfTscpuJob : public ProfDrvJob {
public:
    ProfTscpuJob();
    ~ProfTscpuJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
};

class ProfTsTrackJob : public ProfDrvJob {
public:
    ProfTsTrackJob();
    ~ProfTsTrackJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

protected:
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
};

class ProfStarsSocLogJob : public ProfTsTrackJob {
public:
    ProfStarsSocLogJob();
    ~ProfStarsSocLogJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

protected:
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
};

class ProfStarsBlockLogJob : public ProfTsTrackJob {
public:
    ProfStarsBlockLogJob();
    ~ProfStarsBlockLogJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

protected:
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
};

class ProfFftsProfileJob : public ProfTsTrackJob {
public:
    ProfFftsProfileJob();
    ~ProfFftsProfileJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

protected:
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
    uint32_t cfgMode_;
    uint32_t aicMode_;
    uint32_t aivMode_;
    int aicPeriod_;
    int aivPeriod_;
};

class ProfAivTsTrackJob : public ProfTsTrackJob {
public:
    ProfAivTsTrackJob();
    ~ProfAivTsTrackJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};

class ProfAicoreJob : public ProfDrvJob {
public:
    ProfAicoreJob();
    ~ProfAicoreJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

protected:
    int period_;
    std::string taskType_;
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;

private:
    SHARED_PTR_ALIA<CollectionJobCfg> aiCoreJobConfig_;
};

class ProfAivJob : public ProfAicoreJob {
public:
    ProfAivJob();
    ~ProfAivJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;

private:
    SHARED_PTR_ALIA<CollectionJobCfg> aiCoreJobConfig_;
};

class ProfAicoreTaskBasedJob : public ProfDrvJob {
public:
    ProfAicoreTaskBasedJob();
    ~ProfAicoreTaskBasedJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
protected:
    std::string taskType_;
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
};

class ProfAivTaskBasedJob : public ProfAicoreTaskBasedJob {
public:
    ProfAivTaskBasedJob();
    ~ProfAivTaskBasedJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};

class ProfFmkJob : public ProfDrvJob {
public:
    ProfFmkJob();
    ~ProfFmkJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
};

class ProfHwtsLogJob : public ProfDrvJob {
public:
    ProfHwtsLogJob();
    ~ProfHwtsLogJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

protected:
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
};

class ProfAivHwtsLogJob : public ProfHwtsLogJob {
public:
    ProfAivHwtsLogJob();
    ~ProfAivHwtsLogJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};

class ProfL2CacheTaskJob : public ProfDrvJob {
public:
    ProfL2CacheTaskJob();
    ~ProfL2CacheTaskJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
};

class ProfBiuPerfJob : public ProfDrvJob {
public:
    ProfBiuPerfJob();
    ~ProfBiuPerfJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

protected:
    uint32_t sampleCycle_;
    std::vector<uint32_t> groupIds_;
    AI_DRV_CHANNEL groupChannelIdMap_[BIU_GROUP_MAX_NUM][BIU_GROUP_CHANNEL_NUM] = {
        {PROF_CHANNEL_BIU_GROUP0_AIC, PROF_CHANNEL_BIU_GROUP0_AIV0, PROF_CHANNEL_BIU_GROUP0_AIV1},
        {PROF_CHANNEL_BIU_GROUP1_AIC, PROF_CHANNEL_BIU_GROUP1_AIV0, PROF_CHANNEL_BIU_GROUP1_AIV1},
        {PROF_CHANNEL_BIU_GROUP2_AIC, PROF_CHANNEL_BIU_GROUP2_AIV0, PROF_CHANNEL_BIU_GROUP2_AIV1},
        {PROF_CHANNEL_BIU_GROUP3_AIC, PROF_CHANNEL_BIU_GROUP3_AIV0, PROF_CHANNEL_BIU_GROUP3_AIV1},
        {PROF_CHANNEL_BIU_GROUP4_AIC, PROF_CHANNEL_BIU_GROUP4_AIV0, PROF_CHANNEL_BIU_GROUP4_AIV1},
        {PROF_CHANNEL_BIU_GROUP5_AIC, PROF_CHANNEL_BIU_GROUP5_AIV0, PROF_CHANNEL_BIU_GROUP5_AIV1},
        {PROF_CHANNEL_BIU_GROUP6_AIC, PROF_CHANNEL_BIU_GROUP6_AIV0, PROF_CHANNEL_BIU_GROUP6_AIV1},
        {PROF_CHANNEL_BIU_GROUP7_AIC, PROF_CHANNEL_BIU_GROUP7_AIV0, PROF_CHANNEL_BIU_GROUP7_AIV1},
        {PROF_CHANNEL_BIU_GROUP8_AIC, PROF_CHANNEL_BIU_GROUP8_AIV0, PROF_CHANNEL_BIU_GROUP8_AIV1},
        {PROF_CHANNEL_BIU_GROUP9_AIC, PROF_CHANNEL_BIU_GROUP9_AIV0, PROF_CHANNEL_BIU_GROUP9_AIV1},
        {PROF_CHANNEL_BIU_GROUP10_AIC, PROF_CHANNEL_BIU_GROUP10_AIV0, PROF_CHANNEL_BIU_GROUP10_AIV1},
        {PROF_CHANNEL_BIU_GROUP11_AIC, PROF_CHANNEL_BIU_GROUP11_AIV0, PROF_CHANNEL_BIU_GROUP11_AIV1},
        {PROF_CHANNEL_BIU_GROUP12_AIC, PROF_CHANNEL_BIU_GROUP12_AIV0, PROF_CHANNEL_BIU_GROUP12_AIV1},
        {PROF_CHANNEL_BIU_GROUP13_AIC, PROF_CHANNEL_BIU_GROUP13_AIV0, PROF_CHANNEL_BIU_GROUP13_AIV1},
        {PROF_CHANNEL_BIU_GROUP14_AIC, PROF_CHANNEL_BIU_GROUP14_AIV0, PROF_CHANNEL_BIU_GROUP14_AIV1},
        {PROF_CHANNEL_BIU_GROUP15_AIC, PROF_CHANNEL_BIU_GROUP15_AIV0, PROF_CHANNEL_BIU_GROUP15_AIV1},
        {PROF_CHANNEL_BIU_GROUP16_AIC, PROF_CHANNEL_BIU_GROUP16_AIV0, PROF_CHANNEL_BIU_GROUP16_AIV1},
        {PROF_CHANNEL_BIU_GROUP17_AIC, PROF_CHANNEL_BIU_GROUP17_AIV0, PROF_CHANNEL_BIU_GROUP17_AIV1},
        {PROF_CHANNEL_BIU_GROUP18_AIC, PROF_CHANNEL_BIU_GROUP18_AIV0, PROF_CHANNEL_BIU_GROUP18_AIV1},
        {PROF_CHANNEL_BIU_GROUP19_AIC, PROF_CHANNEL_BIU_GROUP19_AIV0, PROF_CHANNEL_BIU_GROUP19_AIV1},
        {PROF_CHANNEL_BIU_GROUP20_AIC, PROF_CHANNEL_BIU_GROUP20_AIV0, PROF_CHANNEL_BIU_GROUP20_AIV1},
        {PROF_CHANNEL_BIU_GROUP21_AIC, PROF_CHANNEL_BIU_GROUP21_AIV0, PROF_CHANNEL_BIU_GROUP21_AIV1},
        {PROF_CHANNEL_BIU_GROUP22_AIC, PROF_CHANNEL_BIU_GROUP22_AIV0, PROF_CHANNEL_BIU_GROUP22_AIV1},
        {PROF_CHANNEL_BIU_GROUP23_AIC, PROF_CHANNEL_BIU_GROUP23_AIV0, PROF_CHANNEL_BIU_GROUP23_AIV1},
        {PROF_CHANNEL_BIU_GROUP24_AIC, PROF_CHANNEL_BIU_GROUP24_AIV0, PROF_CHANNEL_BIU_GROUP24_AIV1}
    };
};
}}}
#endif
