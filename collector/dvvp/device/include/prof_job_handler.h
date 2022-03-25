/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_PROF_TASK_H
#define ANALYSIS_DVVP_DEVICE_PROF_TASK_H

#include <mutex>

#include "collect_engine.h"
#include "message/codec.h"
#include "message/dispatcher.h"
#include "prof_msg_handler.h"
#include "thread/thread.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace device {
class ProfJobHandler : public IJobCallback {
public:
    explicit ProfJobHandler();
    ~ProfJobHandler() override;

public:
    int Init(int devId, std::string jobId_, SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> transport);
    int Uinit();
    void SetDevIdOnHost(int devIdOnHost);
    const std::string& GetJobId();
    SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> GetTransport(void);

public:
    int OnJobStart(SHARED_PTR_ALIA<analysis::dvvp::proto::JobStartReq> message,
                           analysis::dvvp::message::StatusInfo &statusInfo) override;

    int OnJobEnd(analysis::dvvp::message::StatusInfo &statusInfo) override;

    int OnReplayStart(SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message,
                              analysis::dvvp::message::StatusInfo &statusInfo) override;

    int OnReplayEnd(SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStopReq> message,
                            analysis::dvvp::message::StatusInfo &statusInfo) override;

    int OnConnectionReset() override;

    int GetDevId() override;

    virtual int InitEngine(analysis::dvvp::message::StatusInfo &statusInfo);

private:
    void ResetTask();
    SHARED_PTR_ALIA<std::vector<std::string>> GetControlCpuEvent(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    SHARED_PTR_ALIA<std::vector<std::string>> GetTsCpuEvent(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    SHARED_PTR_ALIA<std::vector<int>> GetAiCoreEventCores(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    SHARED_PTR_ALIA<std::vector<std::string>> GetLlcEvent(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    SHARED_PTR_ALIA<std::vector<std::string>> GetDdrEvent(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    SHARED_PTR_ALIA<std::vector<std::string>> GetAiCoreEvent(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    SHARED_PTR_ALIA<std::vector<std::string>> GetAivEvent(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    SHARED_PTR_ALIA<std::vector<int>> GetAivEventCores(
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);
    int CheckEventValid(SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message);

private:
    bool isInited_;
    SHARED_PTR_ALIA<CollectEngine> engine_;
    int devId_;         // devIndexId
    int devIdOnHost_;   // devPhyId
    std::string jobId_;
    volatile bool _is_started;
    SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> transport_;
};
}  // namespace device
}  // namespace dvvp
}  // namespace analysis

#endif
