/**
* @log_runtime.c
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "log_runtime.h"

#include "print_log.h"
#include "library_load.h"

#if ( OS_TYPE_DEF == 0 )
#define RUNTIME_DLL "libruntime.so"
#else
#define RUNTIME_DLL "libruntime.dll"
#endif

#define DEV_STATE_CALLBACK "rtRegDeviceStateCallback"

typedef int (*RegisterDeviceStateCallback)(const char *name, DeviceStateCallback callback);

static ArgPtr g_libHandle = NULL;
static RegisterDeviceStateCallback g_rtRegDeviceStateCallback = NULL;

int RuntimeFunctionsInit(void)
{
    if (g_rtRegDeviceStateCallback != NULL) {
        return 0;
    }
    g_libHandle = LoadRuntimeDll(RUNTIME_DLL);
    if (g_libHandle == NULL) {
        SELF_LOG_ERROR("load runtime library failed.");
        return -1;
    }
    SELF_LOG_INFO("load runtime library succeed.");
    g_rtRegDeviceStateCallback = (RegisterDeviceStateCallback)LoadDllFuncSingle(g_libHandle, DEV_STATE_CALLBACK);
    if (g_rtRegDeviceStateCallback == NULL) {
        SELF_LOG_ERROR("load runtime library function failed.");
        return -1;
    }
    SELF_LOG_INFO("load runtime library function succeed.");
    return 0;
}

int RuntimeFunctionsUninit(void)
{
    int ret = UnloadRuntimeDll(g_libHandle);
    if (ret != 0) {
        SELF_LOG_WARN("close runtime library handle failed.");
    }
    SELF_LOG_WARN("close runtime library handle succeed.");
    return ret;
}

int LogRegDeviceStateCallback(DeviceStateCallback callback)
{
    if (g_rtRegDeviceStateCallback == NULL) {
        return -1;
    }
    return g_rtRegDeviceStateCallback("log", callback);
}