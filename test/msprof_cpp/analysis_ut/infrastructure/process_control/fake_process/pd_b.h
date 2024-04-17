/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pd_b.h
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PD_B_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_PD_B_H

#include <iostream>
#include <string>
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {

namespace Ps {

class PdB : public Infra::Process {
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
};

struct StructB4c {
    StructB4c() {std::cout << "StructB4c Construct!" << std::endl;}
    ~StructB4c() {std::cout << "StructB4c Destruct!" << std::endl;}
    const char* testCh;
};

struct StructB4g {
    StructB4g() {std::cout << "StructB4g Construct!" << std::endl;}
    ~StructB4g() {std::cout << "StructB4g Destruct!" << std::endl;}
    std::string testStr;
};

}

}

#endif