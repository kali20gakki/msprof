/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msprof manager
 * Author: ly
 * Create: 2020-12-10
 */

#ifndef ANALYSIS_DVVP_MSPROFBIN_MSPROF_MANAGER_H
#define ANALYSIS_DVVP_MSPROFBIN_MSPROF_MANAGER_H
#include "singleton/singleton.h"
#include "prof_task.h"
#include "message/prof_params.h"
#include "running_mode.h"

namespace Analysis {
namespace Dvvp {
namespace Msprof {

class MsprofManager : public analysis::dvvp::common::singleton::Singleton<MsprofManager> {
public:
    MsprofManager();
    ~MsprofManager();
    int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    int UnInit();
    int MsProcessCmd() const;
    void StopNoWait() const;
    SHARED_PTR_ALIA<ProfTask> GetTask(const std::string &jobId);

    SHARED_PTR_ALIA<Collector::Dvvp::Msprofbin::RunningMode> rMode_;
private:
    int GenerateRunningMode();
    // check params dependence and update metrics and events
    int ParamsCheck() const;

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
};
}
}
}
#endif