/**
 * @file analyzer_rt.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_RT_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_RT_H
 
#include <map>
#include <unordered_map>
#include "analyzer_base.h"
#include "utils/utils.h"
#include "data_struct.h"
 
namespace Analysis {
namespace Dvvp {
namespace Analyze {
class AnalyzerRt : public AnalyzerBase {
 
public:
    AnalyzerRt() : totalRtTimes_(0), totalRtMerges_(0) {}
    ~AnalyzerRt() {}
 
public:
    bool IsRtCompactData(const std::string &tag) const;
    void PrintStats() const;
    void RtCompactParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
 
private:
    void ParseRuntimeTrackData(CONST_CHAR_PTR data, uint32_t len, bool ageFlag);
    void HandleRuntimeTrackData(CONST_CHAR_PTR data, bool ageFlag);
 
private:
    uint32_t totalRtTimes_;
    uint32_t totalRtMerges_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
 
#endif