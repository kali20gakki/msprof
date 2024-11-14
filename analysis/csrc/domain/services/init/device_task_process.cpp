/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_task_process.cpp
 * Description        : 初始化DeviceTask结构到DataInventory
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/init/include/device_task_process.h"
#include <map>
#include <vector>
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"

namespace Analysis {
namespace Domain {

using namespace Infra;
using namespace Utils;
using DeviceTaskSummary = std::map<TaskId, std::vector<DeviceTask>>;
DeviceTaskSummary deviceTaskSummary;

uint32_t DeviceTaskProcess::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    std::string sqlitePath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE});
    if (!File::CreateDir(sqlitePath)) {
        ERROR("Failed to create %. Please check that the path is accessible or the disk space is enough", sqlitePath);
        return ANALYSIS_ERROR;
    }
    std::shared_ptr<std::map<TaskId, std::vector<DeviceTask>>> data;
    MAKE_SHARED_RETURN_VALUE(data, DeviceTaskSummary, Analysis::ANALYSIS_ERROR, std::move(deviceTaskSummary));
    if (dataInventory.Inject(data)) {
        return Analysis::ANALYSIS_OK;
    } else {
        ERROR("Init DeviceTask failed");
        return Analysis::ANALYSIS_ERROR;
    }
}

REGISTER_PROCESS_SEQUENCE(DeviceTaskProcess, true);
REGISTER_PROCESS_SUPPORT_CHIP(DeviceTaskProcess, CHIP_ID_ALL);
}
}