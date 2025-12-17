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
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_HWTS_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_HWTS_H

#include <map>

#include "analyzer_base.h"
#include "data_struct.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
class Analyzer;
class AnalyzerHwts : public AnalyzerBase {
friend class Analyzer;
public:
    AnalyzerHwts() : opTimeCount_(0), opRepeatCount_(0), totalHwtsTimes_(0), totalHwtsMerges_(0) {};
    ~AnalyzerHwts() {};

public:
    bool IsHwtsData(const std::string &fileName);

    void HwtsParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq);

private:
    void ParseOptimizeHwtsData(CONST_CHAR_PTR data, uint32_t len);
    uint8_t GetRptType(CONST_CHAR_PTR data, uint32_t len);
    void ParseTaskStartEndData(CONST_CHAR_PTR data, uint32_t len, uint8_t rptType);
    void CheckData(const struct OpTime &draftsOp, std::string key, uint8_t rptType, uint64_t sysTime);
    void PrintStats() const;

    void ParseOptimizeStartEndData(CONST_CHAR_PTR data, uint8_t rptType);

private:
    uint64_t opTimeCount_;
    uint64_t opRepeatCount_;
    uint32_t totalHwtsTimes_;
    uint32_t totalHwtsMerges_;
    std::map<std::string, OpTime> opTimeDrafts_;     // stores incomplete data
    std::multimap<std::string, OpTime> opTimes_;     // key is taskId-streamId-contextId
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
