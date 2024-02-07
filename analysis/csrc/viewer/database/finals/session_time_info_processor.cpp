/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : session_time_info_processor.cpp
 * Description        : 落盘采集时间（开始、结束）数据
 * Author             : msprof team
 * Creation Date      : 2023/12/19
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/session_time_info_processor.h"

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;

SessionTimeInfoProcessor::SessionTimeInfoProcessor(const std::string &reportDBPath,
                                                   const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool SessionTimeInfoProcessor::Run()
{
    INFO("SessionTimeInfoProcessor Run.");
    bool flag = true;
    for (const auto& path : profPaths_) {
        if (!Process(path)) {
            flag = false;
        }
    }
    // 若Process中数据获取失败,理应不落盘数据
    if (!flag) {
        ERROR("Get session time failed.");
        return flag;
    }
    TimeDataFormat timeInfoData = {
        std::make_tuple(record_.startTimeNs, record_.endTimeNs, Analysis::Utils::TIME_BASE_OFFSET_NS)
    };
    flag = SaveData(timeInfoData, TABLE_NAME_SESSION_TIME_INFO);
    PrintProcessorResult(flag, TABLE_NAME_ENUM_API_LEVEL);
    return flag;
}

bool SessionTimeInfoProcessor::Process(const std::string &fileDir)
{
    INFO("SessionTimeInfoProcessor Process, dir is %", fileDir);
    std::string hostDir = Utils::File::PathJoin({fileDir, HOST});
    Utils::ProfTimeRecord tempRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(tempRecord, fileDir)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", fileDir);
        return false;
    }
    // 开始时间取最早的，结束时间取最晚的
    record_.startTimeNs = std::min(record_.startTimeNs, tempRecord.startTimeNs);
    record_.endTimeNs = std::max(record_.endTimeNs, tempRecord.endTimeNs);
    return true;
}


} // Database
} // Viewer
} // Analysis