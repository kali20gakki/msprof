/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

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