/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: lixubo
 * Create: 2018-06-13
 */
#include "prof_msg_handler.h"
#include <cstdlib>
#include "collection_entry.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "param_validation.h"
#include "prof_job_handler.h"
#include "task_manager.h"
#include "uploader_mgr.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace device {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::validation;

#define GET_JOB_BY_TASKID(taskId, job) do {                                                 \
    job = analysis::dvvp::device::TaskManager::instance()->GetTask(taskId);                 \
    if ((job) == nullptr) {                                                                   \
        MSPROF_LOGE("Get Task Failed");                                                     \
        return;                                                                             \
    }                                                                                       \
} while (0)

SHARED_PTR_ALIA<google::protobuf::Message> CreateResponse(analysis::dvvp::message::StatusInfo &statusInfo)
{
    SHARED_PTR_ALIA<analysis::dvvp::proto::Response> rsp;
    do {
        MSVP_MAKE_SHARED0_BREAK(rsp, analysis::dvvp::proto::Response);

        if (statusInfo.status == analysis::dvvp::message::SUCCESS) {
            rsp->set_status(analysis::dvvp::proto::SUCCESS);
        } else {
            rsp->set_status(analysis::dvvp::proto::FAILED);
        }
        rsp->set_message(statusInfo.ToString());
    } while (0);

    return rsp;
}

void JobStartHandler::OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    MSPROF_LOGI("[JobStartHandler::OnNewMessage] Entering handler");
    auto req = std::dynamic_pointer_cast<analysis::dvvp::proto::JobStartReq>(message);
    if (req == nullptr) {
        return;
    }
    analysis::dvvp::message::JobContext jobCtx;
    if (!jobCtx.FromString(req->hdr().job_ctx())) {
        MSPROF_LOGE("Failed to parse jobctx json %s.", req->hdr().job_ctx().c_str());
        return;
    }
    std::string taskId = jobCtx.job_id + "_" + jobCtx.dev_id;
    if (!ParamValidation::instance()->CheckDeviceIdIsValid(jobCtx.dev_id)) {
        MSPROF_LOGE("[JobStartHandler::OnNewMessage]jobCtx.dev_id:%s is not valid!", jobCtx.dev_id.c_str());
        return;
    }
    if (!(ParamValidation::instance()->CheckParamsJobIdRegexMatch(jobCtx.job_id))) {
        MSPROF_LOGE("[JobStartHandler::OnNewMessage]jobCtx.job_id:%s is illegal", jobCtx.job_id.c_str());
        return;
    }
    int devPhyId = std::stoi(jobCtx.dev_id);
    int devIndexId = 0;
    analysis::dvvp::message::StatusInfo statusInfo;
    do {
        auto job = analysis::dvvp::device::TaskManager::instance()->CreateTask(devPhyId, taskId, transport_);
        if (job == nullptr) {
            MSPROF_LOGE("Create Task Failed");
            statusInfo.status = analysis::dvvp::message::ERR;
            statusInfo.info = "Create task failed.";
            break;
        }
        devIndexId = job->GetDevId();
        int retJob = job->OnJobStart(req, statusInfo);
        if (retJob != PROFILING_SUCCESS) {
            statusInfo.status = analysis::dvvp::message::ERR;
            if (statusInfo.info.length() == 0) {
                statusInfo.info = "start job failed";
            }
        }
    } while (0);

    // send status back to host
    int ret = analysis::dvvp::device::CollectionEntry::instance()->SendMsgByDevId(jobCtx.job_id, devIndexId,
        CreateResponse(statusInfo));

    MSPROF_LOGI("Reply job start, status=%s, devIndexId=%d, ret=%d",
        statusInfo.ToString().c_str(), devIndexId, ret);
}

void JobStopHandler::OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    MSPROF_LOGI("[JobStopHandler::OnNewMessage] Entering handler");
    auto req1 = std::dynamic_pointer_cast<analysis::dvvp::proto::JobStopReq>(message);
    if (req1 != nullptr) {
        analysis::dvvp::message::JobContext jobCtx1;
        if (!jobCtx1.FromString(req1->hdr().job_ctx())) {
            MSPROF_LOGE("Failed to parse jobctx json %s.", req1->hdr().job_ctx().c_str());
            return;
        }
        std::string taskId = jobCtx1.job_id + "_" + jobCtx1.dev_id;
        SHARED_PTR_ALIA<ProfJobHandler> job1;
        GET_JOB_BY_TASKID(taskId, job1);

        int devIndexId = job1->GetDevId();
        analysis::dvvp::message::StatusInfo statusInfo;
        int retJob = job1->OnJobEnd(statusInfo);
        if (retJob != PROFILING_SUCCESS) {
            statusInfo.status = analysis::dvvp::message::ERR;
            if (statusInfo.info.length() == 0) {
                statusInfo.info = "stop job failed";
            }
        }

        // send status back to host
        int ret = analysis::dvvp::device::CollectionEntry::instance()->SendMsgByDevId(jobCtx1.job_id, devIndexId,
            CreateResponse(statusInfo));

        MSPROF_LOGI("Reply job stop, status=%s, devIndexId=%d, ret=%d",
            statusInfo.ToString().c_str(), devIndexId, ret);
        bool flag = analysis::dvvp::device::TaskManager::instance()->DeleteTask(job1->GetJobId());
        if (!flag) {
            MSPROF_LOGE("DeleteTask failed.jobId: %s", job1->GetJobId().c_str());
        }
    }
}

void ReplayStartHandler::OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    MSPROF_LOGI("[StartHandler::OnNewMessage] Entering handler");
    auto req2 = std::dynamic_pointer_cast<analysis::dvvp::proto::ReplayStartReq>(message);
    if (req2 != nullptr) {
        analysis::dvvp::message::JobContext jobCtx2;
        if (!jobCtx2.FromString(req2->hdr().job_ctx())) {
            MSPROF_LOGE("Failed to parse jobctx json %s.", req2->hdr().job_ctx().c_str());
            return;
        }
        std::string taskId = jobCtx2.job_id + "_" + jobCtx2.dev_id;
        SHARED_PTR_ALIA<ProfJobHandler> job2;
        GET_JOB_BY_TASKID(taskId, job2);

        int devIndexId = job2->GetDevId();
        analysis::dvvp::message::StatusInfo statusInfo;
        int retJob = job2->OnReplayStart(req2, statusInfo);
        if (retJob != PROFILING_SUCCESS) {
            statusInfo.status = analysis::dvvp::message::ERR;
            if (statusInfo.info.length() == 0) {
                statusInfo.info = "start failed";
            }
        }

        // send status back to host
        int ret = analysis::dvvp::device::CollectionEntry::instance()->SendMsgByDevId(jobCtx2.job_id, devIndexId,
            CreateResponse(statusInfo));

        MSPROF_LOGI("Reply start, status=%s, devIndexId=%d, ret=%d",
            statusInfo.ToString().c_str(), devIndexId,  ret);
    }
}

void ReplayStopHandler::OnNewMessage(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    MSPROF_LOGI("[StopHandler::OnNewMessage] Entering handler");
    auto req = std::dynamic_pointer_cast<analysis::dvvp::proto::ReplayStopReq>(message);
    if (req != nullptr) {
        analysis::dvvp::message::JobContext jobCtx;
        if (!jobCtx.FromString(req->hdr().job_ctx())) {
            MSPROF_LOGE("Failed to parse jobctx json %s.", req->hdr().job_ctx().c_str());
            return;
        }
        std::string taskId = jobCtx.job_id + "_" + jobCtx.dev_id;
        SHARED_PTR_ALIA<ProfJobHandler> job;
        GET_JOB_BY_TASKID(taskId, job);

        int devIndexId = job->GetDevId();
        analysis::dvvp::message::StatusInfo statusInfo;
        int retJob = job->OnReplayEnd(req, statusInfo);
        if (retJob != PROFILING_SUCCESS) {
            statusInfo.status = analysis::dvvp::message::ERR;
            if (statusInfo.info.length() == 0) {
                statusInfo.info = "stop failed";
            }
        }

        // send status back to host
        int ret = analysis::dvvp::device::CollectionEntry::instance()->SendMsgByDevId(jobCtx.job_id, devIndexId,
            CreateResponse(statusInfo));

        MSPROF_LOGI("Stop, status=%s, devIndexId=%d, ret=%d",
            statusInfo.ToString().c_str(), devIndexId, ret);
    }
}
}  // namespace device
}  // namespace dvvp
}  // namespace analysis
