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
#include "hdc_log.h"

#define IDE_CTRL_VALUE_FAILED(err, action, logText, ...) do {          \
    if (!(err)) {                                                      \
        IDE_LOGE(logText, ##__VA_ARGS__);                              \
        action;                                                        \
    }                                                                  \
} while (0)

#define IDE_CTRL_VALUE_WARN(err, action, logText, ...) do {            \
    if (!(err)) {                                                      \
        IDE_LOGW(logText, ##__VA_ARGS__);                              \
        action;                                                        \
    }                                                                  \
} while (0)

#ifndef UNUSED
#define UNUSED(x)   do {(void)(x);} while (0)
#endif
#endif
