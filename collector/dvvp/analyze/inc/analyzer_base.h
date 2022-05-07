/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2019. All rights reserved.
 * Description: buffer data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_BASE_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_BASE_H

#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace proto {
class FileChunkReq;
}}}

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

protected:
    CONST_CHAR_PTR dataPtr_;
    uint32_t dataLen_;

    std::string buffer_;

    uint64_t analyzedBytes_;
    uint64_t totalBytes_;

    double frequency_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
