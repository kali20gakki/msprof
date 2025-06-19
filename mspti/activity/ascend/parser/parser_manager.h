/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser_manager.h
 * Description        : Manager Parser.
 * Author             : msprof team
 * Creation Date      : 2024/08/14
 * *****************************************************************************
*/
#ifndef MSPTI_PARSER_PARSER_MANAGER_H
#define MSPTI_PARSER_PARSER_MANAGER_H

#include <atomic>
#include <tuple>
#include <mutex>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include "common/inject/profapi_inject.h"
#include "common/inject/runtime_inject.h"
#include "common/inject/hccl_inject.h"
#include "activity/ascend/channel/channel_data.h"
#include "external/mspti_activity.h"
#include "common/utils.h"
#include "cann_track_cache.h"

namespace Mspti {
namespace Parser {

// <deviceId, streamId, taskId>
using DstType = std::tuple<uint16_t, uint16_t, uint16_t>;

class ParserManager final {
public:
    static ParserManager* GetInstance();

    // Device Channel
    msptiResult ReportStarsSocLog(uint32_t deviceId, const StarsSocLog* socLog);
    void ReportStepTrace(uint32_t deviceId, const StepTrace* stepTrace);

    // CANN
    msptiResult ReportRtTaskTrack(const MsprofCompactInfo* data);
    msptiResult ReportApi(const MsprofApi* const data);
    msptiResult ReportCommunicationApi(const MsprofApi *const data);
    msptiResult ReportHcclCompactData(const MsprofCompactInfo* compact);

    // Analysis thread
    Mspti::Parser::ProfTask* GetAnalysisTask(msptiActivityKind kind);
    msptiResult StartAnalysisTask(const msptiActivityKind kind);
    msptiResult StartAnalysisTasks(const std::array<std::atomic<bool>, MSPTI_ACTIVITY_KIND_COUNT> &kinds);

    msptiResult StopAnalysisTask(const msptiActivityKind kind);
    msptiResult StopAnalysisTasks(const std::array<std::atomic<bool>, MSPTI_ACTIVITY_KIND_COUNT> &kinds);

    void RegReportTypeInfo(uint16_t level, uint32_t typeId, const std::string& typeName);
    std::string& GetTypeName(uint16_t level, uint32_t typeId);

private:
    ParserManager() = default;
    explicit ParserManager(const ParserManager &obj) = delete;
    ParserManager& operator=(const ParserManager &obj) = delete;
    explicit ParserManager(ParserManager &&obj) = delete;
    ParserManager& operator=(ParserManager &&obj) = delete;
    std::shared_ptr<std::string> TryCacheMarkMsg(const char* msg);

private:
    // map<<deviceId, streamId, taskId>, list<msptiActivityKernel*>>
    std::unordered_map<DstType, std::list<std::shared_ptr<msptiActivityKernel>>, Common::TupleHash> kernel_map_;
    std::mutex kernel_mtx_;

    // <level, <typeid, typename>>
    static std::unordered_map<uint16_t, std::unordered_map<uint32_t, std::string>> typeInfo_map_;
    static std::mutex typeInfoMutex_;
};
}  // Parser
}  // Mspti

#endif
