/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: file slice
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#include "file_slice.h"
#include "file_ageing.h"
#include "config/config.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "utils/utils.h"
#include "mmpa_plugin.h"

using namespace Analysis::Dvvp::Common::Statistics;

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;

FileSlice::FileSlice(int sliceFileMaxKByte, const std::string &storageDir, const std::string &storageLimit)
    : sliceFileMaxKByte_(sliceFileMaxKByte), storageDir_(storageDir),
      needSlice_(true), storageLimit_(storageLimit)
{
}

FileSlice::~FileSlice() {}

int FileSlice::Init(bool needSlice)
{
    static const std::string writePerfcountModuleName = std::string("FileSlice");
    MSVP_MAKE_SHARED1_RET(writeFilePerfCount_, PerfCount, writePerfcountModuleName, PROFILING_FAILED);
    bool isPathValid = analysis::dvvp::common::utils::Utils::IsDirAccessible(storageDir_);
    if (!isPathValid) {
        MSPROF_LOGE("para err, storageDir_:%s, storageDirLen:%d",
                    Utils::BaseName(storageDir_).c_str(), storageDir_.length());
        return PROFILING_FAILED;
    }
    MSVP_MAKE_SHARED2_RET(fileAgeing_, FileAgeing, storageDir_, storageLimit_, PROFILING_FAILED);
    int ret = fileAgeing_->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init file ageing engine");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("StorageDir_:%s, sliceFileMaxKByte:%d, needSlice_:%d, storage_limit:%s",
                Utils::BaseName(storageDir_).c_str(), sliceFileMaxKByte_, needSlice_, storageLimit_.c_str());
    analysis::dvvp::common::utils::Utils::EnsureEndsInSlash(storageDir_);
    needSlice_ = needSlice;

    return PROFILING_SUCCESS;
}

/**
 * @brief  : get key( dir/fileName.slice_ )
 * @param  : [in] std::string dir :
 * @param  : [in] std::string fileName :
 * @return : std::string key
 */
std::string FileSlice::GetSliceKey(const std::string &dir, std::string &fileName)
{
    if (needSlice_) {
        fileName.append(".slice_");
    }
    std::vector<std::string> absolutePathV;
    absolutePathV.push_back(dir);
    absolutePathV.push_back(fileName);
    std::string key = analysis::dvvp::common::utils::Utils::JoinPath(absolutePathV);
    auto iter = sliceNum_.find(key);
    if (iter == sliceNum_.end()) {
        sliceNum_[key] = 0;
        totalSize_[key] = 0;
    }

    return key;
}

/**
 * @brief  : set start time or end time
 * @param  : [in] std::string key :
 * @param  : [in] uint64_t startTime :
 * @param  : [in] uint64_t endTime :
 * @return : PROFILING_FAILED (-1) failed
 *         : PROFILING_SUCCES (0)  success
 */
int FileSlice::SetChunkTime(const std::string &key, uint64_t startTime, uint64_t endTime)
{
    if (key.length() == 0) {
        MSPROF_LOGE("key err");
        return PROFILING_FAILED;
    }

    if (!needSlice_) {
        return PROFILING_SUCCESS;
    }

    std::string sliceName = key;
    if (sliceNum_.find(key) == sliceNum_.end()) {
        sliceNum_[key] = 0;
    }
    sliceName.append(std::to_string(sliceNum_[key]));
    if (!(analysis::dvvp::common::utils::Utils::IsFileExist(sliceName)) ||
        analysis::dvvp::common::utils::Utils::GetFileSize(sliceName) == 0) {
        chunkStartTime_[sliceName] = startTime;
        MSPROF_LOGD("Set start time, slicename:%s, starttime:%lld ns", sliceName.c_str(), startTime);
    }
    chunkEndTime_[sliceName] = endTime;
    return PROFILING_SUCCESS;
}

/**
 * @brief  : write data to slice files
 * @param  : [in] std::string key :
 * @param  : [in] const char* data :
 * @param  : [in] int dataLen :
 * @param  : [in] int offset :
 * @param  : [in] bool isLastChunk :
 * @return : PROFILING_FAILED (-1) failed
 *         : PROFILING_SUCCES (0)  success
 */
int FileSlice::WriteToLocalFiles(const std::string &key, CONST_CHAR_PTR data, int dataLen,
    int offset, bool isLastChunk)
{
    if (key.length() == 0) {
        MSPROF_LOGE("para err!");
        return PROFILING_FAILED;
    }

    if (sliceNum_.find(key) == sliceNum_.end()) {
        sliceNum_[key] = 0;
    }
    std::string absolutePath = key;
    if (needSlice_) {
        absolutePath.append(std::to_string(sliceNum_[key]));
    }

    if (data != nullptr && dataLen > 0) {
        uint64_t startRawTime = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
        std::ofstream out;
        out.open(absolutePath, std::ofstream::app | std::ios::binary);
        if (!out.is_open()) {
            int errorNo = MmpaPlugin::instance()->MsprofMmGetErrorCode();
            char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
            MSPROF_LOGE("Failed to open %s, ErrorCode:%d, errinfo:%s", Utils::BaseName(absolutePath).c_str(),
                errorNo, MmpaPlugin::instance()->MsprofMmGetErrorFormatMessage(errorNo, errBuf, MAX_ERR_STRING_LEN));
            return PROFILING_FAILED;
        }
        if (offset != -1) {
            out.seekp(offset);
        }
        out.write(data, dataLen);
        out.flush();
        out.close();
        totalSize_[key] += static_cast<uint32_t>(dataLen);
        uint64_t endRawTime = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
        writeFilePerfCount_->UpdatePerfInfo(startRawTime, endRawTime, dataLen); // update the PerfCount info
    }
    long long fileSize = analysis::dvvp::common::utils::Utils::GetFileSize(absolutePath);
    if (fileSize >= (long long)sliceFileMaxKByte_ * 1024 ||     // 1024 1k
        (isLastChunk && analysis::dvvp::common::utils::Utils::IsFileExist(absolutePath))) {
        if (!(CreateDoneFile(absolutePath, std::to_string(fileSize), std::to_string(chunkStartTime_[absolutePath]),
            std::to_string(chunkEndTime_[absolutePath]), absolutePath))) {
            MSPROF_LOGE("Failed to create file:%s_%llu", Utils::BaseName(key).c_str(), sliceNum_[key]);
            return PROFILING_FAILED;
        }
        MSPROF_LOGI("create done file:%s.done", Utils::BaseName(absolutePath).c_str());
        sliceNum_[key]++;
    }

    return PROFILING_SUCCESS;
}

int FileSlice::CheckDirAndMessage(analysis::dvvp::message::JobContext &jobCtx,
                                  SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> &fileChunkReq)
{
    if (fileChunkReq ==  nullptr) {
        MSPROF_LOGE("para err!");
        return PROFILING_FAILED;
    }
    if (!jobCtx.FromString(fileChunkReq->hdr().job_ctx())) {
        MSPROF_LOGE("Failed to parse jobCtx json %s. fileName:%s",
                    fileChunkReq->hdr().job_ctx().c_str(), Utils::BaseName(fileChunkReq->filename()).c_str());
        return PROFILING_FAILED;
    }
    if (fileChunkReq->islastchunk() && fileChunkReq->chunksizeinbytes() == 0) {
        return PROFILING_SUCCESS;
    }

    if (fileChunkReq->filename().length() == 0 || fileChunkReq->hdr().job_ctx().length() == 0 ||
        (!(fileChunkReq->islastchunk()) && fileChunkReq->chunksizeinbytes() == 0)) {
        MSPROF_LOGE("para err! filename.length:%d,jobCtx.length:%d,chunksizeinbytes:%d",
            fileChunkReq->filename().length(),
            fileChunkReq->hdr().job_ctx().length(), fileChunkReq->chunksizeinbytes());

        return PROFILING_FAILED;
    }

    std::string dir = analysis::dvvp::common::utils::Utils::DirName(storageDir_ + fileChunkReq->filename());
    if (dir.empty()) {
        MSPROF_LOGE("Failed to get dirname of filechunk");
        return PROFILING_FAILED;
    }
    if (analysis::dvvp::common::utils::Utils::CreateDir(dir) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to create dir %s for writing file", Utils::BaseName(dir).c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int FileSlice::WriteCtrlDataToFile(const std::string &absolutePath, const std::string &data, int dataLen)
{
    std::ofstream file;
    std::unique_lock<std::mutex> lk(sliceFileMtx_);

    if (analysis::dvvp::common::utils::Utils::IsFileExist(absolutePath)) {
        MSPROF_LOGI("file exist: %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_SUCCESS;
    }

    if (data.empty() || dataLen <= 0 || dataLen > MSVP_SMALL_FILE_MAX_LEN) {
        MSPROF_LOGE("Invalid ctrl data length");
        return PROFILING_FAILED;
    }
    file.open(absolutePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        MSPROF_LOGE("Failed to open %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_FAILED;
    }
    file.write(data.c_str(), dataLen);
    file.flush();
    file.close();

    if (!(CreateDoneFile(absolutePath, std::to_string(dataLen), "", "", ""))) {
        MSPROF_LOGE("set device done file failed");
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int FileSlice::SaveDataToLocalFiles(SHARED_PTR_ALIA<google::protobuf::Message> message)
{
    int ret = PROFILING_FAILED;
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunkReq = nullptr;
    fileChunkReq = std::dynamic_pointer_cast<analysis::dvvp::proto::FileChunkReq>(message);
    if (fileChunkReq == nullptr) {
        MSPROF_LOGE("failed to cast to FileTrunkReq");
        return ret;
    }
    analysis::dvvp::message::JobContext jobCtx;
    std::string fileName = fileChunkReq->filename();

    if (CheckDirAndMessage(jobCtx, fileChunkReq) == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    if (fileChunkReq->chunksizeinbytes() == 0 && fileChunkReq->islastchunk()) {
        return FileSliceFlushByJobID(storageDir_ + fileName, jobCtx.dev_id);
    }
    if (fileChunkReq->datamodule() == FileChunkDataModule::PROFILING_IS_CTRL_DATA) {
        return WriteCtrlDataToFile(storageDir_ + fileName,
                                   fileChunkReq->chunk(), fileChunkReq->chunksizeinbytes());
    }

    std::unique_lock<std::mutex> lk(sliceFileMtx_);
    std::string key = GetSliceKey(storageDir_, fileName);
    if (key.length() == 0) {
        MSPROF_LOGE("get key err");
        return PROFILING_FAILED;
    }
    if (fileChunkReq->chunksizeinbytes() > 0 && fileChunkReq->chunk().c_str() != nullptr) {
        ret = SetChunkTime(key, fileChunkReq->chunkstarttime(), fileChunkReq->chunkendtime());
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to set chunk time, chunkstarttime: %llu, chunkendtime: %llu ns",
                        fileChunkReq->chunkstarttime(), fileChunkReq->chunkendtime());
            return PROFILING_FAILED;
        }
    }
    if (fileChunkReq->datamodule() == FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST) {
        long long fileSize = analysis::dvvp::common::utils::Utils::GetFileSize(key);
        if (fileSize != PROFILING_FAILED) {
            fileChunkReq->set_offset(fileSize);
        }
    }
    ret = WriteToLocalFiles(key, fileChunkReq->chunk().c_str(), fileChunkReq->chunksizeinbytes(),
        fileChunkReq->offset(), fileChunkReq->islastchunk());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to write local files, fileName: %s", Utils::BaseName(fileName).c_str());
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

bool FileSlice::CreateDoneFile(const std::string &absolutePath, const std::string &fileSize,
    const std::string &startTime, const std::string &endTime, const std::string &timeKey)
{
    if (!needSlice_) {
        return true;
    }
    std::string tempPath = absolutePath + ".done";
    std::ofstream file;
    file.open(tempPath);
    if (!file.is_open()) {
        int errorNo = MmpaPlugin::instance()->MsprofMmGetErrorCode();
        char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
        MSPROF_LOGE("Failed to open %s, ErrorCode:%d, errinfo:%s", Utils::BaseName(tempPath).c_str(), errorNo,
            MmpaPlugin::instance()->MsprofMmGetErrorFormatMessage(errorNo, errBuf, MAX_ERR_STRING_LEN));
        return false;
    }
    file << "filesize:" << fileSize << std::endl;
    if (!timeKey.empty()) {
        file << "starttime:" << startTime << std::endl;
        file << "endtime  :" << endTime << std::endl;
    }
    file.flush();
    file.close();
    uint64_t doneFileSize = static_cast<uint64_t>(analysis::dvvp::common::utils::Utils::GetFileSize(tempPath));
    fileAgeing_->AppendAgeingFile(absolutePath, tempPath, stoull(fileSize), doneFileSize);
    if (fileAgeing_->IsNeedAgeingFile()) {
        fileAgeing_->RemoveAgeingFile();
    }

    // when gen .done file, erase related key in chunkStartTime_ & chunkEndTime_ map
    auto iter = chunkStartTime_.find(timeKey);
    if (iter != chunkStartTime_.end()) {
        chunkStartTime_.erase(timeKey);
        MSPROF_LOGD("erase key (%s) in chunkStartTime map", Utils::BaseName(timeKey).c_str());
    }
    iter = chunkEndTime_.find(timeKey);
    if (iter != chunkEndTime_.end()) {
        chunkEndTime_.erase(timeKey);
        MSPROF_LOGD("erase key (%s) in chunkEndTime map", Utils::BaseName(timeKey).c_str());
    }
    return true;
}

bool FileSlice::FileSliceFlush()
{
    if (!needSlice_) {
        return true;
    }
    std::map<std::string, uint64_t>::iterator it;
    std::unique_lock<std::mutex> lk(sliceFileMtx_);
    for (it = sliceNum_.begin(); it != sliceNum_.end(); ++it) {
        std::string absolutePath = it->first + std::to_string(it->second);
        if (analysis::dvvp::common::utils::Utils::IsFileExist(absolutePath)) {
            MSPROF_LOGI("[FileSliceFlush]file:%s, total_size_file:%llu",
                        Utils::BaseName(it->first).c_str(), totalSize_[it->first]);
            long long fileSize = analysis::dvvp::common::utils::Utils::GetFileSize(absolutePath);
            if (fileSize < 0) {
                MSPROF_LOGE("[fileSize:%d error", fileSize);
            }
            if (!(CreateDoneFile(absolutePath, std::to_string(fileSize), std::to_string(chunkStartTime_[absolutePath]),
                std::to_string(chunkEndTime_[absolutePath]), absolutePath))) {
                MSPROF_LOGE("[FileSliceFlush]Failed to create file:%s", Utils::BaseName(absolutePath).c_str());
                return false;
            }
            MSPROF_LOGI("[FileSliceFlush]create done file:%s.done", Utils::BaseName(absolutePath).c_str());
            sliceNum_[it->first]++;
        }
    }
    writeFilePerfCount_->OutPerfInfo("FileSliceFlush");

    return true;
}

int FileSlice::FileSliceFlushByJobID(const std::string &jobIDRelative, const std::string &devID)
{
    if (!needSlice_) {
        return PROFILING_SUCCESS;
    }
    std::map<std::string, uint64_t>::iterator it;
    std::string fileSliceName = "";

    MSPROF_LOGI("[FileSliceFlushByJobID]jobIDRelative:%s, devID:%s",
                Utils::BaseName(jobIDRelative).c_str(), devID.c_str());
    fileSliceName.append(".").append(devID).append(".slice_");
    std::unique_lock<std::mutex> lk(sliceFileMtx_);
    for (it = sliceNum_.begin(); it != sliceNum_.end(); ++it) {
        if (it->first.find(fileSliceName) == std::string::npos ||
            it->first.find(jobIDRelative) == std::string::npos) {
            continue;
        }
        std::string absolutePath = it->first + std::to_string(it->second);
        MSPROF_LOGI("[FileSliceFlushByJobID]file:%s, total_size_file:%llu",
                    Utils::BaseName(it->first).c_str(), totalSize_[it->first]);
        if (analysis::dvvp::common::utils::Utils::IsFileExist(absolutePath)) {
            MSPROF_LOGI("[FileSliceFlushByJobID]create done file for:%s", Utils::BaseName(absolutePath).c_str());
            long long fileSize = analysis::dvvp::common::utils::Utils::GetFileSize(absolutePath);
            if (fileSize < 0) {
                MSPROF_LOGE("[fileSize:%d error", fileSize);
            }
            if (!(CreateDoneFile(absolutePath, std::to_string(fileSize), std::to_string(chunkStartTime_[absolutePath]),
                std::to_string(chunkEndTime_[absolutePath]), absolutePath))) {
                MSPROF_LOGE("[FileSliceFlushByJobID]Failed to create file:%s", Utils::BaseName(absolutePath).c_str());
                return PROFILING_FAILED;
            }
            MSPROF_LOGI("[FileSliceFlushByJobID]create done file:%s.done", Utils::BaseName(absolutePath).c_str());
            sliceNum_[it->first]++;
        }
    }
    writeFilePerfCount_->OutPerfInfo("FileSliceFlushByJobID");

    return PROFILING_SUCCESS;
}
}
}
}
