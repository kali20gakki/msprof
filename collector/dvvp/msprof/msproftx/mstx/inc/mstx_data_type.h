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

#ifndef MSTX_DATA_TYPE_H
#define MSTX_DATA_TYPE_H
#define MSTX_INVALID_RANGE_ID 0
#define MSTX_SUCCESS 0
#define MSTX_FAIL 1

typedef enum {
    MODULE_INVALID,
    PROF_MODULE_MSPROF,
    PROF_MODULE_MSPTI
} ProfModule;

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

#endif