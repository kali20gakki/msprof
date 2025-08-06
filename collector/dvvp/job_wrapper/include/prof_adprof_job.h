/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: adprof job header
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-07-22
 */
#ifndef PROF_ADPROF_JOB_H
#define PROF_ADPROF_JOB_H

#include <atomic>
#include "tsdclient/tsdclient_api.h"
#include "prof_job.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {

class ProfAdprofJob : public ProfDrvJob {
public:
    ProfAdprofJob();
    ~ProfAdprofJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

private:
    int32_t InitAdprof();
    void BuildSysProfCmdArg(ProcOpenArgs &procOpenArgs);
    void CloseAdprof();

private:
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
    ProcStatusParam procStatusParam_;
    struct TaskEventAttr eventAttr_;
    std::atomic<uint8_t> processCount_;
    uint32_t phyId_;
    ProfDrvEvent profDrvEvent_;
    std::vector<std::string> cmdVec_;
    std::vector<ProcExtParam> params_;
};

}
}
}

#endif