/**
 * @file ide_hdc_log.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_HDC_LOG_H
#define IDE_HDC_LOG_H
#include "mmpa_api.h"
#include "slog.h"
#include "../common/extra_config.h"
namespace Adx {
#if defined(IDE_DAEMON_DEVICE) || defined (IDE_DAEMON_DEVICE_SO) || defined (ADX_LIB)
const int ADX_MODULE_NAME = IDEDD;
#elif defined(IDE_DAEMON_HOST)
const int ADX_MODULE_NAME = IDEDH;
#else
const int ADX_MODULE_NAME = IDEDH;
#endif

#if defined (IDE_DAEMON_DEVICE) || defined (IDE_DAEMON_HOST) || defined (ADX_LIB) || \
    defined (IDE_DAEMON_DEVICE_SO) || defined (ADX_LIB_HOST)
#define IDE_LOGD(format, ...) do {                                                                 \
    dlog_debug(Adx::ADX_MODULE_NAME, "[tid:%ld]>>> " format "\n", mmGetTid(), ##__VA_ARGS__);      \
} while (0)
#define IDE_LOGI(format, ...) do {                                                                 \
    dlog_info(Adx::ADX_MODULE_NAME, "[tid:%ld]>>> " format "\n", mmGetTid(), ##__VA_ARGS__);       \
} while (0)

#define IDE_LOGW(format, ...) do {                                                                 \
    dlog_warn(Adx::ADX_MODULE_NAME, "[tid:%ld]>>> " format "\n", mmGetTid(), ##__VA_ARGS__);       \
} while (0)

#define IDE_LOGE(format, ...) do {                                                                 \
    dlog_error(Adx::ADX_MODULE_NAME, "[tid:%ld]>>> " format "\n", mmGetTid(), ##__VA_ARGS__);      \
} while (0)

#define IDE_EVENT(format, ...) do {                                                                \
    dlog_event(Adx::ADX_MODULE_NAME, "[tid:%ld]>>> " format "\n", mmGetTid(), ##__VA_ARGS__);      \
} while (0)
#else
#define IDE_LOGD(format, ...) do {             \
} while (0)

#define IDE_LOGI(format, ...) do {             \
} while (0)

#define IDE_LOGW(format, ...) do {             \
} while (0)

#define IDE_LOGE(format, ...) do {             \
    printf(format "\n", ##__VA_ARGS__);        \
} while (0)

#define IDE_EVENT(format, ...) do {            \
} while (0)

#endif

const int MAX_ERRSTR_LEN = 128;
}
#endif // IDE_HDC_LOG_H
