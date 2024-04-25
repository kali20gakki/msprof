/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_task_process.h
 * Description        : 初始化deviceTask到dataInventory中
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DEVICE_TASK_PROCESS_H
#define MSPROF_ANALYSIS_DEVICE_TASK_PROCESS_H

#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {
namespace Domain {

class DeviceTaskProcess : public Infra::Process {
private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context& context) override;
};
}
}
#endif // MSPROF_ANALYSIS_DEVICE_TASK_PROCESS_H
