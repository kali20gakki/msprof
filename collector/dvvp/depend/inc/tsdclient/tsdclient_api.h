/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: tsd client api header
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-07-22
 */
#ifndef TSDCLIENT_API_H
#define TSDCLIENT_API_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

constexpr uint32_t TSD_OK = 0U;
constexpr uint32_t TSD_HDC_CLIENT_CLOSED_EXTERNAL = 105U;

typedef enum {
    TSD_CAPABILITY_PIDQOS         = 0,
    TSD_CAPABILITY_LEVEL          = 1,
    TSD_CAPABILITY_OM_INNER_DEC   = 2,
    TSD_CAPABILITY_BUILTIN_UDF    = 3,
    TSD_CAPABILITY_DRIVER_VERSION = 4,
    TSD_CAPABILITY_ADPROF         = 5,
    TSD_CAPABILITY_MUTIPLE_HCCP   = 6,
    TSD_CAPABILITY_BUT            = 0xFF
} TsdCapabilityType;

typedef enum {
    TSD_SUB_PROC_HCCP           = 0,           // hccp process
    TSD_SUB_PROC_COMPUTE        = 1,           // aicpu_schedule process
    TSD_SUB_PROC_CUSTOM_COMPUTE = 2,           // aicpu_cust_schedule process
    TSD_SUB_PROC_QUEUE_SCHEDULE = 3,           // queue_schedule process
    TSD_SUB_PROC_UDF            = 4,           // udf process
    TSD_SUB_PROC_NPU            = 5,           // npu process
    TSD_SUB_PROC_PROXY          = 6,           // proxy process
    TSD_SUB_PROC_BUILTIN_UDF    = 7,           // build in udf
    TSD_SUB_PROC_ADPROF         = 8,           // adprof proces
    TSD_SUB_PROC_MAX            = 0xFF
} SubProcType;

struct ProcEnvParam {
    const char   *envName;
    uint64_t     nameLen;
    const char   *envValue;
    uint64_t     valueLen;
};

struct ProcExtParam {
    const char  *paramInfo;
    uint64_t    paramLen;
};

struct ProcOpenArgs {
    SubProcType  procType;
    ProcEnvParam *envParaList;
    uint64_t     envCnt;
    const char   *filePath;
    uint64_t     pathLen;
    ProcExtParam *extParamList;
    uint64_t     extParamCnt;
    pid_t        *subPid;
};

typedef enum {
    SUB_PROCESS_STATUS_NORMAL = 0,
    SUB_PROCESS_STATUS_EXITED = 1,
    SUB_PROCESS_STATUS_STOPED = 2,
    SUB_PROCESS_STATUS_UNKNOW = 3,
    SUB_PROCESS_STATUS_MAX    = 0xFF
} SubProcessStatus;

struct ProcStatusParam {
    pid_t pid;
    SubProcType procType;
    SubProcessStatus curStat;
};

#ifdef __cplusplus
}
#endif // __cplusplus

#endif