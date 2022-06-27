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
#include "slog_plugin.h"
#include <unistd.h>
#include <sys/syscall.h>

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
#if (OS_TYPE == LINUX)
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

using SlogPlugin = Collector::Dvvp::Plugin::SlogPlugin;

#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#define MSPROF_LOGD(format, ...) do {                                           \
            printf("[PROFILING] [DEBUG] [%s:%u] >>> (tid:%d) " format "\n",   \
            __FILENAME__, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define MSPROF_LOGI(format, ...) do {                                           \
            printf("[PROFILING] [INFO] [%s:%u] >>> (tid:%d) " format "\n",    \
            __FILENAME__, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define MSPROF_LOGW(format, ...) do {                                           \
            printf("[PROFILING] [WARNING] [%s:%u] >>> (tid:%d) " format "\n", \
            __FILENAME__, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define MSPROF_LOGE(format, ...) do {                                           \
            printf("[PROFILING] [ERROR] [%s:%u] >>> (tid:%d) " format "\n",   \
            __FILENAME__, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)

#define MSPROF_EVENT(format, ...) do {                                          \
            printf("[PROFILING] [EVENT] [%s:%u] >>> (tid:%d) " format "\n",   \
            __FILENAME__, __LINE__, syscall(SYS_gettid), ##__VA_ARGS__);            \
    } while (0)
#endif  // MSPROF_LOG_H

