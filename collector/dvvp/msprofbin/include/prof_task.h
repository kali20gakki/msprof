/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msprof bin prof task
 * Author: ly
 * Create: 2020-12-10
 */

#ifndef ANALYSIS_DVVP_MSPROFBIN_MSPROF_PROFTASK_H
#define ANALYSIS_DVVP_MSPROFBIN_MSPROF_PROFTASK_H
#include <condition_variable>
#include <memory>
#include <mutex>
#include "proto/profiler.pb.h"
#include "thread/thread.h"
#include "message/prof_params.h"
#include "utils/utils.h"
#include "job_adapter.h"
#include "transport/transport.h"
#include "transport/hdc/dev_mgr_api.h"
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

class ProfRpcTask : public ProfTask {
public:
    ProfRpcTask(const int devId,
             SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param);
    ~ProfRpcTask() override;

public:
    int Init() override;
    int UnInit() override;
    int Stop() override;
private:
    void PostSyncDataCtrl() override;
    void WaitSyncDataCtrl();

private:
    // data sync
    std::mutex dataSyncMtx_;
    bool isDataChannelEnd_;
    std::condition_variable cvSyncDataCtrl_;
    analysis::dvvp::transport::DevMgrAPI devMgrAPI_;
};
}
}
}
#endif
