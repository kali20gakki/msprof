#include "toolchain/slog.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void dav_log(int module_id, const char *fmt, ...)
{
}

void DlogErrorInner(int module_id, const char *fmt, ...)
{
    dav_log(module_id, fmt);
}

void DlogWarnInner(int module_id, const char *fmt, ...)
{
    dav_log(module_id, fmt);
}

void DlogInfoInner(int module_id, const char *fmt, ...)
{
    dav_log(module_id, fmt);
}

void DlogDebugInner(int module_id, const char *fmt, ...)
{
    dav_log(module_id, fmt);
}

void DlogEventInner(int module_id, const char *fmt, ...)
{
    dav_log(module_id, fmt);
}

void DlogInner(int moduleId, int level, const char *fmt, ...)
{
    dav_log(moduleId, fmt);
}

void DlogWithKVInner(int moduleId, int level, KeyValue* pstKVArray, int kvNum, const char *fmt, ...)
{
    dav_log(moduleId, fmt);
}

int dlog_getlevel(int moduleId, int *enable_event)
{
    return 0;
}

int CheckLogLevel(int moduleId, int logLevel)
{
    return 0;
}
