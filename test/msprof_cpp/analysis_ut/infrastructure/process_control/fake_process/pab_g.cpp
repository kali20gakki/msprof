/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : PabG.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "pab_g.h"
#include "infrastructure/process/include/process_register.h"
#include "pstart_a.h"
#include "pd_b.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PabG::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    auto a = dataInventory.GetPtr<Ps::StartA>();
    if (!a || a->i != TEST_MAGIC_NUMBER) {
        return 1;
    }
    auto b = dataInventory.GetPtr<Ps::StructB4g>();
    if (!b || b->testStr != "testStr") {
        return 1;
    }

    auto g = std::make_shared<StructG>();
    dataInventory.Inject(g);

    return 0;
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PabG, true, Ps::PstartA, Ps::PdB);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PabG, Ps::StartA, Ps::StructB4g);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PabG, CHIP_ID_ALL);

}
