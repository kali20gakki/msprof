/**
 * @file adx_log.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_LOG_H
#define ADX_LOG_H
#include "msprof_dlog.h"
#define IDE_CTRL_VALUE_FAILED(err, action, logText, ...) do {          \
    if (!(err)) {                                                      \
        MSPROF_LOGE(logText, ##__VA_ARGS__);                              \
        action;                                                        \
    }                                                                  \
} while (0)

#define IDE_CTRL_VALUE_WARN(err, action, logText, ...) do {            \
    if (!(err)) {                                                      \
        MSPROF_LOGW(logText, ##__VA_ARGS__);                              \
        action;                                                        \
    }                                                                  \
} while (0)


#define IDE_CTRL_MUTEX_LOCK(mtx, action) do {                          \
    if (mmMutexLock(mtx) != EN_OK) {                                   \
        MSPROF_LOGE("mutex lock error");                                  \
        action;                                                        \
    }                                                                  \
} while (0)

#define IDE_CTRL_MUTEX_UNLOCK(mtx, action) do {                        \
    if (mmMutexUnLock(mtx) != EN_OK) {                                 \
        MSPROF_LOGE("mutex lock error");                                  \
        action;                                                        \
    }                                                                  \
} while (0)

#endif
