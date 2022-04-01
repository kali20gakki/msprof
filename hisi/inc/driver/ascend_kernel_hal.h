/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description:
 * Author: huawei
 * Create: 2019-10-15
 */
#ifndef __ASCEND_KERNEL_HAL_H__
#define __ASCEND_KERNEL_HAL_H__

#include "ascend_hal_define.h"
#define LPM3_IDLE_CMD 9
#define IPC_CMD_DVPP_MIN    241
#define IPC_CMD_DVPP_MAX    249
#define IPC_CMD_RETR_MIN    250
#define IPC_CMD_RETR_MAX    255

#ifndef u64
typedef unsigned long long u64;
#endif

#ifndef u32
typedef unsigned int u32;
#endif

#ifndef u16
typedef unsigned short u16;
#endif

#ifndef u8
typedef unsigned char u8;
#endif

/**
* @ingroup driver-stub
* @brief interface for dvpp notify LP to adjust ddr frequency
* @param [in]  unsigned int dev_id: device ID
* @param [in]  unsigned char cmd_type0: message target
* @param [in]  unsigned char cmd_type1: module target
* @param [in]  unsigned char *data: message
* @param [in]  unsigned int data_len: message length
* @return   0 for success, others for fail
*/
int hal_kernel_send_ipc_to_lp_async(unsigned int devid, unsigned char cmd_type0,
    unsigned char cmd_type1, unsigned char *data, unsigned int data_len);

/**
* @ingroup driver-stub
* @brief  interface for reporting the in-position information of the optical module
* @param [in]  unsigned int qsfp_index Optical port index
* @param [out] unsigned int *val presence information
* @return   0 for success, others for fail
*/
int drv_cpld_qsfp_present_query(unsigned int qsfp_index, unsigned int *val);
/**
* @ingroup driver-stub
* @brief  obtain the MAC address of the user configuration area.
* @param [in]  unsigned int dev_id Device ID
* @param [in]  unsigned char *buf Buffer for storing information
* @param [in]  unsigned int buf_size Size of the buffer for storing information
* @param [out] unsigned int *info_size Data length of the returned MAC information
* @return   0 for success, others for fail
*/
int devdrv_config_get_mac_info(unsigned int dev_id,
                               unsigned char *buf,
                               unsigned int buf_size,
                               unsigned int *info_size);
/**
* @ingroup driver-stub
* @brief   interface for obtaining the information about the user configuration area
* @param [in]  unsigned int dev_id Device ID
* @param [in]  const char *name: configuration item name
* @param [in]  unsigned char *buf Buffer for storing information
* @param [out] unsigned int *buf_size Obtain the information length
* @return   0 for success, others for fail
*/

int devdrv_get_user_config(unsigned int dev_id, const char *name, unsigned char *buf, unsigned int *buf_size);
/**
* @ingroup driver-stub
* @brief   This interface is used to set the information about the user configuration area.
*          Currently, this interface can be invoked only by the DMP process. In other cases, the permission fails to be returned
* @param [in]  unsigned int dev_id Device ID
* @param [in]  const char *name: configuration item name
* @param [in]  unsigned char *buf Buffer for storing information
* @param [out] unsigned int *buf_size Obtain the information length
*              Due to the storage space limit, when the configuration area information is set,
*              The length of the setting information needs to be limited.
*              The current length range is as follows: For cloud-related forms,
*              the maximum value of buf_size is 0x8000, that is, 32 KB.
*              For mini-related forms, the maximum value of buf_size is 0x800, that is, 2 KB.
*              If the length is greater than the value of this parameter, a message is displayed,
*              indicating that the setting fails.
* @return   0 for success, others for fail
*/
int devdrv_set_user_config(unsigned int dev_id, const char *name, unsigned char *buf, unsigned int buf_size);
/**
* @ingroup driver-stub
* @brief   This interface is used to clear the configuration items in the user configuration area.
*          Currently, this interface can be invoked only by the DMP process.
*          In other cases, a permission failure is returned.
* @param [in]  unsigned int dev_id Device ID
* @param [in]  const char *name: configuration item name
* @param [in]  unsigned char *buf Buffer for storing information
* @return   0 Success, others for fail
*/
int devdrv_clear_user_config(unsigned int devid, const char *name);

#define BUFF_SP_NORMAL 0
#define BUFF_SP_HUGEPAGE_PRIOR (1 << 0)
#define BUFF_SP_HUGEPAGE_ONLY (1 << 1)
#define BUFF_SP_DVPP (1 << 2)

/**
* @ingroup driver-stub
* @brief   This interface is used to switch chip id to numa id.
* @param [in]  unsigned int device_id: device id
* @param [in]  unsigned int type: memory type
* @return   success return numa id, fail return fail code
*/
int hal_kernel_numa_get_nid(unsigned int device_id, unsigned int type);

struct buff_proc_free {
    int pid;
} ;

typedef void (*buff_free_ops)(struct buff_proc_free *arg);

/**
* @ingroup driver
* @brief   This interface is used to register func, which call before buff drv free share pool mem
* @param [in]  func: need to call before recycle
* @param [out]  module_idx: to unreg func module idx
* @return   0 Success, others for fail
*/
int hal_kernel_buff_register_proc_free_notifier(buff_free_ops func, unsigned int *module_idx);

/**
* @ingroup driver-stub
* @brief   This interface is used to unregister the func reg by hal_kernel_buff_register_proc_free_notifier
* @param [in] module_idx: module idx, which is returned in reg func
* @return   0 Success, others for fail
*/
int hal_kernel_buff_unregister_proc_free_notifier(unsigned int module_idx);

enum sensorhub_pps_source_type {
    PPS_FROM_XGMAC = 0x0,
    PPS_FROM_CHIP  = 0x1,
};

enum sensorhub_ssu_ctrl_type {
    SSU_SW      = 0x0,
    PATH_SW     = 0x1,
    SSU_BUT,
};

enum sensorhub_intr_ctrl_type {
    NORMAL1_MASK_CFG = 0x0,
    NORMAL2_MASK_CFG = 0x1,
    ERROR1_MASK_CFG  = 0x2,
    ERROR2_MASK_CFG  = 0x3,
    NORMAL_INT_CLR   = 0x4,
    INT_CTR_BUT,
};

enum sensorhub_fsin_cfg_type {
    PPS_THRESHOLD       = 0x0,
    PPS_PW              = 0x1,
    PPS_BIAS_THRESH     = 0x2,
    EXPO_INIT_PRE       = 0x3,
    FSIN_FRAME_RATE     = 0x4,
    FSIN_CLR_TS         = 0x5,
    FSIN_CAM_MAP        = 0x6,
    FSIN_CTRL_BUT,
};

enum sensorhub_module_type {
    SSU_CTRL_MODULE         = 0x0,
    INTERRUPT_CTRL_MODULE   = 0x1,
    FSIN_CTRL_MODULE        = 0x2,
    IMU_CTRL_MODULE         = 0x3,
    PERI_SUBCTRL_MODULE     = 0x4,
    GPIO_CTRL_MODULE        = 0x5,
    FAULT_CHECK_CFG         = 0x6,
    MODULE_BUT,
};

enum sensorhub_fault_check_type {
    EXPO_CHECK_PRE_THRESHOLD    = 0x0,
    EXPO_CHECK_POST_THRESHOLD   = 0x1,
};

struct sensorhub_sub_cmd_value_stru {
    unsigned int value;
};

struct sensorhub_fsync_info_stru {
    unsigned int value;
    unsigned int sub_mod_id;
};

struct sensorhub_sub_cmd_info_stru {
    unsigned int value_0;
    unsigned int value_1;
};

struct sensorhub_msg_head_stru {
    enum sensorhub_module_type cmd;
    unsigned int sub_cmd;  /* fsin_cfg_type or ssu_ctrl_type or intr_ctrl_type */
    unsigned int len;
    void *param; /* sub_cmd_value or fsync_info or sub_cmd_info */
};

/**
* @ingroup driver-stub
* @brief   This interface is used to configure sensorhub module..
* @param [in]  hal_kernel_buff_notify_handle handle: notify handle
* @return   0 Success, others for fail
*/
int hal_kernel_sensorhub_set_module(struct sensorhub_msg_head_stru *para_cfg);

/**************************** event sched table intf start ***********************************/
#define ESCHED_MAX_KEY_LEN 128
#define ESCHED_MAX_ENTRY_NUM (1024 * 1024)

#define ESCHED_MAX_CQE_SIZE ESCHED_MAX_KEY_LEN
#define ESCHED_CQE_SIZE_ALIGN 4
#define ESCHED_MAX_CQ_DEPTH (64 * 1024)

#define ESCHED_RAW_DATA_ADDR_TYPE_PHY 0
#define ESCHED_RAW_DATA_ADDR_TYPE_VIR 1

enum esched_cq_type {
    ESCHED_CQ_TYPE_PHASE,
    ESCHED_CQ_TYPE_PTR,
    ESCHED_CQ_TYPE_MAX
};

struct esched_cq_phase {
    u8 *mask; /* len is cqe_size */
    u8 init_value;
    u8 ring_step;
    u64 head_addr;
};

struct esched_cq_ptr {
    u64 head_addr; /* consumer: software update head reg notice hardware */
    u64 tail_addr; /* producer: hardware update tail reg notice software */
};

struct esched_raw_data_cq {
    u32 type; /* ESCHED_CQ_TYPE_: phase bit check, tail reg check */
    u32 addr_type; /* 0: phy, 1: vir(stream id, substream id) */
    u16 stream_id;
    u16 substream_id;
    u64 cq_addr;
    u32 cq_depth;
    u32 cqe_size;
    union {
        struct esched_cq_phase cq_phase;
        struct esched_cq_ptr cq_ptr;
    };
};

enum esched_raw_data_type {
    RAW_DATA_TYPE_CQ,
    RAW_DATA_TYPE_MAX,
};

struct esched_table_raw_data {
    u32 raw_data_type;
    u32 max_entry_num;
    char *name; /* for debug, table name */
    u8 *raw_data_key_mask; /* len is cqe_size */
    u32 raw_data_key_mask_len;
    union {
        struct esched_raw_data_cq cq_data;
    };
};

struct esched_hw_info {
    char *name; /* for debug, hw name */
    int irq;
};

int hal_esched_alloc_table(u32 dev_id, struct esched_table_raw_data *raw_data, u32 *table_id);
int hal_esched_free_table(u32 dev_id, u32 table_id);
int hal_esched_hw_bind_table(u32 dev_id, struct esched_hw_info *hw_info, u32 table_id);
int hal_esched_hw_unbind_table(u32 dev_id, struct esched_hw_info *hw_info);
/**************************** event sched table intf end ***********************************/

#endif
