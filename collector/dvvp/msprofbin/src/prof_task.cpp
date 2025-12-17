/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "prof_task.h"
#include "common/config/feature_manager.h"
#include "errno/error_code.h"
#include "config/config.h"
#include "job_factory.h"
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
        ret = CreateIncompatibleFeatureJsonFile();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("ProcessDefMode CreateIncompatibleFeatureJsonFile failed");
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

int ProfTask::CreateCollectionTimeInfo(std::string collectionTime, bool isStartTime)
{
    MSPROF_LOGI("collectionTime:%s us, isStartTime:%d", collectionTime.c_str(), isStartTime);
    // time to unix
    static const int TIME_US = 1000000;
    analysis::dvvp::message::CollectionStartEndTime timeInfo;
    if (!isStartTime) {
        timeInfo.collectionTimeEnd = collectionTime;
        timeInfo.collectionDateEnd = Utils::TimestampToTime(collectionTime, TIME_US);
    } else {
        timeInfo.collectionTimeBegin = collectionTime;
        timeInfo.collectionDateBegin = Utils::TimestampToTime(collectionTime, TIME_US);
    }
    timeInfo.clockMonotonicRaw = std::to_string(Utils::GetClockMonotonicRaw());
    nlohmann::json root;
    timeInfo.ToObject(root);
    std::string content = root.dump();
    MSPROF_LOGI("CreateCollectionTimeInfo, content:%s", content.c_str());
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->job_id = params_->job_id;
    std::string fileName;
    GenerateFileName(isStartTime, fileName);
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
        analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_CTRL_DATA);

    MSPROF_LOGI("job_id: %s,fileName: %s", params_->job_id.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadCtrlFileData(params_->job_id,
        content, fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfTask::CreateIncompatibleFeatureJsonFile()
{
    MSPROF_LOGI("MsprofBin CreateIncompatibleFeatureJsonFile.");
    if (Common::Config::FeatureManager::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init features list.");
        return PROFILING_FAILED;
    }
    return Common::Config::FeatureManager::instance()->CreateIncompatibleFeatureJsonFile(params_);
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
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadCtrlFileData(params_->job_id,
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
    MmTimeval tv;
    const int TIME_US = 1000000;

    if (memset_s(&tv, sizeof(tv), 0, sizeof(tv)) != EOK) {
        MSPROF_LOGE("memset failed");
        return "";
    }
    int ret = MmGetTimeOfDay(&tv, nullptr);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("gettimeofday failed");
    } else {
        hostTime = std::to_string((unsigned long long)tv.tvSec * TIME_US + (unsigned long long)tv.tvUsec);
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

}
}
}
