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
using namespace analysis::dvvp::proto;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::transport;
MsprofReporterMgr::MsprofReporterMgr() : isStarted_(false)
{
}
MsprofReporterMgr::~MsprofReporterMgr()
{
    reportTypeInfoMap_.clear();
    reporters_.clear();
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

int32_t MsprofReporterMgr::SendAdditionalInfo(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk)
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
    if (reportTypeInfoMap_.find(level) != reportTypeInfoMap_.end() &&
        reportTypeInfoMap_[level].find(typeId) != reportTypeInfoMap_[level].end()) {
            MSPROF_LOGW("Type info conflict, old info:%s new info:%s",
                        reportTypeInfoMap_[level][typeId].c_str(), typeName.c_str());
        }
    reportTypeInfoMap_[level][typeId] = typeName;
    return PROFILING_SUCCESS;
}

std::string& MsprofReporterMgr::GetRegReportTypeInfo(uint16_t level, uint32_t typeId)
{
    static std::string nullInfo;
    std::lock_guard<std::mutex> lk(regTypeInfoMtx_);
    if (reportTypeInfoMap_.find(level) != reportTypeInfoMap_.end() &&
        reportTypeInfoMap_[level].find(typeId) != reportTypeInfoMap_[level].end()) {
        return reportTypeInfoMap_[level][typeId];
    }
    MSPROF_LOGW("Get type info failed, level %u, type id %u", level, typeId);
    return nullInfo;
}

void MsprofReporterMgr::FillFileChunkData(const std::string &saveHashData,
    SHARED_PTR_ALIA<FileChunkReq> fileChunk) const
{
    fileChunk->set_filename("unaging.additional");
    fileChunk->set_offset(-1);
    fileChunk->set_islastchunk(true);
    fileChunk->set_needack(false);
    fileChunk->set_tag("type_info_dic");
    fileChunk->set_tagsuffix(std::to_string(DEFAULT_HOST_ID));
    fileChunk->set_chunk(saveHashData);
    fileChunk->set_chunksizeinbytes(saveHashData.size());
    fileChunk->set_datamodule(FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST);
    analysis::dvvp::message::JobContext jobCtx;
    jobCtx.dev_id = std::to_string(DEFAULT_HOST_ID);
    jobCtx.job_id = std::to_string(DEFAULT_HOST_ID);
    fileChunk->mutable_hdr()->set_job_ctx(jobCtx.ToString());
    uint64_t reportTime = Utils::GetClockMonotonicRaw();
    fileChunk->set_chunkstarttime(reportTime);
    fileChunk->set_chunkendtime(reportTime);
}

void MsprofReporterMgr::SaveTypeInfo()
{
    std::lock_guard<std::mutex> lk(regTypeInfoMtx_);
    for (auto &level : reportTypeInfoMap_) {
        if (level.second.empty()) {
            MSPROF_LOGI("Type info is empty, level %u", level.first);
            continue;
        }
        // combined hash map data
        std::string saveHashData;
        for (auto &data : level.second) {
            saveHashData.append(std::to_string(level.first) + "_" + std::to_string(data.first) +
                HASH_DIC_DELIMITER + data.second + "\n");
        }
        // construct FileChunkReq data
        SHARED_PTR_ALIA<FileChunkReq> fileChunk = nullptr;
        MSVP_MAKE_SHARED0_BREAK(fileChunk, FileChunkReq);
        FillFileChunkData(saveHashData, fileChunk);
        // upload FileChunkReq data
        std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunk);
        int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
            std::to_string(DEFAULT_HOST_ID), encoded.c_str(), encoded.size());
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Type info upload data failed, level: %u, dataLen:%u", level.first, saveHashData.size());
            continue;
        }
        MSPROF_EVENT("total_size_type_info[%u], save type info length: %llu, type info size: %llu",
            level.first, saveHashData.size(), level.second.size());
    }
}

int32_t MsprofReporterMgr::StopReporters()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (!isStarted_) {
        MSPROF_LOGI("The reporter isn't started, don't need to be stopped.");
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("ProfReporterMgr stop reporters");
    isStarted_ = false;
    SaveTypeInfo();
    for (auto &report : reporters_) {
        if (report.StopReporter() != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    reportTypeInfoMap_.clear();
    reporters_.clear();
    return PROFILING_SUCCESS;
}
}
}