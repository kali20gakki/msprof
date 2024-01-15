/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : target_info_session_time_processer.cpp
 * Description        : 落盘采集时间（开始、结束）数据
 * Author             : msprof team
 * Creation Date      : 2023/12/19
 * *****************************************************************************
 */
#include "target_info_session_time_processer.h"

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;

TargetInfoSessionTimeProcesser::TargetInfoSessionTimeProcesser(const std::string &reportDBPath,
                                                               const std::set<std::string> &profPaths)
    : TableProcesser(reportDBPath, profPaths)
{
    reportDB_.tableName = TABLE_NAME_TARGET_INFO_SESSION_TIME;
};

bool TargetInfoSessionTimeProcesser::Run()
{
    for (const auto& path : profPaths_) {
        if (!Process(path)) {
            return false;
        }
    }
    TimeDataFormat timeInfoData = {
        std::make_tuple(record_.startTimeNs, record_.endTimeNs, Analysis::Utils::TIME_BASE_OFFSET_NS)
    };
    return SaveData(timeInfoData);
}

bool TargetInfoSessionTimeProcesser::Process(const std::string &fileDir)
{
    std::string hostDir = Utils::File::PathJoin({fileDir, HOST});
    Utils::ProfTimeRecord tempRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(tempRecord, fileDir)) {
        return false;
    }
    // 开始时间取最早的，结束时间取最晚的
    if (record_.startTimeNs > tempRecord.startTimeNs) {
        record_.startTimeNs = tempRecord.startTimeNs;
    }
    if (record_.endTimeNs < tempRecord.endTimeNs) {
        record_.endTimeNs = tempRecord.endTimeNs;
    }
    return true;
}


} // Database
} // Viewer
} // Analysis