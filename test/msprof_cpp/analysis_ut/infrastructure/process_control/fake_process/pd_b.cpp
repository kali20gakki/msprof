/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : PdB.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "pd_b.h"
#include "infrastructure/process/include/process_register.h"
#include "pah_d.h"
#include "pstart_a.h"  // 包含这个头文件是为了测试，实际写代码的时候，请不要包含自己不需要的头文件
#include "process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {

using namespace Infra;
namespace Ps {

uint32_t PdB::ProcessEntry(DataInventory& dataInventory, const Infra::Context&)
{
    PROCESS_TRACE(PdB);
    auto a = dataInventory.GetPtr<Ps::StartA>();
    if (!a) { // 这里是测试，框架应该在进入本流程时，不应该释放这个内存
        return 1;
    }
    auto d4c = std::make_shared<StructB4c>();
    d4c->testCh = "testCh";
    dataInventory.Inject(d4c);
    auto d4g = std::make_shared<StructB4g>();
    d4g->testStr = "testStr";
    dataInventory.Inject(d4g);

    return ProcessSpy::GetResult("PdB");
}

}

REGISTER_PROCESS_SEQUENCE(Ps::PdB, true, Ps::PahD);
REGISTER_PROCESS_DEPENDENT_DATA(Ps::PdB, Ps::StructD);
REGISTER_PROCESS_SUPPORT_CHIP(Ps::PdB, CHIP_ID_ALL);

}
