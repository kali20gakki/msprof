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
    void FftsParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);

private:
    void PrintStats() const;

    void ParseOptimizeFftsData(CONST_CHAR_PTR data, uint32_t len);
    void ParseOptimizeAcsqTaskData(const FftsLogHead *data, uint32_t logType);
    void ParseOptimizeSubTaskThreadData(const FftsLogHead *data, uint32_t logType);

    void StarsRollBackStreamTaskId(uint16_t *streamId, uint16_t *taskId) const;
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
