/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : flip_task_db_dumper.h
 * Description        : FlipTaskDBDumper
 * Author             : msprof team
 * Creation Date      : 2024/3/6
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/drafts/flip_task_db_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
FlipTaskDBDumper::FlipTaskDBDumper(const std::string& hostFilePath) : BaseDumper<FlipTaskDBDumper>(hostFilePath,
                                                                                                   "HostTaskFlip")
{
    MAKE_SHARED0_NO_OPERATION(database_, RtsTrackDB);
}

FlipTaskData FlipTaskDBDumper::GenerateData(const FlipTasks &flipTasks)
{
    FlipTaskData data;
    if (Utils::Reserve(data, flipTasks.size())) {
        for (auto &flipTask: flipTasks) {
            data.emplace_back(flipTask->streamId, flipTask->timeStamp, flipTask->taskId, flipTask->flipNum);
        }
    }
    return data;
}
} // Drafts
} // Database
} // Viewer
} // Analysis