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
#ifndef MSTX_INJECT_H
#define MSTX_INJECT_H

#include <stdint.h>
#include "mstx_data_type.h"

namespace MsprofMstxApi {
void MstxMarkAFunc(const char* msg, aclrtStream stream);
uint64_t MstxRangeStartAFunc(const char* msg, aclrtStream stream);
void  MstxRangeEndFunc(uint64_t id);
mstxDomainHandle_t MstxDomainCreateAFunc(const char* name);
void MstxDomainDestroyFunc(mstxDomainHandle_t domain);
void MstxDomainMarkAFunc(mstxDomainHandle_t domain, const char* msg, aclrtStream stream);
uint64_t MstxDomainRangeStartAFunc(mstxDomainHandle_t domain, const char* msg, aclrtStream stream);
void MstxDomainRangeEndFunc(mstxDomainHandle_t domain, uint64_t id);
int GetModuleTableFunc(MstxGetModuleFuncTableFunc getFuncTable);
int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable);
void MstxRegisterMstxFunc();
void EnableMstxFunc();
}

#endif