/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : target_info_session_time_processer.h
 * Description        : 落盘采集时间（开始、结束）数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_TARGET_INFO_TIME_PROCESSER_H
#define ANALYSIS_VIEWER_DATABASE_TARGET_INFO_TIME_PROCESSER_H

#include "analysis/csrc/viewer/database/finals/table_processer.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Viewer {
namespace Database {

// 该类用于落盘采集任务的开始结束时间
class TargetInfoSessionTimeProcesser : public TableProcesser {
// startTime, endTime, baseTime
using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>;
public:
    TargetInfoSessionTimeProcesser() = default;
    TargetInfoSessionTimeProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir = "") override;
private:
    Utils::ProfTimeRecord record_;
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TARGET_INFO_TIME_PROCESSER_H
