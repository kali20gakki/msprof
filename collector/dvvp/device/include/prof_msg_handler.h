/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_PROF_MSG_HANDLER_H
#define ANALYSIS_DVVP_DEVICE_PROF_MSG_HANDLER_H

#include "message/dispatcher.h"
#include "message/prof_params.h"
#include "proto/profiler.pb.h"
#include "transport/transport.h"
#include "prof_manager.h"
namespace analysis {
namespace dvvp {
namespace device {
extern SHARED_PTR_ALIA<google::protobuf::Message> CreateResponse(
    analysis::dvvp::message::StatusInfo &statusInfo);

class IJobCallback {
public:
    virtual ~IJobCallback() {}

public:
    virtual int OnJobStart(SHARED_PTR_ALIA<analysis::dvvp::proto::JobStartReq> message,
                           analysis::dvvp::message::StatusInfo &statusInfo) = 0;

    virtual int OnJobEnd(analysis::dvvp::message::StatusInfo &statusInfo) = 0;

    virtual int OnReplayStart(SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> message,
                              analysis::dvvp::message::StatusInfo &statusInfo) = 0;

    virtual int OnReplayEnd(SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStopReq> message,
                            analysis::dvvp::message::StatusInfo &statusInfo) = 0;

    virtual int OnConnectionReset() = 0;

    virtual int GetDevId() = 0;
};

class JobStartHandler : public analysis::dvvp::message::IMsgHandler {
public:
    explicit JobStartHandler(SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> transport)
    {
        transport_ = transport;
    }
    virtual ~JobStartHandler() {}

public:
    void OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message) override;

private:
    SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> transport_;
};

class JobStopHandler : public analysis::dvvp::message::IMsgHandler {
public:
    explicit JobStopHandler() {}
    virtual ~JobStopHandler() {}

public:
    void OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message) override;
};

class ReplayStartHandler : public analysis::dvvp::message::IMsgHandler {
public:
    explicit ReplayStartHandler() {}
    virtual ~ReplayStartHandler() {}

public:
    void OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message) override;
};

class ReplayStopHandler : public analysis::dvvp::message::IMsgHandler {
public:
    explicit ReplayStopHandler() {}
    virtual ~ReplayStopHandler() {}

public:
    void OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message) override;
};
}  // namespace device
}  // namespace dvvp
}  // namespace analysis

#endif
