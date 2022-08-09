/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: msprof bin prof task
 * Author: ly
 * Create: 2020-12-10
 */

#include "prof_task.h"
#include "errno/error_code.h"
#include "config/config.h"
#include "hccl_plugin.h"
#include "job_factory.h"
#include "job_device_rpc.h"
#include "transport/uploader_mgr.h"
#include "task_relationship_mgr.h"
#include "info_json.h"
#include "mmpa_api.h"

namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::JobWrapper;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::TaskHandle;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::host;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

ProfTask::ProfTask(const int devId, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param)
    : isInit_(false),
      deviceId_(devId),
      isQuited_(false),
      isExited_(false),
      isStopReplayReady(false),
      params_(param) {}

ProfTask::~ProfTask()
{
}

void ProfTask::WaitStopReplay()
{
    std::unique_lock<std::mutex> lk(mtx_);
    cvSyncStopReplay.wait(lk, [=] { return (isStopReplayReady || isQuited_); });
    isStopReplayReady = false;
}

void ProfTask::PostStopReplay()
{
    std::unique_lock<std::mutex> lk(mtx_);
    isStopReplayReady = true;
    cvSyncStopReplay.notify_one();
}

void ProfTask::PostSyncDataCtrl()
{
}

void ProfTask::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    if (params_ == nullptr || !isInit_) {
        MSPROF_LOGE("ProfTask run failed.");
        return;
    }
    do {
        int ret = CreateCollectionTimeInfo(GetHostTime(), true);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("ProcessDefMode CreateCollectionTimeInfo failed");
        }
        ret = jobAdapter_->StartProf(params_);
        if (ret != PROFILING_SUCCESS) {
            break;
        }
        WaitStopReplay();  // wait SendStopMessage
        ret = jobAdapter_->StopProf();
        if (ret != PROFILING_SUCCESS) {
            break;
        }
        ret = GetHostAndDeviceInfo();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("GetHostAndDeviceInfo failed");
        }
        (void)CreateCollectionTimeInfo(GetHostTime(), false);
    } while (0);
}

int ProfTask::Stop()
{
    PostStopReplay();
    return PROFILING_SUCCESS;
}

int ProfTask::Wait()
{
    MSPROF_LOGI("Device(%d) wait begin", deviceId_);
    isQuited_ = true;
    Join();
    MSPROF_LOGI("Device(%d) wait end", deviceId_);
    WriteDone();
    return 0;
}

void ProfTask::WriteDone()
{
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    UploaderMgr::instance()->GetUploader(params_->job_id, uploader);
    if (uploader != nullptr) {
        MSPROF_LOGI("Flush all data, jobId: %s", params_->job_id.c_str());
        (void)uploader->Flush();
        auto transport = uploader->GetTransport();
        if (transport != nullptr) {
            transport->WriteDone();
        }
        uploader = nullptr;
    }
}

void ProfTask::GenerateFileName(bool isStartTime, std::string &filename)
{
    if (!isStartTime) {
        filename.append("end_info");
    } else {
        filename.append("start_info");
    }
    if (!(params_->host_profiling)) {
        filename.append(".").append(std::to_string(deviceId_));
    }
}

void ProfTask::SaveRankId(analysis::dvvp::proto::CollectionStartEndTime &timeInfo)
{
    if (!Utils::IsClusterRunEnv()) {
        MSPROF_LOGI("It is not cluster run environment.");
        return;
    }
    uint32_t rankId;
    int32_t ret = HcclPlugin::instance()->MsprofHcomGetRankId(&rankId);
    if (ret == 0) {
        MSPROF_LOGI("Get rank id success, rankId=%d.", rankId);
        timeInfo->set_rankid(rankId);
    } else {
        MSPROF_LOGE("Get rank id fail, ret=%d.", ret);
        timeInfo->set_rankid(12345); //TODO: delete
    }
}

int ProfTask::CreateCollectionTimeInfo(std::string collectionTime, bool isStartTime)
{
    MSPROF_LOGI("collectionTime:%s us, isStartTime:%d", collectionTime.c_str(), isStartTime);
    // time to unix
    static const int TIME_US = 1000000;
    SHARED_PTR_ALIA<analysis::dvvp::proto::CollectionStartEndTime> timeInfo = nullptr;
    MSVP_MAKE_SHARED0_RET(timeInfo, analysis::dvvp::proto::CollectionStartEndTime, PROFILING_FAILED);
    if (!isStartTime) {
        timeInfo->set_collectiontimeend(collectionTime);
        timeInfo->set_collectiondateend(Utils::TimestampToTime(collectionTime, TIME_US));
        SaveRankId(timeInfo);
    } else {
        timeInfo->set_collectiontimebegin(collectionTime);
        timeInfo->set_collectiondatebegin(Utils::TimestampToTime(collectionTime, TIME_US));
    }
    timeInfo->set_clockmonotonicraw(std::to_string(Utils::GetClockMonotonicRaw()));
    std::string content =  analysis::dvvp::message::EncodeJson(timeInfo, false, false);
    MSPROF_LOGI("CreateCollectionTimeInfo, content:%s", content.c_str());
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->job_id = params_->job_id;
    std::string fileName;
    GenerateFileName(isStartTime, fileName);
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
        analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);

    MSPROF_LOGI("job_id: %s,fileName: %s", params_->job_id.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadFileData(params_->job_id,
        content, fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfTask::GetHostAndDeviceInfo()
{
    std::string endTime = GetHostTime();
    if (endTime.empty()) {
        MSPROF_LOGE("gettimeofday failed");
        return PROFILING_FAILED;
    }
    InfoJson infoJson(params_->jobInfo, params_->devices, params_->host_sys_pid);
    std::string content;
    if (infoJson.Generate(content) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to generate info.json");
        return PROFILING_FAILED;
    }

    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->job_id = params_->job_id;
    std::string fileName;
    if (!(params_->host_profiling)) {
        fileName.append(INFO_FILE_NAME).append(".").append(params_->devices);
    } else {
        fileName.append(INFO_FILE_NAME);
    }
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
        analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);

    MSPROF_LOGI("storeStartTime.id: %s,fileName: %s", params_->job_id.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadFileData(params_->job_id,
        content, fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}


std::string ProfTask::GetHostTime()
{
    std::string hostTime;
    mmTimeval tv;
    const int TIME_US = 1000000;

    (void)memset_s(&tv, sizeof(tv), 0, sizeof(tv));
    int ret = MmGetTimeOfDay(&tv, nullptr);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("gettimeofday failed");
    } else {
        hostTime = std::to_string((unsigned long long)tv.tv_sec * TIME_US + (unsigned long long)tv.tv_usec);
    }
    return hostTime;
}

ProfSocTask::ProfSocTask(const int deviceId, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param)
    : ProfTask(deviceId, param)
{
}

ProfSocTask::~ProfSocTask() {}

int ProfSocTask::Init()
{
    MSPROF_LOGI("Init SOC JobAdapter");
    auto jobFactory = JobSocFactory();
    jobAdapter_ = jobFactory.CreateJobAdapter(deviceId_);
    if (jobAdapter_ == nullptr) {
        return PROFILING_FAILED;
    }
    isInit_ = true;
    return PROFILING_SUCCESS;
}

int ProfSocTask::UnInit()
{
    jobAdapter_.reset();
    isInit_ = false;
    return PROFILING_SUCCESS;
}

ProfRpcTask::ProfRpcTask(const int deviceId, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param)
    : ProfTask(deviceId, param), isDataChannelEnd_(false)
{
}

ProfRpcTask::~ProfRpcTask() {}

int ProfRpcTask::Init()
{
    analysis::dvvp::transport::LoadDevMgrAPI(devMgrAPI_);
    if (devMgrAPI_.pfDevMgrInit == nullptr) {
        MSPROF_LOGE("pfDevMgrInit is null");
        return PROFILING_FAILED;
    }
    int ret = devMgrAPI_.pfDevMgrInit(params_->job_id, deviceId_, params_->profiling_mode);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to connect device %d", deviceId_);
        return ret;
    }
    MSPROF_LOGI("Init Rpc JobAdapter");
    MSVP_MAKE_SHARED1_RET(jobAdapter_, JobDeviceRpc, deviceId_, PROFILING_FAILED);
    isInit_ = true;
    return PROFILING_SUCCESS;
}

int ProfRpcTask::UnInit()
{
    if (devMgrAPI_.pfDevMgrUnInit != nullptr) {
        (void)devMgrAPI_.pfDevMgrUnInit();
    }
    jobAdapter_.reset();
    isInit_ = false;
    return PROFILING_SUCCESS;
}

int ProfRpcTask::Stop()
{
    PostStopReplay();
    MSPROF_LOGI("Device(%d) WaitSyncDataCtrl begin", deviceId_);
    WaitSyncDataCtrl();
    MSPROF_LOGI("Device(%d) WaitSyncDataCtrl end", deviceId_);
    return PROFILING_SUCCESS;
}


/**
 * @brief Send data sync signal
 */
void ProfRpcTask::PostSyncDataCtrl()
{
    MSPROF_LOGI("Device(%d) jobId(%s) post data channel.", deviceId_, params_->job_id.c_str());
    std::unique_lock<std::mutex> lk(dataSyncMtx_);
    isDataChannelEnd_ = true;
    cvSyncDataCtrl_.notify_one();
}

/**
 * @brief Wait data sync signal
 */
void ProfRpcTask::WaitSyncDataCtrl()
{
    std::unique_lock<std::mutex> lk(dataSyncMtx_);
    cvSyncDataCtrl_.wait(lk, [=] { return (isDataChannelEnd_ || !isExited_); });
    isDataChannelEnd_ = false;
}
}
}
}
