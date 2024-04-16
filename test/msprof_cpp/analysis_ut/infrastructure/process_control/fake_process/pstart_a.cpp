/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pstart_a.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "pstart_a.h"
#include "infrastructure/process/include/process_register.h"
#include "process_spy.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PstartA::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PstartA);
    auto data = std::make_shared<Ps::StartA>();
    data->i = TEST_MAGIC_NUMBER;
    dataInventory.Inject(data);

    return ProcessSpy::GetResult("PstartA");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PstartA, true);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PstartA, CHIP_V1_1_0, CHIP_V2_1_0);

}
