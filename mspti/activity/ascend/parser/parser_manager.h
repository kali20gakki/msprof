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
#include <array>
#include "common/inject/profapi_inject.h"
#include "activity/ascend/channel/channel_data.h"
#include "external/mspti_activity.h"
#include "cann_track_cache.h"

namespace Mspti {
namespace Parser {
class ParserManager final {
public:
    static ParserManager *GetInstance();
    // Device Channel
    void ReportStepTrace(uint32_t deviceId, const StepTrace *stepTrace);

    // CANN
    msptiResult ReportApi(const MsprofApi *data);

    // analysis thread
    msptiResult StartAnalysisTask(msptiActivityKind kind);
    msptiResult StartAnalysisTasks(const std::array<std::atomic<bool>, MSPTI_ACTIVITY_KIND_COUNT> &kinds);

    msptiResult StopAnalysisTask(msptiActivityKind kind);
    msptiResult StopAnalysisTasks(const std::array<std::atomic<bool>, MSPTI_ACTIVITY_KIND_COUNT> &kinds);
private:
    Mspti::Parser::ProfTask *GetAnalysisTask(msptiActivityKind kind);
};
} // Parser
} // Mspti

#endif
