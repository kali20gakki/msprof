/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pstart_h.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "pstart_h.h"
#include "infrastructure/process/include/process_register.h"
#include "process_spy.h"

namespace Analysis {

using namespace Infra;

namespace Ps {

uint32_t PstartH::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PstartH);
    auto data = std::make_shared<StartH>();
    dataInventory.Inject(data);

    return ProcessSpy::GetResult("PstartH");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PstartH, true);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PstartH, CHIP_V1_1_0);

}