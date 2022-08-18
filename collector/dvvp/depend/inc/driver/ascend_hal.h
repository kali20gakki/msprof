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

/*lint -e116 -e17*/
#ifndef int8_t
typedef signed char int8_t;
#endif

#ifndef int32_t
typedef signed int int32_t;
#endif

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

/**
 * @ingroup driver
 * @brief API major version.
 * @attention major version range form 0x00 to 0xff.
 * when delete API, modify API name, should add major version.
 */
#define __HAL_API_VER_MAJOR 0x05
/**
 * @ingroup driver
 * @brief API minor version.
 * @attention minor version range form 0x00 to 0xff.
 * when add new API, should add minor version.
 */
#define __HAL_API_VER_MINOR 0x07
/**
 * @ingroup driver
 * @brief API patch version,
 * @attention patch version range form 0x00 to 0xff.
 * when modify enum para, struct para add patch version.
 * this means when new API compatible with old API, change patch version
 */
#define __HAL_API_VER_PATCH 0x0C

/**
 * @ingroup driver
 * @brief API VERSION NUMBER combines major version, minor version and patch version,
 * @brief example : 020103 means version 0x020103, major 0x02, minor 0x01, patch 0x03
 */
#define __HAL_API_VERSION ((__HAL_API_VER_MAJOR << 16) | (__HAL_API_VER_MINOR << 8) | (__HAL_API_VER_PATCH))

/**
 * @ingroup driver
 * @brief driver unified error numbers.
 * @brief new code must return error numbers based on unified error numbers.
 */
#define HAL_ERROR_CODE_BASE  0x90020000

/**
 * @ingroup driver
 * @brief each error code use definition "HAI_ERROR_CODE(MODULE, ERROR_CODE)"
 * @brief which MODULE is the module and ERROR_CODE is the error number.
 */
#define HAI_ERROR_CODE(MODULE, ERROR_CODE) (HAL_ERROR_CODE_BASE + (ERROR_CODE) + ((MODULE) << 12))
#define HAI_ERROR_CODE_NO_MODULE(ERROR_CODE) ((ERROR_CODE) & 0x00000FFF)
/**
 * @ingroup driver
 * @brief turn deviceID to nodeID
 */
#define DEVICE_TO_NODE(x) x
#define NODE_TO_DEVICE(x) x

/**
 * @ingroup driver
 * @brief memory type
 */
typedef enum tagDrvMemType {
    DRV_MEMORY_HBM, /**< HBM memory on device */
    DRV_MEMORY_DDR, /**< DDR memory on device */
} drvMemType_t;

/**
 * @ingroup driver
 * @brief memcpy kind.
 */
typedef enum tagDrvMemcpyKind {
    DRV_MEMCPY_HOST_TO_HOST,     /**< host to host */
    DRV_MEMCPY_HOST_TO_DEVICE,   /**< host to device */
    DRV_MEMCPY_DEVICE_TO_HOST,   /**< device to host */
    DRV_MEMCPY_DEVICE_TO_DEVICE, /**< device to device */
} drvMemcpyKind_t;

/**
 * @ingroup driver
 * @brief Async memcpy parameter.
 */
typedef struct tagDrvDmaAddr {
    void *dst;   /**< destination address */
    void *src;   /**< source address */
    int32_t len; /**< the number of byte to copy */
    int8_t flag; /**< mulitycopy flag */
} drvDmaAddr_t;

/**
 * @ingroup driver
 * @brief interrupt number that task scheduler set to driver.
 */
typedef enum tagDrvInterruptNum {
    DRV_INTERRUPT_QOS_READY = 0, /**< QoS queue almost empty*/
    DRV_INTERRUPT_REPORT_READY,  /**< Return queue almost full*/
    DRV_INTERRUPT_RESERVED,
} drvInterruptNum_t;

/**
 * @ingroup driver
 * @brief driver command handle.
 */
typedef void *drvCommand_t;

/**
 * @ingroup driver
 * @brief driver task report handle.
 */
typedef void *drvReport_t;

#ifdef __linux
#define DLLEXPORT
#else
#define DLLEXPORT _declspec(dllexport)
#ifndef pid_t
typedef int pid_t;
#endif
#endif

typedef enum tagDrvStatus {
    DRV_STATUS_INITING = 0x0,
    DRV_STATUS_WORK,
    DRV_STATUS_EXCEPTION,
    DRV_STATUS_SLEEP,
    DRV_STATUS_COMMUNICATION_LOST,
    DRV_STATUS_RESERVED,
} drvStatus_t;

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
} DEV_INFO_TYPE;

/**
 * @ingroup driver
 * @brief Get computing power value parameter.
 */
typedef enum {
    INFO_TYPE_COMPUTING_TOKEN = 0,
    INFO_TYPE_MAX_TOKEN,
} INFO_TYPE_COMPUTING;

typedef enum {
    PHY_INFO_TYPE_CHIPTYPE = 0,
} PHY_DEV_INFO_TYPE;

typedef enum {
    DEVS_INFO_TYPE_TOPOLOGY = 0,
} PAIR_DEVS_INFO_TYPE;

#define TOPOLOGY_HCCS  0
#define TOPOLOGY_PIX   1
#define TOPOLOGY_PIB   2
#define TOPOLOGY_PHB   3
#define TOPOLOGY_SYS   4

#define PROCESS_SIGN_LENGTH  49
#define PROCESS_RESV_LENGTH  4

#define COMPUTING_TOKEN_TYPE_INVALID 0xFF
#define COMPUTING_TOKEN_LAD0TKEN01 1
#define COMPUTING_TOKEN_LAD0TKEN02 2
#define COMPUTING_POWER_MAX_VALUE  65535
#define COMPUTING_POWER_MIN_VALUE  0

struct computing_token {
    float value;
    unsigned char type;
    unsigned char reserve_c;
    unsigned short reserve_s;
};

struct process_sign {
    pid_t tgid;
    char sign[PROCESS_SIGN_LENGTH];
    char resv[PROCESS_RESV_LENGTH];
};

enum devdrv_process_type {
    DEVDRV_PROCESS_CP1 = 0,   /* aicpu_scheduler */
    DEVDRV_PROCESS_CP2,       /* custom_process */
    DEVDRV_PROCESS_DEV_ONLY,  /* TDT */
    DEVDRV_PROCESS_QS,        /* queue_scheduler */
    DEVDRV_PROCESS_CPTYPE_MAX,
};

#define HAL_QUERY_RESV_LENGTH 8
struct halQueryDevpidInfo {
    pid_t hostpid;
    uint32_t devid;
    uint32_t vfid;
    enum devdrv_process_type proc_type;
    char resv[HAL_QUERY_RESV_LENGTH];
};

/**
 * @ingroup driver
 * @brief  get device info when open device
 */
struct drvDevInfo {
#ifndef __linux
    mmProcess fd;
#else
    int fd;
#endif
};

typedef enum {
    CMD_TYPE_POWERON,
    CMD_TYPE_POWEROFF,
    CMD_TYPE_CM_ALLOC,
    CMD_TYPE_CM_FREE,
    CMD_TYPE_SC_FREE,
    CMD_TYPE_MAX,
} devdrv_cmd_type_t;

typedef enum {
    MEM_TYPE_PCIE_SRAM = 0,
    MEM_TYPE_PCIE_DDR,
    MEM_TYPE_IMU_DDR,
    MEM_TYPE_BBOX_DDR,
    MEM_TYPE_BBOX_HDR,
    MEM_TYPE_REG_SRAM,
    MEM_TYPE_REG_DDR,
    MEM_CTRL_TYPE_MAX,
} MEM_CTRL_TYPE;

typedef struct tag_alloc_cm_para {
    void **ptr;
    uint64_t size;
} devdrv_alloc_cm_para_t;

typedef struct tag_free_cm_para {
    void *ptr;
} devdrv_free_cm_para_t;

typedef enum {
    DRVDEV_CALL_BACK_SUCCESS = 0,
    DRVDEV_CALL_BACK_FAILED,
} devdrv_callback_state_t;

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

struct drvNotifyInfo {
    uint32_t tsId;
    uint32_t notifyId;
    uint64_t devAddrOffset;
};

struct drvIpcNotifyInfo {
    uint32_t tsId;
    uint32_t devId;
    uint32_t notifyId;
};

struct drvTsExceptionInfo {
    uint32_t tsId;
    uint64_t exception_code;
};

#define CAP_RESERVE_SIZE 30

#define CAP_MEM_SUPPORT_HBM      (1)          /**< mem support  for HBM */
#define CAP_MEM_SUPPORT_L2BUFFER (1 << 1)     /**< mem support  for L2BUFFER */

#define CAP_SDMA_REDUCE_FP32   (1)         /**< sdma_reduce support for FP32 */
#define CAP_SDMA_REDUCE_FP16   (1 << 1)    /**< sdma_reduce support for FP16 */
#define CAP_SDMA_REDUCE_INT16  (1 << 2)    /**< sdma_reduce support for INT16 */

#define CAP_SDMA_REDUCE_INT4   (1 << 3)    /**< sdma_reduce support for INT4 */
#define CAP_SDMA_REDUCE_INT8   (1 << 4)    /**< sdma_reduce support for INT8 */
#define CAP_SDMA_REDUCE_INT32  (1 << 5)    /**< sdma_reduce support for INT32 */
#define CAP_SDMA_REDUCE_BFP16  (1 << 6)    /**< sdma_reduce support for BFP16 */
#define CAP_SDMA_REDUCE_BFP32  (1 << 7)    /**< sdma_reduce support for BFP32 */
#define CAP_SDMA_REDUCE_UINT8  (1 << 8)    /**< sdma_reduce support for UINT8 */
#define CAP_SDMA_REDUCE_UINT16 (1 << 9)    /**< sdma_reduce support for UINT16 */
#define CAP_SDMA_REDUCE_UINT32 (1 << 10)   /**< sdma_reduce support for UINT32 */

#define CAP_SDMA_REDUCE_KIND_ADD   (1)           /**< sdma_reduce support for ADD */
#define CAP_SDMA_REDUCE_KIND_MAX   (1 << 1)      /**< sdma_reduce support for MAX */
#define CAP_SDMA_REDUCE_KIND_MIN   (1 << 2)      /**< sdma_reduce support for MIN */
#define CAP_SDMA_REDUCE_KIND_EQUAL (1 << 3)      /**< sdma_reduce support for EQUAL */

struct halCapabilityInfo {
    uint32_t sdma_reduce_support; /**< bit for CAP_SDMA_REDUCE_* */
    uint32_t memory_support;      /**< bit for CAP_MEM_SUPPORT_* */
    uint32_t ts_group_number;
    uint32_t sdma_reduce_kind;    /**< bit for CAP_SDMA_REDUCE_KIND_* */
    uint32_t res[CAP_RESERVE_SIZE];
};

#define COMPUTE_GROUP_INFO_RES_NUM 8

/* devdrv ts identifier for get ts group info */
typedef enum {
    TS_AICORE = 0,
    TS_AIVECTOR,
}DRV_TS_ID;

struct capability_group_info {
    unsigned int  group_id;
    unsigned int  state; // 0: not create, 1: created
    unsigned int  extend_attribute; // 0: default group attribute
    unsigned int  aicore_number; // 0~9
    unsigned int  aivector_number; // 0~7
    unsigned int  sdma_number; // 0~15
    unsigned int  aicpu_number; // 0~15
    unsigned int  active_sq_number; // 0~31
    unsigned int  res[COMPUTE_GROUP_INFO_RES_NUM];
};

#define MAX_CHIP_NAME 32
typedef struct hal_chip_info {
    unsigned char type[MAX_CHIP_NAME];
    unsigned char name[MAX_CHIP_NAME];
    unsigned char version[MAX_CHIP_NAME];
} halChipInfo;

typedef devdrv_callback_state_t (*drvDeviceStateNotify)(devdrv_state_info_t *state);

typedef int (*drvDeviceExceptionReporFunc)(uint32_t devId, uint32_t exceptionId, struct timespec timeStamp);
typedef int (*drvDeviceStartupNotify)(uint32_t num, uint32_t *devId);

// [Qiu]
#define BUFF_RESERVE_LEN 8
#define BUFF_POOL_NAME_LEN 128
#define BUFF_GRP_NAME_LEN 32
#define BUFF_PUB_POOL_CFG_MAX_NUM 128
#define EVENT_MAX_MSG_LEN  128  /* Maximum message length */
typedef enum group_id_type {
    GROUP_ID_CREATE,
    GROUP_ID_ADD
} GROUP_ID_TYPE;

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
} drvError_t;//lint !e116 !e17

typedef struct {
    unsigned long long maxMemSize; /* max buf size in grp, in KB, if = 0 means no limit */
    int rsv[BUFF_GRP_NAME_LEN];  /* reserve */
} GroupCfg;

#define BUFF_MAX_CFG_NUM 64
typedef struct {
    unsigned int cfg_id;    /* cfg id, start from 0 */
    unsigned long long total_size;  /* one zone total size */
    unsigned int blk_size;  /* blk size, 2^n (0, 2M] */
    unsigned long long max_buf_size; /* max size can alloc from zone */
    unsigned int page_type;  /* page type, small page / huge page */
    int elasticEnable; /* elastic enable */
    int elasticRate;
    int elasticRateMax;
    int elasticHighLevel;
    int elasticLowLevel;
    int rsv[8];
} memZoneCfg;

typedef struct {
    memZoneCfg cfg[BUFF_MAX_CFG_NUM];
}BuffCfg;

typedef struct {
    unsigned int admin : 1;     /* admin permission, can add other proc to grp */
    unsigned int read : 1;     /* read only permission */
    unsigned int write : 1;    /* read and write permission */
    unsigned int alloc : 1;    /* alloc permission (have read and write permission) */
    unsigned int rsv : 28;
}GroupShareAttr;

typedef enum {
    GRP_QUERY_GROUP,                  /* query grp info include proc and permission */
    GRP_QUERY_GROUPS_OF_PROCESS,   /* query process all grp */
    GRP_QUERY_GROUP_ID,               /* query grp ID by grp name */
    GRP_QUERY_CMD_MAX,
} GroupQueryCmdType;


typedef struct {
    struct {
        unsigned int blkSize;     /* blk size */
        unsigned int blkNum;    /* blk num, blkSize * blkNum must < 4G Byte */
        unsigned int align;      /* addr align, must be an integer multiple of 2, 2< algn <4k */
        unsigned int hugePageFlag; /* huge page flag */
        int reserve[2]; /* reserved */
    } pubPoolCfg[BUFF_PUB_POOL_CFG_MAX_NUM]; /* max allo 128 cfg */
} PubPoolAttr;

enum BuffConfCmdType {
    BUFF_CONF_MBUF_TIMEOUT_CHECK = 0,
    BUFF_CONF_MEMZONE_BUFF_CFG = 1,
    BUFF_CONF_MAX,
};

typedef enum event_id {
    EVENT_RANDOM_KERNEL,      /* Random operator event */
    EVENT_DVPP_MSG,           /* operator events commited by DVPP */
    EVENT_FR_MSG,             /* operator events commited by Feature retrieves */
    EVENT_TS_HWTS_KERNEL,     /* operator events commited by ts/hwts */
    EVENT_AICPU_MSG,          /* aicpu activates its own stream events */
    EVENT_TS_CTRL_MSG,        /* controls message events of TS */
    EVENT_QUEUE_ENQUEUE,      /* entry event of Queue(consumer) */
    EVENT_QUEUE_FULL_TO_NOT_FULL,   /* full to non-full events of Queue(producers) */
    EVENT_QUEUE_EMPTY_TO_NOT_EMPTY,   /* empty to non-empty event of Queue(consumer) */
    EVENT_TDT_ENQUEUE,        /* data entry event of TDT */
    EVENT_TIMER,              /* ros timer */
    EVENT_HCFI_SCHED_MSG,     /* scheduling events of HCFI */
    EVENT_HCFI_EXEC_MSG,      /* performs the event of HCFI */
    EVENT_ROS_MSG_LEVEL0,
    EVENT_ROS_MSG_LEVEL1,
    EVENT_ROS_MSG_LEVEL2,
    EVENT_ACPU_MSG_TYPE0,
    EVENT_ACPU_MSG_TYPE1,
    EVENT_ACPU_MSG_TYPE2,
    EVENT_CCPU_CTRL_MSG,
    EVENT_SPLIT_KERNEL,
    EVENT_DVPP_MPI_MSG,
    EVENT_CDQ_MSG,            /* message events commited by CDQM(hardware) */
    EVENT_FFTS_PLUS_MSG,      /* operator events commited by FFTS(hardware) */
    EVENT_DRV_MSG,            /* events of drvier */
    EVENT_QS_MSG,             /* events of queue scheduler */
    EVENT_TS_CALLBACK_MSG,
    /* Add a new event here */
    EVENT_TEST,               /* Reserve for test */
    EVENT_USR_START = 48,
    EVENT_USR_END = 63,
    EVENT_MAX_NUM
} EVENT_ID;

enum BuffGetCmdType {
    BUFF_GET_MBUF_TIMEOUT_INFO = 0,
    BUFF_GET_MBUF_USE_INFO = 1,
    BUFF_GET_MAX,
};

typedef enum schedule_policy {
    ONLY = 0,
    FIRST = 1,
    POLICY_MAX,
} SCHEDULE_POLICY;

typedef enum group_type {
    /* Bound to a AICPU, multiple threads can be woken up simultaneously within a group */
    GRP_TYPE_BIND_DP_CPU = 1,
    GRP_TYPE_BIND_CP_CPU,             /* Bind to the control CPU */
    GRP_TYPE_BIND_DP_CPU_EXCLUSIVE    /* Bound to a AICPU, intra-group threads are mutex awakened */
} GROUP_TYPE;

typedef enum schedule_priority {
    PRIORITY_LEVEL0,
    PRIORITY_LEVEL1,
    PRIORITY_LEVEL2,
    PRIORITY_LEVEL3,
    PRIORITY_LEVEL4,
    PRIORITY_LEVEL5,
    PRIORITY_LEVEL6,
    PRIORITY_LEVEL7,
    PRIORITY_MAX,
} SCHEDULE_PRIORITY;

typedef enum QueEventCmd {
    QUE_PAUSE_EVENT = 1,    /* pause enque event publish in group */
    QUE_RESUME_EVENT        /* resume enque event publish */
} QUE_EVENT_CMD;

// Qiu
/**
* @ingroup driver
* @brief Black box status notification callback function registration Interface
* @attention null
* @param [in] drvDeviceStateNotify state_callback
* @return 0 success, others for fail
*/
DLLEXPORT drvError_t drvDeviceStateNotifierRegister(drvDeviceStateNotify state_callback);
/**
* @ingroup driver
* @brief Black box status Start up callback function registration
* @attention null
* @param [in] startup_callback  callback function poiniter
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceStartupRegister(drvDeviceStartupNotify startup_callback);
/**
* @ingroup driver
* @brief get chip capbility information
* @attention null
* @param [in]  devId device id
* @param [out]  info chip capbility information
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t halGetChipCapability(uint32_t devId, struct halCapabilityInfo *info);

/**
* @ingroup driver
* @brief get ts group info
* @attention null
* @param [in]  devId device id
* @param [in]  ts_id ts id 0 : TS_AICORE, 1 : TS_AIVECTOR
* @param [in]  group_id group id
* @param [in]  group_count group count
* @param [out]  info ts group info
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t halGetCapabilityGroupInfo(int device_id, int ts_id, int group_id,
    struct capability_group_info *group_info, int group_count);

/**
* @ingroup driver
* @brief get hal API Version
* @attention null
* @param [out]  halAPIVersion version of hal API
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t halGetAPIVersion(int *halAPIVersion);

/**
* @ingroup driver
* @brief get device availability information
* @attention null
* @param [in] devId  device id
* @param [out] status  device status
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceStatus(uint32_t devId, drvStatus_t *status);
/**
* @ingroup driver
* @brief open device
* @attention it will return error when reopen device
* @param [in] devId  device id
* @param [out] devInfo  device information
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceOpen(void **devInfo, uint32_t devId);
/**
* @ingroup driver
* @brief close device
* @attention it will return error when reclose device
* @param [in] devid  device id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceClose(uint32_t devId);
/**
* @ingroup driver
* @brief Get the dma handling method of the device
* @attention The transport method based on the source and destination addresses should be implemented
* by the runtime layer. However, since the mini and cloud implementation methods are different,
* the runtime does not have a corresponding macro partition, so DRV sinks to the kernel state and adds
* the macro partition
* @param [in] *src  Memory address of source device
* @param [in] *dest Memory address of the destination device
* @param [out] *trans_type trans type which has two types:
*              DRV_SDMA = 0x0;  SDMA mode move
*              DRV_PCIE_DMA = 0x1;  PCIE mode move
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceGetTransWay(void *src, void *dest, uint8_t *trans_type);
/**
* @ingroup driver
* @brief Get current platform information
* @attention null
* @param [out] *info  0 Means currently on the Device side, 1/Means currently on the host side
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetPlatformInfo(uint32_t *info);
/**
* @ingroup driver
* @brief Get the current number of devices
* @attention null
* @param [out] *num_dev  Number of current devices
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetDevNum(uint32_t *num_dev);
/**
* @ingroup driver
* @brief Convert device-side devId to host-side devId
* @attention null
* @param [in] localDevId  chip ID
* @param [out] *devId  host side devId
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId);
/**
* @ingroup driver
* @brief Get the probe device list
* @attention null
* @param [in] len  device list length
* @param [out] *devices  device phyical id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halGetDevProbeList(uint32_t *devices, uint32_t len);
/**
* @ingroup driver
* @brief The device side and the host side both obtain the host IDs of all the current devices.
* If called in a container, get the host IDs of all devices in the current container.
* @attention null
* @param [out] *devices   host ID
* @param [out] len  Array length
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetDevIDs(uint32_t *devices, uint32_t len);
/**
* @ingroup driver
* @brief Get the chip IDs of all current devices
* @attention null
* @param [out] *devices  host ID
* @param [out] len  Array length
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetDeviceLocalIDs(uint32_t *devices, uint32_t len);
/**
* @ingroup driver
* @brief Get device id via host device id , only called in device side.
* @attention null
* @param [out] *devices  host ID
* @param [out] len  Array length
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetLocalDevIDByHostDevID(uint32_t host_dev_id, uint32_t *local_dev_id);

/**
* @ingroup driver
* @brief Get device information, CPU information and PCIe bus information.
* @attention each  moduleType  and infoType will get a different
* if the type you input is not compatitable with the table below, then will return fail
* --------------------------------------------------------------------------------------------------------
* moduleType                |        infoType             |    value                    |   attention    |
* --------------------------------------------------------------------------------------------------------
* MODULE_TYPE_SYSTEM        |  INFO_TYPE_ENV              |   env type                  |                |
* MODULE_TYPE_SYSTEM        |  INFO_TYPE_VERSION          |   hardware_version          |                |
* MODULE_TYPE_SYSTEM        |  INFO_TYPE_MASTERID         |   masterId                  | used in host   |
* MODULE_TYPE_SYSTEM        |  INFO_TYPE_CORE_NUM         |   ts_num                    |                |
* MODULE_TYPE_SYSTEM        |  INFO_TYPE_SYS_COUNT        |   system count              |                |
* MODULE_TYPE_SYSTEM        |  INFO_TYPE_MONOTONIC_RAW    |   MONOTONIC_RAW time        |                |
* --------------------------------------------------------------------------------------------------------
* MODULE_TYPE_AICPU         |  INFO_TYPE_CORE_NUM         |   ai cpu number(vcpu in vf) |                |
* MODULE_TYPE_AICPU         |  INFO_TYPE_OS_SCHED         |   ai cpu in os sched        | used in device |
* MODULE_TYPE_AICPU         |  INFO_TYPE_IN_USED          |   ai cpu in used            |                |
* MODULE_TYPE_AICPU         |  INFO_TYPE_ERROR_MAP        |   ai cpu error map          |                |
* MODULE_TYPE_AICPU         |  INFO_TYPE_ID               |   ai cpu id                 |                |
* MODULE_TYPE_AICPU         |  INFO_TYPE_OCCUPY           |   ai cpu occupy bitmap      |                |
* --------------------------------------------------------------------------------------------------------
* MODULE_TYPE_CCPU          |  INFO_TYPE_CORE_NUM         |   ctrl cpu number           |                |
* MODULE_TYPE_CCPU          |  INFO_TYPE_ID               |   ctrl cpu id               |                |
* MODULE_TYPE_CCPU          |  INFO_TYPE_IP               |   ctrl cpu ip               |                |
* MODULE_TYPE_CCPU          |  INFO_TYPE_ENDIAN           |   ctrl cpu ENDIAN           |                |
* MODULE_TYPE_CCPU          |  INFO_TYPE_OS_SCHED         |   ctrl cpu  in os sched     | used in device |
* --------------------------------------------------------------------------------------------------------
* ODULE_TYPE_DCPU           |  INFO_TYPE_CORE_NUM         |   data cpu number           | used in device |
* ODULE_TYPE_DCPU           |  INFO_TYPE_OS_SCHED         |   data cpu in os sched      | used in device |
* --------------------------------------------------------------------------------------------------------
* MODULE_TYPE_AICORE        |  INFO_TYPE_CORE_NUM         |   ai core number            |                |
* MODULE_TYPE_AICORE        |  INFO_TYPE_CORE_NUM_LEVEL   |   ai core number level      |                |
* MODULE_TYPE_AICORE        |  INFO_TYPE_IN_USED          |   ai core in used           |                |
* MODULE_TYPE_AICORE        |  INFO_TYPE_ERROR_MAP        |   ai core error map         |                |
* MODULE_TYPE_AICORE        |  INFO_TYPE_ID               |   ai core id                |                |
* MODULE_TYPE_AICORE        |  INFO_TYPE_FREQUE           |   ai core frequence         |                |
* MODULE_TYPE_AICORE        |  INFO_TYPE_FREQUE_LEVEL     |   ai core frequence level   |                |
* --------------------------------------------------------------------------------------------------------
* MODULE_TYPE_VECTOR_CORE   |   INFO_TYPE_CORE_NUM        | vector core number          |                |
* MODULE_TYPE_VECTOR_CORE   |   INFO_TYPE_FREQUE          | vector core frequence       |                |
* --------------------------------------------------------------------------------------------------------
* MODULE_TYPE_TSCPU         |  INFO_TYPE_CORE_NUM         |   ts cpu number             |                |
* MODULE_TYPE_TSCPU         |  INFO_TYPE_OS_SCHED         |   ts cpu in os sched        | used in device |
* MODULE_TYPE_TSCPU         |  INFO_TYPE_FFTS_TYPE        |   ts cpu ffts type          | used in device |
* --------------------------------------------------------------------------------------------------------
* MODULE_TYPE_PCIE          |  INFO_TYPE_ID               |   pcie bdf                  | used in host   |
* --------------------------------------------------------------------------------------------------------
* @param [in] devId  Device ID
* @param [in] moduleType  See enum DEV_MODULE_TYPE
* @param [in] infoType  See enum DEV_INFO_TYPE
* @param [out] *value  device info
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value);

/**
* @ingroup driver
* @brief Get device info using physical device id
* @attention each  moduleType  and infoType will get a different
* if the type you input is not compatitable with the table below, then will return fail
* ------------------------------------------------------------------------------------------
* moduleType            |        infoType        |    value                |   attention    |
* ------------------------------------------------------------------------------------------
* MODULE_TYPE_SYSTEM    | PHY_INFO_TYPE_CHIPTYPE |   chip type             | used in host   |
* ------------------------------------------------------------------------------------------
* @param [in] phyId  Device physical ID
* @param [in] moduleType  See enum DEV_MODULE_TYPE
* @param [in] infoType  See enum PHY_DEV_INFO_TYPE
* @param [out] *value  device info
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halGetPhyDeviceInfo(uint32_t phyId, int32_t moduleType, int32_t infoType, int64_t *value);

/**
* @ingroup driver
* @brief Get devices relationship, etc
* @attention null
* @param [in] devId  Device ID
* @param [in] otherDevId  other device id compared
* @param [in] infoType  See enum PAIR_DEVS_INFO_TYPE
* @param [out] *value   type of relationship
* *value == TOPOLOGY_HCCS, means relationship is hccs
* *value == TOPOLOGY_PIX,  means relationship is pix
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halGetPairDevicesInfo(uint32_t devId, uint32_t otherDevId, int32_t infoType, int64_t *value);

/**
* @ingroup driver
* @brief Used to define the unique interface of the product. The cmd command word remains unified, compatible,
* and functions are implemented independently
* @attention only support lite
* @param [in] devId  Device ID
* @param [in] cmd  cmd command word
* @param [in] *para
* @param [out] None, can be passed in para
* @return    0 for success, others for fail
*/
DLLEXPORT drvError_t drvCustomCall(uint32_t devId, uint32_t cmd, void *para);
/**
* @ingroup driver
* @brief The black box daemon on the host side calls the interface registration exception reporting function
* @attention null
* @param [in] exception_callback_func  Exception reporting function
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceExceptionHookRegister(drvDeviceExceptionReporFunc exception_callback_func);
/**
* @ingroup driver
* @brief Flash cache interface
* @attention
* 1.Virtual address is the virtual address of this process; 2.Note whether the length passed in meets the requirements
* @param [in] base  Virtual address base address
* @param [in] len  cache length
* @return   0 for success, others for fail
*/
DLLEXPORT void drvFlushCache(uint64_t base, uint32_t len);
/**
* @ingroup driver
* @brief Get physical ID (phyId) using logical ID (devIndex)
* @attention null
* @param [in] devIndex  Logical ID
* @param [out] *phyId  Physical ID
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceGetPhyIdByIndex(uint32_t devIndex, uint32_t *phyId);
/**
* @ingroup driver
* @brief Get logical ID (devIndex) using physical ID (phyId)
* @attention null
* @param [in] phyId   Physical ID
* @param [out] devIndex  Logical ID
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDeviceGetIndexByPhyId(uint32_t phyId, uint32_t *devIndex);
/**
* @ingroup driver
* @brief host process random flags get interface
* @attention null
* @param [out] *sign  host process random flag
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetProcessSign(struct process_sign *sign);
/**
* @ingroup driver
* @brief query devpid by info
* @attention null
* @param [in] info: See struct halQueryDevpidInfo
* @param [out] dev_pid: device pid correspond to info
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueryDevpid(struct halQueryDevpidInfo info, pid_t *dev_pid) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief Get non-container internal Tgid number
* @attention null
* @return Tgid number (non-container Tgid)
*/
DLLEXPORT pid_t drvDeviceGetBareTgid(void);
/**
* @ingroup driver
* @brief HP/DELL/LENOVO PC send I2C reset cmd to Device
* @attention only support HP/DELL/LENOVO PC + EVB VB
* @param [in] devId  : Device ID
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvResetDevice(uint32_t devId);
/**
* @ingroup driver
* @brief map kernel space for ddrdump
* @attention null
* @param [in] devId  : Device ID
* @param [in] virAddr : user space addr
* @param [out] *size : kernel space size
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDmaMmap(uint32_t devId, uint64_t virAddr, uint32_t *size);
/**
* @ingroup driver
* @brief read value from bbox hdr addr
* @attention offset + len <= bbox hdr len(512KB)
* @param [in]  devId  : Device ID
* @param [in]  memType: MEM_CTRL_TYPE
* @param [in]  offset : bbox hdr offset
* @param [in]  len : length of read content
* @param [out] *value : read value
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvMemRead(uint32_t devId, MEM_CTRL_TYPE memType, uint32_t offset, uint8_t *value, uint32_t len);
/**
* @ingroup driver
* @brief pcie pre-reset, release pcie related resources applied by each module on the host side
* @attention All functions of pcie are invalid after calling
* @param [in]  devId  : Device ID
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvPciePreReset(uint32_t devId);
/**
* @ingroup driver
* @brief pcie rescan, re-apply the pcie related resources required by each module on the host side
* @attention All functions of pcie are invalid after calling
* @param [in]  devId  : Device ID
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvPcieRescan(uint32_t devId);
#define DRV_P2P_STATUS_ENABLE 1
#define DRV_P2P_STATUS_DISABLE 0
/**
* @ingroup driver
* @brief p2p Enable interface
* @attention Both directions must be set to take effect, and support sdma and vnic interworking
* @param [in]  dev : Logical device id
* @param [in]  peer_dev : Physical device id
* @param [in]  flag : reserve para fill 0
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halDeviceEnableP2P(uint32_t dev, uint32_t peer_dev, uint32_t flag);
/**
* @ingroup driver
* @brief p2p Disable interface
* @attention Both directions must be set to take effect, and support sdma and vnic interworking
* @param [in]  dev : Logical device id
* @param [in]  peer_dev : Physical device id
* @param [in]  flag : reserve para fill 0
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halDeviceDisableP2P(uint32_t dev, uint32_t peer_dev, uint32_t flag);
/**
* @ingroup driver
* @brief p2p Status interface
* @attention Both directions must be set to take effect, and support sdma and vnic interworking
* @param [in]  dev : Logical device id
* @param [in]  peer_dev : Physical device id
* @param [out]  0 for disable, 1 for enable
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetP2PStatus(uint32_t dev, uint32_t peer_dev, uint32_t *status);
/**
* @ingroup driver
* @brief p2p if can access peer interface
* @attention null
* @param [out]  canAccessPeer : 0 for disable, 1 for enable
* @param [in]  dev : Logical device id
* @param [in]  peer_dev : Physical device id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halDeviceCanAccessPeer(int *canAccessPeer, uint32_t dev, uint32_t peer_dev);
/**
* @ingroup driver
* @brief host get device boot status
* @attention null
* @param [in]  phy_id : Physical device id
* @param [out] boot_status : See dsmi_boot_status definition
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvGetDeviceBootStatus(int phy_id, uint32_t *boot_status);
/**
* @ingroup driver
* @brief Get offset address of notify id
* @attention null
* @param [in]  devId  Device number
* @param [in] *info  See struct drvNotifyInfo which includes:
*               tsId: ts id,  ascend310:0, ascend910 :0
*              notifyId, the range of values for notify id [0,1023]
* @param [out] *info: devAddrOffset:  Physical address offset
* @return    0 for success, others for fail
*/
DLLEXPORT drvError_t drvNotifyIdAddrOffset(uint32_t devId, struct drvNotifyInfo *info);
/**
* @ingroup driver
* @brief drvCreateIpcNotify
* @attention null
* @param [in] *name  Ipc notify name to be created
* @param [in] len  name lenth
* @param [in] *info  See struct drvIpcNotifyInfo
*              tsId: ts id,  ascend310:0, ascend910 :0
*              devId: device id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvCreateIpcNotify(struct drvIpcNotifyInfo *info, char *name, unsigned int len);
/**
* @ingroup driver
* @brief Destroy ipc notify
* @attention null
* @param [in] *name  Ipc notify name to be destroyed
* @param [in] *info  See struct drvIpcNotifyInfo
*             tsId: ts id,  ascend310:0, ascend910 :0
*             devId: device id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvDestroyIpcNotify(const char *name, struct drvIpcNotifyInfo *info);
/**
* @ingroup driver
* @brief Open ipc notify
* @attention null
* @param [in] *name  Ipc notify name to open
* @param [in] *info  See struct drvIpcNotifyInfo
*             tsId: ts id,  ascend310:0, ascend910 :0
*             devId: device id
* @param [out] *info  *notifyId  Return opened notification id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvOpenIpcNotify(const char *name, struct drvIpcNotifyInfo *info);
/**
* @ingroup driver
* @brief Close ipc notify
* @attention null
* @param [in] *name  Ipc notify name to close
* @param [in] *info  See struct drvIpcNotifyInfo
*              tsId: ts id,  ascend310:0, ascend910 :0
*              devId: device id
*              notifyId : notification id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvCloseIpcNotify(const char *name, struct drvIpcNotifyInfo *info);
/**
* @ingroup driver
* @brief Set the notification pid whitelist
* @attention null
* @param [in] *name  Ipc notify name to be set
* @param [in] pid[]  Array of whitelisted processes
* @param [in] num  number of whitelisted processes
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvSetIpcNotifyPid(const char *name, pid_t pid[], int num);
/**
* @ingroup driver
* @brief cqsq channel positioning information
* @attention null
* @param [in] devId  Device ID
* @return void
*/
DLLEXPORT void drvDfxShowReport(uint32_t devId);
/**
* @ingroup driver
* @brief send IPC msg to safetyIsland
* @attention null
* @param [in]   devId  Device ID
*               msg : message contents
                msgSize : message size
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halSafeIslandTimeSyncMsgSend(uint32_t devId, void *msg, size_t msgSize);
/**
* @ingroup driver
* @brief send ipc message from Taishan to safetyIsland (non-blocking)
* @attention null
* @param [in]   devId:     Device ID
*               channelId: bit0~bit7/channel Id, bit8~bit15/module Id
*               msgData:   message Data
*               msgSize:   message Size
* @return       0 for success, others for fail
*/
DLLEXPORT drvError_t halDevIpcMsgSend(unsigned int devId, unsigned short channelId, void *msgData, size_t msgSize);
/**
* @ingroup driver
* @brief recv ipc message from safetyIsland to Taishan (blocking)
* @attention null
* @param [in]   devId:     Device ID
*               channelId: bit0~bit7/channel Id, bit8~bit15/module Id
*               msgData:   message Data buffer
*               msgSize:   message Data Size
* @param [out]  msgData :  recv message Data
* @return       0 for success, others for fail
*/
DLLEXPORT drvError_t halDevIpcMsgRecv(unsigned int devId, unsigned short channelId, void *msgData, size_t msgSize);

#define DV_OFFLINE
#define DV_ONLINE
#define DV_OFF_ONLINE
#define DV_LITE

typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

#define ADVISE_TYPE (1UL)       /**< 0: DDR memory 1: HBM memory */
#define ADVISE_EXE (1UL << 1)   /**< setting executable permissions */
#define ADVISE_THP (1UL << 2)   /**< using huge page memory */
#define ADVISE_PLE (1UL << 3)   /**< memory prefetching */
#define ADVISE_PIN (1UL << 4)   /**< pin ddr memory */
#define ADVISE_UNPIN (1UL << 5) /**< unpin ddr memory */

/*
 * each bit of flag
 *    bit0~9 devid
 *    bit10~13: virt mem type(svm\dev\host\dvpp)
 *    bit14~16: phy mem type(DDR\HBM)
 *    bit17~18: phy page size(normal\huge)
 *    bit19: phy continuity
 *    bit20~24: align size(2^n)
 *    bit25~40: mem advise(P2P\4G\TS_NODE_DDR)
 *    bit41~63: reserved
 */
/* devid */
#define MEM_DEVID_WIDTH        10
#define MEM_DEVID_MASK         ((1UL << MEM_DEVID_WIDTH) - 1)
/* virt mem type */
#define MEM_VIRT_BIT           10
#define MEM_VIRT_WIDTH         4

#define MEM_SVM_VAL            0X0
#define MEM_DEV_VAL            0X1
#define MEM_HOST_VAL           0X2
#define MEM_DVPP_VAL           0X3
#define MEM_SVM                (MEM_SVM_VAL << MEM_VIRT_BIT)
#define MEM_DEV                (MEM_DEV_VAL << MEM_VIRT_BIT)
#define MEM_HOST               (MEM_HOST_VAL << MEM_VIRT_BIT)
#define MEM_DVPP               (MEM_DVPP_VAL << MEM_VIRT_BIT)
/* phy mem type */
#define MEM_PHY_BIT            14
#define MEM_TYPE_DDR           (0X0 << MEM_PHY_BIT)
#define MEM_TYPE_HBM           (0X1 << MEM_PHY_BIT)
/* phy page size */
#define MEM_PAGE_BIT           17
#define MEM_PAGE_NORMAL        (0X0 << MEM_PAGE_BIT)
#define MEM_PAGE_HUGE          (0X1 << MEM_PAGE_BIT)
/* phy continuity */
#define MEM_CONTINUTY_BIT      19
#define MEM_DISCONTIGUOUS_PHY  (0X0 << MEM_CONTINUTY_BIT)
#define MEM_CONTIGUOUS_PHY     (0X1 << MEM_CONTINUTY_BIT)
/* advise */
#define MEM_ADVISE_P2P_BIT     25
#define MEM_ADVISE_4G_BIT      26
#define MEM_ADVISE_P2P         (0X1 << MEM_ADVISE_P2P_BIT)
#define MEM_ADVISE_4G          (0X1 << MEM_ADVISE_4G_BIT)
/* alloc ts use mem */
#define MEM_ADVISE_TS_BIT      27
#define MEM_ADVISE_TS          (0X1 << MEM_ADVISE_TS_BIT)
/* alloc pcie bar mem */
#define MEM_ADVISE_BAR_BIT     28
#define MEM_ADVISE_BAR         (0X1 << MEM_ADVISE_BAR_BIT)
/* alloc readonly mem */
#define MEM_READONLY_BIT       29
#define MEM_READONLY           (0X1 << MEM_READONLY_BIT)
/* align size 5 bits width 20-24bit */
#define MEM_ALIGN_BIT          20
#define MEM_ALIGN_SIZE(x)      (1U << (((x) >> MEM_ALIGN_BIT) & 0x1FU))
#define MEM_SET_ALIGN_SIZE(x)  ((((x) & 0x1FU) << MEM_ALIGN_BIT))

/* svm flag for rts and tdt */
#define MEM_SVM_HUGE           (MEM_SVM | MEM_PAGE_HUGE)
#define MEM_SVM_NORMAL         (MEM_SVM | MEM_PAGE_NORMAL)

/* enables different options to be specified that affect the host register */
enum drvRegisterTpye {
    HOST_MEM_MAP_DEV = 0,
    HOST_SVM_MAP_DEV,
    DEV_SVM_MAP_HOST,
    HOST_REGISTER_MAX_TPYE
};

typedef UINT32 DVmem_advise;
typedef UINT32 DVdevice;
typedef UINT64 DVdeviceptr;
typedef drvError_t DVresult;

#define DV_MEM_LOCK_HOST 0x0008
#define DV_MEM_LOCK_DEV 0x0010
#define DV_MEM_LOCK_DEV_DVPP 0x0020

#define DV_MEM_RESV 8
struct DVattribute {
    /**< DV_MEM_SVM_DEVICE : svm memory & mapped device */
    /**< DV_MEM_SVM_HOST : svm memory & mapped host */
    /**< DV_MEM_SVM : svm memory & no mapped */
    /**< DV_MEM_LOCK_HOST :    host mapped memory & lock host */
    /**< DV_MEM_LOCK_DEV : dev mapped memory & lock dev */
    /**< DV_MEM_LOCK_DEV_DVPP : dev_dvpp mapped memory & lock dev */
    UINT32 memType;
    UINT32 resv1;
    UINT32 resv2;

    UINT32 devId;
    UINT32 pageSize;
    UINT32 resv[DV_MEM_RESV];
};

#define DV_LOCK_HOST 0x0001
#define DV_LOCK_DEVICE 0x0002
#define DV_UNLOCK 0
/**
* @ingroup driver
* @brief Set memory allocation strategy for memory range segments
* @attention
* 1. Ensure that the device id corresponding to the execution CPU where the thread calling the interface is consistent
* with the dev_id in the parameter (ids all start from 0), that is, the interface only supports setting the allocation
* strategy of the memory range segment on the device where the current execution thread is located;
* 2. Currently only offline scenarios are supported;
* @param [in] devPtr  Unsigned long, Start address of memory range segment
* @param [in] len  Unsigned long, the size of the memory range segment
* @param [in] type  Signed shaping, the type of memory range segment, currently only supports three: 0 (Local DDR),
* 1 (Local HBM), 2 (Cross HBM)
* @param [in] dev_id  device id
* @return DRV_ERROR_INVALID_VALUE : parameter error, unsupported type or board node number error
* @return DRV_ERROR_INVALID_DEVICE : device id error
* @return DRV_ERROR_MBIND_FAIL : internel memory bind fail
* @return DRV_ERROR_NONE : success
 */
DLLEXPORT DV_OFFLINE DVresult drvMbindHbm(DVdeviceptr devPtr, size_t len, uint32_t type, uint32_t dev_id);
/**
* @ingroup driver
* @brief Load a certain length of data to the specified position of L2BUF
* @attention: offline mode:
* 1. User guarantees *vPtr points to the correct L2BUFF mapped virtual space starting address
* 2. The current interface only takes effect the first time it is successfully invoked throughout the OS life cycle
* @param [in] deviceId  Unsigned shaping, device id, this value is not used in offline scenarios
* @param [in] program  void pointer, a program to be loaded
* @param [in] offset  Offset value of the position to be loaded from the L2BUF starting address, in bytes
* @param [in] ByteCount  The length of the program to be loaded
* @param [out] vPtr  The start address of L2BUF is used as the input. After the load is completed, the address of
* the start position of the load is used as the output
* @return DRV_ERROR_INVALID_VALUE : Parameter error, null pointer, offset exceeds l2buf size,
* copy data exceeds l2buf range, etc
* @return DRV_ERROR_FILE_OPS : Internal error, file operation failed;
* @return DRV_ERROR_IOCRL_FAIL : Internal error, IOCTL operation failed;
* @return DRV_ERROR_INVALID_HANDLE : Internal error, loading program error
* @return DRV_ERROR_NONE : success
*/
DLLEXPORT DV_OFF_ONLINE DVresult drvLoadProgram(DVdevice deviceId, void *program, unsigned int offset,
    size_t ByteCount, void **vPtr);
/**
* @ingroup driver
* @brief Get the corresponding physical address based on the entered virtual address
* @attention
* 1. After applying for memory, you need to call the advise interface to allocate physical memory, and then
* call this interface. That is, the user should ensure that the page table has been established in the space where
* the virtual address is located to ensure that the corresponding physical address is correctly obtained
* @param [in] vptr  Unsigned 64-bit integer, the device memory address must be the shared memory requested
* @param [out] *pptr Unsigned 64-bit integer. The corresponding physical address is returned. The value is valid
* when the return is successful
* @return DRV_ERROR_INVALID_HANDLE : parameter error, pointer is empty, addr is zero.
* @return DRV_ERROR_FILE_OPS : internel error, file operation failed.
* @return DRV_ERROR_IOCRL_FAIL : Internal error, IOCTL operation failed
* @return DRV_ERROR_NONE : success
*/
DLLEXPORT DV_OFF_ONLINE DVresult drvMemAddressTranslate(DVdeviceptr vptr, UINT64 *pptr);
/**
* @ingroup driver
* @brief Get the TTBR and substreamid of the current process
* @attention Can be called multiple times, it is recommended that Runtime be called once; the result record can be
* saved and can be used next time in this process
* @param [in] device  Unsigned shaping, device id, this value is not used in offline scenarios
* @param [out] *SSID  Returns the SubStreamid of the current process
* @return DRV_ERROR_INVALID_VALUE : Parameter error, pointer is empty
* @return DRV_ERROR_FILE_OPS : Internal error, file operation failed
* @return DRV_ERROR_IOCRL_FAIL : Internal error, IOCTL operation failed
* @return DRV_ERROR_NONE : success
*/
DLLEXPORT DV_OFF_ONLINE DVresult drvMemSmmuQuery(DVdevice device, UINT32 *SSID);
/**
* @ingroup driver
* @brief Map the L2buff to the process address space, establish page table, and obtain the starting virtual address
* of the current process L2buff and the corresponding PTE
* @attention 1. It can only be called once during initialization, and a page will be created internally, and multiple
* calls are prohibited; it is released when the process exits.
* @param [in] device  Unsigned shaping, device id, this value is not used in offline scenarios
* @param [out] l2buff  Double pointer, returns a pointer to the starting virtual address of the L2buff
* @param [out] pte  Reserved param
* @return DRV_ERROR_INVALID_HANDLE :  Parameter error, pointer is empty
* @return DRV_ERROR_FILE_OPS :  Internal error, file operation failed
* @return DRV_ERROR_IOCRL_FAIL :  Internal error, IOCTL operation failed
* @return DRV_ERROR_NONE : success
*/
DLLEXPORT DV_OFF_ONLINE DVresult drvMemAllocL2buffAddr(DVdevice device, void **l2buff, UINT64 *pte);
/**
* @ingroup driver
* @brief Release L2buff address space, should be used in conjunction with drvMemAllocL2buffAddr
* @attention null
* @param [in] device  Unsigned shaping, device id, this value is not used in offline scenarios
* @param [in] l2buff  Pointer to the starting virtual address space of L2buff
* @return DRV_ERROR_INVALID_HANDLE : Parameter error, pointer is empty
* @return DRV_ERROR_NONE : success
*/
DLLEXPORT DV_OFF_ONLINE DVresult drvMemReleaseL2buffAddr(DVdevice device, void *l2buff);
/**
* @ingroup driver
* @brief Set the initial memory value according to 8bits (device physical address, unified virtual address
* are supported)
* @attention
*  1. Make sure that the destination buffer can store at least num characters.
*  2. The interface supports processing data larger than 2G
* online:
*  1. The memory to be initialized needs to be on the Host or both on the Device side
*  2. The memory management module is not responsible for the length check of ByteCount. Users need to ensure
*  that the length is legal.
* @param [in] dst  Unsigned 64-bit integer, memory address to be initialized
* @param [in] destMax  The maximum number of valid initial memory values that can be set
* @param [in] value  8-bit unsigned, initial value set
* @param [in] num  Set the initial length of the memory space in bytes
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_INVALID_VALUE : The destination address is 0 and the number of values is 0
* @return DRV_ERROR_INVALID_HANDLE : Internal error, setting failed
*/
DLLEXPORT DV_OFF_ONLINE DVresult drvMemsetD8(DVdeviceptr dst, size_t destMax, UINT8 value, size_t num);
/**
* @ingroup driver
* @brief Copy the data in the source buffer to the destination buffer synchronously
* @attention
* 1. The destination buffer must have enough space to store the contents of the source buffer to be copied.
* 2. (offline) This interface cannot process data larger than 2G
* @param [in] dst  Unsigned 64-bit integer, memory address to be initialized
* @param [in] destMax  The maximum number of valid initial memory values that can be set
* @param [in] value  16-bit unsigned, initial value set
* @param [in] num  Set the number of memory space initial values
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_INVALID_HANDLE : Internal error, copy failed
*/
DLLEXPORT DV_OFF_ONLINE DVresult drvMemcpy(DVdeviceptr dst, size_t destMax, DVdeviceptr src, size_t ByteCount);

/**
* @ingroup driver
* @brief Copy the data in the source buffer to the destination buffer asynchronously
* @attention
* 1. The destination buffer must have enough space to store the contents of the source buffer to be copied.
* 2. (offline) (virtual machine logical grouping) not support
* @param [in] dst  Unsigned 64-bit integer, memory address to be initialized
* @param [in] destMax  The maximum number of valid initial memory values that can be set
* @param [in] value  16-bit unsigned, initial value set
* @param [in] num  Set the number of memory space initial values
* @param [out] copyFd  Asynchronously copy Fd
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_INVALID_HANDLE : Internal error, copy failed
*/
DLLEXPORT DV_OFF_ONLINE DVresult halMemCpyAsync(DVdeviceptr dst, size_t destMax, DVdeviceptr src,
    size_t byteCount, uint64_t *copyFd) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief Copy the data in the source buffer to the destination buffer asynchronously wait finish
* @attention
* 1. The destination buffer must have enough space to store the contents of the source buffer to be copied.
* 2. (offline) (virtual machine logical grouping) not support
* @param [in] copyFd  get from halMemCpyAsync, Asynchronously copy Fd
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_INVALID_HANDLE : Internal error, copy failed
*/
DLLEXPORT DV_OFF_ONLINE DVresult halMemCpyAsyncWaitFinish(uint64_t copyFd) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief Copy the 2D data in the source buffer to the destination buffer
* @attention The destination buffer must have enough space to store the contents of the source buffer to be copied.
* @param [in] *pCopy  see struct MEMCPY2D
* @param [out] *pCopy  see struct MEMCPY2D
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_XXX  : copy failed
*/
DLLEXPORT DV_ONLINE drvError_t halMemcpy2D(struct MEMCPY2D *pCopy) ASCEND_HAL_WEAK;

/**
 * @halSdmaCopy
 * @brief Use sdma device to accelerate memcpy
 * @attention This function is suitable for large size of memcpy. It fallback to normal
 * memcpy_s if the sdma version of memcpy failed.
 * @param [in] dst: destination address
 * @param [in] dst_size: destination memory region size
 * @param [in] src: source address
 * @param [in] len: size of the buffer to be copy
 * @return zero on success otherwise -errno
 */
DLLEXPORT DV_OFFLINE drvError_t halSdmaCopy(
    DVdeviceptr dst, size_t dst_size, DVdeviceptr src, size_t len) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief Converts the address to the physical address for DMA copy, including H2D, D2H, and D2D asynchronous
* copy interfaces.
* @attention The memory management module does not verify the length of ByteCount. You need to ensure that
* the length is valid
* @param [in] pSrc Source address (virtual address)
* @param [in] pDst Destination address (virtual address)
* @param [in] len length
* @param [out] *dmaAddr see struct DMA_ADDR.
* 1. Flag= 0: non-chain, SRC and DST are physical addresses, can directly conduct DMA copy operation
* 2. Flag= 1: chain, SRC is the address of dma chain list, can directly conduct dma copy operation;
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : convert fail
*/
DLLEXPORT DV_ONLINE DVresult drvMemConvertAddr(DVdeviceptr pSrc, DVdeviceptr pDst, UINT32 len,
    struct DMA_ADDR *dmaAddr);
/**
* @ingroup driver
* @brief Releases the physical address information of the DMA copy
* @attention Available online, not offline. This interface is used with drvMemConvertAddr.
* @param [in] struct DMA_ADDR *ptr : Information to be released
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : destyoy fail
*/
DLLEXPORT DV_ONLINE DVresult drvMemDestroyAddr(struct DMA_ADDR *ptr);
/**
* @ingroup driver
* @brief Prefetch data to the memory of the specified device (uniformly shared virtual address)
* @attention Available online, not offline.
* First apply for svm memory, then fill the data, and then prefetch to the target device side.
* The output buffer scenario uses advice to allocate physical memory to the device side.
* If the host does not create a page table, this interface fails.
* The memory management module is not responsible for the length check of ByteCount,
* users need to ensure that the length is legal.
* @param [in] devPtr Memory to prefetch
* @param [in] len Prefetch size
* @param [in] device Destination device for prefetching data
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : prefetch fail
*/
DLLEXPORT DV_ONLINE DVresult drvMemPrefetchToDevice(DVdeviceptr devPtr, size_t len, DVdevice device);
/**
* @ingroup driver
* @brief Create a share corresponding to vptr based on name
* @attention Available online, not offline.
* vptr must be device memory, and must be directly allocated for calling the memory management interface, without offset
* @param [in] vptr Virtual memory to be shared
* @param [in] byte_count User-defined length to be shared
* @param [in] name_len The maximum length of the name array
* @param [out] name  Name used for sharing between processes
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : create mem handle fail
*/
DLLEXPORT DV_ONLINE DVresult halShmemCreateHandle(DVdeviceptr vptr, size_t byte_count, char *name, uint32_t name_len);
/**
* @ingroup driver
* @brief Destroy shared memory created by halShmemCreateHandle
* @attention Available online, not offline.
* @param [in] *name Name used for sharing between processes
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : destroy mem handle fail
*/
DLLEXPORT DV_ONLINE DVresult halShmemDestroyHandle(const char *name);
/**
* @ingroup driver
* @brief Configure the whitelist of nodes with ipc mem shared memory
* @attention Available online, not offline.
* @param [in] *name Name used for sharing between processes
* @param [in] pid host pid whitelist array
* @param [in] num number of pid arrays
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT DV_ONLINE DVresult halShmemSetPidHandle(const char *name, int pid[], int num);
/**
* @ingroup driver
* @brief Open the shared memory corresponding to name, vptr returns the virtual address that can access shared memory
* @attention Available online, not offline.
* @param [in] *name Name used for sharing between processes
* @param [out] *vptr Virtual address with access to shared memory
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : open fail
*/
DLLEXPORT DV_ONLINE DVresult halShmemOpenHandle(const char *name, DVdeviceptr *vptr);
/**
* @ingroup driver
* @brief Close the shared memory corresponding to name
* @attention Available online, not offline.
* @param [in] vptr The virtual address that halShmemOpenHandle can access to shared memory
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : close fail
*/
DLLEXPORT DV_ONLINE DVresult halShmemCloseHandle(DVdeviceptr vptr);
/**
* @ingroup driver
* @brief Get the properties of the virtual memory, if it is device memory, get the deviceID at the same time
* @attention Available online, not offline.
* @param [in] vptr  Virtual address to be queried
* @param [out] *attr  vptr property information corresponding to the page
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : get fail
*/
DLLEXPORT DV_ONLINE DVresult drvMemGetAttribute(DVdeviceptr vptr, struct DVattribute *attr);
/**
* @ingroup driver
* @brief Device mounts memory daemon background thread
* @attention Called by matrix after device os starts
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : setup fail
*/
DLLEXPORT DV_ONLINE int devmm_daemon_setup(void);
/**
* @ingroup driver
* @brief Open the memory management module interface and initialize related information
* @attention null
* @param [in] devid  Device id
* @param [in] devfd  Device file handle
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : open fail
*/
DLLEXPORT DV_ONLINE int drvMemDeviceOpen(uint32_t devid, int devfd);
/**
* @ingroup driver
* @brief Close the memory management module interface
* @attention Used with drvMemDeviceOpen.
* @param [in] devid  Device id
* @return DRV_ERROR_NONE  success
* @return DV_ERROR_XXX  close fail
*/
DLLEXPORT DV_ONLINE int drvMemDeviceClose(uint32_t devid);
/**
* @ingroup driver
* @brief Applying for the Memory with the Execute Permission
* @attention Currently, this interface can be used only for the memory applied by the ion
* @param [in] bytesize Requested byte size
* @param [out] **pp  Level-2 pointer that stores the address of the allocated memory pointer
* @return DRV_ERROR_NONE  success
* @return DV_ERROR_XXX  alloc fail
*/
DLLEXPORT DV_ONLINE DVresult drvMemAllocProgram(void **pp, size_t bytesize);

/**
* @ingroup driver
* @brief This command is used to register src share memory.
* @attention To munmap the registered memory, you should unregister it before.
* @param [in] *srcPtr Requested the src share memory pointer, srcPtr must be page aligned.
* @param [in] size Requested byte size.
* @param [in] flag  Requested memory parameter, the type in enum drvRegisterTpye.
* @param [in] devid  Requested input device id when flag is't DEV_SVM_MAP_HOST.
* @param [out] **dstPtr Level-2 pointer that stores the address of the allocated dst memory pointer.
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT drvError_t halHostRegister(void *srcPtr, UINT64 size, UINT32 flag, UINT32 devid, void **dstPtr);

/**
* @ingroup driver
* @brief This command is used to unregister src share memory.
* @attention null
* @param [in] *srcPtr Requested the src share memory pointer.
* @param [in] devid  Requested input device id when flag is HOST_MEM_MAP_DEV.
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT drvError_t halHostUnregister(void *srcPtr, UINT32 devid);

/**
* @ingroup driver
* @brief This command is used to alloc memory.
* @attention
* 1. When the application phy_mem_type is HBM and no HBM is available on the device side, this command allocates
*    memory from the DDR.
* 2. When advise_4g is true, user needs to set adivse_continuty true at the same time. Besides, alloc continuty
*    physics memory may fail if the system is fragmented seriously. User needs to handle the failure scenario.
* 3. When the virt_mem_type is DVPP, ignore the advise_4g and adivse_continuty flags, and will return DVPP memory
*    directly.
* @param [in] size Requested byte size.
* @param [in] flag  Requested memory parameter.
* @param [out] **pp  Level-2 pointer that stores the address of the allocated memory pointer.
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT drvError_t halMemAlloc(void **pp, unsigned long long size, unsigned long long flag);

/**
* @ingroup driver
* @brief This command is used to release memory resources.
* @attention The memory may not be reclaimed because of the memory caching mechanism.
* @param [in] pp Pointer to the memory space to be released.
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT drvError_t halMemFree(void *pp);

/**
* @ingroup driver
* @brief This command is used to advise memory.
* @attention When advise continuty virtual memory to different devices, only support the devices in same os.
* @param [in] *ptr Requested the svm memory pointer, ptr must be page aligned.
* @param [in] count Requested byte size.
* @param [in] type Requested advise type parameter.
* @param [in] device Requested input device id.
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT drvError_t halMemAdvise(DVdeviceptr ptr, size_t count, unsigned int type, DVdevice device);

struct MemPhyInfo {
#ifndef __linux
    unsigned long long total;
    unsigned long long free;
    unsigned long long huge_total;
    unsigned long long huge_free;
#else
    unsigned long total;        /* normal page total size */
    unsigned long free;         /* normal page free size */
    unsigned long huge_total;   /* huge page total size */
    unsigned long huge_free;    /* huge page free size */
#endif
};

struct MemAddrInfo {
    DVdeviceptr** addr;
    unsigned int cnt;
    unsigned int mem_type;
    unsigned int flag;
};

struct MemInfo {
    union {
        struct MemPhyInfo phy_info;
        struct MemAddrInfo addr_info;
    };
};

typedef drvError_t hdcError_t;
typedef void *HDC_CLIENT;
typedef void *HDC_SESSION;
typedef void *HDC_SERVER;

/**
 * @ingroup driver
 * @brief get device memory info
 * @attention For offline scenarios, return success.
 * @param [in] device: device id
 * @param [in] type: command type
 * @param [in/out] info: memory info
 * @return  0 for success, others for fail
 */
DLLEXPORT DVresult halMemGetInfoEx(DVdevice device, unsigned int type, struct MemInfo *info);

/**
* @ingroup driver
* @brief This command is used to get memory information.
* @attention null
* @param [in] device Requested input device id.
* @param [in] type Requested input memory type.
* @param [out] *info memory information.
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT DVresult halMemGetInfo(DVdevice device, unsigned int type, struct MemInfo *info);

/**
* @ingroup driver
* @brief This command is used to control memory.
* @attention null
* @param [in] type Requested input memory type.
* @param [in] *param_value Requested input param value pointer.
* @param [in] param_value_size Requested input param value size.
* @param [out] *out_value the pointer of output value.
* @param [out] *out_size_ret the pointer of output value size.
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : set fail
*/
DLLEXPORT drvError_t halMemCtl(int type, void *param_value, size_t param_value_size, void *out_value,
    size_t *out_size_ret) ASCEND_HAL_WEAK;

typedef void *HDC_EPOLL;

#define HDC_EPOLL_CTL_ADD 0
#define HDC_EPOLL_CTL_DEL 1

#define HDC_EPOLL_CONN_IN (0x1 << 0)
#define HDC_EPOLL_DATA_IN (0x1 << 1)
#define HDC_EPOLL_FAST_DATA_IN (0x1 << 2)
#define HDC_EPOLL_SESSION_CLOSE (0x1 << 3)

struct drvHdcEvent {
    unsigned int events;
    uintptr_t data;
};

#define RUN_ENV_UNKNOW 0
#define RUN_ENV_PHYSICAL 1
#define RUN_ENV_PHYSICAL_CONTAINER 2
#define RUN_ENV_VIRTUAL 3
#define RUN_ENV_VIRTUAL_CONTAINER 4

/**< The HDC interface is dead and blocked by default. Set HDC_FLAG_NOWAIT to be non-blocked */
/**< Set HDC_FLAG_WAIT_TIMEOUT to timeout after blocking for a period of time. HDC_FLAG_WAIT_TIMEOUT */
/**< takes precedence over HDC_FLAG_NOWAIT */
#define HDC_FLAG_NOWAIT (0x1 << 0)        /**< Occupy bit0 */
#define HDC_FLAG_WAIT_TIMEOUT (0x1 << 1)  /**< Occupy bit1 */
#define HDC_FLAG_MAP_VA32BIT (0x1 << 1)   /**< Use low 32bit memory */
#define HDC_FLAG_MAP_HUGE (0x1 << 2)      /**< Using large pages */

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

enum drvHdcServerAttr {
    HDC_SERVER_ATTR_DEV_ID = 0,
    HDC_SERVER_ATTR_MAX
};

enum drvHdcChanType {
    HDC_CHAN_TYPE_SOCKET = 0,
    HDC_CHAN_TYPE_PCIE,
    HDC_CHAN_TYPE_MAX
};

enum drvHdcMemType {
    HDC_MEM_TYPE_TX_DATA = 0,
    HDC_MEM_TYPE_TX_CTRL = 1,
    HDC_MEM_TYPE_RX_DATA = 2,
    HDC_MEM_TYPE_RX_CTRL = 3,
    HDC_MEM_TYPE_DVPP = 4,
    HDC_MEM_TYPE_MAX
};

#define HDC_SESSION_MEM_MAX_NUM 100

struct drvHdcFastSendMsg {
    unsigned long long srcDataAddr;
    unsigned long long dstDataAddr;
    unsigned long long srcCtrlAddr;
    unsigned long long dstCtrlAddr;
    unsigned int dataLen;
    unsigned int ctrlLen;
};

struct drvHdcFastRecvMsg {
    unsigned long long dataAddr;
    unsigned long long ctrlAddr;
    unsigned int dataLen;
    unsigned int ctrlLen;
};

struct drvHdcCapacity {
    enum drvHdcChanType chanType;
    unsigned int maxSegment;
};

struct drvHdcMsgBuf {
    char *pBuf;
    int len;
};

struct drvHdcMsg {
    int count;
    struct drvHdcMsgBuf bufList[0];
};

struct drvHdcProgInfo {
    char name[256];
    int progress;
    long long int send_bytes;
    long long int rate;
    int remain_time;
};

#define HDC_SESSION_INFO_RES_CNT 8

struct drvHdcSessionInfo {
    unsigned int devid;
    unsigned int fid;
    unsigned int res[HDC_SESSION_INFO_RES_CNT];
};

/**
* @ingroup driver
* @brief Before the HDC sends messages, you need to know the size of the sent packet and
* the channel type through this API.
* @attention null
* @param [out] *capacity : get the packet size and channel type currently supported by HDC
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t drvHdcGetCapacity(struct drvHdcCapacity *capacity);
/**
* @ingroup driver
* @brief Create an HDC client and initialize it based on the maximum number of sessions and service type.
* @attention null
* @param [in]  maxSessionNum : The maximum number of sessions currently required by Client
* @param [in]  serviceType : select service type
* @param [in]  flag : Reserved parameters, be fixed to 0
* @param [out] HDC_CLIENT *client : Created a good HDC Client pointer
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag);
/**
* @ingroup driver
* @brief Release HDC Client
* @attention null
* @param [in]  HDC_CLIENT client : HDC Client to be released
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcClientDestroy(HDC_CLIENT client);
/**
* @ingroup driver
* @brief Create HDC Session for Host and Device communication
* @attention null
* @param [in]  peer_node : The node number of the node where the Device is located. Currently only 1 node is supported.
* Remote nodes are not supported. You need to pass a fixed value of 0
* @param [in]  peer_devid : Device's uniform ID in the host (number in each node)
* @param [in]  HDC_CLIENT client : HDC Client handle corresponding to the newly created Session
* @param [out] HDC_SESSION *session : Created session
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session);
/**
* @ingroup driver
* @brief Create HDC Session for Host and Device communication
* @attention null
* @param [in]  peer_node : The node number of the node where the Device is located. Currently only 1 node is supported.
* Remote nodes are not supported. You need to pass a fixed value of 0
* @param [in]  peer_devid : Device's uniform ID in the host (number in each node)
* @param [in]  peer_pid : server's pid which you want to connect
* @param [in]  HDC_CLIENT client : HDC Client handle corresponding to the newly created Session
* @param [out] HDC_SESSION *session : Created session
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT hdcError_t halHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
    HDC_SESSION *pSession);

/**
* @ingroup driver
* @brief Create and initialize HDC Server
* @attention null
* @param [in]  devid : only support [0, 64)
* @param [in]  serviceType : select server type
* @param [out] HDC_SERVER *server : Created HDC server
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer);
/**
* @ingroup driver
* @brief Release HDC Server
* @attention null
* @param [in]  HDC_SERVER server : HDC server to be released
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcServerDestroy(HDC_SERVER server);
/**
* @ingroup driver
* @brief Open HDC Session for communication between Host and Device
* @attention null
* @param [in]  HDC_SERVER server     : HDC server to which the newly created session belongs
* @param [out] HDC_SESSION *session  : Created session
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session);
/**
* @ingroup driver
* @brief Close HDC Session for communication between Host and Device
* @attention null
* @param [in]  HDC_SESSION session : Specify in which session to receive data
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcSessionClose(HDC_SESSION session);
/**
* @ingroup driver
* @brief Apply for MSG descriptor for sending and receiving
* @attention The user applies for a message descriptor before sending and receiving data, and then releases the
* message descriptor after using it.
* @param [in]  HDC_SESSION session : Specify in which session to receive data
* @param [in]  count : Number of buffers in the message descriptor. Currently only one is supported
* @param [out] struct drvHdcMsg *ppMsg : Message descriptor pointer, used to store the send and receive buffer
* address and length
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count);
/**
* @ingroup driver
* @brief Release MSG descriptor for sending and receiving
* @attention The user applies for a message descriptor before sending and receiving data, and then releases
* the message descriptor after using it.
* @param [in]  struct drvHdcMsg *pMsg   :  Pointer to message descriptor to be released
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcFreeMsg(struct drvHdcMsg *msg);
/**
* @ingroup driver
* @brief Reuse MSG descriptor
* @attention This interface will clear the Buffer pointer in the message descriptor. For offline scenarios, Reuse
* will release the original Buffer. For online scenarios, Reuse will not release the original Buffer (the upper
* layer calls the device memory management interface on the Host to release it).
* @param [in]  struct drvHdcMsg *pMsg : The pointer of message need to Reuse
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcReuseMsg(struct drvHdcMsg *msg);
/**
* @ingroup driver
* @brief Add the receiving and sending buffer to the MSG descriptor
* @attention User applies for a message descriptor before sending and receiving data, and then releases the
* message descriptor after using it.
* @param [in]  struct drvHdcMsg *pMsg : The pointer of the message need to be operated
* @param [in]  char *pBuf : Buffer pointer to be added
* @param [in]  int len : The length of the effective data to be added
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len);
/**
* @ingroup driver
* @brief Add MSG descriptor to send buffer
* @attention null
* @param [in]  struct drvHdcMsg *pMsg : Pointer to the message descriptor to be manipulated
* @param [in]  int index              : The first several buffers need to be obtained, but currently only supports one,
* be fixed to 0
* @param [out] char **ppBuf           : Obtained Buffer pointer
* @param [out] int *pLen              : Length of valid data that can be obtained from the Buffer
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen);
/**
* @ingroup driver
* @brief Block and wait before sending data from the peer end and receive the length of the sent packet
* @attention null
* @param [in]  HDC_SESSION session : session
* @param [in]  int *msgLen         : Data length
* @param [in]  int flag            : Flag, 0 wait always, HDC_FLAG_NOWAIT non-blocking, HDC_FLAG_WAIT_TIMEOUT
* blocking timeout
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcRecvPeek(HDC_SESSION session, int *msgLen, int flag);
/**
* @ingroup driver
* @brief Receive data over normal channel, Save the received data to the upper layer buffer pBuf
* @attention null
* @param [in]  HDC_SESSION session : session
* @param [in]  char *pBuf     : Receive data buf
* @param [in]  int bufLen     : Received data buf length
* @param [out] int *msgLen    : Received data buf length
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcRecvBuf(HDC_SESSION session, char *pBuf, int bufLen, int *msgLen);
/**
* @ingroup driver
* @brief Set session and process affinity
* @attention If the interface is not called after the session is created, and an exception occurs in the process,
* HDC will not detect and release the corresponding
* session resources.
* @param [in]  HDC_SESSION session    :    Specified session
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcSetSessionReference(HDC_SESSION session);
/**
* @ingroup driver
* @brief Get the base trusted path sent to the specified node device, get trusted path, used to combine dst_path
* parameters of drvHdcSendFile
* @attention host call is valid, used to obtain the basic trusted path sent to the device side using
* the drvHdcSendFile interface
* @param [in]  int peer_node         	:	Node number of the node where the Device is located
* @param [in]  int peer_devid         :	Device's unified ID in the host
* @param [in]  unsigned int path_len	:	base_path space size
* @param [out] char *base_path		:	Obtained trusted path
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcGetTrustedBasePath(int peer_node, int peer_devid, char *base_path, unsigned int path_len);
/**
* @ingroup driver
* @brief Send file to the specified path on the specified device
* @attention Only files in the trustlist can be sent using this interface.
* @param [in]  int peer_node        :	Node number of the node where the Device is located
* @param [in]  int peer_devid       :	Device's unified ID in the host
* @param [in]  const char *file		:	Specify the file name of the sent file
* @param [in]  const char *dst_path	:	Specifies the path to send the file to the receiver. If the path is directory,
* the file name remains unchanged after it is sent to the peer; otherwise, the file name is changed to the part of the
* path after the file is sent to the receiver.
* @param [out] void (*progress_notifier)(struct drvHdcProgInfo *) :	  Specify the user's callback handler function;
* when progress of the file transfer increases by at least one percent,file transfer protocol will call this interface.
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcSendFile(int peer_node, int peer_devid, const char *file, const char *dst_path,
    void (*progress_notifier)(struct drvHdcProgInfo *));
/**
* @ingroup driver
* @brief Request to allocate memory
* @attention Call the kernel function to apply for physical memory. If the continuous physical memory is insufficient,
* it will fail. when HDC is used by DVPP, it can only use low 32-bit memory.
* @param [in]  enum drvHdcMemType mem_type  : Memory type, default is 0
* @param [in]  void * addr : Specifies the start address of the application, default is NULL
* @param [in]  unsigned int len : length
* @param [in]  unsigned int align  : The address returned by the application is aligned by align. Currently,
* only 4k is a common multiple
* @param [in]  unsigned int flag : Memory application flag. low 32-bit memory / hugepage / normal, only valid on the
* device side
* @param [in]  int devid : Device id
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT void *drvHdcMallocEx(enum drvHdcMemType mem_type, void *addr, unsigned int align, unsigned int len, int devid,
    unsigned int flag);
/**
* @ingroup driver
* @brief Release memory
* @attention null
* @param [in]  enum drvHdcMemType mem_type  : Memory type
* @param [in]  void *buf : Applied memory address
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcFreeEx(enum drvHdcMemType mem_type, void *buf);
/**
* @ingroup driver
* @brief Map DMA address
* @attention null
* @param [in]  enum drvHdcMemType mem_type   : Memory type
* @param [in]  void *buf : Applied memory address
* @param [in]  int devid : Device id
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcDmaMap(enum drvHdcMemType mem_type, void *buf, int devid);
/**
* @ingroup driver
* @brief UnMap DMA address
* @attention null
* @param [in]  enum drvHdcMemType mem_type   : Memory type
* @param [in]  void *buf : Applied memory address
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcDmaUnMap(enum drvHdcMemType mem_type, void *buf);
/**
* @ingroup driver
* @brief ReMap DMA address
* @attention null
* @param [in]  enum drvHdcMemType mem_type   : Memory type
* @param [in]  void *buf : Applied memory address
* @param [in]  int devid : Device id
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcDmaReMap(enum drvHdcMemType mem_type, void *buf, int devid);

/* hdc epoll func */
/**
* @ingroup driver
* @brief HDC epoll create interface
* @attention null
* @param [in]  int size    : Specify the number of file handles to listen on
* @param [out]  HDC_EPOLL *epoll : Returns the supervised epoll handle
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcEpollCreate(int size, HDC_EPOLL *epoll);
/**
* @ingroup driver
* @brief close HDC epoll interface
* @attention null
* @param [in]  HDC_EPOLL *epoll : Returns the supervised epoll handle
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcEpollClose(HDC_EPOLL epoll);
/**
* @ingroup driver
* @brief HDC epoll control interface
* @attention null
* @param [in]  HDC_EPOLL *epoll : Specify the created epoll handle
* @param [in]  int op : Listen event operation type
* @param [in]  void *target : Specify to add / remove resource topics
* @param [in]  struct drvHdcEvent *event : Used with target, HDC_EPOLL_CONN_IN Used with HDC_SERVER to monitor whether
* there is a new connection; HDC_EPOLL_DATA_IN Cooperate with HDC_SESSION to monitor data entry of normal channels
; HDC_EPOLL_FAST_DATA_IN Cooperate with HDC_SESSION to monitor fast channel data entry
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcEpollCtl(HDC_EPOLL epoll, int op, void *target, struct drvHdcEvent *event);
/**
* @ingroup driver
* @brief wait HDC epoll interface
* @attention null
* @param [in]  HDC_EPOLL *epoll : Specify the created epoll handle
* @param [in]  int maxevents : Specify the maximum number of events returned
* @param [in]  int timeout : Set timeout
* @param [out]  struct drvHdcEvent *events : Returns the triggered event
* @param [out]  int *eventnum : Returns the number of valid events
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t drvHdcEpollWait(HDC_EPOLL epoll, struct drvHdcEvent *events, int maxevents, int timeout,
                                     int *eventnum);
/**
* @ingroup driver
* @brief Get the information of the session.
* @attention null
* @param [in]  HDC_SESSION session : Specify in which session
* @param [out] struct drvHdcSessionInfo *info  : session info
* @param [out] info->devid  : session devid
* @param [out] info->fid  : session fid
* @param [out] info->res  : reserved
* @return DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t halHdcGetSessionInfo(HDC_SESSION session, struct drvHdcSessionInfo *info);
/**
* @ingroup driver
* @brief Send data based on HDC Session
* @attention This interface sends the message encapsulated with the buffer address and length to the peer end.
* @param [in]  HDC_SESSION session    : Specify in which session to send data
* @param [in]  struct drvHdcMsg *msg : Descriptor pointer for sending messages. The maximum sending length
* must be obtained through the drvHdcGetCapacity function
* @param [in]  u64 flag               : Reserved parameter, currently fixed 0
* @param [in]  unsigned int timeout   : Allow time for send timeout determined by user mode
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT hdcError_t halHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout);
/**
* @ingroup driver
* @brief Session zero-copy fast sending interface, applications need to apply for memory through "drvHdcMallocEx"
* in advance
* @attention After send function returns,src address cannot be reused directly. It must wait for peer to receive it.
* @param [in]  HDC_SESSION session    : Specify in which session
* @param [in]  msg : Send and receive information
* @param [in]  int flag : Fill in 0 default blocking, HDC_FLAG_NOWAIT set non-blocking
* @param [in]  unsigned int timeout   : Allow time for sending timeout determined by user mode
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT hdcError_t halHdcFastSend(HDC_SESSION session, struct drvHdcFastSendMsg msg, UINT64 flag, UINT32 timeout);
/**
* @ingroup driver
* @brief Receive data based on HDC Session
* @attention The interface will parse the message sent by the peer, obtain the data buffer address and length,
* save it in the message descriptor, and return it to the upper layer.
* @param [in]  HDC_SESSION session   : Specify in which session to receive data
* @param [in]  int bufLen            : The length of each receive buffer in bytes
* @param [in]  u64 flag              : Fixed 0
* @param [in]  unsigned int timeout   : Allow time for sending timeout determined by user mode
* @param [out] struct drvHdcMsg *msg : Descriptor pointer for receiving messages
* @param [out] int *recvBufCount      : The number of buffers that actually received the data
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT hdcError_t halHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    UINT64 flag, int *recvBufCount, UINT32 timeout);
/**
* @ingroup driver
* @brief Session copy-free fast sending interface, applications need to apply for memory through hdc in advance
* @attention Need to apply for memory through hdc in advance. And after the send function returns, the src address
* cannot be reused directly. It must wait for the peer to receive it.
* @param [in]  HDC_SESSION session    : Specify in which session
* @param [in]  msg : Send and receive information
* @param [in]  u64 flag : Fill in 0 default blocking, HDC_FLAG_NOWAIT set non-blocking
* @param [in]  unsigned int timeout   : Allow time for sending timeout determined by user mode
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT hdcError_t halHdcFastRecv(HDC_SESSION session, struct drvHdcFastRecvMsg *msg, UINT64 flag, UINT32 timeout);
/**
* @ingroup driver
* @brief Get the information of session
* @attention null
* @param [in]  HDC_SESSION session : Specify the session need to query
* @param [in]  int attr : Fill in information type
* @param [out] int *value : Returns information
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t halHdcGetSessionAttr(HDC_SESSION session, int attr, int *value);
/**
* @ingroup driver
* @brief Get the information of server
* @attention null
* @param [in]  HDC_SERVER server : Specify the server need to query
* @param [in]  int attr : Fill in information type
* @param [out] int *value : Returns information
* @return   DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT hdcError_t halHdcGetServerAttr(HDC_SERVER server, int attr, int *value);
/**
* @ingroup driver
* @brief set the state of progress
* @attention null
* @param [in]  int index : which index you want to set(0-1023)
* @param [in]  int value : the valve you want to set
* @return DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t halCentreNotifySet(int index, int value);
/**
* @ingroup driver
* @brief get the state of progress
* @attention null
* @param [in]  int index : which index you want to get(0-1023)
* @param [out] int *value  : the valve you want to get
* @return DRV_ERROR_NONE, DRV_ERROR_INVALID_VALUE
*/
DLLEXPORT drvError_t halCentreNotifyGet(int index, int *value);

/**
* @ingroup driver
* @brief get device gateway address command proc.
* @attention null
* @return 0 success
*/
DLLEXPORT int dsmi_cmd_get_network_device_info(int device_id, const char *inbuf, unsigned int size_in, char *outbuf,
                                               unsigned int *size_out);
enum log_error_code {
    LOG_OK = 0,
    LOG_ERROR = -1,
    LOG_NOT_READY = -2,
    LOG_NOT_SUPPORT = -5,
};

#define LOG_CHANNEL_TYPE_AICPU (10)
#define LOG_DEVICE_ID_MAX (64)
#define LOG_CHANNEL_NUM_MAX (64)
#define LOG_SLOG_BUF_MAX_SIZE (2 * 1024 * 1024)
#define LOG_DRIVER_NAME "log_drv"
enum log_channel_type {
    LOG_CHANNEL_TYPE_TS = 0,
    LOG_CHANNEL_TYPE_TS_DUMP = 1,
    LOG_CHANNEL_TYPE_LPM3 = 30,
    LOG_CHANNEL_TYPE_IMP = 31,
    LOG_CHANNEL_TYPE_IMU = 32,

    LOG_CHANNEL_TYPE_ISP = 33,

    LOG_CHANNEL_TYPE_SIS = 37,
    LOG_CHANNEL_TYPE_HSM = 38,
    LOG_CHANNEL_TYPE_MAX
};
int log_set_level(int device_id, int channel_type, unsigned int log_level);
int log_get_channel_type(int device_id, int *channel_type_set, int *channel_type_num, int set_size);
int log_get_device_id(int *device_id_set, int *device_id_num, int set_size);
int log_read(int device_id, char *buf, unsigned int *size, int timeout);

#ifndef dma_addr_t
typedef unsigned long long dma_addr_t;
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

#define CHANNEL_BIU_GROUP0_AIC (11)
#define CHANNEL_BIU_GROUP0_AIV0 (12)
#define CHANNEL_BIU_GROUP0_AIV1 (13)
#define CHANNEL_BIU_GROUP1_AIC (14)
#define CHANNEL_BIU_GROUP1_AIV0 (15)
#define CHANNEL_BIU_GROUP1_AIV1 (16)
#define CHANNEL_BIU_GROUP2_AIC (17)
#define CHANNEL_BIU_GROUP2_AIV0 (18)
#define CHANNEL_BIU_GROUP2_AIV1 (19)
#define CHANNEL_BIU_GROUP3_AIC (20)
#define CHANNEL_BIU_GROUP3_AIV0 (21)
#define CHANNEL_BIU_GROUP3_AIV1 (22)
#define CHANNEL_BIU_GROUP4_AIC (23)
#define CHANNEL_BIU_GROUP4_AIV0 (24)
#define CHANNEL_BIU_GROUP4_AIV1 (25)
#define CHANNEL_BIU_GROUP5_AIC (26)
#define CHANNEL_BIU_GROUP5_AIV0 (27)
#define CHANNEL_BIU_GROUP5_AIV1 (28)
#define CHANNEL_BIU_GROUP6_AIC (29)
#define CHANNEL_BIU_GROUP6_AIV0 (30)
#define CHANNEL_BIU_GROUP6_AIV1 (31)
#define CHANNEL_BIU_GROUP7_AIC (32)
#define CHANNEL_BIU_GROUP7_AIV0 (33)
#define CHANNEL_BIU_GROUP7_AIV1 (34)
#define CHANNEL_BIU_GROUP8_AIC (35)
#define CHANNEL_BIU_GROUP8_AIV0 (36)
#define CHANNEL_BIU_GROUP8_AIV1 (37)
#define CHANNEL_BIU_GROUP9_AIC (38)
#define CHANNEL_BIU_GROUP9_AIV0 (39)
#define CHANNEL_BIU_GROUP9_AIV1 (40)
#define CHANNEL_BIU_GROUP10_AIC (41)
#define CHANNEL_BIU_GROUP10_AIV0 (42)

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

#define CHANNEL_BIU_GROUP10_AIV1 (55)
#define CHANNEL_BIU_GROUP11_AIC (56)
#define CHANNEL_BIU_GROUP11_AIV0 (57)
#define CHANNEL_BIU_GROUP11_AIV1 (58)
#define CHANNEL_BIU_GROUP12_AIC (59)
#define CHANNEL_BIU_GROUP12_AIV0 (60)
#define CHANNEL_BIU_GROUP12_AIV1 (61)
#define CHANNEL_BIU_GROUP13_AIC (62)
#define CHANNEL_BIU_GROUP13_AIV0 (63)
#define CHANNEL_BIU_GROUP13_AIV1 (64)
#define CHANNEL_BIU_GROUP14_AIC (65)
#define CHANNEL_BIU_GROUP14_AIV0 (66)
#define CHANNEL_BIU_GROUP14_AIV1 (67)
#define CHANNEL_BIU_GROUP15_AIC (68)
#define CHANNEL_BIU_GROUP15_AIV0 (69)
#define CHANNEL_BIU_GROUP15_AIV1 (70)
#define CHANNEL_BIU_GROUP16_AIC (71)
#define CHANNEL_BIU_GROUP16_AIV0 (72)
#define CHANNEL_BIU_GROUP16_AIV1 (73)
#define CHANNEL_BIU_GROUP17_AIC (74)
#define CHANNEL_BIU_GROUP17_AIV0 (75)
#define CHANNEL_BIU_GROUP17_AIV1 (76)
#define CHANNEL_BIU_GROUP18_AIC (77)
#define CHANNEL_BIU_GROUP18_AIV0 (78)
#define CHANNEL_BIU_GROUP18_AIV1 (79)
#define CHANNEL_BIU_GROUP19_AIC (80)
#define CHANNEL_BIU_GROUP19_AIV0 (81)
#define CHANNEL_BIU_GROUP19_AIV1 (82)
#define CHANNEL_BIU_GROUP20_AIC (83)
#define CHANNEL_BIU_GROUP20_AIV0 (84)

#define CHANNEL_AIV (85)

#define CHANNEL_BIU_GROUP20_AIV1 (86)
#define CHANNEL_BIU_GROUP21_AIC (87)
#define CHANNEL_BIU_GROUP21_AIV0 (88)
#define CHANNEL_BIU_GROUP21_AIV1 (89)
#define CHANNEL_BIU_GROUP22_AIC (90)
#define CHANNEL_BIU_GROUP22_AIV0 (91)
#define CHANNEL_BIU_GROUP22_AIV1 (92)
#define CHANNEL_BIU_GROUP23_AIC (93)
#define CHANNEL_BIU_GROUP23_AIV0 (94)
#define CHANNEL_BIU_GROUP23_AIV1 (95)
#define CHANNEL_BIU_GROUP24_AIC (96)
#define CHANNEL_BIU_GROUP24_AIV0 (97)
#define CHANNEL_BIU_GROUP24_AIV1 (98)

#define CHANNEL_TSCPU_MAX (128)
#define CHANNEL_ROCE (129)
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

/* this struct = the one in "prof_drv_dev.h" */
typedef struct prof_poll_info {
    unsigned int device_id;
    unsigned int channel_id;
} prof_poll_info_t;

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

/**
* @ingroup driver
* @brief Trigger to get enable channels
* @attention null
* @param [in] device_id   device ID
* @param [in] channels user's channels list struct
* @return  0 for success, others for fail
*/
DLLEXPORT int prof_drv_get_channels(unsigned int device_id, channel_list_t *channels);

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

/**
* @ingroup driver
* @brief Trigger ts or peripheral devices to start preparing for sampling profile information
* @attention null
* @param [in] device_id   device ID
* @param [in] channel_id  Channel ID(CHANNEL_TSCPU--(CHANNEL_TSCPU_MAX - 1))
* @param [in] channel_type to use prof_tscpu_start or prof_peripheral_start interfaces.
* @param [in] real_time  Real-time mode or non-real-time mode
* @param [in] *file_path  path to save the file
* @param [in] *ts_cpu_data  TS related data buffer
* @param [in] data_size  ts related data length
* @return  0 for success, others for fail
*/
DLLEXPORT int prof_drv_start(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para);
/**
* @ingroup driver
* @brief Trigger Prof sample end
* @attention nul
* @param [in] dev_id  Device ID
* @param [in] channel_id  channel ID(1--(CHANNEL_NUM - 1))
* @return   0 for success, others for fail
*/
DLLEXPORT int prof_stop(unsigned int device_id, unsigned int channel_id);
/**
* @ingroup driver
* @brief Read and collect profile information
* @attention null
* @param [in] device_id  Device ID
* @param [in] channel_id  channel ID(1--(CHANNEL_NUM - 1))
* @param [in] *out_buf  Store read profile information
* @param [in] buf_size  Store the length of the profile to be read
* @return   0   success
* @return positive number for readable buffer length
* @return  -1 for fail
*/
DLLEXPORT int prof_channel_read(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size);
/**
* @ingroup driver
* @brief Querying valid channel information
* @attention null
* @param [in] *out_buf  User mode pointer
* @param [in] num  Number of channels to monitor
* @param [in] timeout  Timeout in seconds
* @return 0  No channels available
* @return positive number for channels Number
* @return -1 for fail
*/
DLLEXPORT int prof_channel_poll(struct prof_poll_info *out_buf, int num, int timeout);

/**
* @ingroup driver
* @brief flush data of a specified channel
* @attention null
* @param [in] device_id  Device ID
* @param [in] channel_id  channel ID(1--(CHANNEL_NUM - 1))
* @param [in] *data_len  Store the length of the profile to be read
* @return PROF_OK flush ok
* @return PROF_STOPPED_ALREADY means channel is stopped
* @return DRV_ERROR_NOT_SUPPORT for not support
*/
DLLEXPORT int halProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len);

/**
 * @ingroup driver
 * @brief ZIP MACRO
 */
#define HZIP_LEVEL_DEFAULT          0
#define HZIP_VERSION                "1.0.1"
#define HZIP_METHOD_DEFAULT         0
#define HZIP_WINDOWBITS_GZIP        16
#define HZIP_MEM_LEVEL_DEFAULT      0
#define HZIP_STRATEGY_DEFAULT       0
#define HZIP_FLUSH_TYPE_SYNC_FLUSH  2
#define HZIP_FLUSH_TYPE_FINISH      3
#define HZIP_OK                     0
#define HZIP_STREAM_END             1
#define HZIP_STREAM_NEED_AGAIN      2

/**
 * @ingroup driver
 * @brief zip stream param
 */
struct zip_stream {
    void            *next_in;   /**< next input byte */
    unsigned long   avail_in;   /**< number of bytes available at next_in */
    unsigned long   total_in;   /**< total nb of input bytes read so far */
    void            *next_out;  /**< next output byte should be put there */
    unsigned long   avail_out;  /**< remaining free space at next_out */
    unsigned long   total_out;  /**< total nb of bytes output so far */
    char            *msg;       /**< last error message, NULL if no error */
    void            *workspace; /**< memory allocated for this stream */
    int             data_type;  /**< the data type: ascii or binary */
    unsigned long   adler;      /**< adler32 value of the uncompressed data */
    void            *reserved;  /**< reserved for future use */
};

/**
 * @ingroup driver
 * @brief zlib deflate init
 * @attention null
 * @param [out] zstrm   zip stream
 * @param [in] level    HZIP_LEVEL_DEFAULT
 * @param [in] version  HZIP_VERSION
 * @param [in] stream_size  size of zstrm
 * @return   HZIP_OK   success
 * @return   other  fail
 */
DLLEXPORT int hw_deflateInit_(struct zip_stream *zstrm, int level, const char *version, int stream_size);

/**
 * @ingroup driver
 * @brief gzip deflate init
 * @attention null
 * @param [out] zstrm  zip stream
 * @param [in] level   HZIP_LEVEL_DEFAULT
 * @param [in] method  HZIP_METHOD_DEFAULT
 * @param [in] windowBits  HZIP_WINDOWBITS_GZIP
 * @param [in] memLevel HZIP_MEM_LEVEL_DEFAULT
 * @param [in] strategy HZIP_STRATEGY_DEFAULT
 * @param [in] version  HZIP_VERSION
 * @param [in] stream_size  size of zstrm
 * @return   HZIP_OK   success
 * @return   other  fail
 */
DLLEXPORT int hw_deflateInit2_(struct zip_stream *zstrm, int level, int method, int windowBits,
                               int memLevel, int strategy, const char *version, int stream_size);

/**
 * @ingroup driver
 * @brief deflat data
 * @attention null
 * @param [in] zstrm  zip stream
 * @param [in] flush  HZIP_FLUSH_TYPE_SYNC_FLUSH/HZIP_FLUSH_TYPE_FINISH
 * @return   HZIP_OK   success
 * @return   HZIP_STREAM_END   stream end
 * @return   HZIP_STREAM_NEED_AGAIN  need again
 * @return   other  fail
 */
DLLEXPORT int hw_deflate(struct zip_stream *zstrm, int flush);

/**
 * @ingroup driver
 * @brief deflate end
 * @attention null
 * @param [in] zstrm  zip stream
 * @return   HZIP_OK   sucess
 * @return   other  fail
 */
DLLEXPORT int hw_deflateEnd(struct zip_stream *zstrm);

/**
 * @ingroup driver
 * @brief zlib deflate init
 * @attention null
 * @param [out] zstrm  zip stream
 * @param [in] version  HZIP_VERSION
 * @param [in] stream_size  size of zstrm
 * @return   HZIP_OK   success
 * @return   other  fail
 */
DLLEXPORT int hw_inflateInit_(struct zip_stream *zstrm, const char *version, int stream_size);

/**
 * @ingroup driver
 * @brief gzip inflate init
 * @attention null
 * @param [out] zstrm  zip stream
 * @param [in] windowBits  HZIP_WINDOWBITS_GZIP
 * @param [in] version  HZIP_VERSION
 * @param [in] stream_size  size of zstrm
 * @return   HZIP_OK   success
 * @return   other  fail
 */
DLLEXPORT int hw_inflateInit2_(struct zip_stream *zstrm, int windowBits, const char *version, int stream_size);

/**
 * @ingroup driver
 * @brief inflate data
 * @attention null
 * @param [in] zstrm  zip stream
 * @param [in] flush  HZIP_FLUSH_TYPE_SYNC_FLUSH/HZIP_FLUSH_TYPE_FINISH
 * @return   HZIP_OK   success
 * @return   HZIP_STREAM_END   stream end
 * @return   HZIP_STREAM_NEED_AGAIN  need again
 * @return   other  fail
 */
DLLEXPORT int hw_inflate(struct zip_stream *zstrm, int flush);

/**
 * @ingroup driver
 * @brief inflate end
 * @attention null
 * @param [in] zstrm  zip stream
 * @return   HZIP_OK   sucess
 * @return   other  fail
 */
DLLEXPORT int hw_inflateEnd(struct zip_stream *zstrm);

/**
 * @ingroup driver
 * @brief Buffer MACRO
 */
#define BUFF_SP_NORMAL 0
#define BUFF_SP_HUGEPAGE_PRIOR (1 << 0)
#define BUFF_SP_HUGEPAGE_ONLY (1 << 1)
#define BUFF_SP_DVPP (1 << 2)
#define BUFF_SP_SVM (1 << 3)

#define BUFF_FLAGS_DEVID_OFFSET 32

/**
 * @ingroup driver
 * @brief Buffer Pool Statistics.
 */
struct buffPoolStat {
    pid_t owner;                         /**< pool create pid */
    unsigned int align;                  /**< pool addr align */
    unsigned int status;                 /**< pool status: normal 0, destroyed 1 */
    unsigned int totalBlkNum;            /**< total block num in pool */
    unsigned int freeBlkNum;             /**< free block num in pool */
    unsigned int blkSize;                /**< block size */
    unsigned int overTimeValue;          /**< overtime value of block in pool */
    unsigned int overTimeBlkNumSum;      /**< overtime block num accumulative total */
    unsigned int overTimeBlkNumCur;      /**< overtime block num current using */
    unsigned int maxBusyBlkNum;          /**< max block num in use */
    unsigned long maxBusyTime;           /**< max using time of block in pool */
    unsigned long allocFailCount;        /**< alloc fail num of block in pool */
    int reserve[BUFF_RESERVE_LEN];       /**< for reserve */
};

struct buff_cfg {
    unsigned int cfg_id;         /**< cfg id, 0~7 */
    unsigned int total_size;  /**< memzone total size, must below 256M */
    unsigned int blk_size;       /**< the size of each blk in this memzone  */
    unsigned int max_buf_size;   /**< max buff size could allocte, */
    unsigned int page_type;      /**< page type of memzone, could be PAGE_NORMAL or PAGE_HUGET_ONLY */
    int reserve[BUFF_RESERVE_LEN];   /**< for reserve */
};

/**
 * @ingroup driver
 * @brief Buffer Mbuf info.
 */
typedef struct mempool_t* poolHandle;//lint !e565
typedef struct Mbuf Mbuf;//lint !e565
typedef struct mp_attr {
    int devid;
    int mGroupId;
    unsigned int blkSize;
    unsigned int blkNum;
    unsigned int align;          /* must be power of 2 */
    unsigned long hugePageFlag; /* huge page support */
    char poolName[BUFF_POOL_NAME_LEN];
    int reserve[BUFF_RESERVE_LEN];
}mpAttr;

struct memzone_buff_cfg {
    unsigned int num;
    struct buff_cfg *cfg_info;
};

/**
* @ingroup driver
* @brief get phy addr of virtual addr buf
* @attention null
* @param [in] void *buf: virtual addr
* @param [out] unsigned long long *phyAddr: physical addr
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffGetPhyAddr(void *buf, unsigned long long *phyAddr);

/**
* @ingroup driver
* @brief alloc buff
* @attention null
* @param [in] unsigned int size: The amount of memory space requested
* @param [out] void **buff: buff addr alloced
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffAlloc(uint64_t size, void **buff);
/**
* @ingroup driver
* @brief alloc buff from pool
* @attention null
* @param [in] poolHandle pHandle: pool handle
* @param [out] void **buff: buff addr alloced
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffAllocByPool(poolHandle pHandle, void **buff);
/**
* @ingroup driver
* @brief free buff addr
* @attention null
* @param [in] *buff: buff addr to free
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffFree(void *buff);
/**
* @ingroup driver
* @brief create mempool
* @attention null
* @param [in] struct mp_attr *attr: mempool config info
* @param [out] struct mempool_t **mp: mempool alloced
* @return   0 for success, others for fail
*/

DLLEXPORT int halBuffCreatePool(struct mp_attr *attr, struct mempool_t **mp);
/**
* @ingroup driver
* @brief delete mempool
* @attention null
* @param [in] struct mempool_t *mp: mempool to delete
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffDeletePool(struct mempool_t *mp);

/**
* @ingroup driver
* @brief buff alloc interface
* @attention null
* @param [in] unsigned int size: size of buff to alloc
* @param [in] unsigned long flag: flag of buff to alloc(bit0~31: mem type, bit32~bit35: devid, bit36~63: resv)
* @param [in] int grp_id: group id num
* @param [out] buff **buff: buff alloced
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffAllocEx(uint64_t size, unsigned long flag, int grp_id, void **buff);

/**
* @ingroup driver
* @brief buff alloc interface
* @attention null
* @param [in] unsigned int size: size of buff to alloc
* @param [in] unsigned int align: align of buff to alloc
* @param [in] unsigned long flag: flag of buff to alloc(bit0~31: mem type, bit32~bit35: devid, bit36~63: resv)
* @param [in] int grp_id: group id
* @param [out] buff **buff: buff alloced
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffAllocAlignEx(uint64_t size, unsigned int align, unsigned long flag, int grp_id, void **buff);

/**
* @ingroup driver
* @brief Mbuf alloc interface
* @attention null
* @param [in] unsigned int size: size of Mbuf to alloc
* @param [out] Mbuf **mbuf: Mbuf alloced
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufAlloc(uint64_t size, Mbuf **mbuf) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief Mbuf alloc from Pool interface
* @attention null
* @param [in] poolHandle pHandle: pool handle
* @param [out] Mbuf **mbuf: Mbuf alloced
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufAllocByPool(poolHandle pHandle, Mbuf **mbuf);

/**
 * @ingroup driver
 * @brief Mbuf alloc interface
 * @attention null
 * @param [out] Mbuf **mbuf: Mbuf alloced
 * @param [in] unsigned int size: size of Mbuf to alloc
 * @param [in] unsigned int align: align of Mbuf to alloc(32~4096)
 * @param [in] unsigned long flag: huge page flag(bit0~31: mem type, bit32~bit35: devid, bit36~63: resv)
 * @param [in] int grp_id: group id
 * @return   0   success
 * @return   other  fail
 */
DLLEXPORT int halMbufAllocEx(uint64_t size, unsigned int align, unsigned long flag, int grp_id, Mbuf **mbuf);

/**
* @ingroup driver
* @brief Mbuf free interface
* @attention null
* @param [in] Mbuf *mbuf: Mbuf to free
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufFree(Mbuf *mbuf) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief verify mbuf is valid
* @attention null
* @param [in]  Mbuf *mbuf: mbuf need to verify
* @param [in]  int type: value 0 check single mbuf, value 1 check mbuf chain
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufVerify(Mbuf *mbuf, unsigned int type);

/**
* @ingroup driver
* @brief get Data addr of Mbuf
* @attention null
* @param [in] Mbuf *mbuf: Mbuf addr
* @param [out] void **buf: Mbuf data addr
* @param [out] unsigned int *size: size of the Mbuf data
* @return   0 for success, others for fail
*/
// Deprecated by 2021-09-17
DLLEXPORT int halMbufGetDataPtr(Mbuf *mbuf, void **buf, uint64_t *size);

/**
* @ingroup driver
* @brief get Data addr of Mbuf
* @attention null
* @param [in] Mbuf *mbuf: Mbuf addr
* @param [out] void **buf: Mbuf data addr
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufGetBuffAddr(Mbuf *mbuf, void **buf) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief get total Buffer size of Mbuf
* @attention null
* @param [in] Mbuf *mbuf: Mbuf addr
* @param [out] unsigned int *totalSize: total buffer size of Mbuf
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufGetBuffSize(Mbuf *mbuf, uint64_t *totalSize) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief set Data len of Mbuf
* @attention null
* @param [in] Mbuf *mbuf: Mbuf addr
* @param [in] unsigned int len: data len
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufSetDataLen(Mbuf *mbuf, uint64_t len);
/**
* @ingroup driver
* @brief get Data len of Mbuf
* @attention null
* @param [in] Mbuf *mbuf: Mbuf addr
* @param [out] unsigned int *len: len of the Mbuf data
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufGetDataLen(Mbuf *mbuf, uint64_t *len);
/**
* @ingroup driver
* @brief Apply for a Mbuf on the shared pool, and then assign the data area of
*        the source Mbuf to the newly applied Mbuf data area
* @attention null
* @param [in] Mbuf *mbuf: Mbuf addr
* @param [out] Mbuf **newMbuf: new Mbuf addr
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufCopyRef(Mbuf *mbuf, Mbuf **newMbuf);

/**
* @ingroup driver
* @brief Get the address and length of its user_data from the specified Mbuf
* @attention null
* @param [in] Mbuf *mbuf: Mbuf addr
* @param [out] void **priv: address of its user_data
* @param [out]  unsigned int *size: length of its user_data
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufGetPrivInfo (Mbuf *mbuf,  void **priv, unsigned int *size) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief create a Mbuf using a normal buff
* @attention null
* @param [in] void *buff: buff addr
* @param [in] unsigned int len: buff len
* @param [out] Mbuf **mbuf: new Mbuf addr
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufBuild(void *buff, uint64_t len, Mbuf **mbuf);

/**
* @ingroup driver
* @brief append mbuf to mbuf chain
* @attention null
* @param [inout] mbufChainHead, the mbuf chain head
* @param [out] mbuf, the mbuf to append
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufChainAppend(Mbuf *mbufChainHead, Mbuf *mbuf);

/**
* @ingroup driver
* @brief get mbuf num in mbuf chain
* @attention null
* @param [in] mbufChainHead, the mbuf chain head to free
* @param [out] num, the mbuf num
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufChainGetMbufNum(Mbuf *mbufChainHead, unsigned int *num);

/**
* @ingroup driver
* @brief get mbuf num in mbuf chain
* @attention null
* @param [in] mbufChainHead, the mbuf chain head to free
* @param [in] index, the mbuf index which to get in chain
* @param [out] mbuf, the mbuf to get
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufChainGetMbuf(Mbuf *mbufChainHead, unsigned int index, Mbuf **mbuf);

/**
* @ingroup driver
* @brief Subscribe event for memory pool size changes
* @attention the maximum subscription amount supported is 4
* @param [in] grpName, the share grp name of buff
* @param [in] threadGrpId, the subscription group to which the event belongs
* @param [in] event_id, 0~EVENT_MAX_NUM
* @param [in] devid, device id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halBufEventSubscribe(
    const char *grpName, unsigned int threadGrpId, unsigned int event_id, unsigned int devid) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief Event notification when memory pool size changes
* @attention null
* @param [in] grpName, the share grp name of buff
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halBufEventReport(const char *grpName);

/**
* @ingroup driver
* @brief add one process to a group, There are two forms, one is to generate the group ID,
* and the other is to specify the group ID.
* @attention null
* @param [in] GROUP_ID_TYPE type: GROUP_ID_CREATE or GROUP_ID_ADD
* @param [in] int pid: pid of the process
* @param [in/out] int *grp_id: As an input parameter when type is GROUP_ID_ADD and must be
* within [1,99999], as an output parameter when the type is GROUP_ID_CREATE.
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffGroupConfig(GROUP_ID_TYPE type, int pid, int *grp_id);

/**
* @ingroup driver
* @brief create proc grp
* @attention null
* @param [in] name, grp name
* @param [in] cfg, grp cfg
* @return   0 for success, others for fail
*/
DLLEXPORT int halGrpCreate(const char *name, GroupCfg *cfg) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief add process to grp
* @attention null
* @param [in] name, grp name
* @param [in] pid, process id
* @param [in] attr, process permission in grp
* @return   0 for success, others for fail
*/
DLLEXPORT int halGrpAddProc(const char *name, int pid, GroupShareAttr attr) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief attach proccess to check permission in grp
* @attention null
* @param [in] name, grp name
* @param [in] timeout, time out ms
* @return   0 for success, others for fail
*/
DLLEXPORT int halGrpAttach(const char *name, int timeout) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief buff grp query
* @attention null
* @param [in] cmd, cmd type
* @param [in] inBuff, query input buff
* @param [in] inLen, query input buff len
* @param [out] outBuff, query output buff
* @param [out] outLen, query output buff len
* @return   0 for success, others for fail
*/
DLLEXPORT int halGrpQuery(GroupQueryCmdType cmd,
    void *inBuff, unsigned int inLen, void *outBuff, unsigned int *outLen) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief buff init
* @attention null
* @param [in] cfg, init cfg
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffInit(BuffCfg *cfg) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief buff pub pool init
* @attention null
* @param [in] attr, pub pool init attr
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffInitPubPool(PubPoolAttr *attr) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief buff pub pool attach
* @attention null
* @param [in] timeout, pub pool attach timeout in (ms)
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffAttachPubPool(int timeout) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief buff pub pool alloc
* @attention null
* @param [in] size, buf size
* @param [out] mbuf, needed mbuf
* @return   0 for success, others for fail
*/
DLLEXPORT int halMbufAllocByPubPool(uint64_t size, Mbuf **mbuf) ASCEND_HAL_WEAK;

/**
 * @ingroup driver
 * @brief buffer config
 * @attention null
 * @param [in] enum BuffSetCmdType cmd, set cmd
 * @param [in] void*  data, data to set
 * @param [in] uint32 len, len of data
 * @return   0   success
 * @return   other  fail
 */
DLLEXPORT int halBuffCfg(enum BuffConfCmdType cmd, void *data, unsigned int len);

/**
 * @ingroup driver
 * @brief buffer get info
 * @attention null
 * @param [in] enum BuffGetCmdType cmd, get cmd
 * @param [in] void*  inBuff, para for get
 * @param [in] uint32 inLen, len of inBuff
 * @param [out] void*  outBuff, address to save get result
 * @param [inout] uint32 outLen, len of outLen
 * @return   0   success
 * @return   other  fail
 */
DLLEXPORT int halBuffGetInfo(enum BuffGetCmdType cmd, void *inBuff, unsigned int inLen,
    void *outBuff, unsigned int *outLen);

/**
* @ingroup driver
* @brief free buff by pid
* @attention null
* @param [in] int pid: pid of the process
* @return   0 for success, others for fail
*/
DLLEXPORT int halBuffRecycleByPid(int pid);

/*============================add from aicpufw_drv_msg.h"==========================================*/
struct hwts_ts_kernel {
    pid_t pid;
    unsigned short kernel_type : 8;
    unsigned short batchMode : 1; // default 0
    unsigned short satMode : 1;
    unsigned short rspMode : 1;
    unsigned short overFlowModeEnable : 1; // enable satMode, rspMode, default 0
    unsigned short resv : 4;
    unsigned short streamID;
    unsigned long long kernelName;
    unsigned long long kernelSo;
    unsigned long long paramBase;
    unsigned long long l2VaddrBase;
    unsigned long long l2Ctrl;
    unsigned short blockId;
    unsigned short blockNum;
    unsigned int l2InMain;
    unsigned long long taskID;
};

#define RESERVED_ARRAY_SIZE         11
typedef struct drv_hwts_task_response {
    volatile unsigned int valid;
    volatile unsigned int state;
    volatile unsigned long long serial_no;
    volatile unsigned int reserved[RESERVED_ARRAY_SIZE];
} drv_aicpufw_task_response_t;

/*=========================== New add below===========================*/
/* Not allow hwts_ts_task to be parameter passing in aicpufw and drv */
struct hwts_ts_task {
    unsigned int mailbox_id;
    volatile unsigned long long serial_no;
    struct hwts_ts_kernel kernel_info;
};

typedef enum hwts_task_status {
    TASK_SUCC = 0,
    TASK_FAIL = 1,
    TASK_OVERFLOW = 2,
    TASK_STATUS_MAX,
} HWTS_TASK_STATUS;

typedef struct hwts_response {
    unsigned int result;        /* RESPONSE_RESULE_E */
    unsigned int mailbox_id;
    unsigned long long serial_no;
    unsigned int status;
} hwts_response_t;

typedef enum tagDrvIdType {
    DRV_STREAM_ID = 0,
    DRV_EVENT_ID,
    DRV_MODEL_ID,
    DRV_NOTIFY_ID,
    DRV_CMO_ID,
    DRV_INVALID_ID,
} drvIdType_t;

#define RESOURCEID_RESV_LENGTH 8

struct halResourceIdInputInfo {
    drvIdType_t type;    // Resource Id Type
    uint32_t tsId;
    uint32_t resourceId; // the id that will be freed, halResourceIdAlloc does not care about this variable
    uint32_t res[RESOURCEID_RESV_LENGTH];
};

struct halResourceIdOutputInfo {
    uint32_t resourceId;
    uint32_t res[RESOURCEID_RESV_LENGTH];
};

/**
* @ingroup driver
* @brief  resource id alloc interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *in   See struct halResourceIdInputInfo
*              type: Resource Id Type
*              tsId: ts id,  ascend310:0, ascend910 :0
*              resourceId: not used
* @param [out] *out  See struct halResourceIdOutputInfo
*              resourceId: applied resource id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halResourceIdAlloc(uint32_t devId, struct halResourceIdInputInfo *in,
    struct halResourceIdOutputInfo *out);

/**
* @ingroup driver
* @brief  resource id free interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *in   See struct halResourceIdInputInfo
*              type: Resource Id Type
*              tsId: ts id,  ascend310:0, ascend910 :0
*              resourceId:  resource id will be freed
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halResourceIdFree(uint32_t devId, struct halResourceIdInputInfo *in);

/**
* @ingroup driver
* @brief  resource enable/disable interface, DRV_STREAM_ID surport
* @attention null
* @param [in] devId: logic devid
* @param [in]  *in   See struct halResourceIdInputInfo
*              type: Resource Id Type
*              tsId: ts id,  ascend310:0, ascend910 :0
*              resourceId:  resource id will be freed
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halResourceEnable(uint32_t devId, struct halResourceIdInputInfo *in) ASCEND_HAL_WEAK;
DLLEXPORT drvError_t halResourceDisable(uint32_t devId, struct halResourceIdInputInfo *in) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  tsdrv IO contrl interface
* @attention null
* @param [in] devId: logic devid
* @param [in] type: IO contrl type
              *param: parameter
              paramSize: parameter size
* @param [out] *out: out data
               *outSize: out data size
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halTsdrvCtl(uint32_t devId, int type, void *param, size_t paramSize, void *out, size_t *outSize);

#define SQCQ_RTS_INFO_LENGTH 5
#define SQCQ_RESV_LENGTH 8
#define SQCQ_UMAX 0xFFFFFFFF

typedef enum tagDrvSqCqType {
    DRV_NORMAL_TYPE = 0,
    DRV_CALLBACK_TYPE,
    DRV_LOGIC_TYPE,
    DRV_SHM_TYPE,
    DRV_INVALID_TYPE,
}  drvSqCqType_t;

struct halSqCqInputInfo {
    drvSqCqType_t type;  // normal : 0, callback : 1
    uint32_t tsId;
    /* The size and depth of each cqsq can be configured in normal mode, but this function is not yet supported */
    uint32_t sqeSize;    // normal : 64Byte
    uint32_t cqeSize;    // normal : 12Byte
    uint32_t sqeDepth;   // normal : 1024
    uint32_t cqeDepth;   // normal : 1024

    uint32_t grpId;   // runtime thread identifier,normal : 0
    uint32_t flag;    // ref to TSDRV_FLAG_*
    uint32_t cqId;    // if flag bit 0 is 0, don't care about it
    uint32_t sqId;    // if flag bit 1 is 0, don't care about it

    uint32_t info[SQCQ_RTS_INFO_LENGTH];  // inform to ts through the mailbox, consider single operator performance
    uint32_t res[SQCQ_RESV_LENGTH];
};

struct halSqCqOutputInfo {
    uint32_t sqId;  // return to UMAX when there is no sq
    uint32_t cqId;  // return to UMAX when there is cq
    uint64_t queueVAddr; /* return shm sq addr */
    uint32_t flag;    // ref to TSDRV_FLAG_*
    uint32_t res[SQCQ_RESV_LENGTH - 3];
};

struct halSqCqFreeInfo {
    drvSqCqType_t type; // normal : 0, callback : 1
    uint32_t tsId;
    uint32_t sqId;
    uint32_t cqId;  // cqId to be freed, if flag bit 0 is 0, don't care about it
    uint32_t flag;  // bit 0 : whether cq is to be freed  0 : free, 1 : no free
    uint32_t res[SQCQ_RESV_LENGTH];
};

#define SQCQ_CONFIG_INFO_LENGTH 8

typedef enum tagDrvSqCqPropType {
    DRV_SQCQ_PROP_SQ_STATUS = 0x0,
    DRV_SQCQ_PROP_SQ_HEAD,
    DRV_SQCQ_PROP_MAX
} drvSqCqPropType_t;

struct halSqCqConfigInfo {
    drvSqCqType_t type;
    uint32_t tsId;
    uint32_t sqId;
    uint32_t cqId;
    drvSqCqPropType_t prop;
    uint32_t value[SQCQ_CONFIG_INFO_LENGTH];
};

/**
* @ingroup driver
* @brief  SqCq alloc interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *in   See struct halSqCqInputInfo
* @param [out] *out  See struct halSqCqOutputInfo
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halSqCqAllocate(uint32_t devId, struct halSqCqInputInfo *in, struct halSqCqOutputInfo *out);
/**
* @ingroup driver
* @brief  SqCq alloc interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *info   See struct halSqCqFreeInfo
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halSqCqFree(uint32_t devId, struct halSqCqFreeInfo *info);

/**
* @ingroup driver
* @brief  SqCq alloc interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *info   See struct halSqCqFreeInfo
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halSqCqConfig(uint32_t devId, struct halSqCqConfigInfo *info) ASCEND_HAL_WEAK;

struct halSqMemGetInput {
    drvSqCqType_t type;   // 0-normal, 1-callback
    uint32_t tsId;        // ts id, ascend310 : 0, ascend910 : 0
    uint32_t sqId;
    uint32_t cmdCount;    // number of slot[1,1023] which will be alloced
    uint32_t res[SQCQ_RESV_LENGTH];
};

struct halSqMemGetOutput {
    uint32_t cmdCount;     // sq cmd slot number alloced actually
    volatile void *cmdPtr; // the first available cmd slot address
    uint32_t pos;          // sqe position
    uint32_t res[SQCQ_RESV_LENGTH - 1];
};

struct halSqMsgInfo {
    drvSqCqType_t type;    // 0-normal, 1-callback
    uint32_t tsId;         // ts id, ascend310 : 0, ascend910 : 0
    uint32_t sqId;
    uint32_t cmdCount;     // sq cmd slot number alloced actually
    uint32_t reportCount;  // cq report count
    uint32_t res[SQCQ_RESV_LENGTH];
};

struct halReportInfoInput {
    drvSqCqType_t type;   // 0-normal, 1-callback
    uint32_t grpId;       // runtime thread identifier, normal : 0
    uint32_t tsId;
    int32_t timeout;      // report irq wait time
    uint32_t res[SQCQ_RESV_LENGTH];
};

struct halReportInfoOutput {
    uint64_t *cqIdBitmap;    // output of callback module
    uint32_t cqIdBitmapSize; // output of callback module
    uint32_t res[SQCQ_RESV_LENGTH];
};

struct halReportGetInput {
    drvSqCqType_t type;  // 0-normal, 1-callback
    uint32_t tsId;
    uint32_t cqId;
    uint32_t res[SQCQ_RESV_LENGTH];
};

struct halReportGetOutput {
    uint32_t count;     // cq report count
    void *reportPtr;    // the first available report slot address
    uint32_t res[SQCQ_RESV_LENGTH];
};

struct halReportReleaseInfo {
    drvSqCqType_t type;  // 0-normal, 1-callback
    uint32_t tsId;
    uint32_t cqId;
    uint32_t count;
    uint32_t res[SQCQ_RESV_LENGTH];
};

/**
* @ingroup driver
* @brief  sq mem get interface
* @attention null
* @param [in] devId: logic devid
* @param [in] *in See struct halSqMemGetInput
* @param [out] *out See struct halSqMemGetOutput
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halSqMemGet(uint32_t devId, struct halSqMemGetInput *in, struct halSqMemGetOutput *out);
/**
* @ingroup driver
* @brief  sq mem send interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *info   See struct halSqMsgInfo
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halSqMsgSend(uint32_t devId, struct halSqMsgInfo *info);
/**
* @ingroup driver
* @brief  cq report irq wait interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *in   See struct halReportInfoInput
* @param [out]  *out  See struct halReportInfoInput
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halCqReportIrqWait(uint32_t devId, struct halReportInfoInput *in, struct halReportInfoOutput *out);
/**
* @ingroup driver
* @brief  cq report get interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *in   See struct halReportGetInput
* @param [out]  *out  See struct halReportGetOutput
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halCqReportGet(uint32_t devId, struct halReportGetInput *in, struct halReportGetOutput *out);
/**
* @ingroup driver
* @brief  report release interface
* @attention null
* @param [in] devId: logic devid
* @param [in]  *info   See struct halReportReleaseInfo
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halReportRelease(uint32_t devId, struct halReportReleaseInfo *info);
/**
* @ingroup driver
* @brief  ACL IO contrl interface
* @attention null
* @param [in] cmd: command
* @param [in]  param_value   param_value addr
* @param [in]  param_value_size   param_value size
* @param [out]  out_value   out_value addr
* @param [out]  out_value_size   out_value size
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halCtl(int cmd, void *param_value, size_t param_value_size, void *out_value, size_t *out_size_ret);

#define CDQ_NAME_LEN  64
#define CDQ_RESV_LEN 4
struct halCdqPara {
    char queName[CDQ_NAME_LEN];    /* name of cdq, length 32 */
    unsigned int batchNum;              /* number of batch in one CDQ */
    unsigned int batchSize;             /* number of CDE in each batch */
    DVdeviceptr queMemAddr;            /* Memory for create cdq */
    int resv[CDQ_RESV_LEN];
};

/**
* @ingroup driver
* @create circle data queue
* @param [in] devId  create on which device
* @param [in] tsId  tsId
* @param [in] cdqPara  parameter of Circle Data Queue
* @param [out] queId  cdq_Id created successed
* @return   0 for success, DRV_ERROR_NO_CDQ_RESOURCES means device full.
*/
DLLEXPORT drvError_t halCdqCreate(unsigned int devId, unsigned int tsId, struct halCdqPara *cdqPara,
    unsigned int *queId) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @destroy circle data queue
* @param [in] devId  create on which device
* @param [in] tsId  tsId
* @param [in] queId  cdq_Id created successed
* @return   0 for success, DRV_ERROR_CDQ_NOT_EXIST means no such cdq.
*/
DLLEXPORT drvError_t halCdqDestroy(unsigned int devId, unsigned int tsId, unsigned int queId) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @alloc free batch in data queue
* @param [in] devId  create on which device
* @param [in] tsId  tsId
* @param [in] queId  cdq_Id created successed
* @return   0 for success, DRV_ERROR_CDQ_ABNORMAL means cdq has batch timeout, DRV_ERROR_WAIT_TIMEOUT for wait timeout.
*/
DLLEXPORT drvError_t halCdqAllocBatch(unsigned int devId, unsigned int tsId, unsigned int queId, unsigned int timeout,
    unsigned int *batchId) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @get ready batch in a circle data queue
* @param [in] queName name of cdq
* @param [out] ready batchAddr
* @param [out] Size of batch
* @return   0 for success, DRV_ERROR_NO_CDQ_RESOURCES means cdq not ready.
*/
DLLEXPORT drvError_t halCdqGetReadyBatch(
    const char *queName, DVdeviceptr *batchAddr, unsigned int *batchSize) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @after get Ready batch and use data, free used batch in circle data queue
* @param [in] queName name of cdq
* @param [in] Addr of batch to be free
* @return 0 for success, others means fail.
*/
DLLEXPORT drvError_t halCdqFreeBatch(const char *queName, DVdeviceptr batchAddr) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @when CDQ is abnormal, such as can't ready for a long time, set CDQ abnormal status to notify another side
* @param [in] name of timeout error cdq
* @return 0 for success, others means fail.
*/
DLLEXPORT drvError_t halCdqSetAbnormal(const char *queName) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @query CDQ instance by cdqname, instance is globally unique to describe a cdq.
* @param [in] queName name of cdq
* @param [out] instance, qid in low 16 bits, tsid in 16~23bits, devid in 24~31bits
* @return 0 for success, others means fail.
*/
DLLEXPORT drvError_t halCdqGetInstance(const char *queName, unsigned int *instance) ASCEND_HAL_WEAK;


/*=========================== Event Sched ===========================*/
struct event_info_common {
    EVENT_ID event_id;
    unsigned int subevent_id;
    int pid;
    int host_pid;
    unsigned int grp_id;
    unsigned long long submit_timestamp; /* The timestamp when the Event is submitted */
    unsigned long long sched_timestamp; /* The timestamp when the event is scheduled */
};

struct event_info_priv {
    unsigned int msg_len;
    char msg[EVENT_MAX_MSG_LEN];
};

struct event_info {
    struct event_info_common comm;
    struct event_info_priv priv;
};

#define EVENT_SUMMARY_RSV 4

struct event_summary {
    int pid; /* dst PID */
    unsigned int grp_id;
    EVENT_ID event_id;
    unsigned int subevent_id;
    unsigned int msg_len;
    char *msg; /* sync event should point to struct event_sync_msg */
    unsigned int dst_engine; /* dst system cpu type, SCHEDULE_DST_ENGINE */
    SCHEDULE_POLICY policy;
    int rsv[EVENT_SUMMARY_RSV];
};

/**
* @ingroup driver
* @brief  attach one process to a device
* @attention null
* @param [in] devId: logic devid
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedAttachDevice(unsigned int devId) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  dettach one process from a device
* @attention null
* @param [in] devId: logic devid
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedDettachDevice(unsigned int devId) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  create a group for process
* @attention A process can create up to 32 groups
* @param [in] devId: logic devid
* @param [in] grpId: group id
* @param [in] GROUP_TYPE type: group type
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedCreateGrp(unsigned int devId, unsigned int grpId, GROUP_TYPE type) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  Subscribe events by thread
* @attention null
* @param [in] devId: logic devid
* @param [in] grpId: group id
* @param [in] threadId: thread id
* @param [in] eventBitmap: event bitmap
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedSubscribeEvent(unsigned int devId, unsigned int grpId,
    unsigned int threadId, unsigned long long eventBitmap) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  Sets the maximum number of events in a group, essentially setting the queue depth of events in a group.
* @attention null
* @param [in] devId: logic devid
* @param [in] grpId: group id
* @param [in] eventId: event id
* @param [in] maxNum: max event num
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedSetGrpEventQos(unsigned int devId, unsigned int grpId,
    EVENT_ID eventId, struct event_sched_grp_qos *qos);

/**
* @ingroup driver
* @brief  set the priority of pid
* @attention null
* @param [in] devId: logic devid
* @param [in] priority: pid priority
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedSetPidPriority(unsigned int devId, SCHEDULE_PRIORITY priority);
/**
* @ingroup driver
* @brief  set the priority of event
* @attention null
* @param [in] devId: logic devid
* @param [in] priority: event priority
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedSetEventPriority(unsigned int devId, EVENT_ID eventId, SCHEDULE_PRIORITY priority);

/**
* @ingroup driver
* @brief  set the event finish callback func
* @attention null
* @param [in] grpId: group id
* @param [in] eventId: event id
* @param [in] finishFunc: finish callback func
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedRegisterFinishFunc(unsigned int grpId, unsigned int event_id,
    void (*finishFunc)(unsigned int devId, unsigned int grpId, unsigned int event_id, unsigned int subevent_id));

/**
* @ingroup driver
* @brief  Export the latest scheduling trace
* @attention null
* @param [in] devId: logic devid
* @param [in] buff: input buff to store scheduling trace
* @param [in] buffLen: len of the input buff
* @param [out] *dataLen: real length of the scheduling trace
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedDumpEventTrace(unsigned int devId, char *buff,
    unsigned int buffLen, unsigned int *dataLen);
/**
* @ingroup driver
* @brief  trigger to record the latest scheduling trace
* @attention null
* @param [in] devId: logic devid
* @param [in] recordReason: reason to recore the event trace
* @param [in] key: Identifies the uniqueness of the track file
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedTraceRecord(unsigned int devId, const char *recordReason, const char *key);
/**
* @ingroup driver
* @brief  Commit the event to a specific process
* @attention null
* @param [in] devId: logic devid
* @param [in] event: event summary info
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedSubmitEvent(unsigned int devId, struct event_summary *event) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  Commit and wait the event to a specific process
* @attention null
* @param [in] devId: logic devid
* @param [in] event: event summary info
* @param [in] timeout: timeout value
* @param [out] reply: event reply info
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedSubmitEventSync(unsigned int devId,
    struct event_summary *event, int timeout, struct event_reply *reply) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  Wait for an event to be scheduled
* @attention null
* @param [in] devId: logic devid
* @param [in] grpId: group id
* @param [in] threadId: thread id
* @param [in] timeout: timeout value
* @param [out] event: The event that is scheduled
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedWaitEvent(unsigned int devId, unsigned int grpId,
    unsigned int threadId, int timeout, struct event_info *event) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  Actively retrieve an event
* @attention null
* @param [in] devId: logic devid
* @param [in] grpId: group id
* @param [in] threadId: thread id
* @param [in] eventId: event id
* @param [out] event: The event that is scheduled
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedGetEvent(unsigned int devId, unsigned int grpId, unsigned int threadId,
    EVENT_ID eventId, struct event_info *event);
/**
* @ingroup driver
* @brief  Respond to events
* @attention null
* @param [in] devId: logic devid
* @param [in] eventId: event id
* @param [in] subeventId: sub event id
* @param [in] msg: message info
* @param [in] msgLen: len of message
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedAckEvent(unsigned int devId, EVENT_ID eventId, unsigned int subeventId,
    char *msg, unsigned int msgLen) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  add a table entry
* @attention null
* @param [in] devId: logic devid
* @param [in] tableId: table id
* @param [in] key: entry key
* @param [in] entry: entry item
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedTableAddEntry(unsigned int devId, unsigned int tableId,
    struct esched_table_key *key, struct esched_table_entry *entry);

/**
* @ingroup driver
* @brief  del a table entry
* @attention null
* @param [in] devId: logic devid
* @param [in] tableId: table id
* @param [in] key: entry key
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halEschedTableDelEntry(unsigned int devId, unsigned int tableId, struct esched_table_key *key);

/*=========================== Queue Manage ===========================*/
#define QUEUE_MAX_STR_LEN        128
#define QUEUE_RESERVE_LEN 8

typedef enum queue_status {
    QUEUE_NORMAL = 0,
    QUEUE_EMPTY,
    QUEUE_FULL,
}QUEUE_STATUS;

typedef enum queue_work_mode {
    QUEUE_MODE_PUSH = 1,
    QUEUE_MODE_PULL,
}QUEUE_WORK_MODE;

typedef enum queue_type {
    QUEUE_TYPE_GROUP = 1,
    QUEUE_TYPE_SINGLE,
}QUEUE_TYPE;

typedef struct {
    unsigned long long enqueCnt;
    unsigned long long dequeCnt;
    unsigned long long enqueFailCnt;
    unsigned long long dequeFailCnt;
    unsigned long long enqueEventOk;
    unsigned long long enqueEventFail;
    unsigned long long f2nfEventOk;
    unsigned long long f2nfEventFail;
    struct timeval lastEnqueTime;
    struct timeval lastDequeTime;
    int reserve[QUEUE_RESERVE_LEN];
}QueueStat;

typedef struct {
    int id;
    char name[QUEUE_MAX_STR_LEN];
    int size;
    int depth;
    int status;
    int workMode;
    int type;
    int subGroupId;
    int subPid;
    int subF2NFGroupId;
    int subF2NFPid;
    void* headDataPtr;
    int reserve[QUEUE_RESERVE_LEN];
    QueueStat stat;
}QueueInfo;

typedef struct {
    char name[QUEUE_MAX_STR_LEN];
    unsigned int depth;
    unsigned int workMode;
    bool flowCtrlFlag;
    unsigned int flowCtrlDropTime;
    bool overWriteFlag;
    int resv[1];
}QueueAttr;

typedef struct {
    unsigned int *qids;
    unsigned int queNum;
    unsigned int reserve[QUEUE_RESERVE_LEN];
}QidsOfPid;

typedef struct {
    unsigned long long enqueDropCnt;
    unsigned long long dequeDropCnt;
}QUEUE_DROP_PKT_STAT;

typedef enum queue_query_item {
    QUERY_QUEUE_DEPTH,
    QUERY_QUEUE_STATUS,
    QUERY_QUEUE_DROP_STAT,
    QUERY_QUEUE_BUTTOM,
}QUEUE_QUERY_ITEM;

#define QUEUE_MAX_RESERV 5

struct QueueSubscriber {
    unsigned int devId;
    int pid;
    int spGrpId;
    int groupId;
    int rsv[QUEUE_MAX_RESERV];
};

typedef struct {
    unsigned int manage : 1;
    unsigned int read : 1;
    unsigned int write : 1;
    unsigned int rsv : 29;
} QueueShareAttr;

typedef struct {
    unsigned int devid;
    unsigned int qid;
    int pid;
    QueueShareAttr attr;
} QueueGrantPara;

typedef struct {
    unsigned int devid;
    unsigned int qid;
    int timeout;
} QueueAttachPara;

typedef enum {
    QUEUE_QUERY_QUE_ATTR_OF_CUR_PROC,
    QUEUE_QUERY_QUES_OF_CUR_PROC,
    QUEUE_QUERY_CMD_MAX,
} QueueQueryCmdType;

typedef struct {
    void *inBuff;
    unsigned int inLen;
}QueueQueryInputPara;

typedef struct {
    void *outBuff;
    unsigned int outLen;
}QueueQueryOutputPara;

typedef struct {
    int qid;
} QueQueryQueueAttr;

typedef union {
    QueQueryQueueAttr queQueryQueueAttr;
} QueueQueryInput;

typedef struct {
    QueueShareAttr attr;
} QueQueryQueueAttrInfo;

typedef struct {
    int qid;
    QueueShareAttr attr;
} QueQueryQuesOfProcInfo;

#define MAX_QUEUE_NUM 8192
typedef union {
    QueQueryQueueAttrInfo queQueryQueueAttrInfo;
    QueQueryQuesOfProcInfo queQueryQuesOfProcInfo[MAX_QUEUE_NUM];
} QueueQueryOutput;

typedef enum {
    QUEUE_SET_WORK_MODE,
    QUEUE_SET_CMD_MAX,
} QueueSetCmdType;

typedef struct {
    void *inBuff;
    unsigned int inLen;
} QueueSetInputPara;

typedef struct {
    unsigned int qid;
    unsigned int workMode;
} QueueSetWorkMode;

typedef union {
    QueueSetWorkMode queSetWorkMode;
} QueueSetInput;
/*lint +e116 +e17*/

/**
* @ingroup driver
* @brief  queue init
* @attention null
* @param [in] devId: logic devid
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueInit(unsigned int devId) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  create queue
* @attention null
* @param [in] devId: logic devid
* @param [in] queAttr: queue attribute
* @param [out] qid: queue id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueCreate(unsigned int devId, const QueueAttr *queAttr,
                unsigned int *qid) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  grant queue
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] pid: pid
* @param [in] attr: queue share attr
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueGrant(unsigned int devId, int qid, int pid, QueueShareAttr attr) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  attach queue
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] timeOut: timeOut
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueAttach(unsigned int devId, unsigned int qid, int timeOut) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  destroy queue
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueDestroy(unsigned int devId, unsigned int qid) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  enqueue
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] mbuf: enqueue mbuf
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueEnQueue(unsigned int devId, unsigned int qid, void *mbuf) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  dequeue
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [out] mbuf: dequeue to mbuf
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueDeQueue(unsigned int devId, unsigned int qid, void **mbuf) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  subscribe queue
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] groupId: queue group id
* @param [in] type: single or group
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueSubscribe(
    unsigned int devId, unsigned int qid, unsigned int groupId, int type) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  unsubscribe queue
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueUnsubscribe(unsigned int devId, unsigned int qid) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  sub full to not full event
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] groupId: queue group id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueSubF2NFEvent(unsigned int devId, unsigned int qid, unsigned int groupId) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  unsub full to not full event
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueUnsubF2NFEvent(unsigned int devId, unsigned int qid) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  get queue status
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] queryItem: query item
* @param [in] len: size of query result
* @param [out] data: query result
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueGetStatus(unsigned int devId, unsigned int qid, QUEUE_QUERY_ITEM queryItem,
    unsigned int len,  void *data) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  query queue info
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [out] queInfo: queue info
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueQueryInfo(unsigned int devId, unsigned int qid, QueueInfo *queInfo) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  get qid by name
* @attention null
* @param [in] devId: logic devid
* @param [in] name: queue name
* @param [out] qid: queue id
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueGetQidbyName(unsigned int devId, const char *name,
        unsigned int *qid) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  get qids by pid
* @attention null
* @param [in] devId: logic devid
* @param [in] pid: pid
* @param [in] maxQueSize: max number of return queue
* @param [inout] info: qids is an array for output result and queNum is the number of all qids
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueGetQidsbyPid(unsigned int devId, unsigned int pid,
       unsigned int maxQueSize, QidsOfPid *info) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @Pause or recover Enqueue event in same queue-group
* @param [in] struct QueueSubscriber subscriber: info of pause event subscriber
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueCtrlEvent(struct QueueSubscriber *subscriber, QUE_EVENT_CMD cmdType) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief get len of queue tail data
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] timeout: timeout value
* @param [out] buf_len: len of queue tail data
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueuePeek(
    unsigned int devId, unsigned int qid, uint64_t *buf_len, int timeout) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  enqueue buff vector
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] vector: see struct buff_iovec
* @param [in] timeout: timeout value
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueEnQueueBuff(unsigned int devId, unsigned int qid,
    struct buff_iovec *vector, int timeout) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  dequeue buff vector
* @attention null
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] timeout: timeout value
* @param [out] vector: see struct buff_iovec
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueDeQueueBuff(unsigned int devId, unsigned int qid,
    struct buff_iovec *vector, int timeout) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  queue event handle api
* @attention null
* @param [in] devId: logic devid
* @param [in] event: see struct event_info
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueEventProc(unsigned int devId, struct event_info *event) ASCEND_HAL_WEAK;
/**
* @ingroup driver
* @brief  query queue status
* @Pause or recover Enqueue event in same queue-group
* @param [in] devId: logic devid
* @param [in] cmd: query cmd
* @param [in] inBuff: query cmd
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueQuery(unsigned int devId, QueueQueryCmdType cmd,
    QueueQueryInputPara *inPut, QueueQueryOutputPara *outPut) ASCEND_HAL_WEAK;

/**
* @ingroup driver
* @brief  set queue workmode
* @attention null
* @param [in] devId: logic devid
* @param [in] cmd: set cmd type
* @param [in] ipPut: cmd related parameters
* @return   0 for success, others for fail
*/
DLLEXPORT drvError_t halQueueSet(unsigned int devId, QueueSetCmdType cmd, QueueSetInputPara *input);

#define DRV_NOTIFY_TYPE_TOTAL_SIZE 1
#define DRV_NOTIFY_TYPE_NUM 2
/**
 * @ingroup driver
 * @brief get notify num or memory size
 * @attention This function is only can be called by components in driver of device,
 *  if the components is not in driver of device, don't use this function.
 * @param [in] devId logic_id
 * @param [in] tsId: ascend910 tsid is 0
 * @param [in] type: get notify info type
 * @param [out] val: return corresponding value according to type
 * @return  0  success if chip type is ascend910 , return others fail
 * @return  0xfffe means not supported
 */
drvError_t halNotifyGetInfo(uint32_t devId, uint32_t tsId, uint32_t type, uint32_t *val);

/**
* @ingroup driver
* @brief Get the number of chips
* @attention NULL
* @param [out] chip_count  The space requested by the user is used to store the number of returned chips
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t __attribute__((weak)) halGetChipCount(int *chip_count);

/**
* @ingroup driver
* @brief Get the id of all chips
* @attention NULL
* @param [out] count The space requested by the user is used to store the id of all returned chips
* @param [in] count Number of chip equipment
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t __attribute__((weak)) halGetChipList(int chip_list[], int count);

/**
* @ingroup driver
* @brief Get the device number of one chip
* @attention NULL
* @param [in] chip_id  The chip id
* @param [out] device_count  The space requested by the user is used to store the number of returned devices
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t __attribute__((weak)) halGetDeviceCountFromChip(int chip_id, int *device_count);

/**
* @ingroup driver
* @brief Get the id of all devices of one chip
* @attention NULL
* @param [in] chip_id  The chip id
* @param [out] count The space requested by the user is used to store the id of all returned devices of one chip
* @param [in] count Number of equipment
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t __attribute__((weak)) halGetDeviceFromChip(int chip_id, int device_list[], int count);

/**
* @ingroup driver
* @brief Get the id of chip of one device
* @attention NULL
* @param [in] device_id  the device id
* @param [out] the chip id
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t __attribute__((weak)) halGetChipFromDevice(int device_id, int *chip_id);

/**
* @ingroup driver
* @brief Get the id of chip of one device
* @attention NULL
* @param [in] devID  the device id
* @param [out] chipInfo chip name/type/version information
* @return  0 for success, others for fail
*/
DLLEXPORT drvError_t halGetChipInfo(unsigned int devId, halChipInfo *chipInfo);

#ifdef __cplusplus
}
#endif
#endif

