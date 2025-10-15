/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : prof_mstx.cpp
 * Description        : 存储转发mstx实际init方法
 * Author             : msprof team
 * Creation Date      : 2025/08/07
 * *****************************************************************************
*/

#include "prof_mstx.h"
#include "external/prof_common.h"
#include "mspti/common/plog_manager.h"

int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable)
{
    if (getFuncTable == nullptr) {
        return MSTX_FAIL;
    }
    return Common::Mstx::GetModuleTableFunc(getFuncTable);
}

void ProfRegisterMstxFunc(MstxInitInjectionFunc mstxInitFunc, ProfModule module)
{
    if (mstxInitFunc != nullptr) {
        Common::Mstx::ProfRegisterMstxFunc(mstxInitFunc, module);
    }
    return;
}

void EnableMstxFunc(ProfModule module)
{
    Common::Mstx::EnableMstxFunc(module);
}