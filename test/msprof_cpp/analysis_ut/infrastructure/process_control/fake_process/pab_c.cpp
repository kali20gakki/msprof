/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_analyzer_utest.cpp
 * Description        : TreeAnalyzer UT
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#include "pab_c.h"
#include "infrastructure/process/include/process_register.h"
#include "pstart_a.h"
#include "pd_b.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PabC::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    auto a = dataInventory.GetPtr<Ps::StartA>();
    if (!a || a->i != TEST_MAGIC_NUMBER) {
        return 1;
    }
    auto b = dataInventory.GetPtr<Ps::StructB4c>();
    if (!b) {
        return 1;
    }

    auto c = std::make_shared<StructC>();
    dataInventory.Inject(c);

    return 0;
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PabC, true, Ps::PstartA, Ps::PdB);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PabC, Ps::StartA, Ps::StructB4c);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PabC, CHIP_V1_1_0);

}