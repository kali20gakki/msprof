/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling request
 * Author: xp
 * Create: 2020-12-24
 */
#include "hdc_sender.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;

HdcSender::HdcSender()
    : engineName_(""), chunkPool_(nullptr), sender_(nullptr), transport_(nullptr) {}

HdcSender::~HdcSender()
{
}

int HdcSender::Init(SHARED_PTR_ALIA<ITransport> transport, const std::string &engineName)
{
    if (transport == nullptr || engineName.empty()) {
        MSPROF_LOGE("[Init]transport is null");
        return PROFILING_FAILED;
    }
    engineName_ = engineName;
    transport_ = transport;

    int ret = SenderPool::instance()->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Init sender pool failed");
        return PROFILING_FAILED;
    }
    const int chunkPoolNum = 64; // 64 : pool num
    const int chunkPoolSize = 64 * 1024; // 64 * 1024 chunk size:64K
    MSVP_MAKE_SHARED2_RET(chunkPool_, analysis::dvvp::common::memory::ChunkPool, chunkPoolNum,
        chunkPoolSize, PROFILING_FAILED);
    if (!(chunkPool_->Init())) {
        MSPROF_LOGE("Init chunk pool failed.");
        SenderPool::instance()->Uninit();
        return PROFILING_FAILED;
    }

    MSVP_MAKE_SHARED3_RET(sender_, Sender, transport_, engineName_, chunkPool_, PROFILING_FAILED);
    ret = sender_->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[HdcSender::Init]Sender init failed!");
        chunkPool_->Uninit();
        SenderPool::instance()->Uninit();
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("[%s] Init.", engineName.c_str());
    return PROFILING_SUCCESS;
}

int HdcSender::SendData(const std::string &jobCtx, const struct data_chunk &data)
{
    if (sender_ == nullptr) {
        MSPROF_LOGE("[HdcSender::SendData]sender_ is null");
        return PROFILING_FAILED;
    }
    int ret = sender_->Send(jobCtx, data);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[HdcSender::SendData]parameters is invalid.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void HdcSender::Flush() const
{
    if (sender_ == nullptr) {
        MSPROF_LOGE("[HdcSender::Flush]sender_ is null");
        return;
    }
    sender_->Flush();
}

void HdcSender::Uninit() const
{
    MSPROF_LOGI("[HdcSender]Uninit begin.");
    if (sender_ != nullptr) {
        sender_->Uninit();
    }
    if (transport_ != nullptr) {
        transport_->CloseSession();
    }
    if (chunkPool_ != nullptr) {
        chunkPool_->Uninit();
    }
    SenderPool::instance()->Uninit();
    MSPROF_LOGI("[HdcSender]Uninit end.");
}
}
}
}