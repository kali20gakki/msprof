/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pstart_h.h
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PSTART_H_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PSTART_H_H
#include <iostream>
#include "infrastructure/process/include/process.h"
#include "infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis {

namespace Ps {

class PstartH : public Infra::Process {
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
};

struct StartH {
    StartH() {}
    ~StartH() {}
    const char* member;
};

}

}

#endif