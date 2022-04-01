/**
 * @library_load.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LIBRARY_LOAD_H
#define LIBRARY_LOAD_H
#if (OS_TYPE_DEF == LINUX)
#include "dlfcn.h"
#else
#include "mmpa_api.h"
#endif
#include "log_sys_package.h"

typedef struct {
    const char *symbol;
    ArgPtr handle;
} SymbolInfo;

ArgPtr LoadRuntimeDll(const char *dllName);
int UnloadRuntimeDll(ArgPtr handle);
int LoadDllFunc(ArgPtr handle, SymbolInfo *symbolInfos, unsigned int symbolNum);
void *LoadDllFuncSingle(ArgPtr handle, const char *symbol);

#endif
