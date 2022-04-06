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
    printf("%s\n", buffer);
    va_end(args);
}

void ide_log(int priority, const char *format, ...) {
    va_list args;

    char buffer[4096] = {0};

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s\n", buffer);
    va_end(args);
}

int CheckLogLevel(int moduleId, int level)
{
    return 1;
}
