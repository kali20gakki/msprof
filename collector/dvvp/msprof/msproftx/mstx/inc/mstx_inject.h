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

#include <memory>
#include <map>
#include <stdint.h>
#include "singleton/singleton.h"

#define MSTX_INVALID_RANGE_ID 0
#define MSTX_SUCCESS 0
#define MSTX_FAIL 1

typedef enum {
    MSTX_FUNC_START                         = 0,
    MSTX_FUNC_MARKA                         = 1,
    MSTX_FUNC_RANGE_STARTA                  = 2,
    MSTX_FUNC_RANGE_END                     = 3,
    MSTX_API_CORE_MEMHEAP_REGISTER          = 4,
    MSTX_API_CORE_MEMHEAP_UNREGISTER        = 5,
    MSTX_API_CORE_MEM_REGIONS_REGISTER      = 6,
    MSTX_API_CORE_MEM_REGIONS_UNREGISTER    = 7,
    MSTX_FUNC_END
} MstxCoreFuncId;

typedef enum {
    MSTX_FUNC_DOMAIN_START        = 0,
    MSTX_FUNC_DOMAIN_CREATEA      = 1,
    MSTX_FUNC_DOMAIN_DESTROY      = 2,
    MSTX_FUNC_DOMAIN_MARKA        = 3,
    MSTX_FUNC_DOMAIN_RANGE_STARTA = 4,
    MSTX_FUNC_DOMAIN_RANGE_END    = 5,
    MSTX_FUNC_DOMAIN_END
} MstxCore2FuncId;

typedef enum {
    MSTX_API_MODULE_INVALID                 = 0,
    MSTX_API_MODULE_CORE                    = 1,
    MSTX_API_MODULE_CORE_DOMAIN             = 2,
    MSTX_API_MODULE_SIZE,                   // end of the enum, new enum items must be added before this
    MSTX_API_MODULE_FORCE_INT               = 0x7fffffff
} MstxFuncModule;

struct MstxDomainRegistrationSt {};
typedef struct MstxDomainRegistrationSt MstxDomainHandle;
typedef MstxDomainHandle* mstxDomainHandle_t;

typedef void* aclrtStream;
typedef void (*MstxFuncPointer)(void);
typedef MstxFuncPointer** MstxFuncTable;
typedef int (*MstxGetModuleFuncTableFunc)(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize);

namespace Collector {
namespace Dvvp {
namespace Mstx {

constexpr uint32_t MARK_MAX_CACHE_NUM = std::numeric_limits<uint32_t>::max();

struct MstxDomainAttr {
    MstxDomainAttr() = default;
    ~MstxDomainAttr() = default;

    std::shared_ptr<MstxDomainHandle> handle{nullptr};
    uint64_t nameHash{0};
};

class MstxInject {};

class MstxDomainMgr : public analysis::dvvp::common::singleton::Singleton<MstxDomainMgr> {
public:
    MstxDomainMgr() = default;
    ~MstxDomainMgr() = default;

    mstxDomainHandle_t CreateDomainHandle(const char* name);
    void DestroyDomainHandle(mstxDomainHandle_t domain);
    bool GetDomainNameHashByHandle(mstxDomainHandle_t domain, uint64_t &name);
    uint64_t GetDefaultDomainNameHash();

private:
    // domainHandle, domainAttr
    static std::map<mstxDomainHandle_t, std::shared_ptr<MstxDomainAttr>> domainHandleMap_;
    std::mutex domainMutex_;
};
}
}
}

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