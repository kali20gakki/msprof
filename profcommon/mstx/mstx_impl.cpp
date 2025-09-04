/* ******************************************************************************
    版权所有 (c) 华为技术有限公司 2025-2025
    Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
* File Name          : mstx_impl.h
* Description        : mstx接口实现类
* Author             : msprof team
* Creation Date      : 2025/08/30
* *****************************************************************************
*/

#include "mstx_impl.h"

#include <vector>
#include <atomic>

#include "common/util.h"

namespace Common {
namespace Mstx {
namespace {
    std::atomic<ProfModule> curModule{PROF_MODULE_MSPROF};
}

typedef struct MstxContext_t {
    MstxGetModuleFuncTableFunc getModuleFuncTable;

    MstxMarkAFunc mstxMarkAPtr;
    MstxRangeStartAFunc mstxRangeStartAPtr;
    MstxRangeEndFunc mstxRangeEndPtr;
    MstxGetToolIdFunc mstxGetToolIdPtr;

    MstxDomainCreateAFunc mstxDomainCreateAPtr;
    MstxDomainDestroyFunc mstxDomainDestroyPtr;
    MstxDomainMarkAFunc mstxDomainMarkAPtr;
    MstxDomainRangeStartAFunc mstxDomainRangeStartAPtr;
    MstxDomainRangeEndFunc mstxDomainRangeEndPtr;

    MstxFuncPointer* funcTableCore[MSTX_FUNC_END + 1];
    MstxFuncPointer* funcTableCore2[MSTX_FUNC_DOMAIN_END + 1];
};

MstxContext_t g_profMstxContext = {
    MstxGetModuleFuncTable,

    nullptr,
    nullptr,
    nullptr,
    nullptr,

    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    {
        nullptr,
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxMarkAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxRangeStartAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxRangeEndPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxGetToolIdPtr),
        nullptr
    },
    {
        nullptr,
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxDomainCreateAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxDomainDestroyPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxDomainMarkAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxDomainRangeStartAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_profMstxContext.mstxDomainRangeEndPtr),
        nullptr
    }
};

MstxContext_t g_msptiMstxContext = {
    MstxGetModuleFuncTable,

    nullptr,
    nullptr,
    nullptr,
    nullptr,

    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    {
        nullptr,
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxMarkAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxRangeStartAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxRangeEndPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxGetToolIdPtr),
        nullptr
    },
    {
        nullptr,
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxDomainCreateAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxDomainDestroyPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxDomainMarkAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxDomainRangeStartAPtr),
        reinterpret_cast<MstxFuncPointer*>(&g_msptiMstxContext.mstxDomainRangeEndPtr),
        nullptr
    }
};


void MstxMarkAImpl(const char* message, aclrtStream stream)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxMarkAPtr) {
            g_profMstxContext.mstxMarkAPtr(message, stream);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxMarkAPtr) {
            g_msptiMstxContext.mstxMarkAPtr(message, stream);
        }
    }
}

mstxRangeId MstxRangeStartAImpl(const char* message, aclrtStream stream)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxRangeStartAPtr) {
            return g_profMstxContext.mstxRangeStartAPtr(message, stream);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxRangeStartAPtr) {
            return g_msptiMstxContext.mstxRangeStartAPtr(message, stream);
        }
    }
}

void MstxRangeEndImpl(mstxRangeId id)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxRangeEndPtr) {
            g_profMstxContext.mstxRangeEndPtr(id);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxRangeEndPtr) {
            g_msptiMstxContext.mstxRangeEndPtr(id);
        }
    }
}

void MstxGetToolIdImpl(uint64_t *id)
{
    *id = MSTX_TOOL_MSPROF_ID;
}

mstxDomainHandle_t MstxDomainCreateAImpl(const char *name)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxDomainCreateAPtr) {
            return g_profMstxContext.mstxDomainCreateAPtr(name);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxDomainCreateAPtr) {
            return g_msptiMstxContext.mstxDomainCreateAPtr(name);
        }
    }
}

void MstxDomainDestroyImpl(mstxDomainHandle_t domain)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxDomainDestroyPtr) {
            g_profMstxContext.mstxDomainDestroyPtr(domain);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxDomainDestroyPtr) {
            g_msptiMstxContext.mstxDomainDestroyPtr(domain);
        }
    }
}

void MstxDomainMarkAImpl(mstxDomainHandle_t domain, const char *message, aclrtStream stream)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxDomainMarkAPtr) {
            g_profMstxContext.mstxDomainMarkAPtr(domain, message, stream);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxDomainMarkAPtr) {
            g_msptiMstxContext.mstxDomainMarkAPtr(domain, message, stream);
        }
    }
}

mstxRangeId MstxDomainRangeStartAImpl(mstxDomainHandle_t domain, const char *message,
                                      aclrtStream stream)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxDomainRangeStartAPtr) {
            return g_profMstxContext.mstxDomainRangeStartAPtr(domain, message, stream);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxDomainRangeStartAPtr) {
            return g_msptiMstxContext.mstxDomainRangeStartAPtr(domain, message, stream);
        }
    }
}

void MstxDomainRangeEndImpl(mstxDomainHandle_t domain, mstxRangeId id)
{
    if (curModule == PROF_MODULE_MSPROF) {
        if (g_profMstxContext.mstxDomainRangeStartAPtr) {
            g_profMstxContext.mstxDomainRangeEndPtr(domain, id);
        }
    } else if (curModule == PROF_MODULE_MSPTI) {
        if (g_msptiMstxContext.mstxDomainRangeStartAPtr) {
            g_msptiMstxContext.mstxDomainRangeEndPtr(domain, id);
        }
    }
}

void SetMstxModuleCoreApi(MstxFuncTable outTable, unsigned int size)
{
    if (size >= static_cast<unsigned int>(MSTX_FUNC_MARKA)) {
        *(outTable[MSTX_FUNC_MARKA]) = reinterpret_cast<MstxFuncPointer>(MstxMarkAImpl);
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_RANGE_STARTA)) {
        *(outTable[MSTX_FUNC_RANGE_STARTA]) = reinterpret_cast<MstxFuncPointer>(MstxRangeStartAImpl);
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_RANGE_END)) {
        *(outTable[MSTX_FUNC_RANGE_END]) = reinterpret_cast<MstxFuncPointer>(MstxRangeEndImpl);
    }
}

void SetMstxModuleCoreDomainApi(MstxFuncTable outTable, unsigned int size)
{
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_CREATEA)) {
        *(outTable[MSTX_FUNC_DOMAIN_CREATEA]) = reinterpret_cast<MstxFuncPointer>(MstxDomainCreateAImpl);
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_DESTROY)) {
        *(outTable[MSTX_FUNC_DOMAIN_DESTROY]) = reinterpret_cast<MstxFuncPointer>(MstxDomainDestroyImpl);
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_MARKA)) {
        *(outTable[MSTX_FUNC_DOMAIN_MARKA]) = reinterpret_cast<MstxFuncPointer>(MstxDomainMarkAImpl);
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_RANGE_STARTA)) {
        *(outTable[MSTX_FUNC_DOMAIN_RANGE_STARTA]) = reinterpret_cast<MstxFuncPointer>(MstxDomainRangeStartAImpl);
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_RANGE_END)) {
        *(outTable[MSTX_FUNC_DOMAIN_RANGE_END]) = reinterpret_cast<MstxFuncPointer>(MstxDomainRangeEndImpl);
    }
}

int GetModuleTableFunc(MstxGetModuleFuncTableFunc getFuncTable)
{
    int retVal = MSTX_SUCCESS;
    unsigned int outSize = 0;
    MstxFuncTable outTable;
    static std::vector<unsigned int> CheckOutTableSizes = {
        0,
        MSTX_FUNC_END,
        MSTX_FUNC_DOMAIN_END,
        0
    };
    for (size_t i = MSTX_API_MODULE_CORE; i < MSTX_API_MODULE_SIZE; i++) {
        if (getFuncTable(static_cast<MstxFuncModule>(i), &outTable, &outSize) != MSTX_SUCCESS) {
            continue;
        }
        switch (i) {
            case MSTX_API_MODULE_CORE:
                SetMstxModuleCoreApi(outTable, outSize);
                break;
            case MSTX_API_MODULE_CORE_DOMAIN:
                SetMstxModuleCoreDomainApi(outTable, outSize);
                break;
            default:
                retVal = MSTX_FAIL;
                break;
        }
        if (retVal == MSTX_FAIL) {
            break;
        }
    }
    return retVal;
}

int MstxGetModuleFuncTable(
    MstxFuncModule module, MstxFuncTable* outTable, unsigned int* outSize)
{
    MstxFuncTable table = nullptr;
    if (!outSize || !outTable) {
        return MSTX_FAIL;
    }
    MstxContext_t* context = nullptr;
    if (curModule == PROF_MODULE_MSPROF) {
        context = &g_profMstxContext;
    } else {
        context = &g_msptiMstxContext;
    }
    switch (module) {
        case MSTX_API_MODULE_CORE:
            *outTable = context->funcTableCore;
            *outSize = LengthOf(context->funcTableCore);
            break;
        case MSTX_API_MODULE_CORE_DOMAIN:
            *outTable = context->funcTableCore2;
            *outSize = LengthOf(context->funcTableCore);
            break;
        default:
            return MSTX_FAIL;
    }

    return MSTX_SUCCESS;
}

void ProfRegisteMstxFunc(MstxInitInjectionFunc mstxInitFunc, ProfModule module)
{
    if (mstxInitFunc) {
        curModule = module;
        if (module == PROF_MODULE_MSPROF) {
            mstxInitFunc(g_profMstxContext.getModuleFuncTable);
        } else if (module == PROF_MODULE_MSPTI) {
            mstxInitFunc(g_msptiMstxContext.getModuleFuncTable);
        }
    }
}

void EnableMstxFunc(ProfModule module)
{
    curModule = module;
}
}
}