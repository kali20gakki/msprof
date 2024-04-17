/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pg_f.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "pg_f.h"
#include "infrastructure/process/include/process_register.h"
#include "pab_g.h"
#include "pab_c.h"
#include "process_spy.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PgF::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PgF);
    auto a = dataInventory.GetPtr<Ps::StructG>();
    auto c = dataInventory.GetPtr<Ps::StructC>();
    if (!a) { // 这里不能强判断c，c是可选流程，在chipv2场景不运行
        return 1;
    }
    auto f = std::make_shared<StructF>();
    dataInventory.Inject(f);

    return ProcessSpy::GetResult("PgF");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PgF, true, Ps::PabG, Ps::PabC);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PgF, Ps::StructG, Ps::StructC);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PgF, CHIP_ID_ALL);

}
