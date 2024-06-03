/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : load_host_data.h
 * Description        : 结构化context模块读取host信息
 * Author             : msprof team
 * Creation Date      : 2024/5/20
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_LOAD_HOST_DATA_H
#define MSPROF_ANALYSIS_LOAD_HOST_DATA_H

#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {
namespace Domain {
class LoadHostData : public Infra::Process {
private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &deviceContext) override;
};
}
}
#endif // MSPROF_ANALYSIS_LOAD_HOST_DATA_H
