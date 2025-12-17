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
    void RtCompactParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);
 
private:
    void ParseRuntimeTrackData(CONST_CHAR_PTR data, uint32_t len, bool ageFlag);
    void HandleRuntimeTrackData(CONST_CHAR_PTR data, bool ageFlag);
    void MatchDeviceOpInfo(std::map<std::string, RtOpInfo> &rtOpInfo,
        std::multimap<std::string, RtOpInfo> &tsTmpOpInfo,
        std::multimap<uint32_t, GeOpFlagInfo> &geOpInfo);
 
private:
    uint32_t totalRtTimes_;
    uint32_t totalRtMerges_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
 
#endif