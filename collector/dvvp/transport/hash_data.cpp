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
    Start();
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
    Stop();
    inited_ = false;
    MSPROF_LOGI("HashData uninitialize success");
    return PROFILING_SUCCESS;
}

bool HashData::IsInit() const
{
    return inited_;
}

int HashData::Start()
{
    if (isStarted_) {
        MSPROF_LOGW("hash upload has been started!");
        return PROFILING_SUCCESS;
    }

    Thread::SetThreadName(analysis::dvvp::common::config::MSVP_HASH_DATA_UPLOAD_THREAD_NAME);
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to start the upload in HashData::Start().");
        return PROFILING_FAILED;
    } else {
        MSPROF_LOGI("Succeeded in starting the upload in HashData::Start().");
    }
    isStarted_ = true;
    return PROFILING_SUCCESS;
}

void HashData::Run(const struct error_message::Context &errorContext)
{
    while (!IsQuit()) {
        Collector::Dvvp::Mmpa::MmSleep(1000);    // 1000 表示 1s，等待1s再重新检测落盘.防止少量数据频繁落盘
        SaveNewHashData(false);
    }
}

int HashData::Stop()
{
    if (!inited_) {
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("Stop hash read thread begin");
    if (!isStarted_) {
        MSPROF_LOGW("Hash read thread not started");
        return PROFILING_FAILED;
    }
    isStarted_ = false;

    int ret = analysis::dvvp::common::thread::Thread::Stop();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Hash read Thread stop failed");
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Stop hash read Thread success");
    return PROFILING_SUCCESS;
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
        std::pair<uint64_t, std::string> tmpHashInfo(hashId, hashInfo);
        hashVector_.emplace_back(tmpHashInfo);
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
    const std::string &saveHashData, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk, bool isLastChunk)
{
    fileChunk->fileName = Utils::PackDotInfo(module, HASH_TAG);
    fileChunk->offset = -1;
    fileChunk->isLastChunk = isLastChunk;
    fileChunk->chunk = saveHashData;
    fileChunk->chunkSize = saveHashData.size();
    fileChunk->chunkModule = FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST;
    fileChunk->extraInfo = Utils::PackDotInfo(std::to_string(upDevId), std::to_string(upDevId));
    uint64_t reportTime = Utils::GetClockMonotonicRaw();
    fileChunk->chunkStartTime = reportTime;
    fileChunk->chunkEndTime = reportTime;
}

void HashData::SaveHashData()
{
    if (!inited_) {
        MSPROF_LOGW("HashData not inited");
        return;
    }
    Stop();
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
        // construct ProfileFileChunk data
        SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk = nullptr;
        MSVP_MAKE_SHARED0_BREAK(fileChunk, analysis::dvvp::ProfileFileChunk);
        FillPbData(module.name, DEFAULT_HOST_ID, saveHashData, fileChunk, true);
        // upload ProfileFileChunk data
        int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
            std::to_string(DEFAULT_HOST_ID), fileChunk);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("HashData upload data failed, module:%s, dataLen:%u",
                        module.name.c_str(), saveHashData.size());
            continue;
        }
        MSPROF_EVENT("total_size_HashData, module:%s, saveLen:%llu, dataKeyMapSize:%llu, idKeyMapSize:%llu",
            module.name.c_str(), saveHashData.size(),
            hashDataKeyMap_[module.name].size(), hashIdKeyMap_[module.name].size());
    }
    SaveNewHashData(true);
    readIndex_ = 0;
}

void HashData::SaveNewHashData(bool isLastChunk)
{
    if (!inited_) {
        MSPROF_LOGW("HashData not inited");
        return;
    }
    std::unique_lock<std::mutex> lock(hashMutex_);
    SHARED_PTR_ALIA<Uploader> uploader = nullptr;
    UploaderMgr::instance()->GetUploader(std::to_string(DEFAULT_HOST_ID), uploader);
    if (uploader == nullptr || readIndex_ == hashVector_.size()) {
        return;
    }
    // combined hash map data
    std::string saveHashData;
    size_t currentHashSize = hashVector_.size();
    for (size_t i = readIndex_; i < currentHashSize; i++) {
        saveHashData.append(std::to_string(hashVector_[i].first) + HASH_DIC_DELIMITER + hashVector_[i].second + "\n");
    }
    readIndex_ = currentHashSize;
    lock.unlock();
    // construct ProfileFileChunk data
    SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk = nullptr;
    MSVP_MAKE_SHARED0_VOID(fileChunk, analysis::dvvp::ProfileFileChunk);
    FillPbData("unaging.additional", DEFAULT_HOST_ID, saveHashData, fileChunk, isLastChunk);
    // upload ProfileFileChunk data
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->UploadData(
        std::to_string(DEFAULT_HOST_ID), fileChunk);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("HashData upload data failed, dataLen:%u", saveHashData.size());
    }
    MSPROF_EVENT("total_size_HashData, saveLen:%llu, hashIdMap size:%llu, hashInfoMap size:%llu",
        saveHashData.size(), hashIdMap_.size(), hashInfoMap_.size());
}
}
}
}
