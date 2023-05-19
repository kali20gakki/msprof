/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2023. All rights reserved.
 * Description: parse hwts data
 * Author: zcj
 * Create: 2020-10-24
 */
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
    void Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);

private:
    void ParseHwtsData(CONST_CHAR_PTR data, uint32_t len);
    void ParseOptimizeHwtsData(CONST_CHAR_PTR data, uint32_t len);
    uint8_t GetRptType(CONST_CHAR_PTR data, uint32_t len);
    void ParseTaskStartEndData(CONST_CHAR_PTR data, uint32_t len, uint8_t rptType);
    void CheckData(const struct OpTime &draftsOp, std::string key, uint8_t rptType, uint64_t sysTime);
    void PrintStats();

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
