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
    int GenerateCollectRunningMode();
    int GenerateAnalyzeRunningMode();
    // check params dependence and update metrics and events
    int ParamsCheck() const;

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
};
}
}
}
#endif