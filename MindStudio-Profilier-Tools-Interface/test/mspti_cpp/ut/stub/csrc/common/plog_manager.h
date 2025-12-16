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
 * -------------------------------------------------------------------------
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
