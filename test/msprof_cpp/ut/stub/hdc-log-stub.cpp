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
#include "hdc-log-stub.h"
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

void DlogErrorInner(int moduleId, const char *format, ...) {
    va_list args;

    char buffer[4096] = {0};

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("[ERROR]%s\n", buffer);
    va_end(args);
}

void DlogInfoInner(int moduleId, const char *format, ...) {
    va_list args;

    char buffer[4096] = {0};

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("[INFO]%s\n", buffer);
    va_end(args);
}

void DlogWarnInner(int moduleId, const char *format, ...) {
    va_list args;

    char buffer[4096] = {0};

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("[WARN]%s\n", buffer);
    va_end(args);
}

void DlogEventInner(int moduleId, const char *format, ...) {
    va_list args;

    char buffer[4096] = {0};

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("[EVENT]%s\n", buffer);
    va_end(args);
}

void DlogDebugInner(int moduleId, const char *format, ...) {
    va_list args;

    char buffer[4096] = {0};

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("[DEBUG]%s\n", buffer);
    va_end(args);
}

void ide_log(int priority, const char *format, ...) {
    va_list args;

    char buffer[4096] = {0};

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("[IDE]%s\n", buffer);
    va_end(args);
}

int CheckLogLevel(int moduleId, int level)
{
    return 1;
}
