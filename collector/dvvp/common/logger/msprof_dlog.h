/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: msprof dlog
 * Author: dml
 * Create: 2018-06-13
 */
#ifndef MSPROF_DLOG_H
#define MSPROF_DLOG_H

#include <cstdio>
#include <cstring>
#include "slog.h"
#include <unistd.h>
#include <sys/syscall.h>

#if (defined(linux) || defined(__linux__))
#include <syslog.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MSPROF_MODULE_NAME PROFILING

#define MSPROF_LOGD(format, ...) do {                                                                      \
    dlog_debug(MSPROF_MODULE_NAME, " >>> (tid:%ld) " format "\n", syscall(SYS_gettid), ##__VA_ARGS__);    \
} while (0)

#define MSPROF_LOGI(format, ...) do {                                                                      \
    dlog_info(MSPROF_MODULE_NAME, " >>> (tid:%ld) " format "\n", syscall(SYS_gettid), ##__VA_ARGS__);     \
} while (0)

#define MSPROF_LOGW(format, ...) do {                                                                      \
    dlog_warn(MSPROF_MODULE_NAME, " >>> (tid:%ld) " format "\n", syscall(SYS_gettid), ##__VA_ARGS__);     \
} while (0)

#define MSPROF_LOGE(format, ...) do {                                                                      \
    dlog_error(MSPROF_MODULE_NAME, " >>> (tid:%ld) " format "\n", syscall(SYS_gettid), ##__VA_ARGS__);    \
} while (0)

#define MSPROF_EVENT(format, ...) do {                                                                     \
    dlog_event(MSPROF_MODULE_NAME, " >>> (tid:%ld) " format "\n", syscall(SYS_gettid), ##__VA_ARGS__);    \
} while (0)

#ifdef __cplusplus
}
#endif

#endif  // MSPROF_LOG_H
