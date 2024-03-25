/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : session_time_info_processor.h
 * Description        : 落盘采集时间（开始、结束）数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_SESSION_TIME_INFO_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_SESSION_TIME_INFO_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {

// 该类用于落盘采集任务的开始结束时间
class SessionTimeInfoProcessor : public TableProcessor {
// startTime, endTime
using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t>>;
public:
    SessionTimeInfoProcessor() = default;
    SessionTimeInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir = "") override;
private:
    Utils::ProfTimeRecord record_;
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_SESSION_TIME_INFO_PROCESSOR_H
