/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: dump file data
 * Author: dml
 * Create: 2018-06-13
 */
#include "rpc_data_handle.h"
#include "config/config.h"
#include "msprof_dlog.h"
#include "transport/transport.h"
#include "transport/hdc/hdc_transport.h"
#include "transport/uploader.h"
#include "transport/uploader_mgr.h"
#include "adx_prof_api.h"
#include "proto/profiler.pb.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::proto;
using namespace Analysis::Dvvp::Common::Statistics;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::transport;
using namespace analysis::dvvp::common::utils;

HdcDataHandle::HdcDataHandle(const std::string &moduleNameWithId, int32_t hostPid, int32_t devId)
    : moduleNameWithId_(moduleNameWithId), hostPid_(hostPid), devId_(devId), hdcSender_(nullptr) {}

HdcDataHandle::~HdcDataHandle() {}

int HdcDataHandle::Init()
{
    HDC_CLIENT client = Analysis::Dvvp::Adx::AdxHdcClientCreate(HDC_SERVICE_TYPE_PROFILING);
    if (client == nullptr) {
        MSPROF_LOGE("HDC client is invalid");
        return PROFILING_FAILED;
    }
    // wait dir chmod and chown until it could be created
    auto transport = HDCTransportFactory().CreateHdcClientTransport(hostPid_, devId_, client);
    if (transport == nullptr) {
        Analysis::Dvvp::Adx::AdxHdcClientDestroy(client);
        MSPROF_LOGW("Can not create HdcClientTransport");
        return PROFILING_FAILED;
    }
    MSVP_MAKE_SHARED0_RET(hdcSender_, HdcSender, PROFILING_FAILED);
    int ret = hdcSender_->Init(transport, moduleNameWithId_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("hdcSender Init failed");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int HdcDataHandle::UnInit()
{
    int ret = Flush();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("HdcDataHandle UnInit, flush failed");
    }
    if (hdcSender_ != nullptr) {
        hdcSender_->Uninit();
    }
    return PROFILING_SUCCESS;
}

int HdcDataHandle::Flush()
{
    if (hdcSender_ != nullptr) {
        hdcSender_->Flush();
    }
    return PROFILING_SUCCESS;
}

int HdcDataHandle::SendData(CONST_VOID_PTR data, uint32_t dataLen, const std::string fileName, const std::string jobCtx)
{
    if (data == nullptr || dataLen == 0 || dataLen > PROFILING_PACKET_MAX_LEN || hdcSender_ == nullptr) {
        MSPROF_LOGE("HdcDataHandle SendData failed, dataLen:%d", dataLen);
        return PROFILING_FAILED;
    }
    auto message = analysis::dvvp::message::DecodeMessage(std::string((CONST_CHAR_PTR)data, dataLen));
    if (message == nullptr) {
        MSPROF_LOGE("receive stream data, message = nullptr");
        return PROFILING_FAILED;
    }
    auto fileChunkReq = std::dynamic_pointer_cast<FileChunkReq>(message);
    struct data_chunk dataChunk;
    if (fileChunkReq->islastchunk() && fileChunkReq->chunksizeinbytes() == 0) {
        dataChunk.isLastChunk = 1;
        dataChunk.bufLen = 0;
        dataChunk.dataBuf = nullptr;
    } else {
        dataChunk.isLastChunk = 0;
        dataChunk.bufLen = fileChunkReq->chunksizeinbytes();
        dataChunk.dataBuf = reinterpret_cast<UNSIGNED_CHAR_PTR>(const_cast<CHAR_PTR>(fileChunkReq->chunk().c_str()));
    }
    dataChunk.offset = -1;
    dataChunk.relativeFileName = const_cast<CHAR_PTR>(fileName.c_str());
    return hdcSender_->SendData(jobCtx, dataChunk);
}

RpcDataHandle::RpcDataHandle(const std::string &moduleNameWithId, const std::string &module,
    int32_t hostPid, int32_t devId)
    : moduleNameWithId_(moduleNameWithId), module_(module), hostPid_(hostPid), devId_(devId), dataHandle_(nullptr)
{
}

RpcDataHandle::~RpcDataHandle() {}

int RpcDataHandle::Init()
{
    return PROFILING_SUCCESS;
}

int RpcDataHandle::UnInit()
{
    if (dataHandle_ != nullptr) {
        return dataHandle_->UnInit();
    }
    return PROFILING_SUCCESS;
}

bool RpcDataHandle::IsReady()
{
    if (dataHandle_ != nullptr) {
        return true;
    }
    return false;
}

int RpcDataHandle::TryToConnect()
{
    if (dataHandle_ != nullptr) {
        MSPROF_LOGI("TryToConnect connect Ok");
        return PROFILING_SUCCESS;
    }
    // connect to app by socket, connect to app by hdc client
    SHARED_PTR_ALIA<IDataHandle> hdcHandle;
    MSVP_MAKE_SHARED3_RET(hdcHandle, HdcDataHandle, moduleNameWithId_, hostPid_, devId_, PROFILING_FAILED);
    int ret = hdcHandle->Init();
    if (ret == PROFILING_SUCCESS) {
        MSPROF_LOGI("CreateHdcUploader success");
        dataHandle_ = hdcHandle;
        return PROFILING_SUCCESS;
    } else {
        hdcHandle.reset();
    }
    MSPROF_LOGE("TryToConnect failed, moduleNameWithId:%s, module:%s, hostPid:%d, devId:%d",
        moduleNameWithId_.c_str(), module_.c_str(), hostPid_, devId_);

    return PROFILING_FAILED;
}

int RpcDataHandle::Flush()
{
    if (dataHandle_ != nullptr) {
        return dataHandle_->Flush();
    }
    return PROFILING_SUCCESS;
}

int RpcDataHandle::SendData(CONST_VOID_PTR data, uint32_t dataLen, const std::string fileName, const std::string jobCtx)
{
    int ret = PROFILING_SUCCESS;
    if (data == nullptr || dataLen == 0 || dataLen > PROFILING_PACKET_MAX_LEN) {
        MSPROF_LOGE("RpcDataHandle SendData failed, dataLen:%d", dataLen);
        return PROFILING_FAILED;
    }
    if (dataHandle_ != nullptr) {
        ret = dataHandle_->SendData(data, dataLen, fileName, jobCtx);
    } else {
        MSPROF_LOGW("RpcDataHandle SendData dataHandle is null, dataLen:%d, file:%s", dataLen, fileName.c_str());
    }
    return ret;
}
}
}
