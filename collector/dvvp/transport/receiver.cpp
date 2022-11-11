/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: lixubo
 * Create: 2018-06-13
 */
#include "receiver.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "task_manager.h"

namespace analysis {
namespace dvvp {
namespace device {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::MsprofErrMgr;

Receiver::Receiver(SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> transport)
    : dispatcher_(nullptr), transport_(transport), devId_(-1), devIdOnHost_(-1), inited_(false)
{
}

Receiver::~Receiver()
{
    Uinit();
}

int Receiver::Init(int devId)
{
    int ret = PROFILING_FAILED;

    do {
        MSVP_MAKE_SHARED0_BREAK(dispatcher_, analysis::dvvp::message::MsgDispatcher);

        SHARED_PTR_ALIA<JobStartHandler> jobStartHandler;
        MSVP_MAKE_SHARED1_BREAK(jobStartHandler, JobStartHandler, transport_);
        dispatcher_->RegisterMessageHandler<analysis::dvvp::proto::JobStartReq>(jobStartHandler);

        SHARED_PTR_ALIA<JobStopHandler> jobStopHandler;
        MSVP_MAKE_SHARED0_BREAK(jobStopHandler, JobStopHandler);
        dispatcher_->RegisterMessageHandler<analysis::dvvp::proto::JobStopReq>(jobStopHandler);

        SHARED_PTR_ALIA<ReplayStartHandler> replayStartHandler;
        MSVP_MAKE_SHARED0_BREAK(replayStartHandler, ReplayStartHandler);
        dispatcher_->RegisterMessageHandler<analysis::dvvp::proto::ReplayStartReq>(replayStartHandler);

        SHARED_PTR_ALIA<ReplayStopHandler> replayStopHandler;
        MSVP_MAKE_SHARED0_BREAK(replayStopHandler, ReplayStopHandler);
        dispatcher_->RegisterMessageHandler<analysis::dvvp::proto::ReplayStopReq>(replayStopHandler);

        devId_ = devId;
        inited_ = true;
        ret = PROFILING_SUCCESS;
    } while (0);

    return ret;
}

int Receiver::Uinit()
{
    MSPROF_LOGI("Receiver Unint");
    if (transport_ != nullptr) {
        transport_->CloseSession();
        if (Stop() != PROFILING_SUCCESS) {
            MSPROF_LOGW("Faied to stop thread: %s", GetThreadName().c_str());
        }
        transport_.reset();
    }

    devId_ = -1;
    inited_ = false;

    return PROFILING_SUCCESS;
}

void Receiver::SetDevIdOnHost(int devIdOnHost)
{
    devIdOnHost_ = devIdOnHost;
}

int Receiver::SendMessage(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    int ret = PROFILING_FAILED;

    if (message != nullptr) {
        std::string encoded = analysis::dvvp::message::EncodeMessage(message);
        ret = transport_->SendBuffer(encoded.c_str(), encoded.size());
        MSPROF_LOGI("DeviceOnHost(%d) Send message size %u ret %d",
            devIdOnHost_, encoded.size(), ret);
    }

    return ret;
}

const SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport> Receiver::GetTransport()
{
    return transport_;
}

void Receiver::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    MSPROF_LOGI("Receiver begin, devId:%d, devIdOnHost:%d", devId_, devIdOnHost_);
    if (!inited_) {
        MSPROF_LOGE("Receiver is not inited.");
        MSPROF_INNER_ERROR("EK9999", "Receiver is not inited.");
        return;
    }

    do {
        // wait for next message
        TLV_REQ_PTR packet = nullptr;
        int bytesRecved = transport_->RecvPacket(&packet);
        if (bytesRecved < 0 || packet == nullptr) {
            MSPROF_LOGW("devIdOnHost(%d) Failed to recv packet from host", devIdOnHost_);
            analysis::dvvp::device::TaskManager::instance()->ConnectionReset(transport_);
            StopNoWait();
            break;
        }

        MSPROF_LOGI("DeviceOnHost(%d) Receiver message size %d", devIdOnHost_, bytesRecved);
        std::string buf(packet->value, packet->len);
        transport_->DestroyPacket(packet);
        packet = nullptr;

        dispatcher_->OnNewMessage(analysis::dvvp::message::DecodeMessage(buf));
    } while (!IsQuit());
    MSPROF_LOGI("Receiver end, devId:%d, devIdOnHost:%d", devId_, devIdOnHost_);
}
}  // namespace device
}  // namespace dvvp
}  // namespace analysis
