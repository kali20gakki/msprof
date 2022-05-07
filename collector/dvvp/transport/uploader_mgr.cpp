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
#include "proto/profiler.pb.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::proto;
using namespace analysis::dvvp::common::error;

UploaderMgr::UploaderMgr()
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
        MSPROF_LOGI("Upadate devModeJobMap_");
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

int UploaderMgr::UploadFileData(const std::string &id,
    const std::string &data,
    const struct FileDataParams &fileDataParams,
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx)
{
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

    SHARED_PTR_ALIA<FileChunkReq> fileChunk;
    MSVP_MAKE_SHARED0_RET(fileChunk, FileChunkReq, PROFILING_FAILED);

    fileChunk->set_filename(fileDataParams.fileName);
    fileChunk->set_offset(-1);
    fileChunk->set_chunk(data.c_str(), data.size());
    fileChunk->set_chunksizeinbytes(data.size());
    fileChunk->set_islastchunk(fileDataParams.isLastChunk);
    fileChunk->set_needack(false);
    fileChunk->mutable_hdr()->set_job_ctx(jobCtx->ToString());
    fileChunk->set_datamodule(fileDataParams.mode);
    auto enc = analysis::dvvp::message::EncodeMessageShared(fileChunk);
    if (enc == nullptr) {
        MSPROF_LOGE("fileChunk encode failed. fileName:%s", fileDataParams.fileName.c_str());
        MSPROF_INNER_ERROR("EK9999", "fileChunk encode failed. fileName:%s", fileDataParams.fileName.c_str());
        return PROFILING_FAILED;
    }
    return analysis::dvvp::transport::UploaderMgr::instance()->UploadData(id,
        static_cast<CONST_VOID_PTR>(const_cast<CHAR_PTR>(enc->c_str())),
        (uint32_t)enc->size());
}
}
}
}

