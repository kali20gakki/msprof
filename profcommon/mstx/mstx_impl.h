/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_impl.cpp
 * Description        : mstx接口实现类
 * Author             : msprof team
 * Creation Date      : 2025/08/30
 * *****************************************************************************
*/

#ifndef COMMON_MSTX_MSTX_IMPL_H
#define COMMON_MSTX_MSTX_IMPL_H

#include <cstdint>

#include "external/prof_common.h"

#define MSTX_TOOL_MSPROF_ID 0x1000

#define MSTX_FAIL 1
#define MSTX_SUCCESS 0

#define MSTX_INVALID_RANGE_ID 0

namespace Common {
namespace Mstx {

typedef enum {
    MSTX_API_CORE_INVALID                   = 0,
    MSTX_FUNC_MARKA                         = 1,
    MSTX_FUNC_RANGE_STARTA                  = 2,
    MSTX_FUNC_RANGE_END                     = 3,
    MSTX_API_CORE_GET_TOOL_ID               = 4,
    MSTX_FUNC_END
} MstxCoreFuncId;

typedef enum {
    MSTX_API_CORE2_INVALID        = 0,
    MSTX_FUNC_DOMAIN_CREATEA      = 1,
    MSTX_FUNC_DOMAIN_DESTROY      = 2,
    MSTX_FUNC_DOMAIN_MARKA        = 3,
    MSTX_FUNC_DOMAIN_RANGE_STARTA = 4,
    MSTX_FUNC_DOMAIN_RANGE_END    = 5,
    MSTX_FUNC_DOMAIN_END
} MstxCore2FuncId;

using AclError = int;
using AclrtStream = void *;
using AclrtContext = void *;

typedef uint64_t mstxRangeId;
typedef void* aclrtStream;

struct mstxDomainRegistration_st;
typedef struct mstxDomainRegistration_st mstxDomainRegistration_t;
typedef mstxDomainRegistration_t* mstxDomainHandle_t;

void MstxMarkAImpl(const char* message, aclrtStream stream);
mstxRangeId MstxRangeStartAImpl(const char* message, aclrtStream stream);
void MstxRangeEndImpl(mstxRangeId id);
void MstxGetToolIdImpl(uint64_t *id);
mstxDomainHandle_t MstxDomainCreateAImpl(const char *name);
void MstxDomainDestroyImpl(mstxDomainHandle_t domain);
void MstxDomainMarkAImpl(mstxDomainHandle_t domain, const char *message, aclrtStream stream);
mstxRangeId MstxDomainRangeStartAImpl(mstxDomainHandle_t domain, const char *message,
    aclrtStream stream);
void MstxDomainRangeEndImpl(mstxDomainHandle_t domain, mstxRangeId id);
int GetModuleTableFunc(MstxGetModuleFuncTableFunc getFuncTable);
void ProfRegisteMstxFunc(MstxInitInjectionFunc mstxInitFunc, ProfModule module);
void EnableMstxFunc(ProfModule module);
int MsptiMstxGetModuleFuncTable(MstxFuncModule module, MstxFuncTable* outTable, unsigned int* outSize);
int ProfMstxGetModuleFuncTable(MstxFuncModule module, MstxFuncTable* outTable, unsigned int* outSize);

using MstxMarkAFunc = decltype(&MstxMarkAImpl);
using MstxRangeStartAFunc = decltype(&MstxRangeStartAImpl);
using MstxRangeEndFunc = decltype(&MstxRangeEndImpl);
using MstxGetToolIdFunc = decltype(&MstxGetToolIdImpl);
using MstxDomainCreateAFunc = decltype(&MstxDomainCreateAImpl);
using MstxDomainDestroyFunc = decltype(&MstxDomainDestroyImpl);
using MstxDomainMarkAFunc = decltype(&MstxDomainMarkAImpl);
using MstxDomainRangeStartAFunc = decltype(&MstxDomainRangeStartAImpl);
using MstxDomainRangeEndFunc = decltype(&MstxDomainRangeEndImpl);
using MstxGetModuleFuncTableFunc = int (*)(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize);
using MstxInitInjectionFunc = int (*)(MstxGetModuleFuncTableFunc);
}
}

#endif
