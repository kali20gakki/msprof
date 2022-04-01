/**
 * @log_recv.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_RECV_H
#define LOG_RECV_H
#if (OS_TYPE_DEF == 0)
#include "ascend_hal.h"
#endif
#include "constant.h"
#include "dlog_error_code.h"
#include "log_recv_interface.h"
#include "log_queue.h"
#include "log_to_file.h"
#include "log_sys_package.h"

#define MAX_NODE_BUFF           (1 * 1024 * 1024)
#define WRITE_INTERVAL          1
#define TWO_SECOND              2000
#define TEN_SECOND              10000
#define ONE_HUNDRED_MILLISECOND 100
#define TWO_HUNDRED_MILLISECOND 200

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ParentSource parentSource;
    ChildSource **childSources;
    ToolUserBlock *stMmThreadArg;
} GetIDArg;

typedef struct {
    char *buff;
    unsigned int buffLen;
    short slogFlag; // flag to divide os and device, 1 for device-os and 0 for device
    short smpFlag;
} LogBuff;

typedef struct {
    toolMutex *lock;
    LogQueue *devLogQueue;
    ParentSource parentSource;
    ChildSource *childSource;
    StLogFileList *pstLogFileList;
    LogBuff *logBuff;
    unsigned int nodeCnt;
    ToolUserBlock *mmRecvThreadArg;
    ToolUserBlock *mmWriteThreadArg;
} DeviceRes;


/**
 * @brief LogRecvInit: create thread HostRTGetDeviceIDThread to monitor device and deviceThread
 * @param [in]parentSource: parent source
 * @param [in]childSources: pointer to array of child sources
 * @param [in]childSourceNum: number of child source
 * @return: LogRt, SUCCESS/OTHER
 */
LogRt LogRecvInit(const ParentSource parentSource, ChildSource **childSources, unsigned int childSourceNum);
unsigned char IsInitError(void);
unsigned char IsAllThreadExit(void);
unsigned int IsDeviceThreadAlive(const int deviceId);
#if (OS_TYPE_DEF == LINUX)
/* adx dump server callback */
extern int32_t AdxDevStartupNotifyCb(uint32_t num, uint32_t* devIds);
extern int32_t AdxDevPwStateNotifyCb(devdrv_state_info_t *stat);
#endif

#ifdef __cplusplus
}
#endif
#endif
