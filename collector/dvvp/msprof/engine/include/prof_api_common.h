/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle acl request
 * Author: xiepeng
 * Create: 2020-07-30
 */
#ifndef MSPROF_ENGINE_ACL_API_COMMON_H
#define MSPROF_ENGINE_ACL_API_COMMON_H

#include <cstdint>

#include "prof_acl_api.h"
#include "prof_callback.h"
#include "utils.h"

static const uint64_t PROF_SWITCH_SUPPORT = PROF_ACL_API | PROF_TASK_TIME | PROF_AICORE_METRICS | PROF_AICPU_TRACE |
    PROF_L2CACHE | PROF_HCCL_TRACE | PROF_KEYPOINT_TRACE | PROF_MSPROFTX | PROF_RUNTIME_API;

/**
 * @name  ProfAicoreMetrics
 * @brief aicore metrics enum
 */
enum ProfAicoreMetrics {
    PROF_AICORE_ARITHMETIC_UTILIZATION = 0,
    PROF_AICORE_PIPE_UTILIZATION = 1,
    PROF_AICORE_MEMORY_BANDWIDTH = 2,
    PROF_AICORE_L0B_AND_WIDTH = 3,
    PROF_AICORE_RESOURCE_CONFLICT_RATIO = 4,
    PROF_AICORE_MEMORY_UB = 5,
    PROF_AICORE_L2_CACHE = 6,
    PROF_AICORE_PIPE_EXECUTE_UTILIZATION = 7,
    PROF_AICORE_METRICS_COUNT,
    PROF_AICORE_NONE = 0xff,
};

/**
 * @name  ProfConfig
 * @brief struct of aclprofStart/aclprofStop
 */
struct ProfConfig {
    uint32_t devNums;                       // length of device id list
    uint32_t devIdList[analysis::dvvp::common::utils::MSVP_MAX_DEV_NUM + 1];   // physical device id list
    ProfAicoreMetrics aicoreMetrics;        // aicore metric
    uint64_t dataTypeConfig;                // data type to start profiling
};
using PROF_CONF_CONST_PTR = const ProfConfig *;

/**
 * @name  ProfSubscribeConfig
 * @brief config of subscribe api
 */
struct ProfSubscribeConfig {
    bool timeInfo;                          // subscribe op time
    ProfAicoreMetrics aicoreMetrics;        // subscribe ai core metrics
    void* fd;                               // pipe fd
};
using PROF_SUB_CONF_CONST_PTR = const ProfSubscribeConfig *;

/**
 * @name  aclprofSubscribeConfig
 * @brief config of subscribe common api
 */
struct aclprofSubscribeConfig {
    struct ProfSubscribeConfig config;
};
using ACL_PROF_SUB_CONFIG_PTR = aclprofSubscribeConfig *;
using ACL_PROF_SUB_CINFIG_CONST_PTR = const aclprofSubscribeConfig *;

#define RETURN_IF_NOT_SUCCESS(ret)  \
    do {                            \
        if ((ret) != ACL_SUCCESS) { \
            return ret;             \
        }                           \
    } while (0)
#endif // MSPROF_ENGINE_ACL_API_COMMON_H
