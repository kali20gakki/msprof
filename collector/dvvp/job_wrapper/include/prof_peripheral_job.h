/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_JOB_WRAPPER_PROF_PERIPHERAL_JOB_H
#define ANALYSIS_DVVP_JOB_WRAPPER_PROF_PERIPHERAL_JOB_H
#include "prof_job.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
class ProfPeripheralJob : public ProfDrvJob {
public:
    ProfPeripheralJob();
    ~ProfPeripheralJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    virtual int SetPeripheralConfig();
    int Process() override;
    int Uninit() override;

protected:
    uint32_t samplePeriod_;
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
    analysis::dvvp::driver::DrvPeripheralProfileCfg peripheralCfg_;
    std::string eventsStr_;
};

class ProfDdrJob : public ProfPeripheralJob {
public:
    ProfDdrJob();
    ~ProfDdrJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int SetPeripheralConfig() override;
};

class ProfHbmJob : public ProfPeripheralJob {
public:
    ProfHbmJob();
    ~ProfHbmJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int SetPeripheralConfig() override;
};

class ProfHccsJob : public ProfPeripheralJob {
public:
    ProfHccsJob();
    ~ProfHccsJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};

class ProfPcieJob : public ProfPeripheralJob {
public:
    ProfPcieJob();
    ~ProfPcieJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};

class ProfNicJob : public ProfPeripheralJob {
public:
    ProfNicJob();
    ~ProfNicJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};

class ProfDvppJob : public ProfPeripheralJob {
public:
    ProfDvppJob();
    ~ProfDvppJob() override;
    int Process() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Uninit() override;

private:
    std::vector<analysis::dvvp::driver::AI_DRV_CHANNEL> channelList_;
    std::map<analysis::dvvp::driver::AI_DRV_CHANNEL, std::string> fileNameList_;
};

class ProfLlcJob : public ProfPeripheralJob {
public:
    ProfLlcJob();
    ~ProfLlcJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int SetPeripheralConfig() override;
    int Process() override;
    int Uninit() override;

public:
    bool IsGlobalJobLevel() override;

private:
    int GetCollectLlcEventsCmd(int devId, const std::vector<std::string> &events,
                                   std::string &profLlcCmd);
    void SendData();

private:
    MmProcess llcProcess_;
};

class ProfRoceJob : public ProfPeripheralJob {
public:
    ProfRoceJob();
    ~ProfRoceJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};

class ProfStarsSocProfileJob : public ProfPeripheralJob {
public:
    ProfStarsSocProfileJob();
    ~ProfStarsSocProfileJob() override;
    int SetPeripheralConfig() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
};
}
}
}

#endif
