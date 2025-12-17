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
#ifndef ANALYSIS_DVVP_TRANSPORT_HASH_DATA_H
#define ANALYSIS_DVVP_TRANSPORT_HASH_DATA_H
#include <map>
#include <unordered_map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "singleton/singleton.h"
#include "utils/utils.h"
#include "thread/thread.h"
#include "prof_callback.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace analysis::dvvp::common::utils;

class HashData : public analysis::dvvp::common::singleton::Singleton<HashData> {
public:
    HashData();
    ~HashData();

public:
    int32_t Init();
    int32_t Uninit();
    bool IsInit() const;
    void SaveHashData();
    void SaveNewHashData(bool isLastChunk);
    std::string &GetHashData(const std::string &module, uint64_t hashId);
    uint64_t GenHashId(const std::string &module, CONST_CHAR_PTR data, uint32_t dataLen);

    // for new struct data
    uint64_t GenHashId(const std::string &hashInfo);
    std::string &GetHashInfo(uint64_t hashId);

private:
    uint64_t DoubleHash(const std::string &data) const;
    void FillPbData(const std::string &module, int32_t upDevId, const std::string &saveHashData,
                    SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk, bool isLastChunk);
private:
    bool inited_;
    std::mutex initMutex_;
    std::mutex hashMutex_;
    bool isStarted_ = false;
    size_t readIndex_ = 0;
    std::vector<std::pair<uint64_t, std::string>> hashVector_;
    std::unordered_map<std::string, uint64_t> hashInfoMap_;   // <hashInfo, hashId>
    std::unordered_map<uint64_t, std::string> hashIdMap_;     // <hashId, hashInfo>
    std::map<std::string, std::shared_ptr<std::mutex>> hashMapMutex_;       // <module, hashMutex>
    std::map<std::string, std::map<std::string, uint64_t>> hashDataKeyMap_; // <module, <data, hashId>>
    std::map<std::string, std::map<uint64_t, std::string>> hashIdKeyMap_;   // <module, <hashId, data>>
};

}
}
}
#endif
