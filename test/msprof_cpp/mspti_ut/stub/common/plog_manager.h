/**
* @file plog_manager.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/
#ifndef MSPTI_COMMON_PLOG_MANAGER_H
#define MSPTI_COMMON_PLOG_MANAGER_H

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/syscall.h>

#define FILENAME (strrchr("/" __FILE__, '/') + 1)
#define MSPTI_LOGD(format, ...) do {                                           \
            printf("[PROFILING] [DEBUG] [%s:%d] >>> (tid:%ld) " format "\n",   \
            FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define MSPTI_LOGI(format, ...) do {                                           \
            printf("[PROFILING] [INFO] [%s:%d] >>> (tid:%ld) " format "\n",    \
            FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define MSPTI_LOGW(format, ...) do {                                           \
            printf("[PROFILING] [WARNING] [%s:%d] >>> (tid:%ld) " format "\n", \
            FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define MSPTI_LOGE(format, ...) do {                                           \
            printf("[PROFILING] [ERROR] [%s:%d] >>> (tid:%ld) " format "\n",   \
            FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define PRINT_LOGW(format, ...) do {                                           \
            printf("[PROFILING] [WARNING] [%s:%d] >>> (tid:%ld) " format "\n", \
            FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define PRINT_LOGE(format, ...) do {                                           \
            printf("[PROFILING] [ERROR] [%s:%d] >>> (tid:%ld) " format "\n",   \
            FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)
#endif
