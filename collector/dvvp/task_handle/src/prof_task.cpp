/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "prof_task.h"
#include <algorithm>
#include "config/config.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "prof_acl_mgr.h"
#include "prof_manager.h"
#include "proto/msprofiler.pb.h"
#include "securec.h"
#include "transport/transport.h"
#include "transport/file_transport.h"
#include "transport/uploader.h"
#include "transport/uploader_mgr.h"
#include "utils/utils.h"
#include "info_json.h"
#include "config/config_manager.h"
#include "mmpa_api.h"

namespace analysis {
namespace dvvp {
namespace host {
using namespace analysis::dvvp::proto;
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

ProfTask::ProfTask(const std::vector<std::string> &devices,
                   SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param)
    : params_(param), currDevicesV_(devices), isInited_(false), isFinished_(false) {}

ProfTask::~ProfTask()
{
    if (isInited_) {
        devicesMap_.clear();
        StopNoWait();
        cv_.notify_one();
        Join();
        isInited_ = false;
    }
    MSPROF_LOGI("Destroy ProfTask.");
}

int ProfTask::Init()
{
    MSPROF_LOGI("Init ProfTask");
    if (params_ == nullptr) {
        MSPROF_LOGE("Params for prof task is null");
        MSPROF_INNER_ERROR("EK9999", "Params for prof task is null");
        return PROFILING_FAILED;
    }

    analysis::dvvp::transport::UploaderMgr::instance()->GetUploader(params_->job_id, uploader_);
    if (uploader_ == nullptr) {
        MSPROF_LOGE("Failed to get correct uploader");
        MSPROF_INNER_ERROR("EK9999", "Failed to get correct uploader");
        return PROFILING_FAILED;
    }

    isInited_ = true;
    return PROFILING_SUCCESS;
}

int ProfTask::Uinit()
{
    if (isInited_) {
        WriteDone();
        analysis::dvvp::transport::UploaderMgr::instance()->DelUploader(params_->job_id);
        isInited_ = false;
        MSPROF_EVENT("Uninit ProfTask succesfully");
    }

    return PROFILING_SUCCESS;
}

bool ProfTask::IsDeviceRunProfiling(const std::string &devStr)
{
    std::lock_guard<std::mutex> lck(devicesMtx_);
    auto iter = std::find(currDevicesV_.begin(), currDevicesV_.end(), devStr);
    if (iter != currDevicesV_.end()) {
        return true;
    }
    return false;
}

std::string ProfTask::GetDevicesStr(const std::vector<std::string> &events)
{
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;
    return builder.Join(events, ",");
}

void ProfTask::GenerateFileName(bool isStartTime, std::string &filename)
{
    if (!isStartTime) {
        filename.append("end_info");
    } else {
        filename.append("start_info");
    }
    if (!(params_->host_profiling)) {
        filename.append(".").append(params_->devices);
    }
}

int ProfTask::CreateCollectionTimeInfo(std::string collectionTime, bool isStartTime)
{
    MSPROF_LOGI("[CreateCollectionTimeInfo]collectionTime:%s us, isStartTime:%d", collectionTime.c_str(), isStartTime);
    // time to unix
    SHARED_PTR_ALIA<analysis::dvvp::proto::CollectionStartEndTime> timeInfo = nullptr;
    MSVP_MAKE_SHARED0_RET(timeInfo, analysis::dvvp::proto::CollectionStartEndTime, PROFILING_FAILED);
    static const int TIME_US = 1000000;
    if (!isStartTime) {
        timeInfo->set_collectiontimeend(collectionTime);
        timeInfo->set_collectiondateend(Utils::TimestampToTime(collectionTime, TIME_US));
    } else {
        timeInfo->set_collectiontimebegin(collectionTime);
        timeInfo->set_collectiondatebegin(Utils::TimestampToTime(collectionTime, TIME_US));
    }

    timeInfo->set_clockmonotonicraw(std::to_string(Utils::GetClockMonotonicRaw()));
    std::string content =  analysis::dvvp::message::EncodeJson(timeInfo, false, false);
    MSPROF_LOGI("[CreateCollectionTimeInfo]content:%s", content.c_str());
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->job_id = params_->job_id;
    std::string fileName;
    GenerateFileName(isStartTime, fileName);
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
        FileChunkDataModule::PROFILING_IS_CTRL_DATA);
    MSPROF_LOGI("[CreateCollectionTimeInfo]job_id: %s,fileName: %s", params_->job_id.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadCtrlFileData(
        params_->job_id, content, fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to upload data for %s", fileName.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to upload data for %s", fileName.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfTask::GetHostAndDeviceInfo()
{
    std::vector<std::string> devicesListV;
    devicesListV = _devices_v;
    std::string devicesStr = GetDevicesStr(devicesListV);
    MSPROF_LOGI("GetHostAndDeviceInfo, devices: %s", devicesStr.c_str());
    std::string endTime = GetHostTime();
    if (endTime.empty()) {
        MSPROF_LOGE("gettimeofday failed");
        MSPROF_INNER_ERROR("EK9999", "gettimeofday failed");
        return PROFILING_FAILED;
    }
    InfoJson infoJson(params_->jobInfo, devicesStr, params_->host_sys_pid);
    std::string content;
    if (infoJson.Generate(content) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetHostAndDeviceInfo]Failed to generate info.json");
        MSPROF_INNER_ERROR("EK9999", "Failed to generate info.json");
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    jobCtx->job_id = params_->job_id;
    std::string fileName;
    if (params_->host_profiling) {
        fileName.append(INFO_FILE_NAME);
    } else {
        fileName.append(INFO_FILE_NAME).append(".").append(params_->devices);
    }
    analysis::dvvp::transport::FileDataParams fileDataParams(fileName, true,
        FileChunkDataModule::PROFILING_IS_CTRL_DATA);
    MSPROF_LOGI("[GetHostAndDeviceInfo]storeStartTime.id: %s,fileName: %s", params_->job_id.c_str(), fileName.c_str());
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadCtrlFileData(params_->job_id, content,
        fileDataParams, jobCtx);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetHostAndDeviceInfo]Failed to upload data for %s", fileName.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to upload data for %s", fileName.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

std::string ProfTask::GetHostTime()
{
    std::string hostTime;
    MmTimeval tv;

    if (memset_s(&tv, sizeof(tv), 0, sizeof(tv)) != EOK) {
        MSPROF_LOGE("memset failed");
        return "";
    }
    int ret = MmGetTimeOfDay(&tv, nullptr);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[GetHostTime]gettimeofday failed");
        MSPROF_INNER_ERROR("EK9999", "gettimeofday failed");
    } else {
        const int TIME_US = 1000000;
        hostTime = std::to_string((unsigned long long)tv.tvSec * TIME_US + (unsigned long long)tv.tvUsec);
    }
    return hostTime;
}

void ProfTask::StartDevices(const std::vector<std::string> &devicesVec)
{
    std::lock_guard<std::mutex> lck(devicesMtx_);
    for (size_t i = 0; i < devicesVec.size(); i++) {
        if (devicesVec[i].compare("") == 0) {
            continue;
        }
        auto iter = devicesMap_.find(devicesVec[i]);
        if (iter != devicesMap_.end()) {
            MSPROF_LOGE("Device %s is already running profiling, skip the device.", devicesVec[i].c_str());
            MSPROF_INNER_ERROR("EK9999", "Device %s is already running profiling, skip the device.",
                devicesVec[i].c_str());
            continue;
        }

        // insert the new device
        _devices_v.push_back(devicesVec[i]);
        MSPROF_LOGI("Device %s begin init.", devicesVec[i].c_str());

        SHARED_PTR_ALIA<Device> dev;
        MSVP_MAKE_SHARED2_CONTINUE(dev, Device, params_, devicesVec[i]);

        int ret = dev->Init();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Device %s init failed, ret:%d", devicesVec[i].c_str(), ret);
            MSPROF_INNER_ERROR("EK9999", "Device %s init failed, ret:%d", devicesVec[i].c_str(), ret);
            continue;
        }

        MSPROF_LOGI("Device set Response %s", devicesVec[i].c_str());
        dev->SetResponseCallback(Msprofiler::Api::DeviceResponse);
        static const int DEVICE_THREAD_NAME_LEN = 6;
        dev->SetThreadName(MSVP_DEVICE_THREAD_NAME_PREFIX + devicesVec[i].substr(0, DEVICE_THREAD_NAME_LEN));

        ret = dev->Start();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Device %s start failed, ret:%d", devicesVec[i].c_str(), ret);
            MSPROF_INNER_ERROR("EK9999", "Device %s start failed, ret:%d", devicesVec[i].c_str(), ret);
            continue;
        }

        devicesMap_[devicesVec[i]] = dev;

        MSPROF_LOGI("Device %s init success.", devicesVec[i].c_str());
    }
}

void ProfTask::ProcessDefMode()
{
    MSPROF_LOGI("Profiling running task");
    StartDevices(currDevicesV_);
    int ret = GetHostAndDeviceInfo();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("ProcessDefMode GetHostAndDeviceInfo failed");
        MSPROF_INNER_ERROR("EK9999", "ProcessDefMode GetHostAndDeviceInfo failed");
    }
    // wait stop notify
    std::unique_lock<std::mutex> lk(taskMtx_);
    MSPROF_EVENT("ProfTask %s started to wait for task stop cv", params_->job_id.c_str());
    cv_.wait(lk, [&] { return this->IsQuit(); });
    MSPROF_EVENT("ProfTask %s finished waiting for task stop cv", params_->job_id.c_str());
}

int ProfTask::NotifyFileDoneForDevice(const std::string &fileName, const std::string &devId) const
{
    SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk;
    MSVP_MAKE_SHARED0_RET(fileChunk, analysis::dvvp::ProfileFileChunk, PROFILING_FAILED);

    analysis::dvvp::message::JobContext jobCtx;
    jobCtx.job_id = params_->job_id;
    jobCtx.dev_id = devId;

    fileChunk->fileName = Utils::PackDotInfo(fileName, jobCtx.tag);
    fileChunk->offset = -1;
    fileChunk->chunkSize = 0U;
    fileChunk->isLastChunk = true;
    fileChunk->extraInfo = Utils::PackDotInfo(jobCtx.job_id, jobCtx.dev_id);
    fileChunk->chunkStartTime = 0U;
    fileChunk->chunkEndTime = 0U;
    fileChunk->chunk.assign("");
    fileChunk->chunkModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_DEFAULT_DATA_MODULE;
    int ret = WriteStreamData(fileChunk);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("NotifyFileDoneForDevice failed, jobId:%s, filename:%s, devId:%s", jobCtx.job_id.c_str(),
            fileName.c_str(), devId.c_str());
    }
    return PROFILING_SUCCESS;
}

void ProfTask::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    // process prepare
    MSPROF_LOGI("Task %s begins to run", params_->job_id.c_str());

    int ret = CreateCollectionTimeInfo(GetHostTime(), true);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("ProcessDefMode CreateCollectionTimeInfo failed");
        MSPROF_INNER_ERROR("EK9999", "ProcessDefMode CreateCollectionTimeInfo failed");
    }
    ProcessDefMode();

    // wait all device thread stop
    for (auto iter = devicesMap_.begin(); iter != devicesMap_.end(); iter++) {
        iter->second->PostStopReplay();
        iter->second->Wait();
        (void)NotifyFileDoneForDevice("", iter->first);
    }
    MSPROF_LOGI("Prof task begins to finish");
    (void)CreateCollectionTimeInfo(GetHostTime(), false);
    (void)uploader_->Flush();
    uploader_ = nullptr;

    MSPROF_LOGI("Prof task OnTaskFinished");
    ProfManager::instance()->OnTaskFinished(params_->job_id);

    MSPROF_EVENT("Task %s finished", params_->job_id.c_str());
}

int ProfTask::Stop()
{
    MSPROF_EVENT("Task send finished cv");
    StopNoWait();
    cv_.notify_one();
    auto ret = Join();
    if (ret == PROFILING_SUCCESS) {
        MSPROF_LOGI("Task %s stopped", params_->job_id.c_str());
    }
    WriteDone();
    return ret;
}

void ProfTask::WriteDone()
{
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    analysis::dvvp::transport::UploaderMgr::instance()->GetUploader(params_->job_id, uploader);
    if (uploader == nullptr) {
        return;
    }
    MSPROF_LOGI("[WriteDone]Flush all data, jobId: %s", params_->job_id.c_str());
    (void)uploader->Flush();
    auto transport = uploader->GetTransport();
    if (transport != nullptr) {
        transport->WriteDone();
    }
    uploader = nullptr;
}

int ProfTask::WriteStreamData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk) const
{
    if (GetIsFinished()) {
        MSPROF_LOGW("Profiling is already finished, jobId: %s", params_->job_id.c_str());
        return PROFILING_SUCCESS;
    }

    if (uploader_ != nullptr && fileChunk != nullptr) {
        return uploader_->UploadData(fileChunk);
    }
    return PROFILING_FAILED;
}

void ProfTask::SetIsFinished(bool finished)
{
    isFinished_ = finished;
}

bool ProfTask::GetIsFinished() const
{
    return isFinished_;
}
}  // namespace host
}  // namespace dvvp
}  // namespace analysis
