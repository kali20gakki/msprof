/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: parse ts data
 * Author: zcj
 * Create: 2020-10-24
 */
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_TS_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_TS_H

#include <map>
#include "analyzer_base.h"
#include "utils/utils.h"
#include "data_struct.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
class Analyzer;
class AnalyzerTs : public AnalyzerBase {
friend class Analyzer;
public:
    AnalyzerTs() : opTimeCount_(0), totalTsMerges_(0) {}
    ~AnalyzerTs() {}

public:
    bool IsTsData(const std::string &fileName);
    void Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);

private:
    void ParseTsTrackData(CONST_CHAR_PTR data, uint32_t len);
    void ParseTsTimelineData(CONST_CHAR_PTR data, uint32_t len);
    void ParseTsKeypointData(CONST_CHAR_PTR data, uint32_t len);
    uint8_t GetRptType(CONST_CHAR_PTR data, uint32_t len);
    void PrintStats();

private:
    uint64_t opTimeCount_;
    int32_t totalTsMerges_;
    std::map<std::string, OpTime> opTimeDrafts_;      // stores incomplete data
    std::multimap<std::string, OpTime> opTimes_;      // key is taskId-streamId-contextId
    std::vector<KeypointOp> keypointOpInfo_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
