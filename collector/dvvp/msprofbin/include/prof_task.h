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

#ifndef ANALYSIS_DVVP_MSPROFBIN_MSPROF_PROFTASK_H
#define ANALYSIS_DVVP_MSPROFBIN_MSPROF_PROFTASK_H
#include <condition_variable>
#include <memory>
#include <mutex>
#include "thread/thread.h"
#include "message/prof_params.h"
#include "message/data_define.h"
#include "utils/utils.h"
#include "job_adapter.h"
#include "transport/transport.h"
namespace Analysis {
namespace Dvvp {
namespace Msprof {
class ProfTask : public analysis::dvvp::common::thread::Thread {
public:
    ProfTask(const int devId,
             SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param);
    virtual ~ProfTask();

    virtual int Init() = 0;
    virtual int UnInit() = 0;
    virtual void WaitStopReplay();
    virtual void PostStopReplay();
    virtual void PostSyncDataCtrl();
    void Run(const struct error_message::Context &errorContext) override;
    int Stop() override;
    virtual int Wait();
protected:
    void WriteDone();

protected:
    bool isInit_;
    int deviceId_;
    bool isQuited_;
    bool isExited_;
    // stop
    std::mutex mtx_;
    bool isStopReplayReady;
    std::condition_variable cvSyncStopReplay;

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
    SHARED_PTR_ALIA<Analysis::Dvvp::JobWrapper::JobAdapter> jobAdapter_;

private:
    int GetHostAndDeviceInfo();
    std::string GetHostTime();
    void GenerateFileName(bool isStartTime, std::string &filename);
    int CreateCollectionTimeInfo(std::string collectionTime, bool isStartTime);
    int CreateIncompatibleFeatureJsonFile();
};

class ProfSocTask : public ProfTask {
public:
    ProfSocTask(const int devId,
             SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param);
    ~ProfSocTask() override;

public:
    int Init() override;
    int UnInit() override;
};

}
}
}
#endif
