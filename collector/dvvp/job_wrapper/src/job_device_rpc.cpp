/**
* @file job_device_rpc.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "job_device_rpc.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "prof_manager.h"
#include "transport/hdc/dev_mgr_api.h"
#include "ai_drv_dev_api.h"
#include "transport/hdc/device_transport.h"
#include "task_relationship_mgr.h"
#include "config/config.h"
#include "uploader_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::TaskHandle;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::transport;

JobDeviceRpc::JobDeviceRpc(int indexId)
    : indexId_(indexId),
      isStarted_(false)
{
    jobCtx_.dev_id = std::to_string(indexId);
}

JobDeviceRpc::~JobDeviceRpc()
{
}

void JobDeviceRpc::GetAndStoreStartTime(bool hostProfiling)
{
    if (hostProfiling) {
        return;
    }
    analysis::dvvp::common::utils::Utils::GetTime(startRealtime_, startMono_, cntvct_);
    DrvGetDeviceTime(indexId_, deviceStartMono_, deviceCntvct_);
    MSPROF_LOGI("devId:%d, startRealtime=%llu ns, startMono=%llu ns, cntvct=%llu, deviceStartMono=%llu ns,"
        " deviceCntvct=%llu", indexId_, startRealtime_, startMono_, cntvct_, deviceStartMono_, deviceCntvct_);
    std::string deviceId = std::to_string(
        TaskRelationshipMgr::instance()->GetFlushSuffixDevId(params_->job_id, indexId_));
    std::stringstream timeData;
    timeData << "[" << std::string(DEVICE_TAG_KEY) << deviceId << "]" << std::endl;
    std::string startTime = analysis::dvvp::common::utils::Utils::GenerateStartTime(startRealtime_,
        startMono_, cntvct_);
    timeData << startTime;
    std::string fileName = "host_start.log." + deviceId;
    int ret = StoreTime(fileName, timeData.str());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
        return;
    }
    fileName = "dev_start.log." + deviceId;
    startTime = analysis::dvvp::common::utils::Utils::GenerateStartTime(startRealtime_,
        deviceStartMono_, deviceCntvct_);
    ret = StoreTime(fileName, startTime);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
        return;
    }
}

int JobDeviceRpc::StoreTime(const std::string &fileName, const std::string &startTime)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->dev_id = std::to_string(indexId_);
    jobCtx->job_id = params_->job_id;
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
        analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);

    MSPROF_LOGI("storeTime.id: %s,fileName: %s", params_->job_id.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadFileData(params_->job_id,
        startTime, fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
    }
    return ret;
}

int JobDeviceRpc::StartProf(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    int ret = PROFILING_FAILED;
    do {
        if (isStarted_ || params == nullptr) {
            break;
        }
        params_ = params;
        jobCtx_.job_id = params_->job_id;

        SHARED_PTR_ALIA<analysis::dvvp::proto::JobStartReq> startJobMessage;
        MSVP_MAKE_SHARED0_BREAK(startJobMessage, analysis::dvvp::proto::JobStartReq);

        // set job params
        startJobMessage->mutable_hdr()->set_job_ctx(jobCtx_.ToString());
        startJobMessage->set_sampleconfig(params_->ToString());
        GetAndStoreStartTime(params->host_profiling);
        ret = SendMsgAndHandleResponse(startJobMessage);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to start job, devId:%d", indexId_);
            break;
        }
        pmuCfg_ = CreatePmuEventConfig(params, indexId_);
        MSPROF_LOGI("Starting, devId:%d", indexId_);
        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage = nullptr;
        MSVP_MAKE_SHARED0_BREAK(startReplayMessage, analysis::dvvp::proto::ReplayStartReq);

        BuildStartReplayMessage(pmuCfg_, startReplayMessage);
        startReplayMessage->mutable_hdr()->set_job_ctx(jobCtx_.ToString());
        startReplayMessage->set_replayid(0);
        ret = SendMsgAndHandleResponse(startReplayMessage);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to start, devId:%d", indexId_);
            break;
        }
        isStarted_ = true;
    } while (0);

    return ret;
}

void JobDeviceRpc::BuildAiCoreEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage)
{
    if (cfg->aiCoreEvents.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_ai_core_events:%s.",
            indexId_, Utils::GetEventsStr(cfg->aiCoreEvents).c_str());
        for (auto iter = cfg->aiCoreEvents.begin(); iter != cfg->aiCoreEvents.end(); ++iter) {
            startReplayMessage->add_ai_core_events(iter->c_str());
        }
    }
    if (cfg->aiCoreEventsCoreIds.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_ai_core_events_cores:%s.",
            indexId_, Utils::GetCoresStr(cfg->aiCoreEventsCoreIds).c_str());
        for (auto iter = cfg->aiCoreEventsCoreIds.begin(); iter != cfg->aiCoreEventsCoreIds.end(); ++iter) {
            startReplayMessage->add_ai_core_events_cores(*iter);
        }
    }
}
void JobDeviceRpc::BuildAivEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage)
{
    if (cfg->aivEvents.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_aivEvents:%s.",
            indexId_, Utils::GetEventsStr(cfg->aivEvents).c_str());
        for (auto iter = cfg->aivEvents.begin(); iter != cfg->aivEvents.end(); ++iter) {
            startReplayMessage->add_aiv_events(iter->c_str());
        }
    }
    if (cfg->aivEventsCoreIds.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_aivEventsCoreIds:%s.",
            indexId_, Utils::GetCoresStr(cfg->aivEventsCoreIds).c_str());
        for (auto iter = cfg->aivEventsCoreIds.begin(); iter != cfg->aivEventsCoreIds.end(); ++iter) {
            startReplayMessage->add_aiv_events_cores(*iter);
        }
    }
}

void JobDeviceRpc::BuildCtrlCpuEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage)
{
    if (cfg->ctrlCPUEvents.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_ctrl_cpu_events:%s.",
            indexId_, Utils::GetEventsStr(cfg->ctrlCPUEvents).c_str());
        for (auto iter = cfg->ctrlCPUEvents.begin(); iter != cfg->ctrlCPUEvents.end(); ++iter) {
            startReplayMessage->add_ctrl_cpu_events(iter->c_str());
        }
    }
}
void JobDeviceRpc::BuildTsCpuEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage)
{
    if (cfg->tsCPUEvents.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_ts_cpu_events:%s.",
            indexId_, Utils::GetEventsStr(cfg->tsCPUEvents).c_str());
        for (auto iter = cfg->tsCPUEvents.begin(); iter != cfg->tsCPUEvents.end(); ++iter) {
            startReplayMessage->add_ts_cpu_events(iter->c_str());
        }
    }
}
void JobDeviceRpc::BuildLlcEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage)
{
    if (cfg->llcEvents.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_llc_events:%s.",
            indexId_, Utils::GetEventsStr(cfg->llcEvents).c_str());
        for (auto iter = cfg->llcEvents.begin(); iter != cfg->llcEvents.end(); ++iter) {
            startReplayMessage->add_llc_events(iter->c_str());
        }
    }
}
void JobDeviceRpc::BuildDdrEventMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage)
{
    if (cfg->ddrEvents.size() > 0) {
        MSPROF_LOGI("Dev id=%d, add_ddr_events:%s.",
            indexId_, Utils::GetEventsStr(cfg->ddrEvents).c_str());
        for (auto iter = cfg->ddrEvents.begin(); iter != cfg->ddrEvents.end(); ++iter) {
            startReplayMessage->add_ddr_events(iter->c_str());
        }
    }
}

void JobDeviceRpc::BuildStartReplayMessage(SHARED_PTR_ALIA<PMUEventsConfig> cfg,
    SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStartReq> startReplayMessage)
{
    BuildCtrlCpuEventMessage(cfg, startReplayMessage);
    BuildTsCpuEventMessage(cfg, startReplayMessage);
    BuildAiCoreEventMessage(cfg, startReplayMessage);
    BuildAivEventMessage(cfg, startReplayMessage);
    BuildLlcEventMessage(cfg, startReplayMessage);
    BuildDdrEventMessage(cfg, startReplayMessage);
}

int JobDeviceRpc::StopProf(void)
{
    int ret = PROFILING_FAILED;
    do {
        if (!isStarted_) {
            break;
        }
        MSPROF_LOGI("Stop profiling begin, devId:%d", indexId_);

        SHARED_PTR_ALIA<analysis::dvvp::proto::ReplayStopReq> stopReplayMessage;
        MSVP_MAKE_SHARED0_BREAK(stopReplayMessage, analysis::dvvp::proto::ReplayStopReq);

        stopReplayMessage->mutable_hdr()->set_job_ctx(jobCtx_.ToString());
        stopReplayMessage->set_replayid(0);
        ret = SendMsgAndHandleResponse(stopReplayMessage);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to stop, devId:%d", indexId_);
            break;
        }
        SHARED_PTR_ALIA<analysis::dvvp::proto::JobStopReq> stopJobMessage;
        MSVP_MAKE_SHARED0_BREAK(stopJobMessage, analysis::dvvp::proto::JobStopReq);

        stopJobMessage->mutable_hdr()->set_job_ctx(jobCtx_.ToString());
        ret = SendMsgAndHandleResponse(stopJobMessage);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to stop job, devId:%d", indexId_);
            break;
        }
        MSPROF_LOGI("Stop profiling success, devId:%d", indexId_);
    } while (0);
    return ret;
}

int JobDeviceRpc::SendMsgAndHandleResponse(SHARED_PTR_ALIA<google::protobuf::Message> msg)
{
    int ret = PROFILING_FAILED;
    do {
        auto devTran = analysis::dvvp::transport::DevTransMgr::instance()->GetDevTransport(params_->job_id, indexId_);
        if (devTran == nullptr) {
            MSPROF_LOGE("DevTransport is null or not inited, jobId: %s , devId: %d", params_->job_id.c_str(), indexId_);
            break;
        }

        // encode message
        auto enc = analysis::dvvp::message::EncodeMessageShared(msg);
        if (enc == nullptr) {
            MSPROF_LOGE("Failed to encode message, devId:%d", indexId_);
            break;
        }

        // send ctrl message and recv response
        TLV_REQ_PTR packet = nullptr;
        ret = devTran->SendMsgAndRecvResponse(*enc, &packet);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to send message, devId:%d", indexId_);
            break;
        }

        ret = devTran->HandlePacket(packet, status_);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to handle response, devId:%d, info=%s", indexId_, status_.info.c_str());
            break;
        }
    } while (0);

    return ret;
}
}}}
