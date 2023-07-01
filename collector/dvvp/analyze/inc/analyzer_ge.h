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
    AnalyzerGe() : isAllStaticShape_(true), totalEventTimes_(0), totalApiTimes_(0), totalNodeTimes_(0),
        totalGeMerges_(0) {}
    ~AnalyzerGe() {}

public:
    bool IsGeData(const std::string &fileName) const;
    bool IsGeApiOrEventData(const std::string &fileName) const;
    bool IsGeCompactData(const std::string &tag) const;
    bool IsGeGraphIdMapData(const std::string &tag) const;
    bool IsGeContextData(const std::string &tag) const;

    void Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeCompactParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeApiAndEventParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeGraphIdMapParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);
    void GeContextParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message);

    bool IsOpInfoCompleted(const std::string &opId);
    uint32_t GetModelId(const std::string &opId) const;
    uint32_t GetModelId(uint32_t modelId);
    std::string GetOpName(const std::string &opId);
    std::string GetOpType(const std::string &opId);
    bool GetIsAllStaticShape() const;
    bool GetStreamType(const int &streamId, int &streamType);

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
    void PrintStats() const;

    void ParseNodeBasicInfo(CONST_CHAR_PTR data, uint32_t len);
    void HandleNodeBasicInfo(CONST_CHAR_PTR data) const;
    void ParseGraphIdMap(CONST_CHAR_PTR data, uint32_t len);
    void HandleModelInfo(CONST_CHAR_PTR data, bool ageFlag) const;
    void ParseApiAndEventInfo(CONST_CHAR_PTR data, uint32_t len, bool ageFlag);
    void HandleApiInfo(CONST_CHAR_PTR data, bool ageFlag) const;
    void ParseContextIdInfo(CONST_CHAR_PTR data, uint32_t len);
    void HandleContextIdInfo(CONST_CHAR_PTR data) const;
    bool HandleContextWithNode(std::multimap<uint32_t, GeOpFlagInfo> &nodeInfo,
        std::multimap<uint32_t, GeOpFlagInfo> &contextInfo) const;

    void MatchApiInfoByModelInfo(const uint32_t &threadId, struct GeOpFlagInfo &info,
        std::multimap<uint32_t, GeOpFlagInfo> &modelInfo) const;
    void MatchApiInfoByNodeInfo(const uint32_t &threadId, struct GeOpFlagInfo &info,
        std::multimap<uint32_t, GeOpFlagInfo> &nodeInfo);
    void MatchApiInfo(std::multimap<uint32_t, GeOpFlagInfo> &apiInfo,
        std::multimap<uint32_t, GeOpFlagInfo> &modelInfo,
        std::multimap<uint32_t, GeOpFlagInfo> &nodeInfo,
        std::multimap<uint32_t, GeOpFlagInfo> &contextInfo);
    void MatchDeviceOpInfo(std::vector<RtOpInfo> &devTmpOpInfo,
        std::multimap<uint32_t, GeOpFlagInfo> &geOpInfo);

private:
    std::map<std::string, GeOpInfo> opInfos_;       // <taskId-streamId, GeOpInfo>
    uint32_t totalEventTimes_;
    uint32_t totalApiTimes_;
    uint32_t totalNodeTimes_;
    uint32_t totalGeMerges_;
    std::map<uint32_t, StreamInfo> steamState_;  // <streamid, StreamInfo>
    bool isAllStaticShape_;
};
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis

#endif
