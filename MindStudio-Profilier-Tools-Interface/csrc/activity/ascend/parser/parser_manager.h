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
 * -------------------------------------------------------------------------
*/
#ifndef MSPTI_PARSER_PARSER_MANAGER_H
#define MSPTI_PARSER_PARSER_MANAGER_H

#include <atomic>
#include <array>
#include "csrc/common/inject/profapi_inject.h"
#include "csrc/activity/ascend/channel/channel_data.h"
#include "csrc/include/mspti_activity.h"
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
