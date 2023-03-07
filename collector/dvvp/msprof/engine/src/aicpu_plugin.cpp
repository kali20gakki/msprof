/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: creat aicpu plugin
 * Author:
 * Create: 2020-09-18
 */
#include "aicpu_plugin.h"
#include "codec.h"
#include "error_code.h"
#include "msprof_dlog.h"
#include "utils.h"
#include "transport/hdc/hdc_transport.h"
#include "config/config.h"
#include "proto/profiler.pb.h"
#include "proto/profiler_ext.pb.h"
#include "transport/transport.h"
#include "msprof_callback_handler.h"
#include "param_validation.h"
#include "adx_prof_api.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::MsprofErrMgr;

AicpuPlugin::AicpuPlugin()
    : dataInitialized_(false), logicDevId_(-1), dataTran_(nullptr), server_(nullptr)
{}

AicpuPlugin::~AicpuPlugin()
{
    UnInit();
}

int AicpuPlugin::Init(const int32_t logicDevId)
{
    logicDevId_ = logicDevId;
    logicDevIdStr_ = std::to_string(logicDevId_);
    dataInitialized_ = true;
    MSPROF_LOGI("AicpuPlugin init, logicDevId:%d", logicDevId);
    if (!ParamValidation::instance()->CheckDeviceIdIsValid(logicDevIdStr_)) {
        MSPROF_LOGE("[AicpuPlugin]devId: %d is not valid!", logicDevId);
        MSPROF_INNER_ERROR("EK9999", "devId: %d is not valid!", logicDevId);
        return PROFILING_FAILED;
    }
    server_ = Analysis::Dvvp::Adx::AdxHdcServerCreate(logicDevId, HDC_SERVICE_TYPE_PROFILING);
    if (server_ == nullptr) {
        MSPROF_LOGW("HDC server is invalid");
    }
    Thread::SetThreadName(analysis::dvvp::common::config::MSVP_HDC_DUMPER_THREAD_NAME);
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start the logicDevId:%d in AicpuPlugin", logicDevId_);
        MSPROF_INNER_ERROR("EK9999", "Failed to start the logicDevId:%d in AicpuPlugin", logicDevId_);
        return PROFILING_FAILED;
    } else {
        MSPROF_LOGI("Succeeded in starting the logicDevId:%d in AicpuPlugin", logicDevId_);
    }
    return PROFILING_SUCCESS;
}

int AicpuPlugin::UnInit()
{
    MSPROF_LOGI("Uinit AicpuPlugin Transport Begin, logicDevId:%d", logicDevId_);
    if (dataInitialized_) {
        dataInitialized_ = false;
        if (dataTran_ != nullptr) {
            dataTran_.reset();
        }
        if (server_ != nullptr) {
            Analysis::Dvvp::Adx::AdxHdcServerDestroy(server_);
        }
        StopNoWait();
    }
    MSPROF_LOGI("Uinit AicpuPlugin Transport End, logicDevId:%d", logicDevId_);
    return PROFILING_SUCCESS;
}

int AicpuPlugin::ReceiveStreamData(CONST_VOID_PTR data, unsigned int dataLen)
{
    if ((data == nullptr) || (dataLen == 0) || (dataLen > analysis::dvvp::common::config::PROFILING_PACKET_MAX_LEN)) {
        MSPROF_LOGE("receive stream data, args invalid, data=%d, dataLen=%d.",
            ((data == nullptr) ? 0 : 1), dataLen);
        MSPROF_INNER_ERROR("EK9999", "receive stream data, args invalid, data=%d, dataLen=%d.",
            ((data == nullptr) ? 0 : 1), dataLen);
        return PROFILING_FAILED;
    }
    auto message = analysis::dvvp::message::DecodeMessage(std::string((CONST_CHAR_PTR)data, dataLen));
    if (message == nullptr) {
        MSPROF_LOGE("receive stream data, message = nullptr");
        MSPROF_INNER_ERROR("EK9999", "receive stream data, message = nullptr");
        return PROFILING_FAILED;
    }
    auto fileChunkReq = std::dynamic_pointer_cast<analysis::dvvp::proto::FileChunkReq>(message);
    if (fileChunkReq == nullptr) {
        MSPROF_LOGE("Failed to call dynamic_pointer_cast.");
        return PROFILING_FAILED;
    }
    if (fileChunkReq->islastchunk() && fileChunkReq->chunksizeinbytes() == 0) {
        Msprof::Engine::FlushModule(logicDevIdStr_);
        return PROFILING_SUCCESS;
    }
    fileChunkReq->set_tagsuffix(logicDevIdStr_);
    analysis::dvvp::message::JobContext jobCtx;
    if (!jobCtx.FromString(fileChunkReq->hdr().job_ctx())) {
        MSPROF_LOGE("Failed to parse jobCtx:%s, devId:%d", fileChunkReq->hdr().job_ctx().c_str(), logicDevId_);
        MSPROF_INNER_ERROR("EK9999", "Failed to parse jobCtx:%s, devId:%d",
            fileChunkReq->hdr().job_ctx().c_str(), logicDevId_);
        return PROFILING_FAILED;
    }
    jobCtx.dev_id = logicDevIdStr_;
    fileChunkReq->mutable_hdr()->clear_job_ctx();
    fileChunkReq->mutable_hdr()->set_job_ctx(jobCtx.ToString());
    return Msprof::Engine::SendAiCpuData(fileChunkReq);
}

void AicpuPlugin::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    if (!dataInitialized_) {
        MSPROF_LOGE("AicpuPlugin is not inited, logicDevId:%d", logicDevId_);
        MSPROF_INNER_ERROR("EK9999", "AicpuPlugin is not inited, logicDevId:%d", logicDevId_);
        return;
    }
    MSPROF_LOGI("Device(%d) AicpuPlugin is running", logicDevId_);
    while (!IsQuit()) {
        MSPROF_LOGI("Device(%d) AicpuPlugin CreateHdcServerTransport begin", logicDevId_);
        dataTran_ = analysis::dvvp::transport::HDCTransportFactory().CreateHdcServerTransport(logicDevId_, server_);
        if (dataTran_ == nullptr) {
            MSPROF_LOGW("Device(%d) can not CreateHdcServerTransport", logicDevId_);
            return;
        }
        MSPROF_LOGI("Device(%d) AicpuPlugin CreateHdcServerTransport success", logicDevId_);
        TLV_REQ_PTR packet = nullptr;
        int ret = PROFILING_SUCCESS;
        while (!IsQuit()) {
            packet = nullptr;
            ret = dataTran_->RecvPacket(&packet);
            if (ret == IDE_DAEMON_SOCK_CLOSE) {
                MSPROF_EVENT("Device(%d) AicpuPlugin session closed, exits", logicDevId_);
                return;
            } else if (ret < 0 || packet == nullptr) {
                MSPROF_EVENT("Device(%d) AicpuPlugin recv data ends, exits", logicDevId_);
                break;
            }
            MSPROF_LOGD("[HdcTransport] RecvDataPacket %d bytes", packet->len);

            ret = ReceiveStreamData(packet->value, packet->len);
            if (ret != PROFILING_SUCCESS) {
                MSPROF_LOGE("Device(%d) ReceiveStreamData failed", logicDevId_);
                MSPROF_INNER_ERROR("EK9999", "Device(%d) ReceiveStreamData failed", logicDevId_);
            }
            dataTran_->DestroyPacket(packet);
            packet = nullptr;
        }
    }
    MSPROF_LOGI("Device(%d) AicpuPlugin exit", logicDevId_);
}
}
}
