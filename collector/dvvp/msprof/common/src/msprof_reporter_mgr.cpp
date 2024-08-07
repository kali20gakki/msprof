/**
* @file msprof_reporter_mgr.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "msprof_reporter_mgr.h"
#include "errno/error_code.h"
#include "hash_data.h"
#include "uploader_mgr.h"
 
namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::transport;
MsprofReporterMgr::MsprofReporterMgr() : isStarted_(false), isUploadStarted_(false), isSyncReporter_(false)
{
    indexMap_ = {
        {MSPROF_REPORT_ACL_LEVEL, 0},
        {MSPROF_REPORT_MODEL_LEVEL, 0},
        {MSPROF_REPORT_NODE_LEVEL, 0},
        {MSPROF_REPORT_HCCL_NODE_LEVEL, 0},
        {MSPROF_REPORT_RUNTIME_LEVEL, 0}
    };
    reportTypeInfoMapVec_ = {
        {MSPROF_REPORT_ACL_LEVEL, std::vector<std::pair<uint32_t, std::string>>()},
        {MSPROF_REPORT_MODEL_LEVEL, std::vector<std::pair<uint32_t, std::string>>()},
        {MSPROF_REPORT_NODE_LEVEL, std::vector<std::pair<uint32_t, std::string>>()},
        {MSPROF_REPORT_HCCL_NODE_LEVEL, std::vector<std::pair<uint32_t, std::string>>()},
        {MSPROF_REPORT_RUNTIME_LEVEL, std::vector<std::pair<uint32_t, std::string>>()}
    };
}
MsprofReporterMgr::~MsprofReporterMgr()
{
    StopReporters();
    reportTypeInfoMap_.clear();
    reportTypeInfoMapVec_.clear();
}

int MsprofReporterMgr::Start()
{
    if (isUploadStarted_) {
        MSPROF_LOGW("type info upload has been started!");
        return PROFILING_SUCCESS;
    }

    Thread::SetThreadName(analysis::dvvp::common::config::MSVP_TYPE_INFO_UPLOAD_THREAD_NAME);
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start the upload in MsprofReporterMgr::Start().");
        return PROFILING_FAILED;
    } else {
        MSPROF_LOGI("Succeeded in starting the upload in MsprofReporterMgr::Start().");
    }
    isUploadStarted_ = true;
    return PROFILING_SUCCESS;
}

void MsprofReporterMgr::Run(const struct error_message::Context &errorContext)
{
    while (!IsQuit()) {
        std::unique_lock<std::mutex> lk(notifyMtx_);
        cv_.wait_for(lk, std::chrono::seconds(1), [this] { return this->IsQuit(); });
        SaveTypeInfo(false);
        HashData::instance()->SaveNewHashData(false);
    }
}

int MsprofReporterMgr::Stop()
{
    MSPROF_LOGI("Stop type info upload thread begin");
    if (!isUploadStarted_) {
        MSPROF_LOGE("type info upload thread not started");
        return PROFILING_FAILED;
    }
    isUploadStarted_ = false;

    int ret = analysis::dvvp::common::thread::Thread::Stop();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("type info upload Thread stop failed");
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Stop type info upload Thread success");
    return PROFILING_SUCCESS;
}

void MsprofReporterMgr::SetSyncReporter()
{
    // set Sync Reporter. This will not start thread.
    isSyncReporter_ = true;
}

int32_t MsprofReporterMgr::StartReporters()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (isStarted_) {
        MSPROF_LOGI("The reporter have been started, don't need to repeat.");
        return PROFILING_SUCCESS;
    }
    
    if (reporters_.empty()) {
        for (auto &module : MSPROF_MODULE_REPORT_TABLE) {
            reporters_.emplace_back(MsprofCallbackHandler(module.name));
        }
    }
    
    MSPROF_LOGI("ProfReporterMgr start reporters");
    for (auto &repoter : reporters_) {
        if (repoter.StartReporter() != PROFILING_SUCCESS) {
            MSPROF_LOGE("ProfReporterMgr start reporters failed.");
            return PROFILING_FAILED;
        }
    }
    for (auto &level : DEFAULT_TYPE_INFO) {
        for (auto &type : level.second) {
            RegReportTypeInfo(level.first, type.first, type.second);
        }
    }
    if (!isSyncReporter_) {
        Start();
    }
    isStarted_ = true;
    return PROFILING_SUCCESS;
}

void MsprofReporterMgr::FlushAllReporter(const std::string &devId)
{
    for (auto &report : reporters_) {
        report.ForceFlush(devId);
    }
}
 
int32_t MsprofReporterMgr::ReportData(uint32_t agingFlag, const MsprofApi &data)
{
    if (!isStarted_) {
        MSPROF_LOGW("The reporter has not been started.");
        return PROFILING_NOTSUPPORT;
    }
    return reporters_[agingFlag ? AGING_API : UNAGING_API].ReportData(data);
}

int32_t MsprofReporterMgr::ReportData(uint32_t agingFlag, const MsprofCompactInfo &data)
{
    if (!isStarted_) {
        MSPROF_LOGW("The reporter has not been started.");
        return PROFILING_NOTSUPPORT;
    }
    return reporters_[agingFlag ?  AGING_COMPACT_INFO : UNAGING_COMPACT_INFO].ReportData(data);
}

int32_t MsprofReporterMgr::ReportData(uint32_t agingFlag, const MsprofAdditionalInfo &data)
{
    if (!isStarted_) {
        MSPROF_LOGW("The reporter has not been started.");
        return PROFILING_NOTSUPPORT;
    }
    return reporters_[agingFlag ?  AGING_ADDITIONAL_INFO : UNAGING_ADDITIONAL_INFO].ReportData(data);
}

int32_t MsprofReporterMgr::SendAdditionalInfo(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk)
{
    if (!isStarted_) {
        MSPROF_LOGE("The reporter has not been started.");
        return PROFILING_FAILED;
    }
    return reporters_[AGING_ADDITIONAL_INFO].SendData(fileChunk);
}

uint64_t MsprofReporterMgr::GetHashId(const std::string &info)
{
    return HashData::instance()->GenHashId(info);
}

int32_t MsprofReporterMgr::RegReportTypeInfo(uint16_t level, uint32_t typeId, const std::string &typeName)
{
    MSPROF_LOGD("Register type info of reporter level[%u], type id %u, type name %s", level, typeId, typeName.c_str());
    std::lock_guard<std::mutex> lk(regTypeInfoMtx_);
    reportTypeInfoMap_[level][typeId] = typeName;
    auto itr = std::find_if(reportTypeInfoMapVec_[level].begin(), reportTypeInfoMapVec_[level].end(),
        [typeId](const std::pair<uint32_t, std::string>& pair) { return pair.first == typeId; });
    if (itr != std::end(reportTypeInfoMapVec_[level])) {
        itr->second = typeName;
    } else {
        reportTypeInfoMapVec_[level].emplace_back(std::pair<uint32_t, std::string>(typeId, typeName));
    }
    return PROFILING_SUCCESS;
}

std::string& MsprofReporterMgr::GetRegReportTypeInfo(uint16_t level, uint32_t typeId)
{
    std::lock_guard<std::mutex> lk(regTypeInfoMtx_);
    return reportTypeInfoMap_[level][typeId];
}

void MsprofReporterMgr::FillFileChunkData(const std::string &saveHashData,
    SHARED_PTR_ALIA<ProfileFileChunk> fileChunk, bool isLastChunk) const
{
    fileChunk->fileName = Utils::PackDotInfo("unaging.additional", "type_info_dic");
    fileChunk->offset = -1;
    fileChunk->isLastChunk = isLastChunk;
    fileChunk->chunk = saveHashData;
    fileChunk->chunkSize = saveHashData.size();
    fileChunk->chunkModule = FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST;
    fileChunk->extraInfo = Utils::PackDotInfo(std::to_string(DEFAULT_HOST_ID), std::to_string(DEFAULT_HOST_ID));
    uint64_t reportTime = Utils::GetClockMonotonicRaw();
    fileChunk->chunkStartTime = reportTime;
    fileChunk->chunkEndTime = reportTime;
}

void MsprofReporterMgr::SaveTypeInfo(bool isLastChunk)
{
    std::lock_guard<std::mutex> lk(regTypeInfoMtx_);

    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    UploaderMgr::instance()->GetUploader(std::to_string(DEFAULT_HOST_ID), uploader);
    if (uploader == nullptr) {
        return;
    }
    for (auto &typeInfo  : reportTypeInfoMapVec_) {
        // combined hash map data
        std::string saveHashData;
        size_t currentHashSize = typeInfo.second.size();
        for (size_t i = indexMap_[typeInfo.first]; i < currentHashSize; i++) {
            saveHashData.append(std::to_string(typeInfo.first) + "_" + std::to_string(typeInfo.second[i].first) +
                                HASH_DIC_DELIMITER + typeInfo.second[i].second + "\n");
        }
        if (saveHashData.empty()) {
            continue;
        }
        indexMap_[typeInfo.first] = currentHashSize;
        // construct ProfileFileChunk data
        SHARED_PTR_ALIA<ProfileFileChunk> fileChunk = nullptr;
        MSVP_MAKE_SHARED0_BREAK(fileChunk, ProfileFileChunk);
        FillFileChunkData(saveHashData, fileChunk, isLastChunk);
        // upload ProfileFileChunk data
        int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
            std::to_string(DEFAULT_HOST_ID), fileChunk);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Type info upload data failed, level: %u, dataLen:%u", typeInfo.first, saveHashData.size());
            continue;
        }
        MSPROF_EVENT("total_size_type_info[%u], save type info length: %llu, type info size: %llu",
            typeInfo.first, saveHashData.size(), typeInfo.second.size());
    }
}

void MsprofReporterMgr::NotifyQuit()
{
    std::lock_guard<std::mutex> lk(notifyMtx_);
    StopNoWait();
    cv_.notify_one();
}

int32_t MsprofReporterMgr::StopReporters()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (!isStarted_) {
        MSPROF_LOGI("The reporter isn't started, don't need to be stopped.");
        return PROFILING_SUCCESS;
    }
    if (!isSyncReporter_) {
        NotifyQuit();
        Stop();
    }
    MSPROF_LOGI("ProfReporterMgr stop reporters");
    isStarted_ = false;
    SaveTypeInfo(true);
    try {
        for (auto &report : reporters_) {
            if (report.StopReporter() != PROFILING_SUCCESS) {
                return PROFILING_FAILED;
            }
        }
    } catch (const std::exception& e) {
        MSPROF_LOGE("Failed to ~StopReporters.");
        return PROFILING_FAILED;
    }
    isSyncReporter_ = false;
    for (auto &index : indexMap_) { // 为了同一进程里多次采集，每次读地址要从0开始
        index.second = 0;
    }
    reporters_.clear();
    return PROFILING_SUCCESS;
}
}
}