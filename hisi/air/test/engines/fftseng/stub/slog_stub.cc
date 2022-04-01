/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "toolchain/slog.h"
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <stdarg.h>

#define MSG_LENGTH_STUB   (1024)
#define SET_MOUDLE_ID_MAP_NAME(x) { #x, x}

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LINUX
#define LINUX 0
#endif

#ifndef OS_TYPE
#define OS_TYPE 0
#endif

#if (OS_TYPE == LINUX)
#define PATH_SLOG "/usr/slog/slog"
#else
#define PATH_SLOG "C:\\Program Files\\Huawei\\HiAI Foundation\\log"
#endif

typedef struct _dcode_stub {
  const char *c_name;
  int c_val;
} DCODE_STUB;

static DCODE_STUB module_id_name_stub[] =
    {
        SET_MOUDLE_ID_MAP_NAME(SLOG),
        SET_MOUDLE_ID_MAP_NAME(IDEDD),
        SET_MOUDLE_ID_MAP_NAME(IDEDH),
        SET_MOUDLE_ID_MAP_NAME(HCCL),
        SET_MOUDLE_ID_MAP_NAME(FMK),
        SET_MOUDLE_ID_MAP_NAME(HIAIENGINE),
        SET_MOUDLE_ID_MAP_NAME(DVPP),
        SET_MOUDLE_ID_MAP_NAME(RUNTIME),
        SET_MOUDLE_ID_MAP_NAME(CCE),
#if (OS_TYPE == LINUX)
        SET_MOUDLE_ID_MAP_NAME(HDC),
#else
        SET_MOUDLE_ID_MAP_NAME(HDCL),
#endif
        SET_MOUDLE_ID_MAP_NAME(DRV),
        SET_MOUDLE_ID_MAP_NAME(MDCFUSION),
        SET_MOUDLE_ID_MAP_NAME(MDCLOCATION),
        SET_MOUDLE_ID_MAP_NAME(MDCPERCEPTION),
        SET_MOUDLE_ID_MAP_NAME(MDCFSM),
        SET_MOUDLE_ID_MAP_NAME(MDCCOMMON),
        SET_MOUDLE_ID_MAP_NAME(MDCMONITOR),
        SET_MOUDLE_ID_MAP_NAME(MDCBSWP),
        SET_MOUDLE_ID_MAP_NAME(MDCDEFAULT),
        SET_MOUDLE_ID_MAP_NAME(MDCSC),
        SET_MOUDLE_ID_MAP_NAME(MDCPNC),
        SET_MOUDLE_ID_MAP_NAME(MLL),
        SET_MOUDLE_ID_MAP_NAME(DEVMM),
        SET_MOUDLE_ID_MAP_NAME(KERNEL),
        SET_MOUDLE_ID_MAP_NAME(LIBMEDIA),
        SET_MOUDLE_ID_MAP_NAME(CCECPU),
        SET_MOUDLE_ID_MAP_NAME(ASCENDDK),
        SET_MOUDLE_ID_MAP_NAME(ROS),
        SET_MOUDLE_ID_MAP_NAME(HCCP),
        SET_MOUDLE_ID_MAP_NAME(ROCE),
        SET_MOUDLE_ID_MAP_NAME(TEFUSION),
        SET_MOUDLE_ID_MAP_NAME(PROFILING),
        SET_MOUDLE_ID_MAP_NAME(DP),
        SET_MOUDLE_ID_MAP_NAME(APP),
        SET_MOUDLE_ID_MAP_NAME(TS),
        SET_MOUDLE_ID_MAP_NAME(TSDUMP),
        SET_MOUDLE_ID_MAP_NAME(AICPU),
        SET_MOUDLE_ID_MAP_NAME(TDT),
        SET_MOUDLE_ID_MAP_NAME(MD),
        SET_MOUDLE_ID_MAP_NAME(FE),
        SET_MOUDLE_ID_MAP_NAME(MB),
        SET_MOUDLE_ID_MAP_NAME(ME),
        SET_MOUDLE_ID_MAP_NAME(IMU),
        SET_MOUDLE_ID_MAP_NAME(IMP),
        SET_MOUDLE_ID_MAP_NAME(GE),
        SET_MOUDLE_ID_MAP_NAME(MDCFUSA),
        SET_MOUDLE_ID_MAP_NAME(CAMERA),
        SET_MOUDLE_ID_MAP_NAME(ASCENDCL),
        SET_MOUDLE_ID_MAP_NAME(TEEOS),
        SET_MOUDLE_ID_MAP_NAME(ISP),
        SET_MOUDLE_ID_MAP_NAME(SIS),
        SET_MOUDLE_ID_MAP_NAME(HSM),
        SET_MOUDLE_ID_MAP_NAME(DSS),
        SET_MOUDLE_ID_MAP_NAME(PROCMGR),
        SET_MOUDLE_ID_MAP_NAME(BBOX),
        SET_MOUDLE_ID_MAP_NAME(AIVECTOR),
        SET_MOUDLE_ID_MAP_NAME(TBE),
        SET_MOUDLE_ID_MAP_NAME(FV),
        SET_MOUDLE_ID_MAP_NAME(MDCMAP),
        SET_MOUDLE_ID_MAP_NAME(TUNE),
        {NULL, -1}
    };
//#if (OS_TYPE == LINUX)
//extern void dlog_init(void);
//void DlogErrorInner(int module_id, const char *fmt, ...);
//void DlogWarnInner(int module_id, const char *fmt, ...);
//void DlogInfoInner(int module_id, const char *fmt, ...);
//void DlogDebugInner(int module_id, const char *fmt, ...);
//void DlogEventInner(int module_id, const char *fmt, ...);
//#endif
}

int CheckLogLevel(int moduleId, int level)
{
    return level < 2;
}

void DlogErrorInner(int module_id, const char *fmt, ...)
{
    if(module_id < 0 || module_id >= INVLID_MOUDLE_ID){
        return;
    }

    if (!CheckLogLevel(module_id, 0)) {
        return;
    }

    int len;
    char msg[MSG_LENGTH_STUB] = {0};
    snprintf(msg,MSG_LENGTH_STUB,"[%s] [ERROR] ",module_id_name_stub[module_id].c_name);
    va_list ap;

    va_start(ap,fmt);
    len = strlen(msg);
    vsnprintf(msg + len, MSG_LENGTH_STUB- len, fmt, ap);
    va_end(ap);

    printf("%s",msg);
    return;
}

void DlogWarnInner(int module_id, const char *fmt, ...)
{
    if(module_id < 0 || module_id >= INVLID_MOUDLE_ID){
        return;
    }

    if (!CheckLogLevel(module_id, 1)) {
        return;
    }

    int len;
    char msg[MSG_LENGTH_STUB] = {0};
    snprintf(msg,MSG_LENGTH_STUB,"[%s] [WARNING] ",module_id_name_stub[module_id].c_name);
    va_list ap;

    va_start(ap,fmt);
    len = strlen(msg);
    vsnprintf(msg + len, MSG_LENGTH_STUB- len, fmt, ap);
    va_end(ap);

    printf("%s",msg);
    return;
}

void DlogInfoInner(int module_id, const char *fmt, ...)
{
    if(module_id < 0 || module_id >= INVLID_MOUDLE_ID){
        return;
    }

    if (!CheckLogLevel(module_id, 2)) {
        return;
    }

    int len;
    char msg[MSG_LENGTH_STUB] = {0};
    snprintf(msg,MSG_LENGTH_STUB,"[%s] [INFO] ",module_id_name_stub[module_id].c_name);
    va_list ap;

    va_start(ap,fmt);
    len = strlen(msg);
    vsnprintf(msg + len, MSG_LENGTH_STUB- len, fmt, ap);
    va_end(ap);

    printf("%s",msg);
    return;
}

void DlogDebugInner(int module_id, const char *fmt, ...)
{
    if(module_id < 0 || module_id >= INVLID_MOUDLE_ID){
        return;
    }

    if (!CheckLogLevel(module_id, 3)) {
        return;
    }

    int len;
    char msg[MSG_LENGTH_STUB] = {0};
    snprintf(msg,MSG_LENGTH_STUB,"[%s] [DEBUG] ",module_id_name_stub[module_id].c_name);
    va_list ap;

    va_start(ap,fmt);
    len = strlen(msg);
    vsnprintf(msg + len, MSG_LENGTH_STUB- len, fmt, ap);
    va_end(ap);

    printf("%s",msg);
    return;
}

void DlogEventInner(int module_id, const char *fmt, ...)
{
    if(module_id < 0 || module_id >= INVLID_MOUDLE_ID){
        return;
    }

    if (!CheckLogLevel(module_id, 4)) {
        return;
    }

    int len;
    char msg[MSG_LENGTH_STUB] = {0};
    snprintf(msg,MSG_LENGTH_STUB,"[%s] [EVENT] ",module_id_name_stub[module_id].c_name);
    va_list ap;

    va_start(ap,fmt);
    len = strlen(msg);
    vsnprintf(msg + len, MSG_LENGTH_STUB- len, fmt, ap);
    va_end(ap);

    printf("%s",msg);
    return;
}

