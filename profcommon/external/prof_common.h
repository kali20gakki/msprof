/**
* @file prof_common.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef PROF_COMMON_H
#define PROF_COMMON_H

#include <cstdint>
#include <cstddef>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__GNUC__)
#pragma GCC visibility push(default)
#endif

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

void ProfRegisterMstxFunc(MstxInitInjectionFunc mstxInitFunc, ProfModule module);

void EnableMstxFunc(ProfModule module);
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif

#if defined(__cplusplus)
}
#endif

#endif // MSPTI_PROJECT_PROF_COMMON_H
