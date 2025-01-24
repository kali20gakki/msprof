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
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Domain::Environment::Context;

SessionTimeInfoProcessor::SessionTimeInfoProcessor(const std::string &msprofDBPath,
                                                   const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

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
        PrintProcessorResult(flag, PROCESSOR_NAME_SESSION_TIME_INFO);
        return flag;
    }
    TimeDataFormat timeInfoData = {std::make_tuple(record_.startTimeNs, record_.endTimeNs)};
    flag = SaveData(timeInfoData, TABLE_NAME_SESSION_TIME_INFO);
    PrintProcessorResult(flag, PROCESSOR_NAME_SESSION_TIME_INFO);
    return flag;
}

bool SessionTimeInfoProcessor::Process(const std::string &fileDir)
{
    INFO("SessionTimeInfoProcessor Process, dir is %", fileDir);
    Utils::ProfTimeRecord tempRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(tempRecord, fileDir)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", fileDir);
        return false;
    }
    if (tempRecord.endTimeNs == Utils::DEFAULT_END_TIME_NS) {
        ERROR("No end_info.json, can't get session time info.");
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