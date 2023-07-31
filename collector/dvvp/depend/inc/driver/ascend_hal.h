/**
 * @file ascend_hal.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description:
 * Author: huawei
 * Create: 2020-01-21
 * @brief driver interface.
 * @version 1.0
 *
 */


#ifndef __ASCEND_HAL_H__
#define __ASCEND_HAL_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>

#ifndef __linux
#include "mmpa_api.h"
#define ASCEND_HAL_WEAK
#else
#define ASCEND_HAL_WEAK __attribute__((weak))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**< profile drv user */
#define PROF_DRIVER_NAME "prof_drv"
#define PROF_OK (0)
#define PROF_ERROR (-1)
#define PROF_TIMEOUT (-2)
#define PROF_STARTED_ALREADY (-3)
#define PROF_STOPPED_ALREADY (-4)
#define PROF_ERESTARTSYS (-5)
#define CHANNEL_NUM 160

#define CHANNEL_HBM (1)
#define CHANNEL_BUS (2)
#define CHANNEL_PCIE (3)
#define CHANNEL_NIC (4)
#define CHANNEL_DMA (5)
#define CHANNEL_DVPP (6)
#define CHANNEL_DDR (7)
#define CHANNEL_LLC (8)
#define CHANNEL_HCCS (9)
#define CHANNEL_TSCPU (10)

#define CHANNEL_INSTR_GROUP0_AIC (11)
#define CHANNEL_INSTR_GROUP0_AIV0 (12)
#define CHANNEL_INSTR_GROUP0_AIV1 (13)
#define CHANNEL_INSTR_GROUP1_AIC (14)
#define CHANNEL_INSTR_GROUP1_AIV0 (15)
#define CHANNEL_INSTR_GROUP1_AIV1 (16)
#define CHANNEL_INSTR_GROUP2_AIC (17)
#define CHANNEL_INSTR_GROUP2_AIV0 (18)
#define CHANNEL_INSTR_GROUP2_AIV1 (19)
#define CHANNEL_INSTR_GROUP3_AIC (20)
#define CHANNEL_INSTR_GROUP3_AIV0 (21)
#define CHANNEL_INSTR_GROUP3_AIV1 (22)
#define CHANNEL_INSTR_GROUP4_AIC (23)
#define CHANNEL_INSTR_GROUP4_AIV0 (24)
#define CHANNEL_INSTR_GROUP4_AIV1 (25)
#define CHANNEL_INSTR_GROUP5_AIC (26)
#define CHANNEL_INSTR_GROUP5_AIV0 (27)
#define CHANNEL_INSTR_GROUP5_AIV1 (28)
#define CHANNEL_INSTR_GROUP6_AIC (29)
#define CHANNEL_INSTR_GROUP6_AIV0 (30)
#define CHANNEL_INSTR_GROUP6_AIV1 (31)
#define CHANNEL_INSTR_GROUP7_AIC (32)
#define CHANNEL_INSTR_GROUP7_AIV0 (33)
#define CHANNEL_INSTR_GROUP7_AIV1 (34)
#define CHANNEL_INSTR_GROUP8_AIC (35)
#define CHANNEL_INSTR_GROUP8_AIV0 (36)
#define CHANNEL_INSTR_GROUP8_AIV1 (37)
#define CHANNEL_INSTR_GROUP9_AIC (38)
#define CHANNEL_INSTR_GROUP9_AIV0 (39)
#define CHANNEL_INSTR_GROUP9_AIV1 (40)
#define CHANNEL_INSTR_GROUP10_AIC (41)
#define CHANNEL_INSTR_GROUP10_AIV0 (42)

#define CHANNEL_AICORE (43)
#define CHANNEL_HWTS_CNT CHANNEL_AICORE
#define CHANNEL_TSFW (44)      // add for ts0 as tsfw channel
#define CHANNEL_HWTS_LOG (45)  // add for ts0 as hwts channel
#define CHANNEL_KEY_POINT (46)
#define CHANNEL_TSFW_L2 (47)   /* add for ascend910 and ascend610 */
#define CHANNEL_HWTS_LOG1 (48) // add for ts1 as hwts channel
#define CHANNEL_TSFW1 (49)     // add for ts1 as tsfw channel
#define CHANNEL_STARS_SOC_LOG_BUFFER (50)       /* add for ascend920 */
#define CHANNEL_STARS_BLOCK_LOG_BUFFER (51)     /* add for ascend920 */
#define CHANNEL_STARS_SOC_PROFILE_BUFFER (52)   /* add for ascend920 */
#define CHANNEL_FFTS_PROFILE_BUFFER_TASK (53)   /* add for ascend920 */
#define CHANNEL_FFTS_PROFILE_BUFFER_SAMPLE (54) /* add for ascend920 */

#define CHANNEL_INSTR_GROUP10_AIV1 (55)
#define CHANNEL_INSTR_GROUP11_AIC (56)
#define CHANNEL_INSTR_GROUP11_AIV0 (57)
#define CHANNEL_INSTR_GROUP11_AIV1 (58)
#define CHANNEL_INSTR_GROUP12_AIC (59)
#define CHANNEL_INSTR_GROUP12_AIV0 (60)
#define CHANNEL_INSTR_GROUP12_AIV1 (61)
#define CHANNEL_INSTR_GROUP13_AIC (62)
#define CHANNEL_INSTR_GROUP13_AIV0 (63)
#define CHANNEL_INSTR_GROUP13_AIV1 (64)
#define CHANNEL_INSTR_GROUP14_AIC (65)
#define CHANNEL_INSTR_GROUP14_AIV0 (66)
#define CHANNEL_INSTR_GROUP14_AIV1 (67)
#define CHANNEL_INSTR_GROUP15_AIC (68)
#define CHANNEL_INSTR_GROUP15_AIV0 (69)
#define CHANNEL_INSTR_GROUP15_AIV1 (70)
#define CHANNEL_INSTR_GROUP16_AIC (71)
#define CHANNEL_INSTR_GROUP16_AIV0 (72)
#define CHANNEL_INSTR_GROUP16_AIV1 (73)
#define CHANNEL_INSTR_GROUP17_AIC (74)
#define CHANNEL_INSTR_GROUP17_AIV0 (75)
#define CHANNEL_INSTR_GROUP17_AIV1 (76)
#define CHANNEL_INSTR_GROUP18_AIC (77)
#define CHANNEL_INSTR_GROUP18_AIV0 (78)
#define CHANNEL_INSTR_GROUP18_AIV1 (79)
#define CHANNEL_INSTR_GROUP19_AIC (80)
#define CHANNEL_INSTR_GROUP19_AIV0 (81)
#define CHANNEL_INSTR_GROUP19_AIV1 (82)
#define CHANNEL_INSTR_GROUP20_AIC (83)
#define CHANNEL_INSTR_GROUP20_AIV0 (84)

#define CHANNEL_AIV (85)

#define CHANNEL_INSTR_GROUP20_AIV1 (86)
#define CHANNEL_INSTR_GROUP21_AIC (87)
#define CHANNEL_INSTR_GROUP21_AIV0 (88)
#define CHANNEL_INSTR_GROUP21_AIV1 (89)
#define CHANNEL_INSTR_GROUP22_AIC (90)
#define CHANNEL_INSTR_GROUP22_AIV0 (91)
#define CHANNEL_INSTR_GROUP22_AIV1 (92)
#define CHANNEL_INSTR_GROUP23_AIC (93)
#define CHANNEL_INSTR_GROUP23_AIV0 (94)
#define CHANNEL_INSTR_GROUP23_AIV1 (95)
#define CHANNEL_INSTR_GROUP24_AIC (96)
#define CHANNEL_INSTR_GROUP24_AIV0 (97)
#define CHANNEL_INSTR_GROUP24_AIV1 (98)

#define CHANNEL_TSCPU_MAX (128)
#define CHANNEL_ROCE (129)
#define CHANNEL_NPU_APP_MEM (130) /* HBM and DDR used on app level */
#define CHANNEL_NPU_MEM (131)     /* HBM and DDR used on device level */
#define CHANNEL_LP (132)     /* low power */
#define CHANNEL_DVPP_VENC (135)  /* add for ascend610 */
#define CHANNEL_DVPP_JPEGE (136) /* add for ascend610 */
#define CHANNEL_DVPP_VDEC (137)  /* add for ascend610 */
#define CHANNEL_DVPP_JPEGD (138) /* add for ascend610 */
#define CHANNEL_DVPP_VPC (139)   /* add for ascend610 */
#define CHANNEL_DVPP_PNG (140)   /* add for ascend610 */
#define CHANNEL_DVPP_SCD (141)   /* add for ascend610 */
#define CHANNEL_IDS_MAX CHANNEL_NUM

#define PROF_NON_REAL 0
#define PROF_REAL 1
#define DEV_NUM 64

typedef enum tagDrvError {
    DRV_ERROR_NONE = 0,                /**< success */
    DRV_ERROR_NO_DEVICE = 1,           /**< no valid device */
    DRV_ERROR_INVALID_DEVICE = 2,      /**< invalid device */
    DRV_ERROR_INVALID_VALUE = 3,       /**< invalid value */
    DRV_ERROR_INVALID_HANDLE = 4,      /**< invalid handle */
    DRV_ERROR_INVALID_MALLOC_TYPE = 5, /**< invalid malloc type */
    DRV_ERROR_OUT_OF_MEMORY = 6,       /**< out of memory */
    DRV_ERROR_INNER_ERR = 7,           /**< driver inside error */
    DRV_ERROR_PARA_ERROR = 8,          /**< driver wrong parameter */
    DRV_ERROR_UNINIT = 9,              /**< driver uninit */
    DRV_ERROR_REPEATED_INIT = 10,          /**< driver repeated init */
    DRV_ERROR_NOT_EXIST = 11,        /**< there is resource*/
    DRV_ERROR_REPEATED_USERD = 12,
    DRV_ERROR_BUSY = 13,                /**< task already running */
    DRV_ERROR_NO_RESOURCES = 14,        /**< driver short of resouces */
    DRV_ERROR_OUT_OF_CMD_SLOT = 15,
    DRV_ERROR_WAIT_TIMEOUT = 16,       /**< driver wait timeout*/
    DRV_ERROR_IOCRL_FAIL = 17,         /**< driver ioctl fail*/

    DRV_ERROR_SOCKET_CREATE = 18,      /**< driver create socket error*/
    DRV_ERROR_SOCKET_CONNECT = 19,     /**< driver connect socket error*/
    DRV_ERROR_SOCKET_BIND = 20,        /**< driver bind socket error*/
    DRV_ERROR_SOCKET_LISTEN = 21,      /**< driver listen socket error*/
    DRV_ERROR_SOCKET_ACCEPT = 22,      /**< driver accept socket error*/
    DRV_ERROR_CLIENT_BUSY = 23,        /**< driver client busy error*/
    DRV_ERROR_SOCKET_SET = 24,         /**< driver socket set error*/
    DRV_ERROR_SOCKET_CLOSE = 25,       /**< driver socket close error*/
    DRV_ERROR_RECV_MESG = 26,          /**< driver recv message error*/
    DRV_ERROR_SEND_MESG = 27,          /**< driver send message error*/
    DRV_ERROR_SERVER_BUSY = 28,
    DRV_ERROR_CONFIG_READ_FAIL = 29,
    DRV_ERROR_STATUS_FAIL = 30,
    DRV_ERROR_SERVER_CREATE_FAIL = 31,
    DRV_ERROR_WAIT_INTERRUPT = 32,
    DRV_ERROR_BUS_DOWN = 33,
    DRV_ERROR_DEVICE_NOT_READY = 34,
    DRV_ERROR_REMOTE_NOT_LISTEN = 35,
    DRV_ERROR_NON_BLOCK = 36,

    DRV_ERROR_OVER_LIMIT = 37,
    DRV_ERROR_FILE_OPS = 38,
    DRV_ERROR_MBIND_FAIL = 39,
    DRV_ERROR_MALLOC_FAIL = 40,

    DRV_ERROR_REPEATED_SUBSCRIBED = 41,
    DRV_ERROR_PROCESS_EXIT = 42,
    DRV_ERROR_DEV_PROCESS_HANG = 43,

    DRV_ERROR_REMOTE_NO_SESSION = 44,

    DRV_ERROR_HOT_RESET_IN_PROGRESS = 45,

    DRV_ERROR_OPER_NOT_PERMITTED = 46,

    DRV_ERROR_NO_EVENT_RESOURCES = 47,
    DRV_ERROR_NO_STREAM_RESOURCES = 48,
    DRV_ERROR_NO_NOTIFY_RESOURCES = 49,
    DRV_ERROR_NO_MODEL_RESOURCES = 50,
    DRV_ERROR_TRY_AGAIN = 51,

    DRV_ERROR_DST_PATH_ILLEGAL = 52,                    /**< send file dst path illegal*/
    DRV_ERROR_OPEN_FAILED = 53,                         /**< send file open failed */
    DRV_ERROR_NO_FREE_SPACE = 54,                       /**< send file no free space */
    DRV_ERROR_LOCAL_ABNORMAL_FILE = 55,                 /**< send file local file abnormal*/
    DRV_ERROR_DST_PERMISSION_DENIED = 56,               /**< send file dst Permission denied*/
    DRV_ERROR_DST_NO_SUCH_FILE = 57,                    /**< pull file no such file or directory*/

    DRV_ERROR_MEMORY_OPT_FAIL = 58,
    DRV_ERROR_RUNTIME_ON_OTHER_PLAT = 59,
    DRV_ERROR_SQID_FULL = 60,                           /**< driver SQ   is full */

    DRV_ERROR_SERVER_HAS_BEEN_CREATED = 61,
    DRV_ERROR_NO_PROCESS = 62,
    DRV_ERROR_NO_SUBSCRIBE_THREAD = 63,
    DRV_ERROR_NON_SCHED_GRP_MUL_THREAD = 64,
    DRV_ERROR_NO_GROUP = 65,
    DRV_ERROR_GROUP_EXIST = 66,
    DRV_ERROR_THREAD_EXCEEDS_SPEC = 67,
    DRV_ERROR_THREAD_NOT_RUNNIG = 68,
    DRV_ERROR_PROCESS_NOT_MATCH = 69,
    DRV_ERROR_EVENT_NOT_MATCH = 70,
    DRV_ERROR_PROCESS_REPEAT_ADD = 71,
    DRV_ERROR_GROUP_NON_SCHED = 72,
    DRV_ERROR_NO_EVENT = 73,
    DRV_ERROR_COPY_USER_FAIL = 74,
    DRV_ERROR_QUEUE_EMPTY = 75,
    DRV_ERROR_QUEUE_FULL = 76,
    DRV_ERROR_RUN_IN_ILLEGAL_CPU = 77,
    DRV_ERROR_SUBSCRIBE_THREAD_TIMEOUT = 78,
    DRV_ERROR_BAD_ADDRESS = 79,
    DRV_ERROR_DST_FILE_IS_BEING_WRITTEN = 80,           /**< send file The dts file is being written */
    DRV_ERROR_EPOLL_CLOSE = 81,                         /**< epoll close */
    DRV_ERROR_CDQ_ABNORMAL = 82,
    DRV_ERROR_CDQ_NOT_EXIST = 83,
    DRV_ERROR_NO_CDQ_RESOURCES = 84,
    DRV_ERROR_CDQ_QUIT = 85,
    DRV_ERROR_PARTITION_NOT_RIGHT = 86,
    DRV_ERROR_RESOURCE_OCCUPIED = 87,
    DRV_ERROR_PERMISSION = 88,

    DRV_ERROR_NOT_SUPPORT = 0xfffe,
    DRV_ERROR_RESERVED,
} drvError_t;

typedef enum {
    MODULE_TYPE_SYSTEM = 0,  /**< system info*/
    MODULE_TYPE_AICPU,       /** < aicpu info*/
    MODULE_TYPE_CCPU,        /**< ccpu_info*/
    MODULE_TYPE_DCPU,        /**< dcpu info*/
    MODULE_TYPE_AICORE,      /**< AI CORE info*/
    MODULE_TYPE_TSCPU,       /**< tscpu info*/
    MODULE_TYPE_PCIE,        /**< PCIE info*/
    MODULE_TYPE_VECTOR_CORE, /**< VECTOR CORE info*/
    MODULE_TYPE_COMPUTING = 0x8000, /* computing power info */
} DEV_MODULE_TYPE;


typedef enum {
    INFO_TYPE_ENV = 0,
    INFO_TYPE_VERSION,
    INFO_TYPE_MASTERID,
    INFO_TYPE_CORE_NUM,
    INFO_TYPE_FREQUE,
    INFO_TYPE_OS_SCHED,
    INFO_TYPE_IN_USED,
    INFO_TYPE_ERROR_MAP,
    INFO_TYPE_OCCUPY,
    INFO_TYPE_ID,
    INFO_TYPE_IP,
    INFO_TYPE_ENDIAN,
    INFO_TYPE_P2P_CAPABILITY,
    INFO_TYPE_SYS_COUNT,
    INFO_TYPE_MONOTONIC_RAW,
    INFO_TYPE_CORE_NUM_LEVEL,
    INFO_TYPE_FREQUE_LEVEL,
    INFO_TYPE_FFTS_TYPE,
    INFO_TYPE_PHY_CHIP_ID,
    INFO_TYPE_PHY_DIE_ID,
    INFO_TYPE_PF_CORE_NUM,
    INFO_TYPE_PF_OCCUPY,
    INFO_TYPE_WORK_MODE,
    INFO_TYPE_UTILIZATION,
    INFO_TYPE_HOST_OSC_FREQUE,
    INFO_TYPE_DEV_OSC_FREQUE,
} DEV_INFO_TYPE;

enum drvHdcServiceType {
    HDC_SERVICE_TYPE_DMP = 0,
    HDC_SERVICE_TYPE_PROFILING = 1, /**< used by profiling tool */
    HDC_SERVICE_TYPE_IDE1 = 2,
    HDC_SERVICE_TYPE_FILE_TRANS = 3,
    HDC_SERVICE_TYPE_IDE2 = 4,
    HDC_SERVICE_TYPE_LOG = 5,
    HDC_SERVICE_TYPE_RDMA = 6,
    HDC_SERVICE_TYPE_BBOX = 7,
    HDC_SERVICE_TYPE_FRAMEWORK = 8,
    HDC_SERVICE_TYPE_TSD = 9,
    HDC_SERVICE_TYPE_TDT = 10,
    HDC_SERVICE_TYPE_PROF = 11, /* used by drv prof */
    HDC_SERVICE_TYPE_IDE_FILE_TRANS = 12,
    HDC_SERVICE_TYPE_DUMP = 13,
    HDC_SERVICE_TYPE_USER3 = 14, /* used by user */
    HDC_SERVICE_TYPE_DVPP = 15,
    HDC_SERVICE_TYPE_QUEUE = 16,
    HDC_SERVICE_TYPE_MAX
};

enum drvHdcSessionAttr {
    HDC_SESSION_ATTR_DEV_ID = 0,
    HDC_SESSION_ATTR_UID = 1,
    HDC_SESSION_ATTR_RUN_ENV = 2,
    HDC_SESSION_ATTR_VFID = 3,
    HDC_SESSION_ATTR_LOCAL_CREATE_PID = 4,
    HDC_SESSION_ATTR_PEER_CREATE_PID = 5,
    HDC_SESSION_ATTR_MAX
};

typedef drvError_t hdcError_t;
typedef void *HDC_CLIENT;
typedef void *HDC_SESSION;
typedef void *HDC_SERVER;
typedef void *HDC_EPOLL;

struct drvHdcMsgBuf {
    char *pBuf;
    int len;
};

struct drvHdcMsg {
    int count;
    struct drvHdcMsgBuf bufList[0];
};

#define MAX_CHIP_NAME 32
typedef struct hal_chip_info {
    unsigned char type[MAX_CHIP_NAME];
    unsigned char name[MAX_CHIP_NAME];
    unsigned char version[MAX_CHIP_NAME];
} halChipInfo;

/* add for get prof channel list */
#define PROF_CHANNEL_NAME_LEN 32
#define PROF_CHANNEL_NUM_MAX 160
struct channel_info {
    char channel_name[PROF_CHANNEL_NAME_LEN];
    unsigned int channel_type; /* system / APP */
    unsigned int channel_id;
};

typedef struct channel_list {
    unsigned int chip_type;  /* 1910/1980/1951 */
    unsigned int channel_num;
    struct channel_info channel[PROF_CHANNEL_NUM_MAX];
} channel_list_t;

typedef enum prof_channel_type {
    PROF_TS_TYPE,
    PROF_PERIPHERAL_TYPE,
    PROF_CHANNEL_TYPE_MAX,
} PROF_CHANNEL_TYPE;

typedef struct prof_start_para {
    PROF_CHANNEL_TYPE channel_type;     /* for ts and other device */
    unsigned int sample_period;
    unsigned int real_time;             /* real mode */
    void *user_data;                    /* ts data's pointer */
    unsigned int user_data_size;        /* user data's size */
} prof_start_para_t;

/* this struct = the one in "prof_drv_dev.h" */
typedef struct prof_poll_info {
    unsigned int device_id;
    unsigned int channel_id;
} prof_poll_info_t;

enum drvHdcChanType {
    HDC_CHAN_TYPE_SOCKET = 0,
    HDC_CHAN_TYPE_PCIE,
    HDC_CHAN_TYPE_MAX
};

struct drvHdcCapacity {
    enum drvHdcChanType chanType;
    unsigned int maxSegment;
};

#define RUN_ENV_UNKNOW 0
#define RUN_ENV_PHYSICAL 1
#define RUN_ENV_PHYSICAL_CONTAINER 2
#define RUN_ENV_VIRTUAL 3
#define RUN_ENV_VIRTUAL_CONTAINER 4

typedef enum {
    GO_TO_SO = 0,
    GO_TO_SUSPEND,
    GO_TO_S3,
    GO_TO_S4,
    GO_TO_D0,
    GO_TO_D3,
    GO_TO_DISABLE_DEV,
    GO_TO_ENABLE_DEV,
    GO_TO_STATE_MAX,
} devdrv_state_t;

typedef struct tag_state_info {
    devdrv_state_t state;
    uint32_t devId;
} devdrv_state_info_t;

typedef enum {
    DRVDEV_CALL_BACK_SUCCESS = 0,
    DRVDEV_CALL_BACK_FAILED,
} devdrv_callback_state_t;

#ifdef __cplusplus
}
#endif
#endif

