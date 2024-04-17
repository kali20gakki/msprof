/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pab_g.h
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PAB_G_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PAB_G_H

#include <iostream>
#include "base_gc.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {

namespace Ps {

class PabG : public BaseGc, public Infra::Process {
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
};

struct StructG {
    StructG() {}
    ~StructG() {}
};

}

}

#endif