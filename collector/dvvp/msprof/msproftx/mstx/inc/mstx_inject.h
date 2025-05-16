/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2025
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.h
 * Description        : Common definition of mstx inject func.
 * Author             : msprof team
 * Creation Date      : 2024/07/31
 * *****************************************************************************
*/
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
int GetModuleTableFunc(MstxGetModuleFuncTableFunc getFuncTable);
}

#endif