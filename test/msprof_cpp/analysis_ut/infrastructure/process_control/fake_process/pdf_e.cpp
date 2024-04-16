/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pdf_e.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "pdf_e.h"
#include "infrastructure/process/include/process_register.h"
#include "pah_d.h"
#include "pg_f.h"
#include "process_spy.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PdfE::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PdfE);
    auto d = dataInventory.GetPtr<Ps::StructD>();
    auto f = dataInventory.GetPtr<Ps::StructF>();
    if (!d || !f) { // 这里是测试，框架应该在进入本流程时，不应该释放这个内存
        return 1;
    }

    return ProcessSpy::GetResult("PdfE");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PdfE, true, Ps::PahD, Ps::PgF);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PdfE, Ps::StructD, Ps::StructF);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PdfE, CHIP_ID_ALL);

}
