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
#ifndef MSPROF_DLOG_H
#define MSPROF_DLOG_H

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/syscall.h>
#include "slog_plugin.h"

#if (defined(linux) || defined(__linux__))
#include <syslog.h>
#endif

enum {
    SLOG,          /**< Slog */
    IDEDD,         /**< IDE daemon device */
    IDEDH,         /**< IDE daemon host */
    HCCL,          /**< HCCL */
    FMK,           /**< Adapter */
    HIAIENGINE,    /**< Matrix */
    DVPP,          /**< DVPP */
    RUNTIME,       /**< Runtime */
    CCE,           /**< CCE */
#if (defined(linux) || defined(__linux__))
    HDC,         /**< HDC */
#else
    HDCL,
#endif // OS_TYPE
    DRV,           /**< Driver */
    MDCFUSION,     /**< Mdc fusion */
    MDCLOCATION,   /**< Mdc location */
    MDCPERCEPTION, /**< Mdc perception */
    MDCFSM,
    MDCCOMMON,
    MDCMONITOR,
    MDCBSWP,    /**< MDC base software platform */
    MDCDEFAULT, /**< MDC undefine */
    MDCSC,      /**< MDC spatial cognition */
    MDCPNC,
    MLL,      /**< abandon */
    DEVMM,    /**< Dlog memory managent */
    KERNEL,   /**< Kernel */
    LIBMEDIA, /**< Libmedia */
    CCECPU,   /**< aicpu shedule */
    ASCENDDK, /**< AscendDK */
    ROS,      /**< ROS */
    HCCP,
    ROCE,
    TEFUSION,
    PROFILING, /**< Profiling */
    DP,        /**< Data Preprocess */
    APP,       /**< User Application */
    TS,        /**< TS module */
    TSDUMP,    /**< TSDUMP module */
    AICPU,     /**< AICPU module */
    LP,        /**< LP module */
    TDT,       /**< tsdaemon or aicpu shedule */
    FE,
    MD,
    MB,
    ME,
    IMU,
    IMP,
    GE, /**< Fmk */
    MDCFUSA,
    CAMERA,
    ASCENDCL,
    TEEOS,
    ISP,
    SIS,
    HSM,
    DSS,
    PROCMGR,     // Process Manager, Base Platform
    BBOX,
    AIVECTOR,
    TBE,
    FV,
    MDCMAP,
    TUNE,
    HSS, /**< helper */
    FFTS,
    INVLID_MOUDLE_ID
};
#define MSPROF_MODULE_NAME PROFILING
#define FILENAME (strrchr("/" __FILE__, '/') + 1)
using SlogPlugin = Collector::Dvvp::Plugin::SlogPlugin;

#define MSPROF_LOGD(format, ...) do {                                                                           \
    if (SlogPlugin::instance()->MsprofCheckLogLevelForC(MSPROF_MODULE_NAME,                                     \
        Collector::Dvvp::Plugin::DLOG_DEBUG) == 1) {                                                            \
        SlogPlugin::instance()->MsprofDlogInnerForC(MSPROF_MODULE_NAME, Collector::Dvvp::Plugin::DLOG_DEBUG,    \
        "[%s:%d]" " >>> (tid:%ld) " format "\n", FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);       \
    }                                                                                                           \
} while (0)

#define MSPROF_LOGI(format, ...) do {                                                                           \
    if (SlogPlugin::instance()->MsprofCheckLogLevelForC(MSPROF_MODULE_NAME,                                     \
        Collector::Dvvp::Plugin::DLOG_INFO) == 1) {                                                             \
        SlogPlugin::instance()->MsprofDlogInnerForC(MSPROF_MODULE_NAME, Collector::Dvvp::Plugin::DLOG_INFO,     \
        "[%s:%d]" " >>> (tid:%ld) " format "\n", FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);       \
    }                                                                                                           \
} while (0)

#define MSPROF_LOGW(format, ...) do {                                                                           \
    if (SlogPlugin::instance()->MsprofCheckLogLevelForC(MSPROF_MODULE_NAME,                                     \
        Collector::Dvvp::Plugin::DLOG_WARN) == 1) {                                                             \
        SlogPlugin::instance()->MsprofDlogInnerForC(MSPROF_MODULE_NAME, Collector::Dvvp::Plugin::DLOG_WARN,     \
        "[%s:%d]" " >>> (tid:%ld) " format "\n", FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);       \
    }                                                                                                           \
} while (0)

#define MSPROF_LOGE(format, ...) do {                                                                           \
    if (SlogPlugin::instance()->MsprofCheckLogLevelForC(MSPROF_MODULE_NAME,                                     \
        Collector::Dvvp::Plugin::DLOG_ERROR) == 1) {                                                            \
        SlogPlugin::instance()->MsprofDlogInnerForC(MSPROF_MODULE_NAME, Collector::Dvvp::Plugin::DLOG_ERROR,    \
        "[%s:%d]" " >>> (tid:%ld) " format "\n", FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);       \
    }                                                                                                           \
} while (0)

#define MSPROF_EVENT(format, ...) do {                                                                          \
    if (SlogPlugin::instance()->MsprofCheckLogLevelForC(MSPROF_MODULE_NAME,                                     \
        Collector::Dvvp::Plugin::DLOG_EVENT) == 1) {                                                            \
        SlogPlugin::instance()->MsprofDlogInnerForC(MSPROF_MODULE_NAME, Collector::Dvvp::Plugin::DLOG_EVENT,    \
        "[%s:%d]" " >>> (tid:%ld) " format "\n", FILENAME, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);       \
    }                                                                                                           \
} while (0)
#endif  // MSPROF_LOG_H
