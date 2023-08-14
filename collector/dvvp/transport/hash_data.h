/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: hash data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2021-11-30
 */
#ifndef ANALYSIS_DVVP_TRANSPORT_HASH_DATA_H
#define ANALYSIS_DVVP_TRANSPORT_HASH_DATA_H
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include "singleton/singleton.h"
#include "utils/utils.h"
#include "proto/msprofiler.pb.h"
#include "prof_callback.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::proto;

class HashData : public analysis::dvvp::common::singleton::Singleton<HashData> {
public:
    HashData();
    ~HashData();

public:
    int32_t Init();
    int32_t Uninit();
    bool IsInit() const;
    void SaveHashData(int32_t devId);
    void SaveNewHashData();
    std::string &GetHashData(const std::string &module, uint64_t hashId);
    uint64_t GenHashId(const std::string &module, CONST_CHAR_PTR data, uint32_t dataLen);

    // for new struct data
    uint64_t GenHashId(const std::string &hashInfo);
    std::string &GetHashInfo(uint64_t hashId);
    void SaveHashDataRealtime(const uint64_t hashId, const std::string &hashData);

private:
    uint64_t DoubleHash(const std::string &data) const;
    void FillPbData(const std::string &module, int32_t upDevId, const std::string &saveHashData,
                    SHARED_PTR_ALIA<FileChunkReq> fileChunk, bool isLastChunk);
private:
    bool inited_;
    std::mutex initMutex_;
    std::mutex hashMutex_;
    std::unordered_map<std::string, uint64_t> hashInfoMap_;   // <hashInfo, hashId>
    std::unordered_map<uint64_t, std::string> hashIdMap_;     // <hashId, hashInfo>
    std::set<uint64_t> savedHashId_;
    std::map<std::string, std::shared_ptr<std::mutex>> hashMapMutex_;       // <module, hashMutex>
    std::map<std::string, std::map<std::string, uint64_t>> hashDataKeyMap_; // <module, <data, hashId>>
    std::map<std::string, std::map<uint64_t, std::string>> hashIdKeyMap_;   // <module, <hashId, data>>
};

}
}
}
#endif
