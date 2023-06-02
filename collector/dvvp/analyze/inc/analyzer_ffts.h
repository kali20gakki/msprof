/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: parse ffts data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-02-10
 */
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_FFTS_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_FFTS_H

#include <map>

#include "analyzer_base.h"
#include "data_struct.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
class Analyzer;
class AnalyzerFfts : public AnalyzerBase {
friend class Analyzer;
public:
    AnalyzerFfts() : opTimeCount_(0), opRepeatCount_(0), totalFftsTimes_(0), totalFftsMerges_(0) {};
    ~AnalyzerFfts() {};

public:
    bool IsFftsData(const std::string &fileName) const;
    void Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void FftsParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);

private:
    void ParseData(CONST_CHAR_PTR data, uint32_t len);
    void ParseAcsqTaskData(const FftsLogHead *data, uint32_t logType);
    void ParseSubTaskThreadData(const FftsLogHead *data, uint32_t logType);
    void PrintStats() const;

    void ParseOptimizeFftsData(CONST_CHAR_PTR data, uint32_t len);
    void ParseOptimizeAcsqTaskData(const FftsLogHead *data, uint32_t logType);
    void ParseOptimizeSubTaskThreadData(const FftsLogHead *data, uint32_t logType);

private:
    uint64_t opTimeCount_;
    uint64_t opRepeatCount_;
    uint32_t totalFftsTimes_;
    uint32_t totalFftsMerges_;
    std::map<std::string, OpTime> opDrafts_;     // stores incomplete data
    std::multimap<std::string, OpTime> opTimes_;     // key is taskId-streamId-contextId
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
