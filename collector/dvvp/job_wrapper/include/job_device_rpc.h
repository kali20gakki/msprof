/**
* @file job_device_rpc.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_JOB_DEVICE_RPC_H
#define ANALYSIS_DVVP_JOB_DEVICE_RPC_H

#include "collection_register.h"
#include "job_adapter.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "proto/profiler.pb.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
class JobDeviceRpc : public JobAdapter {
public:
    explicit JobDeviceRpc(int indexId);
    ~JobDeviceRpc() override;

public:
    int StartProf(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params) override;
    int StopProf(void) override;

private:
    int SendMsgAndHandleResponse(SHARED_PTR_ALIA<google::protobuf::Message> msg);
    void BuildStartReplayMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
                                SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage);
    void BuildAiCoreEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
                                SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage);
    void BuildAivEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
                                SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage);
    void BuildCtrlCpuEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
                                SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage);
    void BuildTsCpuEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
                                SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage);
    void BuildLlcEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
                                SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage);
    void BuildDdrEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
                                SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage);
    void GetAndStoreStartTime(bool hostProfiling);
    int StoreTime(const std::string &fileName, const std::string &startTime);

private:
    int indexId_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
    bool isStarted_;
    analysis::dvvp::message::JobContext jobCtx_;
    SHARED_PTR_ALIA<PMUEventsConfig> pmuCfg_;
};
}}}
#endif