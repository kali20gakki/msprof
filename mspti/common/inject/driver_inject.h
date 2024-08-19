/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : driver_inject.h
 * Description        : Injection of libascend_hal.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#ifndef MSPTI_COMMON_INJECT_DRIVER_INJECT_H
#define MSPTI_COMMON_INJECT_DRIVER_INJECT_H

#include "common/inject/inject_base.h"
#include "external/mspti_result.h"

typedef enum {
    DRV_INFO_TYPE_ENV = 0,
    DRV_INFO_TYPE_VERSION,
    DRV_INFO_TYPE_MASTERID,
    DRV_INFO_TYPE_CORE_NUM,
    DRV_INFO_TYPE_FREQUE,
    DRV_INFO_TYPE_OS_SCHED,
    DRV_INFO_TYPE_IN_USED,
    DRV_INFO_TYPE_ERROR_MAP,
    DRV_INFO_TYPE_OCCUPY,
    DRV_INFO_TYPE_ID,
    DRV_INFO_TYPE_IP,
    DRV_INFO_TYPE_ENDIAN,
    DRV_INFO_TYPE_P2P_CAPABILITY,
    DRV_INFO_TYPE_SYS_COUNT,
    DRV_INFO_TYPE_MONOTONIC_RAW,
    DRV_INFO_TYPE_CORE_NUM_LEVEL,
    DRV_INFO_TYPE_FREQUE_LEVEL,
    DRV_INFO_TYPE_FFTS_TYPE,
    DRV_INFO_TYPE_PHY_CHIP_ID,
    DRV_INFO_TYPE_PHY_DIE_ID,
    DRV_INFO_TYPE_PF_CORE_NUM,
    DRV_INFO_TYPE_PF_OCCUPY,
    DRV_INFO_TYPE_WORK_MODE,
    DRV_INFO_TYPE_UTILIZATION,
    DRV_INFO_TYPE_HOST_OSC_FREQUE,
    DRV_INFO_TYPE_DEV_OSC_FREQUE,
    DRV_INFO_TYPE_SDID,
    DRV_INFO_TYPE_SERVER_ID,
    DRV_INFO_TYPE_SCALE_TYPE,
    DRV_INFO_TYPE_SUPER_POD_ID,
    DRV_INFO_TYPE_ADDR_MODE,
    DRV_INFO_TYPE_RUN_MACH,
    DRV_INFO_TYPE_CURRENT_FREQ,
    DRV_INFO_TYPE_CONFIG,
} DrvInfoType;

typedef enum {
    DRV_MODULE_TYPE_SYSTEM = 0,
    DRV_MODULE_TYPE_AICPU,
    DRV_MODULE_TYPE_CCPU,        /**< ccpu_info*/
    DRV_MODULE_TYPE_DCPU,        /**< dcpu info*/
    DRV_MODULE_TYPE_AICORE,      /**< AI CORE info*/
    DRV_MODULE_TYPE_TSCPU,       /**< tscpu info*/
    DRV_MODULE_TYPE_PCIE,        /**< PCIE info*/
    DRV_MODULE_TYPE_VECTOR_CORE, /**< VECTOR CORE info*/
    DRV_MODULE_TYPE_HOST_AICPU,  /* Host Aicpu info */
    DRV_MODULE_TYPE_QOS,         /**<qos info> */
    DRV_MODULE_TYPE_COMPUTING = 0x8000, /* computing power info */
} DrvModuleType;

#if defined(__cplusplus)
extern "C" {
#endif

// Inner
int ProfDrvGetChannels(unsigned int deviceId, ChannelListT* channelList);
DrvError DrvGetDevIDs(uint32_t* devices, uint32_t len);
DrvError DrvGetDevNum(uint32_t* count);
int ProfDrvStart(unsigned int deviceId, unsigned int channelId, struct ProfStartPara* startPara);
int ProfStop(unsigned int deviceId, unsigned int channelId);
int ProfChannelRead(unsigned int deviceId, unsigned int channelId, char *outBuf,
    unsigned int bufSize);
int ProfChannelPoll(struct ProfPollInfo* outBuf, int num, int timeout);
DrvError HalGetDeviceInfo(uint32_t deviceId, int32_t moduleType, int32_t infoType, int64_t* value);
DrvError halGetAPIVersion(int32_t* apiVersion);
#if defined(__cplusplus)
}
#endif

#endif
