/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description:
 * Author: huawei
 * Create: 2020-8-27
 */
#ifndef __ASCEND_HAL_DEFINE_H__
#define __ASCEND_HAL_DEFINE_H__

#include "ascend_hal_error.h"

/*========================== Queue Manage ===========================*/
#define DRV_ERROR_QUEUE_INNER_ERROR  DRV_ERROR_INNER_ERR             /**< queue error code */
#define DRV_ERROR_QUEUE_PARA_ERROR  DRV_ERROR_PARA_ERROR
#define DRV_ERROR_QUEUE_OUT_OF_MEM  DRV_ERROR_OUT_OF_MEMORY
#define DRV_ERROR_QUEUE_NOT_INIT  DRV_ERROR_UNINIT
#define DRV_ERROR_QUEUE_OUT_OF_SIZE  DRV_ERROR_OVER_LIMIT
#define DRV_ERROR_QUEUE_REPEEATED_INIT  DRV_ERROR_REPEATED_INIT
#define DRV_ERROR_QUEUE_IOCTL_FAIL  DRV_ERROR_IOCRL_FAIL
#define DRV_ERROR_QUEUE_NOT_CREATED  DRV_ERROR_NOT_EXIST
#define DRV_ERROR_QUEUE_RE_SUBSCRIBED  DRV_ERROR_REPEATED_SUBSCRIBED
#define DRV_ERROR_QUEUE_MULIPLE_ENTRY  DRV_ERROR_BUSY
#define DRV_ERROR_QUEUE_NULL_POINTER  DRV_ERROR_INVALID_HANDLE

/*=========================== Event Sched ===========================*/
#define EVENT_MAX_MSG_LEN  128  /* Maximum message length */
#define DRV_ERROR_SCHED_INNER_ERR  DRV_ERROR_INNER_ERR                   /**< event sched add error*/
#define DRV_ERROR_SCHED_PARA_ERR DRV_ERROR_PARA_ERROR
#define DRV_ERROR_SCHED_OUT_OF_MEM  DRV_ERROR_OUT_OF_MEMORY
#define DRV_ERROR_SCHED_UNINIT  DRV_ERROR_UNINIT
#define DRV_ERROR_SCHED_NO_PROCESS DRV_ERROR_NO_PROCESS
#define DRV_ERROR_SCHED_PROCESS_EXIT DRV_ERROR_PROCESS_EXIT
#define DRV_ERROR_SCHED_NO_SUBSCRIBE_THREAD DRV_ERROR_NO_SUBSCRIBE_THREAD
#define DRV_ERROR_SCHED_NON_SCHED_GRP_MUL_THREAD DRV_ERROR_NON_SCHED_GRP_MUL_THREAD
#define DRV_ERROR_SCHED_GRP_INVALID DRV_ERROR_NO_GROUP
#define DRV_ERROR_SCHED_PUBLISH_QUE_FULL DRV_ERROR_QUEUE_FULL
#define DRV_ERROR_SCHED_NO_GRP DRV_ERROR_NO_GROUP
#define DRV_ERROR_SCHED_GRP_EXIT DRV_ERROR_GROUP_EXIST
#define DRV_ERROR_SCHED_THREAD_EXCEEDS_SPEC DRV_ERROR_THREAD_EXCEEDS_SPEC
#define DRV_ERROR_SCHED_RUN_IN_ILLEGAL_CPU DRV_ERROR_RUN_IN_ILLEGAL_CPU
#define DRV_ERROR_SCHED_WAIT_TIMEOUT  DRV_ERROR_WAIT_TIMEOUT
#define DRV_ERROR_SCHED_WAIT_FAILED   DRV_ERROR_INNER_ERR
#define DRV_ERROR_SCHED_WAIT_INTERRUPT DRV_ERROR_WAIT_INTERRUPT
#define DRV_ERROR_SCHED_THREAD_NOT_RUNNIG DRV_ERROR_THREAD_NOT_RUNNIG
#define DRV_ERROR_SCHED_PROCESS_NOT_MATCH DRV_ERROR_PROCESS_NOT_MATCH
#define DRV_ERROR_SCHED_EVENT_NOT_MATCH DRV_ERROR_EVENT_NOT_MATCH
#define DRV_ERROR_SCHED_PROCESS_REPEAT_ADD DRV_ERROR_PROCESS_REPEAT_ADD
#define DRV_ERROR_SCHED_GRP_NON_SCHED DRV_ERROR_GROUP_NON_SCHED
#define DRV_ERROR_SCHED_NO_EVENT DRV_ERROR_NO_EVENT
#define DRV_ERROR_SCHED_COPY_USER DRV_ERROR_COPY_USER_FAIL
#define DRV_ERROR_SCHED_SUBSCRIBE_THREAD_TIMEOUT DRV_ERROR_SUBSCRIBE_THREAD_TIMEOUT

/*lint -e116 -e17*/
typedef enum group_type {
    /* Bound to a AICPU, multiple threads can be woken up simultaneously within a group */
    GRP_TYPE_BIND_DP_CPU = 1,
    GRP_TYPE_BIND_CP_CPU,             /* Bind to the control CPU */
    GRP_TYPE_BIND_DP_CPU_EXCLUSIVE    /* Bound to a AICPU, intra-group threads are mutex awakened */
} GROUP_TYPE;

/* Events can be released between different systems. This parameter specifies the destination type of events
   to be released. The destination type is defined based on the CPU type of the destination system. */
typedef enum schedule_dst_engine {
    ACPU_DEVICE = 0,
    ACPU_HOST = 1,
    CCPU_DEVICE = 2,
    CCPU_HOST = 3,
    DCPU_DEVICE = 4,
    TS_CPU = 5,
    DVPP_CPU = 6,
    ACPU_LOCAL = 7,
    CCPU_LOCAL = 8,
    DST_ENGINE_MAX,
} SCHEDULE_DST_ENGINE;

/* When the destination engine is AICPU, select a policy.
   ONLY: The command is executed only on the local AICPU.
   FIRST: The local AICPU is preferentially executed. If the local AICPU is busy, the remote AICPU can be used. */
typedef enum schedule_policy {
    ONLY = 0,
    FIRST = 1,
    POLICY_MAX,
} SCHEDULE_POLICY;

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

typedef enum drv_queue_subevent_id {
    DRV_SUBEVENT_INIT_MSG,
    DRV_SUBEVENT_CREATE_MSG,
    DRV_SUBEVENT_GRANT_MSG,
    DRV_SUBEVENT_ATTACH_MSG,
    DRV_SUBEVENT_DESTROY_MSG,
    DRV_SUBEVENT_SUBE2NE_MSG,
    DRV_SUBEVENT_UNSUBE2NE_MSG,
    DRV_SUBEVENT_SUBF2NF_MSG,
    DRV_SUBEVENT_UNSUBF2NF_MSG,
    DRV_SUBEVENT_QUERY_MSG,
    DRV_SUBEVENT_PEEK_MSG,
    DRV_SUBEVENT_ENQUEUE_MSG,
    DRV_SUBEVENT_DEQUEUE_MSG,
    DRV_SUBEVENT_QUEUE_MAX_NUM
} DRV_QUEUE_SUBEVENT_ID;

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

struct event_sched_grp_qos {
    unsigned int maxNum;
    unsigned int rsv[7]; /* rsv 7 int */
};

struct event_sync_msg {
    int pid; /* local pid */
    unsigned int dst_engine : 4; /* local engine */
    unsigned int gid : 6;
    unsigned int event_id : 6;
    unsigned int subevent_id : 16;
    char msg[0];
};

#define EVENT_PROC_RSP_LEN 32
struct event_proc_result {
    int ret;
    char data[EVENT_PROC_RSP_LEN];
};

struct event_reply {
    char *buf;
    unsigned int buf_len;
    unsigned int reply_len;
};

struct iovec_info {
    void *iovec_base;
    unsigned long long len;
};

struct buff_iovec {
    void *context_base;
    unsigned long long context_len;
    unsigned int count;
    struct iovec_info ptr[0];
};

struct callback_event_info {
    unsigned int cqid : 16;
    unsigned int cb_groupid : 16;
    unsigned int devid : 16;
    unsigned int stream_id : 16;
    unsigned int event_id : 16;
    unsigned int is_block : 16;
    unsigned int res1;
    unsigned int host_func_low;
    unsigned int host_func_high;
    unsigned int fn_data_low;
    unsigned int fn_data_high;
    unsigned int res2;
    unsigned int res3;
};

enum esched_table_op_type {
    ESCHED_TABLE_OP_SEND_EVENT, /* send a event */
    ESCHED_TABLE_OP_NEXT_TABLE, /* continue query next table */
    ESCHED_TABLE_OP_DROP, /* drop */
    ESCHED_TABLE_OP_MAX
};

enum esched_data_src_type {
    ESCHED_DATA_SRC_NONE = 0,
    ESCHED_DATA_SRC_RAW_DATA = 1,
    ESCHED_DATA_SRC_KEY = 2,
    ESCHED_DATA_SRC_USR_CFG = 3,
    ESCHED_DATA_SRC_MAX
};

#define ESCHED_USR_CFG_DATA_MAX_LEN 32
struct esched_table_op_send_event {
    unsigned int dev_id;
    unsigned int dst_engine;
    unsigned int policy;
    unsigned int gid;
    unsigned int event_id;
    unsigned int sub_event_id;
    enum esched_data_src_type data_src;
    unsigned char data[ESCHED_USR_CFG_DATA_MAX_LEN];
    unsigned int data_len;
};

struct esched_table_op_next_table {
    unsigned int dev_id;
    unsigned int table_id;
};

struct esched_table_entry {
    enum esched_table_op_type op;
    union {
        struct esched_table_op_send_event send_event;
        struct esched_table_op_next_table next_table;
    };
};

/* Keys are aligned by bytes. Key_len is less than or equal to cqe_size.
  If the key is less than one byte, zeros are added to the high bits. */
struct esched_table_key {
    unsigned char *key;
    unsigned int key_len;
};

/*=========================== Queue Manage ===========================*/

typedef enum QueEventCmd {
    QUE_PAUSE_EVENT = 1,    /* pause enque event publish in group */
    QUE_RESUME_EVENT        /* resume enque event publish */
} QUE_EVENT_CMD;

/*=========================== Buffer Manage ===========================*/

#define UNI_ALIGN_MAX       4096
#define UNI_ALIGN_MIN       32
#define BUFF_POOL_NAME_LEN 128
#define BUFF_GRP_NAME_LEN 32
#define BUFF_RESERVE_LEN 8

#define BUFF_GRP_MAX_NUM 1024
#define BUFF_PROC_IN_GRP_MAX_NUM 1024
#define BUFF_GRP_IN_PROC_MAX_NUM 32
#define BUFF_PUB_POOL_CFG_MAX_NUM 128

#define XSMEM_BLK_NOT_AUTO_RECYCLE (1UL << 63)

typedef enum group_id_type
{
    GROUP_ID_CREATE,
    GROUP_ID_ADD
} GROUP_ID_TYPE;

typedef struct {
    unsigned long long maxMemSize; /* max buf size in grp, in KB, if = 0 means no limit */
    int rsv[BUFF_GRP_NAME_LEN];  /* reserve */
} GroupCfg;

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
    char grpName[BUFF_GRP_NAME_LEN];
} GrpQueryGroup; /* cmd: GRP_QUERY_GROUP */

typedef struct {
    int pid;
} GrpQueryGroupsOfProc; /* cmd: GRP_QUERY_GROUPS_OF_PROCESS */

typedef struct {
    char grpName[BUFF_GRP_NAME_LEN];
} GrpQueryGroupId; /* cmd: GRP_QUERY_GROUP_ID */

typedef union {
    GrpQueryGroup grpQueryGroup; /* cmd: GRP_QUERY_GROUP */
    GrpQueryGroupsOfProc grpQueryGroupsOfProc; /* cmd: GRP_QUERY_GROUPS_OF_PROCESS */
    GrpQueryGroupId grpQueryGroupId; /* cmd: GRP_QUERY_GROUP_ID */
} GroupQueryInput;

typedef struct {
    int pid; /* pid in grp */
    GroupShareAttr attr; /* process in grp attribute */
} GrpQueryGroupInfo;  /* cmd: GRP_QUERY_GROUP */

typedef struct {
    char groupName[BUFF_GRP_NAME_LEN];  /* grp name */
    GroupShareAttr attr; /* process in grp attribute */
} GrpQueryGroupsOfProcInfo; /* cmd: GRP_QUERY_GROUPS_OF_PROCESS */

typedef struct {
    int groupId; /* grp Id */
} GrpQueryGroupIdInfo; /* cmd: GRP_QUERY_GROUP_ID */

typedef union {
    GrpQueryGroupInfo grpQueryGroupInfo[BUFF_PROC_IN_GRP_MAX_NUM];  /* cmd: GRP_QUERY_GROUP */
    GrpQueryGroupsOfProcInfo grpQueryGroupsOfProcInfo[BUFF_GRP_MAX_NUM]; /* cmd: GRP_QUERY_GROUPS_OF_PROCESS */
    GrpQueryGroupIdInfo grpQueryGroupIdInfo; /* cmd: GRP_QUERY_GROUP_ID */
} GroupQueryOutput;

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
    struct {
        unsigned int blkSize;     /* blk size */
        unsigned int blkNum;    /* blk num, blkSize * blkNum must < 4G Byte */
        unsigned int align;      /* addr align, must be an integer multiple of 2, 2< algn <4k */
        unsigned int hugePageFlag; /* huge page flag */
        int reserve[2]; /* reserved */
    } pubPoolCfg[BUFF_PUB_POOL_CFG_MAX_NUM]; /* max allo 128 cfg */
} PubPoolAttr;
/*lint +e116 +e17*/

enum BuffConfCmdType {
    BUFF_CONF_MBUF_TIMEOUT_CHECK = 0,
    BUFF_CONF_MEMZONE_BUFF_CFG = 1,
    BUFF_CONF_MAX,
};

enum BuffGetCmdType {
    BUFF_GET_MBUF_TIMEOUT_INFO = 0,
    BUFF_GET_MBUF_USE_INFO = 1,
    BUFF_GET_MAX,
};

struct MbufTimeoutCheckPara {
    unsigned int enableFlag;     /* enable: 1; disable: 0 */
    unsigned int maxRecordNum;   /* maximum number timeout mbuf info recored */
    unsigned int timeout;        /* mbuf timeout value,  unit:ms, minimum 10ms, default 1000 ms */
    unsigned int checkPeriod;    /* mbuf check thread work period, uinit:ms, minimum 1000ms, default:1s */
};

struct MbufDataInfo {
    void *mp;
    int owner;
    unsigned int ref;
    unsigned int blkSize;
    unsigned int totalBlkNum;
    unsigned int availableNum;
    unsigned int allocFailCnt;
};

struct MbufDebugInfo {
    unsigned long long timeStamp;
    void *mbuf;
    int usePid;
    int allocPid;
    unsigned int useTime;
    unsigned int round;
    struct MbufDataInfo dataInfo;
    char poolName[BUFF_POOL_NAME_LEN];
    int reserve[BUFF_RESERVE_LEN];
};

struct MbufUseInfo {
    int allocPid;                /* mbuf alloc pid */
    int usePid;                  /* mbuf use pid */
    unsigned int ref;              /* mbuf reference num */
    unsigned int status;           /* mbuf status, 1 means in use, not support other status currently */
    unsigned long long timestamp;  /* mbuf alloc timestamp, cpu tick */
    int reserve[BUFF_RESERVE_LEN]; /* for reserve */
};

struct buf_scale_event {
    int type; /* 0: del, 1: add */
    int grpId; /* share buff group id */
    unsigned long long addr;
    unsigned long long size; /* size is invalid in del */
};

enum halCtlCmdType {
    HAL_CTL_REGISTER_LOG_OUT_HANDLE = 1,
    HAL_CTL_UNREGISTER_LOG_OUT_HANDLE = 2,
    HAL_CTL_CMD_MAX,
};

struct log_out_handle {
    void (*DlogInner)(int moduleId, int level, const char *fmt, ...);
    unsigned int logLevel;
};

/*=========================== Memory Manage ===========================*/
#define DV_MEM_SVM 0x0001
#define DV_MEM_SVM_HOST 0x0002
#define DV_MEM_SVM_DEVICE 0x0004

#define  DEVMM_MAX_MEM_TYPE_VALUE   4       /**< max memory type */

#define  MEM_INFO_TYPE_DDR_SIZE     1       /**< DDR memory type */
#define  MEM_INFO_TYPE_HBM_SIZE     2       /**< HBM memory type */
#define  MEM_INFO_TYPE_DDR_P2P_SIZE 3       /**< DDR P2P memory type */
#define  MEM_INFO_TYPE_HBM_P2P_SIZE 4       /**< HBM P2P memory type */
#define  MEM_INFO_TYPE_ADDR_CHECK   5       /**< check addr */
#define  MEM_INFO_TYPE_MAX          6       /**< max type */

enum DEVMM_MEMCPY2D_TYPE {
    DEVMM_MEMCPY2D_SYNC = 0,
    DEVMM_MEMCPY2D_ASYNC_CONVERT = 1,
    DEVMM_MEMCPY2D_ASYNC_DESTROY = 2,
    DEVMM_MEMCPY2D_TYPE_MAX
};

enum ADVISE_MEM_TYPE {
    ADVISE_PERSISTENT = 0,
    ADVISE_DEV_MEM = 1,
    ADVISE_TYPE_MAX
};

struct DMA_OFFSET_ADDR {
    unsigned long long offset;
    unsigned int devid;
};

struct DMA_PHY_ADDR {
    void *src;           /**< src addr(physical addr) */
    void *dst;           /**< dst addr(physical addr) */
    unsigned int len;    /**< length */
    unsigned char flag;  /**< Flag=0 Non-chain, SRC and DST are physical addresses, can be directly DMA copy operations*/
                         /**< Flag=1 chain, SRC is the address of the dma list and can be used for direct dma copy operations*/
    void *priv;
};

struct DMA_ADDR {
    union {
        struct DMA_PHY_ADDR phyAddr;
        struct DMA_OFFSET_ADDR offsetAddr;
    };
    unsigned int fixed_size; /**< Output: the actual conversion size */
    unsigned int virt_id;    /**< store logic id for destroy addr */
};

struct drvMem2D {
    unsigned long long *dst;        /**< destination memory address */
    unsigned long long dpitch;      /**< pitch of destination memory */
    unsigned long long *src;        /**< source memory address */
    unsigned long long spitch;      /**< pitch of source memory */
    unsigned long long width;       /**< width of matrix transfer */
    unsigned long long height;      /**< height of matrix transfer */
    unsigned long long fixed_size;  /**< Input: already converted size. if fixed_size < width*height,
                                         need to call halMemcpy2D multi times */
    unsigned int direction;         /**< copy direction */
    unsigned int resv1;
    unsigned long long resv2;
};

struct drvMem2DAsync {
    struct drvMem2D copy2dInfo;
    struct DMA_ADDR *dmaAddr;
};

struct MEMCPY2D {
    unsigned int type;      /**< DEVMM_MEMCPY2D_SYNC: memcpy2d sync */
                            /**< DEVMM_MEMCPY2D_ASYNC_CONVERT: memcpy2d async convert */
                            /**< DEVMM_MEMCPY2D_ASYNC_DESTROY: memcpy2d async destroy */
    unsigned int resv;
    union {
        struct drvMem2D copy2d;
        struct drvMem2DAsync copy2dAsync;
    };
};

enum ctrlType {
    CTRL_TYPE_ADDR_MAP = 0,
    CTRL_TYPE_ADDR_UNMAP = 1,
    CTRL_TYPE_SUPPORT_FEATURE = 2,
    CTRL_TYPE_MAX
};

#define CTRL_SUPPORT_NUMA_TS_BIT 0
#define CTRL_SUPPORT_NUMA_TS_MASK (1ul << CTRL_SUPPORT_NUMA_TS_BIT)
#define CTRL_SUPPORT_PCIE_BAR_MEM_BIT 1
#define CTRL_SUPPORT_PCIE_BAR_MEM_MASK (1ul << CTRL_SUPPORT_PCIE_BAR_MEM_BIT)

struct supportFeaturePara {
    unsigned long long support_feature;
    unsigned int devid;
};

enum addrMapType {
    ADDR_MAP_TYPE_L2_BUFF = 0,
    ADDR_MAP_TYPE_REG_C2C_CTRL = 1,
    ADDR_MAP_TYPE_MAX
};

struct AddrMapInPara {
    unsigned int addr_type;
    unsigned int devid;
};

struct AddrMapOutPara {
    unsigned long long ptr;
    unsigned long long len;
};

struct AddrUnmapInPara {
    unsigned int addr_type;
    unsigned int devid;
    unsigned long long ptr;
    unsigned long long len;
};
/*=========================== Memory Manage End =======================*/

/*=============================== TSDRV START =============================*/
#define TSDRV_FLAG_REUSE_CQ (0x1 << 0)
#define TSDRV_FLAG_REUSE_SQ (0x1 << 1)
#define TSDRV_FLAG_THREAD_BIND_IRQ (0x1 << 2)
#define TSRRV_FLAG_SQ_RDONLY    (0x1 << 3)
/*=============================== TSDRV END ===============================*/


#endif
