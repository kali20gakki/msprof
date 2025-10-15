/**
* @file uploader_mgr.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "uploader_mgr.h"
#include "errno/error_code.h"
#include "utils.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
namespace {
const std::set<std::string> FLIP_DATA_NAME_SET{"data/ts_track", "data/aicpu.data"};
}

UploaderMgr::UploaderMgr() : isStarted_(true)
{
}

UploaderMgr::~UploaderMgr()
{
    uploaderMap_.clear();
    devModeJobMap_.clear();
}

int UploaderMgr::Init() const
{
    return PROFILING_SUCCESS;
}

int UploaderMgr::Uninit()
{
    uploaderMap_.clear();
    devModeJobMap_.clear();

    return PROFILING_SUCCESS;
}

void UploaderMgr::AddMapByDevIdMode(int devId, const std::string &mode, const std::string &jobId)
{
    std::string devModeKey = std::to_string(devId) + "_" + mode;
    if (mode.empty()) {
        devModeKey += analysis::dvvp::message::PROFILING_MODE_DEF;
    }
    MSPROF_LOGI("devModeKey:%s, jobId:%s Entering UpdateDevModeJobMap...", devModeKey.c_str(), jobId.c_str());
    std::lock_guard<std::mutex> lock(uploaderMutex_);
    auto mapIter = devModeJobMap_.find(devModeKey);
    if (mapIter != devModeJobMap_.end()) {
        MSPROF_LOGI("Update devModeJobMap_");
    } else {
        MSPROF_LOGI("Add to devModeJobMap_");
    }
    devModeJobMap_[devModeKey] = jobId;
}

std::string UploaderMgr::GetJobId(int devId, const std::string &mode)
{
    std::string devModeKey = std::to_string(devId) + "_" + mode;
    if (mode.empty()) {
        devModeKey += analysis::dvvp::message::PROFILING_MODE_DEF;
    }
    MSPROF_LOGI("devModeKey:%s, Entering GetJobId...", devModeKey.c_str());
    std::lock_guard<std::mutex> lock(uploaderMutex_);
    auto mapIter = devModeJobMap_.find(devModeKey);
    if (mapIter != devModeJobMap_.end()) {
        return mapIter->second;
    } else {
        return "";
    }
}

int UploaderMgr::CreateUploader(const std::string &id, SHARED_PTR_ALIA<ITransport> transport, size_t queueSize)
{
    if (transport == nullptr) {
        MSPROF_LOGE("Transport is invalid!");
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    MSVP_MAKE_SHARED1_RET(uploader, Uploader, transport, PROFILING_FAILED);
    int ret = uploader->Init(queueSize);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init uploader");
        MSPROF_INNER_ERROR("EK9999", "Failed to init uploader");
        return ret;
    }
    std::string uploaderName = analysis::dvvp::common::config::MSVP_UPLOADER_THREAD_NAME;
    uploaderName.append("_").append(id);
    uploader->SetThreadName(uploaderName);
    ret = uploader->Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start uploader thread");
        MSPROF_INNER_ERROR("EK9999", "Failed to start uploader thread");
        return ret;
    }
    AddUploader(id, uploader);
    return PROFILING_SUCCESS;
}

void UploaderMgr::AddUploader(const std::string &id, SHARED_PTR_ALIA<Uploader> uploader)
{
    MSPROF_LOGI("id: %s Entering AddUploader...", id.c_str());
    std::lock_guard<std::mutex> lock(uploaderMutex_);
    if (uploader != nullptr && !id.empty()) {
        uploaderMap_[id] = uploader;
    }
}

void UploaderMgr::GetUploader(const std::string &id, SHARED_PTR_ALIA<Uploader> &uploader)
{
    MSPROF_LOGD("Get id %s uploader...", id.c_str());
    std::lock_guard<std::mutex> lock(uploaderMutex_);
    auto iter = uploaderMap_.find(id);
    if (iter != uploaderMap_.end() && iter->second != nullptr) {
        uploader = iter->second;
    }
}

void UploaderMgr::DelUploader(const std::string &id)
{
    MSPROF_LOGI("Del id %s uploader...", id.c_str());
    std::lock_guard<std::mutex> lock(uploaderMutex_);
    auto iter = uploaderMap_.find(id);
    if (iter != uploaderMap_.end()) {
        uploaderMap_.erase(iter);
    }
}

void UploaderMgr::DelAllUploader()
{
    MSPROF_LOGI("DelAllUploader");
    std::lock_guard<std::mutex> lock(uploaderMutex_);
    uploaderMap_.clear();
}

int UploaderMgr::UploadData(const std::string &id, CONST_VOID_PTR data, uint32_t dataLen)
{
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    GetUploader(id, uploader);
    if (uploader != nullptr) {
        return uploader->UploadData(data, dataLen);
    } else {
        MSPROF_LOGE("Failed to find uploader for %s", id.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to find uploader for %s", id.c_str());
    }
    MSPROF_LOGE("Get id[%s] uploader failed, dataLen:%d", id.c_str(), dataLen);
    MSPROF_INNER_ERROR("EK9999", "Get id[%s] uploader failed, dataLen:%d", id.c_str(), dataLen);

    return PROFILING_FAILED;
}

int UploaderMgr::UploadData(const std::string &id, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (!IsUploadDataStart(id, fileChunkReq->fileName)) {
        return PROFILING_IN_WARMUP;
    }

    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    GetUploader(id, uploader);
    if (uploader != nullptr) {
        return uploader->UploadData(fileChunkReq);
    }

    MSPROF_LOGE("Get id[%s] uploader failed", id.c_str());
    MSPROF_INNER_ERROR("EK9999", "Get id[%s] uploader failed", id.c_str());
    return PROFILING_FAILED;
}

int UploaderMgr::UploadCtrlFileData(const std::string &id,
    const std::string &data,
    const struct FileDataParams &fileDataParams,
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx)
{
    MSPROF_LOGI("UploadCtrlFileData id[%s] uploader start", id.c_str());
    if (data.empty()) {
        MSPROF_LOGE("data is empty");
        MSPROF_INNER_ERROR("EK9999", "data is empty");
        return PROFILING_FAILED;
    }

    if (jobCtx == nullptr) {
        MSPROF_LOGE("jobCtx is null");
        MSPROF_INNER_ERROR("EK9999", "jobCtx is null");
        return PROFILING_FAILED;
    }

    SHARED_PTR_ALIA<ProfileFileChunk> fileChunk;
    MSVP_MAKE_SHARED0_RET(fileChunk, ProfileFileChunk, PROFILING_FAILED);

    fileChunk->fileName = fileDataParams.fileName;
    fileChunk->offset = -1;
    fileChunk->chunk = std::move(data);
    fileChunk->chunkSize = data.size();
    fileChunk->isLastChunk = fileDataParams.isLastChunk;
    fileChunk->chunkModule = fileDataParams.mode;
    fileChunk->extraInfo = Utils::PackDotInfo(jobCtx->job_id, jobCtx->dev_id);
    fileChunk->chunkStartTime = 0U;
    fileChunk->chunkEndTime = 0U;
    return analysis::dvvp::transport::UploaderMgr::instance()->UploadData(id, fileChunk);
}

bool UploaderMgr::IsUploadDataStart(const std::string& id, const std::string& fileName)
{
    // 仅拦截device数据（均为data开头），同时确保ts_track中的flip相关数据正常落盘解析
    if (isStarted_) {
        return true;
    }

    if (id == std::to_string(common::utils::DEFAULT_HOST_ID)) {
        return true;
    }

    for (const auto& flipDataName : FLIP_DATA_NAME_SET) {
        if (fileName.rfind(flipDataName, 0) == 0) {
            return true;
        }
    }

    if (fileName.rfind("data", 0) == 0) {
        return false;
    }
    return true;
}

void UploaderMgr::SetUploaderStatus(bool status)
{
    MSPROF_LOGI("Uploader status will be set %d.", status);
    isStarted_ = status;
}
}
}
}

