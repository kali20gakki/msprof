/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pah_d.h
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PAH_D_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PAH_D_H
#include <iostream>
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {

namespace Ps {

class PahD : public Infra::Process {
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
};

struct StructD {
    StructD() {}
    ~StructD() {}
};

}

}

#endif