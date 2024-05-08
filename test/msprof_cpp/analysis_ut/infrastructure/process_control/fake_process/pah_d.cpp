/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : PahD.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "pah_d.h"
#include "infrastructure/process/include/process_register.h"
#include "pstart_a.h"
#include "pstart_h.h"
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PahD::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PahD);
    auto a = dataInventory.GetPtr<Ps::StartA>();
    auto h = dataInventory.GetPtr<Ps::StartH>();  // H流程在chipv2时不生成，因此下面不判断h
    if (!a) {
        return 1;
    }
    if (a->i != TEST_MAGIC_NUMBER) {
        return 1;
    }
    auto d = std::make_shared<StructD>();
    dataInventory.Inject(d);

    return ProcessSpy::GetResult("PahD");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PahD, true, Ps::PstartA, Ps::PstartH);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PahD, Ps::StartA, Ps::StartH);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PahD, CHIP_V1_1_0, CHIP_V2_1_0);

}
