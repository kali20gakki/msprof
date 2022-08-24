/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: wrapper header
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#ifndef ACL_BASE_H
#define ACL_BASE_H
#include "prof_api.h"
typedef enum {
    // common 0-19
    ACL_PROF_STORAGE_LIMIT = 0,
    ACL_PROF_AIV_METRICS = 1,
    // device-sys 20-39
    ACL_PROF_SYS_USAGE_FREQ = 20,
    ACL_PROF_SYS_PID_USAGE_FREQ = 21,
    ACL_PROF_SYS_CPU_FREQ = 22,
    ACL_PROF_SYS_HARDWARE_MEM_FREQ = 23,
    ACL_PROF_LLC_MODE = 24,
    ACL_PROF_SYS_IO_FREQ = 25,
    ACL_PROF_SYS_INTERCONNECTION_FREQ = 26,
    ACL_PROF_DVPP_FREQ = 27,
    // host-sys 40-
    ACL_PROF_HOST_SYS = 40,
    ACL_PROF_ARGS_MAX
} aclprofConfigType;

aclError aclprofSetConfig(aclprofConfigType configType, const char *val, uint32_t valLen);
#endif