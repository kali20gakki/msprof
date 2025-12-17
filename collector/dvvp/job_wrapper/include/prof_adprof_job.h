/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
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