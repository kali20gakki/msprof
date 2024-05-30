/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : load_host_data.cpp
 * Description        : 结构化context模块读取host信息
 * Author             : msprof team
 * Creation Date      : 2024/4/29
 * *****************************************************************************
 */
#include <sqlite3.h>
#include <iostream>
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/viewer/database/database.h"
#include "analysis/csrc/viewer/database/db_runner.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/services/modeling/include/log_modeling.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/device_context/device_context_error_code.h"

using namespace Analysis;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;

namespace Analysis {
namespace Domain {

using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;

uint32_t ReadHostGeinfo(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    GEInfoDB geInfoDb;
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "../", "/host", "/sqlite", geInfoDb.GetDBName()});
    DBRunner hostGeInfoDBRunner(hostDbDirectory);

    std::string sql{"SELECT stream_id, batch_id, task_id, context_id, "
                    "block_dim, mix_block_dim FROM TaskInfo"};

    OriDataFormat result;
    bool rc =  hostGeInfoDBRunner.QueryData(sql, result);
    if (!rc) {
        ERROR("Failed to obtain data from the % table.", geInfoDb.GetDBName());
        return ANALYSIS_ERROR;
    }

    int noExistCNt = 0;
    auto deviceTaskMap = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!deviceTaskMap) {
        return ANALYSIS_ERROR;
    }

    for (auto row : result) {
        uint32_t stream_id, batch_id, task_id, context_id, blockDim, mixBlockDim;
        std::tie(stream_id, batch_id, task_id, context_id, blockDim, mixBlockDim) = row;
        TaskId id = {(uint16_t)stream_id, (uint16_t)batch_id, (uint16_t)task_id, context_id};
        auto item = deviceTaskMap->find(id);
        if (item != deviceTaskMap->end()) {
            std::vector<DeviceTask> &deviceTasks = item->second;
            for (auto& deviceTask : deviceTasks) {
                deviceTask.blockDim = (uint16_t)blockDim;
                deviceTask.mixBlockDim = (uint16_t)mixBlockDim;
            }
        } else {
            noExistCNt++;
        }
    }

    INFO("Read host geinfo % not in table.", noExistCNt);
    return ANALYSIS_OK;
}

class LoadHostData : public Infra::Process {
private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &deviceContext) override;
};

uint32_t LoadHostData::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    auto devTaskSummary = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!devTaskSummary) {
        ERROR("LoadHostData get devTaskSummary fail.");
        return ANALYSIS_ERROR;
    }
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);

    return ReadHostGeinfo(dataInventory, deviceContext);
}

REGISTER_PROCESS_SEQUENCE(LoadHostData, false, Analysis::Domain::LogModeling);
REGISTER_PROCESS_DEPENDENT_DATA(LoadHostData, std::map<TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(LoadHostData, CHIP_ID_ALL);

}
}
