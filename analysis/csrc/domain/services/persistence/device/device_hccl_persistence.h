/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_hccl_persistence.h
 * Description        : device 侧 hccl数据落盘，op，task，stastics
 * Author             : msprof team
 * Creation Date      : 2024/6/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_DEVICE_HCCL_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_DEVICE_HCCL_PERSISTENCE_H

#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
class DeviceHcclPersistence : public Process {
private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_DEVICE_HCCL_PERSISTENCE_H
