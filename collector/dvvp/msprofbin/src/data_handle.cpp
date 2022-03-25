/**
* @file data_handle.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "data_handle.h"
#include "ai_drv_dev_api.h"
#include "errno/error_code.h"
#include "uploader_mgr.h"
#include "param_validation.h"
#include "config/config.h"
#include "platform/platform.h"
#include "task_relationship_mgr.h"
#include "msprof_manager.h"

namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::transport;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Msprof;

std::map<const google::protobuf::Descriptor *, PFMessagehandler> HdcTransportDataHandle::handlerMap_ =
    HdcTransportDataHandle::CreateHandlerMap();

IDataHandleCB::~IDataHandleCB()
{
}

HdcTransportDataHandle::HdcTransportDataHandle()
{
}

HdcTransportDataHandle::~HdcTransportDataHandle()
{
}

int HdcTransportDataHandle::ReceiveStreamData(CONST_VOID_PTR data, unsigned int dataLen)
{
    if ((data == nullptr) || (dataLen == 0) || (dataLen > PROFILING_PACKET_MAX_LEN)) {
        MSPROF_LOGE("receive stream data, args invalid, data=%d, dataLen=%d.",
            ((data == nullptr) ? 0 : 1), dataLen);
        return PROFILING_FAILED;
    }

    auto message = analysis::dvvp::message::DecodeMessage(std::string((CONST_CHAR_PTR)data, dataLen));
    if (message != nullptr) {
        MSPROF_LOGD("[ReceiveStreamData] Hdc message: %s", message->GetDescriptor()->name().c_str());
        auto iter = handlerMap_.find(message->GetDescriptor());
        if (iter != handlerMap_.end()) {
            return iter->second(message);
        }
    }
    MSPROF_LOGE("receive stream data, message = nullptr");
    return PROFILING_FAILED;
}

std::map<const google::protobuf::Descriptor *, PFMessagehandler> HdcTransportDataHandle::CreateHandlerMap()
{
    std::map<const google::protobuf::Descriptor *, PFMessagehandler> handlerMap;
    handlerMap[analysis::dvvp::proto::FileChunkReq::descriptor()] = ProcessStreamFileChunk;
    handlerMap[analysis::dvvp::proto::DataChannelFinish::descriptor()] = ProcessDataChannelFinish;
    handlerMap[analysis::dvvp::proto::FinishJobRsp::descriptor()] = ProcessFinishJobRspMsg;
    handlerMap[analysis::dvvp::proto::Response::descriptor()] = ProcessResponseMsg;
    return handlerMap;
}

int HdcTransportDataHandle::ProcessStreamFileChunk(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    auto fileChunkReq = std::dynamic_pointer_cast<analysis::dvvp::proto::FileChunkReq>(message);
    if (fileChunkReq == nullptr) {
        MSPROF_LOGE("Failed to parse fileChunkReq");
        return PROFILING_FAILED;
    }

    std::string name = fileChunkReq->filename();
    MSPROF_LOGD("Handle FileChunkReq, filename: %s, job_ctx: %s, datamodule: %d",
        name.c_str(), fileChunkReq->hdr().job_ctx().c_str(), fileChunkReq->datamodule());

    analysis::dvvp::message::JobContext jobCtx;
    if (!jobCtx.FromString(fileChunkReq->hdr().job_ctx())) {
        MSPROF_LOGE("Failed to parse jobCtx json %s. fileName:%s", fileChunkReq->hdr().job_ctx().c_str(), name.c_str());
        return PROFILING_FAILED;
    }

    std::string jobId = jobCtx.job_id;
    if (fileChunkReq->datamodule() == FileChunkDataModule::PROFILING_IS_FROM_MSPROF) {
        if (!ParamValidation::instance()->CheckDeviceIdIsValid(jobCtx.dev_id)) {
            MSPROF_LOGE("[ProcessStreamFileChunk]jobCtx.dev_id: %s is not valid!", jobCtx.dev_id.c_str());
            return PROFILING_FAILED;
        }
        int devId = std::stoi(jobCtx.dev_id);
        jobId = UploaderMgr::instance()->GetJobId(devId, analysis::dvvp::message::PROFILING_MODE_DEF);
        fileChunkReq->set_datamodule(FileChunkDataModule::PROFILING_IS_FROM_MSPROF_DEVICE);
    }

    std::string encode = analysis::dvvp::message::EncodeMessage(fileChunkReq);
    int ret = UploaderMgr::instance()->UploadData(jobId, encode.c_str(), (unsigned int)encode.size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("ProcessStreamFileChunk failed jobid:%s, datamode:%d, name:%s", jobId.c_str(),
            fileChunkReq->datamodule(), name.c_str());
    }
    return ret;
}

int HdcTransportDataHandle::ProcessDataChannelFinish(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    auto dataChannFinish = std::dynamic_pointer_cast<analysis::dvvp::proto::DataChannelFinish>(message);
    if (dataChannFinish == nullptr) {
        MSPROF_LOGE("Failed to parse DataChannelFinish");
        return PROFILING_FAILED;
    }
    analysis::dvvp::message::JobContext jobCtx;
    if (!jobCtx.FromString(dataChannFinish->hdr().job_ctx())) {
        MSPROF_LOGE("Failed to parse jobCtx json %s.", dataChannFinish->hdr().job_ctx().c_str());
        return PROFILING_FAILED;
    }
    auto task = MsprofManager::instance()->GetTask(jobCtx.job_id);
    if (task == nullptr) {
        MSPROF_LOGE("[ProcessDataChannelFinish]Failed to find task by job_id %s", jobCtx.job_id.c_str());
        return PROFILING_FAILED;
    }

    task->PostSyncDataCtrl();

    return PROFILING_SUCCESS;
}

int HdcTransportDataHandle::ProcessRspCommon(const std::string &jobId, const std::string &encoded)
{
    auto task = MsprofManager::instance()->GetTask(jobId);
    if (task == nullptr) {
        MSPROF_LOGE("[RspCommon]Failed to find task by job_id %s", jobId.c_str());
        return PROFILING_FAILED;
    }

    int ret = UploaderMgr::instance()->UploadData(jobId, encoded.c_str(), encoded.size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("[RspCommon]Failed to transfer message by job_id %s", jobId.c_str());
    }

    task->StopNoWait();

    return PROFILING_SUCCESS;
}

int HdcTransportDataHandle::ProcessResponseMsg(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    MSPROF_LOGI("Received Response message");
    auto response = std::dynamic_pointer_cast<analysis::dvvp::proto::Response>(message);
    if (response == nullptr) {
        MSPROF_LOGE("Failed to parse Response");
        return PROFILING_FAILED;
    }

    return ProcessRspCommon(response->jobid(), analysis::dvvp::message::EncodeMessage(response));
}

int HdcTransportDataHandle::ProcessFinishJobRspMsg(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    MSPROF_LOGI("Received FinishJobRsp message");
    auto finishJobRsp = std::dynamic_pointer_cast<analysis::dvvp::proto::FinishJobRsp>(message);
    if (finishJobRsp == nullptr) {
        MSPROF_LOGE("Failed to parse FinishJobRsp");
        return PROFILING_FAILED;
    }

    return ProcessRspCommon(finishJobRsp->jobid(), analysis::dvvp::message::EncodeMessage(finishJobRsp));
}
}
}
}

