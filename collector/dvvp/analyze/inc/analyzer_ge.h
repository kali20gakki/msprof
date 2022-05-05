/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: parse ge data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#ifndef ANALYSIS_DVVP_ANALYZE_ANALYZER_GE_H
#define ANALYSIS_DVVP_ANALYZE_ANALYZER_GE_H

#include <map>

#include "analyzer_base.h"
#include "utils/utils.h"
#include "prof_common.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
class Analyzer;
class AnalyzerGe : public AnalyzerBase {
friend class Analyzer;
public:
    AnalyzerGe() {}
    ~AnalyzerGe() {}

public:
    bool IsGeData(const std::string &fileName);
    void Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);

    bool IsOpInfoCompleted(const std::string &opId);
    uint32_t GetModelId(const std::string &opId) const;
    uint32_t GetModelId(uint32_t modelId);
    std::string GetOpName(const std::string &opId);
    std::string GetOpType(const std::string &opId);

private:
struct GeOpInfo {
    std::string opId;   // taskId-streamId-contextId-iterId
    std::string opName;
    std::string opType;
    uint32_t modelId;
};

private:
    void ParseIdMap(CONST_CHAR_PTR data, uint32_t len);
    void ParseTaskDesc(CONST_CHAR_PTR data, uint32_t len);
    int32_t ParseOpData(CONST_CHAR_PTR data);
    void ParseOpName(const MsprofGeProfTaskData &data, struct GeOpInfo &opInfo) const;
    void ParseOpType(const MsprofGeProfTaskData &data, struct GeOpInfo &opInfo) const;
    void PrintStats();

private:
    std::map<std::string, GeOpInfo> opInfos_;       // <taskId-streamId, GeOpInfo>
    std::map<uint32_t, uint32_t> idMap_;            // <graphId, modeId>
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
