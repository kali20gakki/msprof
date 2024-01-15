/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : flip_task_db_dumper.cpp
 * Description        : FlipTaskDBDumper实现
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/drafts/flip_task_db_dumper.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/utils/utils.h"

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
            data.emplace_back(flipTask->deviceId, flipTask->streamId, flipTask->timeStamp, flipTask->taskId,
                              flipTask->flipNum);
        }
    }
    return data;
}
} // Drafts
} // Database
} // Viewer
} // Analysis