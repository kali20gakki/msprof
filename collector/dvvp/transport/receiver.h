/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_RECEIVER_H
#define ANALYSIS_DVVP_DEVICE_RECEIVER_H

#include "message/codec.h"
#include "message/dispatcher.h"
#include "prof_msg_handler.h"
#include "prof_job_handler.h"
#include "thread/thread.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace device {
class Receiver : public analysis::dvvp::common::thread::Thread {
public:
    explicit Receiver(SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> transport);
    virtual ~Receiver();

public:
    int Init(int devId);
    int Uinit();
    const SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> GetTransport();
    int SendMessage(SHARED_PTR_ALIA<google::protobuf::Message> message);
    void SetDevIdOnHost(int devIdOnHost);
protected:
    void Run(const struct error_message::Context &errorContext);

private:
    SHARED_PTR_ALIA<analysis::dvvp::message::MsgDispatcher> dispatcher_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> transport_;
    int devId_;
    int devIdOnHost_;
    volatile bool inited_;
};
}  // namespace device
}  // namespace dvvp
}  // namespace analysis

#endif
