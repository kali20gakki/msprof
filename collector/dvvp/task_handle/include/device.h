/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_HOST_DEVICE_H
#define ANALYSIS_DVVP_HOST_DEVICE_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include "ai_drv_dev_api.h"
#include "job_adapter.h"
#include "message/prof_params.h"
#include "thread/thread.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace host {
using DeviceCallback = void (*) (int devId);
class Device : public analysis::dvvp::common::thread::Thread {
public:
    Device(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params, const std::string &devId);
    virtual ~Device();

public:
    void Run(const struct error_message::Context &errorContext) override;
    int Stop() override;
    int Wait();
    void PostStopReplay();
    const SHARED_PTR_ALIA<analysis::dvvp::message::StatusInfo> GetStatus();
    int Init();
    int SetResponseCallback(DeviceCallback callback);

private:
    int InitJobAdapter();
    void WaitStopReplay();

private:
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
    // for host app/training, devId from Constructor is phyId
    // for device app, devId from Constructor is phyId(on host), indexId(on device)
    std::string indexIdStr_;     // devId from Constructor
    int indexId_;
    int hostId_;
    bool isQuited_;
    // sync start/stop
    std::mutex mtx_;
    bool isStopReplayReady_;
    std::condition_variable cvSyncStopReplay;
    // store result
    SHARED_PTR_ALIA<analysis::dvvp::message::StatusInfo> status_;
    // init
    bool isInited_;
    DeviceCallback deviceResponseCallack_;
    SHARED_PTR_ALIA<Analysis::Dvvp::JobWrapper::JobAdapter> jobAdapter_;
};
}  // namespace host
}  // namespace dvvp
}  // namespace analysis

#endif
