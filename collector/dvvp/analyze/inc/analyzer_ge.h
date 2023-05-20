/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2023. All rights reserved.
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
    AnalyzerGe() : totalEventTimes_(0), totalApiTimes_(0), totalNodeTimes_(0), totalGeMerges_(0) {}
    ~AnalyzerGe() {}

public:
    bool IsGeData(const std::string &fileName);
    bool IsGeApiData(const std::string &fileName);
    bool IsGeEventData(const std::string &fileName);
    bool IsGeCompactData(const std::string &tag);
    bool IsGeGraphIdMapData(const std::string &tag);

    void Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeCompactParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeEventParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeApiParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeGraphIdMapParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);

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

    void ParseNodeBasicInfo(CONST_CHAR_PTR data, uint32_t len);
    void HandleNodeBasicInfo(CONST_CHAR_PTR data);
    void ParseGraphIdMap(CONST_CHAR_PTR data, uint32_t len);
    void ParseModelInfo(CONST_CHAR_PTR data, uint32_t len, bool ageFlag);
    void HandleModelInfo(CONST_CHAR_PTR data, bool ageFlag);
    void ParseApiInfo(CONST_CHAR_PTR data, uint32_t len, bool ageFlag);
    void HandleApiInfo(CONST_CHAR_PTR data, bool ageFlag);
    void MatchApiInfoByModelInfo(const std::string &threadId, struct GeOpFlagInfo &info,
        std::multimap<std::string, GeOpFlagInfo> &modelInfo);
    void MatchApiInfo(std::multimap<std::string, GeOpFlagInfo> &apiInfo,
        std::multimap<std::string, GeOpFlagInfo> &modelInfo,
        std::multimap<std::string, GeOpFlagInfo> &nodeInfo);

private:
    std::map<std::string, GeOpInfo> opInfos_;       // <taskId-streamId, GeOpInfo>
    uint32_t totalEventTimes_;
    uint32_t totalApiTimes_;
    uint32_t totalNodeTimes_;
    uint32_t totalGeMerges_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
