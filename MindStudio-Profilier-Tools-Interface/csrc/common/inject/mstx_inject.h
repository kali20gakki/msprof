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
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_COMMON_INJECT_MSPROFTX_INJECT_H
#define MSPTI_COMMON_INJECT_MSPROFTX_INJECT_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <unordered_set>
#include "csrc/common/inject/inject_base.h"
#include "csrc/include/mspti_result.h"
#include "csrc/include/mspti_activity.h"

constexpr int MSTX_FAIL = 1;
constexpr int MSTX_SUCCESS = 0;

constexpr uint64_t MSTX_INVALID_RANGE_ID = 0;

typedef enum {
    MSTX_FUNC_START                         = 0,
    MSTX_FUNC_MARKA                         = 1,
    MSTX_FUNC_RANGE_STARTA                  = 2,
    MSTX_FUNC_RANGE_END                     = 3,
    MSTX_API_CORE_GET_TOOL_ID               = 4,
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
    MODULE_INVALID,
    PROF_MODULE_MSPROF,
    PROF_MODULE_MSPTI,
    PROF_MODULE_SIZE
} ProfModule;

typedef enum {
    MSTX_API_MODULE_INVALID                 = 0,
    MSTX_API_MODULE_CORE                    = 1,
    MSTX_API_MODULE_CORE_DOMAIN             = 2,
    MSTX_API_MODULE_SIZE,                   // end of the enum, new enum items must be added before this
    MSTX_API_MODULE_FORCE_INT               = 0x7fffffff
} MstxFuncModule;

typedef void (*MstxFuncPointer)(void);
typedef MstxFuncPointer** MstxFuncTable;
typedef int (*MstxGetModuleFuncTableFunc)(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize);
typedef int(*MstxInitInjectionFunc)(MstxGetModuleFuncTableFunc);

// 注册mstx接口
void ProfRegisterMstxFunc(MstxInitInjectionFunc mstxInitFunc, ProfModule module);
void EnableMstxFunc(ProfModule module);

struct mstxDomainRegistration_st {};
typedef struct mstxDomainRegistration_st MstxDomainHandle;
typedef MstxDomainHandle* mstxDomainHandle_t;

void MstxMarkAFunc(const char* msg, AclrtStream stream);
uint64_t MstxRangeStartAFunc(const char* msg, AclrtStream stream);
void MstxRangeEndFunc(uint64_t rangeId);

namespace Mspti {
class MstxInject {};

struct MstxDomainAttr {
    MstxDomainAttr() = default;
    ~MstxDomainAttr() = default;

    std::shared_ptr<MstxDomainHandle> handle{nullptr};
    std::shared_ptr<std::string> name{nullptr};
    bool isDestroyed{false};
    bool isEnabled{true};
};

class MstxDomainMgr {
public:
    static MstxDomainMgr* GetInstance();
    mstxDomainHandle_t CreateDomainHandle(const char* name);
    void DestroyDomainHandle(mstxDomainHandle_t domain);

    std::shared_ptr<std::string> GetDomainNameByHandle(mstxDomainHandle_t domain);
    bool isDomainEnable(mstxDomainHandle_t domainHandle);
    bool isDomainEnable(const char* name);
    msptiResult SetMstxDomainEnableStatus(const char* name, bool flag);
    mstxDomainHandle_t GetDefaultDomainHandle() { return defaultDomainHandle_t; }

private:
    MstxDomainMgr() = default;
    explicit MstxDomainMgr(const MstxDomainMgr &obj) = delete;
    MstxDomainMgr& operator=(const MstxDomainMgr &obj) = delete;
    explicit MstxDomainMgr(MstxDomainMgr &&obj) = delete;
    MstxDomainMgr& operator=(MstxDomainMgr &&obj) = delete;

private:
    static std::once_flag onceFlag;
    static mstxDomainHandle_t defaultDomainHandle_t;
    // domainHandle, domainStatus
    static std::unordered_map<mstxDomainHandle_t, std::shared_ptr<MstxDomainAttr>> domainHandleMap_;
    // domainName
    static std::unordered_set<std::string> disableDomains_;
    std::mutex domainMutex_;
};
};

namespace MsptiMstxApi {
int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable);
msptiResult MstxRegisterMstxFunc();
void MsptiEnableMstxFunc();
}

#endif
