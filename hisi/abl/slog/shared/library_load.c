/**
* @library_load.c
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "library_load.h"
#include "print_log.h"

/**
 * @brief LoadRuntimeDll: load library
 * @param [in]dllName: library name
 * @return: library handle
 */
ArgPtr LoadRuntimeDll(const char *dllName)
{
    ONE_ACT_NO_LOG(dllName == NULL, return NULL);

#if (OS_TYPE_DEF == LINUX)
    ArgPtr handle = dlopen(dllName, RTLD_LAZY);
#else
    ArgPtr handle = LoadLibrary(dllName);
#endif
    if (handle == NULL) {
        SELF_LOG_WARN("load %s, strerr=%s.", dllName, strerror(ToolGetErrorCode()));
        return NULL;
    }
    return handle;
}

/**
 * @brief UnloadRuntimeDll: free library load handle
 * @param [in]handle: library handle
 * @return: 0: succeed, -1: failed
 */
int UnloadRuntimeDll(ArgPtr handle)
{
    ONE_ACT_NO_LOG(handle == NULL, return -1);

#if (OS_TYPE_DEF == LINUX)
    ONE_ACT_NO_LOG(dlclose(handle) != 0, return -1);
#else
    ONE_ACT_NO_LOG(!FreeLibrary(handle), return -1);
#endif
    return 0;
}

/**
* @brief LoadDllFunc: find library symbols and load it
* @param [in]handle: library load handle
* @param [in]symbolInfos: symbol info and handle
* @param [in]symbolNum: symbol nums
* @return: On success, return 0
*     On failure, return -1
*/
int LoadDllFunc(ArgPtr handle, SymbolInfo *symbolInfos, unsigned int symbolNum)
{
    ONE_ACT_NO_LOG(handle == NULL || symbolInfos == NULL, return -1);
    ONE_ACT_NO_LOG(symbolNum == 0, return -1);

    unsigned int i = 0;
    for (; i < symbolNum; i++) {
        const char *symbol = symbolInfos[i].symbol;
        if (symbol == NULL) {
            continue;
        }
#if (OS_TYPE_DEF == LINUX)
        ArgPtr value = dlsym(handle, symbol);
#else
        ArgPtr value = GetProcAddress(handle, symbol);
#endif
        if (value == NULL) {
            SELF_LOG_INFO("find function symbol failed.");
            continue;
        }
        symbolInfos[i].handle = value;
    }
    return 0;
}

/**
* @brief LoadDllFuncSingle: find one library symbol and load it
* @param [in]handle: library load handle
* @param [in]symbol: function name
* @return: On success, return the address associated with symbol
*     On failure, return NULL
*/
ArgPtr LoadDllFuncSingle(ArgPtr handle, const char *symbol)
{
    ONE_ACT_NO_LOG(handle == NULL || symbol == NULL, return NULL);

#if (OS_TYPE_DEF == LINUX)
    ArgPtr function = dlsym(handle, symbol);
#else
    ArgPtr function = GetProcAddress(handle, symbol);
#endif
    if (function == NULL) {
        SELF_LOG_WARN("find function symbol failed.");
    }
    return function;
}
