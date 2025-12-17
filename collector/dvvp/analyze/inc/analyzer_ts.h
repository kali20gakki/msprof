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
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_TS_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_TS_H

#include <map>
#include <unordered_map>
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
    void Parse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);

private:
    void ParseTsTrackData(CONST_CHAR_PTR data, uint32_t len);
    void ParseTsTimelineData(CONST_CHAR_PTR data, uint32_t len);
    void ParseTsKeypointData(CONST_CHAR_PTR data, uint32_t len);
    uint8_t GetRptType(CONST_CHAR_PTR data, uint32_t len);
    void PrintStats() const;
    void CreateTsKeyPointStartData(const TsProfileKeypoint *tsData, std::string opInfoKey);

private:
    uint64_t opTimeCount_;
    uint32_t  totalTsMerges_;
    std::map<std::string, OpTime> opTimeDrafts_;      // stores incomplete data
    std::multimap<std::string, OpTime> opTimes_;      // key is taskId-streamId-contextId
    std::unordered_map<std::string, KeypointOp> keypointOpInfo_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
