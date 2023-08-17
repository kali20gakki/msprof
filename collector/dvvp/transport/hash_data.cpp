/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: hash data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2021-11-30
 */
#include "hash_data.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "uploader_mgr.h"
#include "prof_api_common.h"
#include "receive_data.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace Msprof::Engine;

HashData::HashData() : inited_(false)
{
}

HashData::~HashData()
{
    (void)Uninit(); // clean sc warning
}

int32_t HashData::Init()
{
    std::lock_guard<std::mutex> lock(initMutex_);
    if (inited_) {
        MSPROF_LOGW("HashData repeated initialize");
        return PROFILING_SUCCESS;
    }
    for (auto &module : MSPROF_MODULE_ID_NAME_MAP) {
        // init hashMapMutex_
        SHARED_PTR_ALIA<std::mutex> mapMutex = nullptr;
        MSVP_MAKE_SHARED0_RET(mapMutex, std::mutex, PROFILING_FAILED);
        hashMapMutex_.insert(std::make_pair(module.name, mapMutex));
        // init hashDataKeyMap_
        std::map<std::string, uint64_t> hashDataMap;
        hashDataKeyMap_.insert(std::make_pair(module.name, hashDataMap));
        // init hashIdKeyMap_
        std::map<uint64_t, std::string> hashIdMap;
        hashIdKeyMap_.insert(std::make_pair(module.name, hashIdMap));
    }
    inited_ = true;
    MSPROF_LOGI("HashData initialize success");
    return PROFILING_SUCCESS;
}

int32_t HashData::Uninit()
{
    hashDataKeyMap_.clear();
    hashIdKeyMap_.clear();
    hashMapMutex_.clear();
    hashInfoMap_.clear();
    hashIdMap_.clear();
    inited_ = false;
    MSPROF_LOGI("HashData uninitialize success");
    return PROFILING_SUCCESS;
}

bool HashData::IsInit() const
{
    return inited_;
}

uint64_t HashData::DoubleHash(const std::string &data) const
{
    static const uint32_t UINT32_BITS = 32;  // the number of unsigned int bits
    uint32_t prime[2] = {29, 131};  // hash step size,
    uint32_t hash[2] = {0};

    for (char d : data) {
        hash[0] = hash[0] * prime[0] + static_cast<uint32_t>(d);
        hash[1] = hash[1] * prime[1] + static_cast<uint32_t>(d);
    }

    return (((static_cast<uint64_t>(hash[0])) << UINT32_BITS) | hash[1]);
}

uint64_t HashData::GenHashId(const std::string &module, CONST_CHAR_PTR data, uint32_t dataLen)
{
    if (hashMapMutex_.find(module) == hashMapMutex_.end()) {
        MSPROF_LOGE("HashData not support module:%s", module.c_str());
        return 0;
    }

    std::string strData(data, dataLen);
    std::lock_guard<std::mutex> lock(*hashMapMutex_[module]);
    auto &moduleHashData = hashDataKeyMap_[module];
    auto iter = moduleHashData.find(strData);
    if (iter != moduleHashData.end()) {
        return iter->second;
    } else {
        uint64_t hashId = DoubleHash(strData);
        moduleHashData[strData] = hashId;
        auto &moduleHashId = hashIdKeyMap_[module];
        if (moduleHashId.find(hashId) != moduleHashId.end()) {
            MSPROF_LOGW("HashData GenHashId conflict, hashId:%llu oldStr:%s newStr:%s",
                        hashId, moduleHashId[hashId].c_str(), strData.c_str());
        } else {
            moduleHashId[hashId] = strData;
        }
        MSPROF_LOGD("HashData GenHashId id:%llu data:%s", hashId, strData.c_str());
        return hashId;
    }
}

std::string &HashData::GetHashData(const std::string &module, uint64_t hashId)
{
    static std::string nullData;
    if (hashMapMutex_.find(module) == hashMapMutex_.end()) {
        MSPROF_LOGE("HashData not support module:%s", module.c_str());
        return nullData;
    }

    std::lock_guard<std::mutex> lock(*hashMapMutex_[module]);
    auto &moduleHashId = hashIdKeyMap_[module];
    auto iter = moduleHashId.find(hashId);
    if (iter != moduleHashId.end()) {
        return iter->second;
    } else {
        MSPROF_LOGW("HashData not find hashId:%llu, module:%s", hashId, module.c_str());
        return nullData;
    }
}

uint64_t HashData::GenHashId(const std::string &hashInfo)
{
    std::lock_guard<std::mutex> lock(hashMutex_);
    const auto iter = hashInfoMap_.find(hashInfo);
    if (iter != hashInfoMap_.end()) {
        return iter->second;
    } else {
        uint64_t hashId = DoubleHash(hashInfo);
        hashInfoMap_[hashInfo] = hashId;
        if (hashIdMap_.find(hashId) != hashIdMap_.end()) {
            MSPROF_LOGW("HashData GenHashId conflict, hashId:%llu oldStr:%s newStr:%s",
                        hashId, hashIdMap_[hashId].c_str(), hashInfo.c_str());
        } else {
            hashIdMap_[hashId] = hashInfo;
        }
        MSPROF_LOGD("HashData GenHashId id:%llu data:%s", hashId, hashInfo.c_str());
        return hashId;
    }
}

std::string &HashData::GetHashInfo(uint64_t hashId)
{
    static std::string nullInfo;
    std::lock_guard<std::mutex> lock(hashMutex_);
    const auto iter = hashIdMap_.find(hashId);
    if (iter != hashIdMap_.end()) {
        return iter->second;
    } else {
        MSPROF_LOGW("HashData not find hashId:%llu", hashId);
        return nullInfo;
    }
}

void HashData::FillPbData(const std::string &module, int32_t upDevId,
    const std::string &saveHashData, SHARED_PTR_ALIA<FileChunkReq> fileChunk)
{
    fileChunk->set_filename(module);
    fileChunk->set_offset(-1);
    fileChunk->set_islastchunk(true);
    fileChunk->set_needack(false);
    fileChunk->set_tag(HASH_TAG, strlen(HASH_TAG));
    fileChunk->set_tagsuffix(std::to_string(upDevId));
    fileChunk->set_chunk(saveHashData);
    fileChunk->set_chunksizeinbytes(saveHashData.size());
    fileChunk->set_datamodule(FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST);
    analysis::dvvp::message::JobContext jobCtx;
    jobCtx.dev_id = std::to_string(upDevId);
    jobCtx.job_id = std::to_string(upDevId);
    fileChunk->mutable_hdr()->set_job_ctx(jobCtx.ToString());
    uint64_t reportTime = Utils::GetClockMonotonicRaw();
    fileChunk->set_chunkstarttime(reportTime);
    fileChunk->set_chunkendtime(reportTime);
}

void HashData::SaveHashData(int32_t devId)
{
    if (devId < 0 || devId >= MSVP_MAX_DEV_NUM) {
        MSPROF_LOGI("HashData devId %d is invalid", devId);
        return;
    }

    if (!inited_) {
        MSPROF_LOGW("HashData not inited");
        return;
    }
    for (auto &module : MSPROF_MODULE_ID_NAME_MAP) {
        std::lock_guard<std::mutex> lock(*hashMapMutex_[module.name]);
        if (hashDataKeyMap_[module.name].empty()) {
            MSPROF_LOGI("HashData is null, module:%s", module.name.c_str());
            continue;
        }
        // combined hash map data
        std::string saveHashData;
        for (auto &data : hashDataKeyMap_[module.name]) {
            saveHashData.append(std::to_string(data.second) + HASH_DIC_DELIMITER + data.first + "\n");
        }
        // construct FileChunkReq data
        SHARED_PTR_ALIA<FileChunkReq> fileChunk = nullptr;
        MSVP_MAKE_SHARED0_BREAK(fileChunk, FileChunkReq);
        FillPbData(module.name, DEFAULT_HOST_ID, saveHashData, fileChunk);
        // upload FileChunkReq data
        std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunk);
        int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
            std::to_string(DEFAULT_HOST_ID), encoded.c_str(), encoded.size());
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("HashData upload data failed, module:%s, dataLen:%u",
                        module.name.c_str(), saveHashData.size());
            continue;
        }
        MSPROF_EVENT("total_size_HashData, module:%s, saveLen:%llu, dataKeyMapSize:%llu, idKeyMapSize:%llu",
            module.name.c_str(), saveHashData.size(),
            hashDataKeyMap_[module.name].size(), hashIdKeyMap_[module.name].size());
    }
    SaveNewHashData();
}

void HashData::SaveNewHashData()
{
    if (!inited_) {
        MSPROF_LOGW("HashData not inited");
        return;
    }
    std::unique_lock<std::mutex> lock(hashMutex_);
    // combined hash map data
    std::string saveHashData;
    for (auto &data : hashIdMap_) {
        saveHashData.append(std::to_string(data.first) + HASH_DIC_DELIMITER + data.second + "\n");
    }
    lock.unlock();
    // construct FileChunkReq data
    SHARED_PTR_ALIA<FileChunkReq> fileChunk = nullptr;
    MSVP_MAKE_SHARED0_VOID(fileChunk, FileChunkReq);
    FillPbData("unaging.additional", DEFAULT_HOST_ID, saveHashData, fileChunk);
    // upload FileChunkReq data
    std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunk);
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        std::to_string(DEFAULT_HOST_ID), encoded.c_str(), encoded.size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("HashData upload data failed, dataLen:%u", saveHashData.size());
    }
    MSPROF_EVENT("total_size_HashData, saveLen:%llu, hashIdMap size:%llu, hashInfoMap size:%llu",
        saveHashData.size(), hashIdMap_.size(), hashInfoMap_.size());
}
}
}
}
