/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2023. All rights reserved.
 * Description: buffer data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2019-10-24
 */
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_BASE_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_BASE_H

#include "utils/utils.h"
#include "proto/profiler.pb.h"
#include "data_struct.h"
#include "op_desc_parser.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::utils;
class AnalyzerBase {
public:
    AnalyzerBase(): dataPtr_(nullptr), dataLen_(0), analyzedBytes_(0), totalBytes_(0), frequency_(1.0) {}
    ~AnalyzerBase() {}

protected:
    /**
     * @brief append new data to buffer (if buffer not empty)
     * @param data ptr to data
     * @param len length of data
     */
    void AppendToBufferedData(CONST_CHAR_PTR data, uint32_t len);

    /**
     * @brief copy remaining data to buffer, or clear buffer if offset > buffer.size
     * @param offset offset of remaining data ptr
     */
    void BufferRemainingData(uint32_t offset);

    /**
     * @brief initialize frequency, op's time = (syscnt / frequency)
     */
    int32_t InitFrequency();

    void EraseRtMapByTaskId(uint32_t taskId, uint16_t streamId, std::map<std::string, RtOpInfo> &rtOpInfo) const;
    void EraseRtMapByStreamId(uint16_t streamId, std::map<std::string, RtOpInfo> &rtOpInfo) const;
    void HandleDeviceData(const std::string &key, RtOpInfo &devData, uint32_t &time);
    void ConstructAndUploadOptimizeData(GeOpFlagInfo &opFlagData, RtOpInfo &rtTsOpdata);
    uint32_t GetGraphModelId(uint32_t modelId) const;
    void SetGraphModelId(uint32_t modelId, uint32_t graphId) const;
    void ClearAnalyzerData();

protected:
    CONST_CHAR_PTR dataPtr_;
    uint32_t dataLen_;

    std::string buffer_;

    uint64_t analyzedBytes_;
    uint64_t totalBytes_;

    double frequency_;
    static bool isFftsPlus_;
    static std::map<std::string, RtOpInfo> rtOpInfo_;
    static std::map<std::string, RtOpInfo> tsOpInfo_;
    static std::multimap<uint32_t, GeOpFlagInfo> geNodeInfo_;
    static std::multimap<uint32_t, GeOpFlagInfo> geApiInfo_;
    static std::multimap<uint32_t, GeOpFlagInfo> geModelInfo_;
    static std::multimap<uint32_t, GeOpFlagInfo> geContextInfo_;
    static std::multimap<uint32_t, GeOpFlagInfo> geOpInfo_;
    static std::map<uint32_t, uint32_t> graphIdMap_;    // <modeId, graphId>
    static std::vector<ProfOpDesc> opDescInfos_;
    static std::mutex opDescInfoMtx_;
    static std::mutex graphIdMtx_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
