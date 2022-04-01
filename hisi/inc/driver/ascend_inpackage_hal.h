/**
 * @file ascend_inpackage_hal.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description:
 * Author: huawei
 * Create: 2020-06-13
 * @brief driver interface.
 * @version 1.0
 *
 */

#ifndef __ASCEND_INPACKAGE_HAL_H__
#define __ASCEND_INPACKAGE_HAL_H__

#include <stdint.h>
#include "ascend_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup driver
 * @brief module definition of drv
 */
enum devdrv_module_type {
    HAL_MODULE_TYPE_VNIC,
    HAL_MODULE_TYPE_HDC,
    HAL_MODULE_TYPE_DEVMM,
    HAL_MODULE_TYPE_DEV_MANAGER,
    HAL_MODULE_TYPE_DMP,
    HAL_MODULE_TYPE_FAULT,
    HAL_MODULE_TYPE_UPGRADE,
    HAL_MODULE_TYPE_PROCESS_MON,
    HAL_MODULE_TYPE_LOG,
    HAL_MODULE_TYPE_PROF,
    HAL_MODULE_TYPE_DVPP,
    HAL_MODULE_TYPE_PCIE,
    HAL_MODULE_TYPE_IPC,
    HAL_MODULE_TYPE_TS_DRIVER,
    HAL_MODULE_TYPE_SAFETY_ISLAND,
    HAL_MODULE_TYPE_BSP,
    HAL_MODULE_TYPE_USB,
    HAL_MODULE_TYPE_NET,
    HAL_MODULE_TYPE_EVENT_SCHEDULE,
    HAL_MODULE_TYPE_BUF_MANAGER,
    HAL_MODULE_TYPE_QUEUE_MANAGER,
    HAL_MODULE_TYPE_COMMON,
    HAL_MODULE_TYPE_MAX,
};

enum tagAicpufwPlat {
    AICPUFW_ONLINE_PLAT = 0,
    AICPUFW_OFFLINE_PLAT,
    AICPUFW_MAX_PLAT,
};

struct drvBindHostpidInfo {
    pid_t host_pid;
    uint32_t vfid;
    uint32_t chip_id;
    int32_t mode;
    enum devdrv_process_type cp_type;
    uint32_t len;
    char sign[PROCESS_SIGN_LENGTH];
};

/* verify type for select, include soc and cms */
typedef enum {
    VERIFY_TYPE_SOC = 0,
    VERIFY_TYPE_CMS,
    VERIFY_TYPE_MAX
} HAL_VERIFY_TYPE;//lint !e116 !e17

/* image id for cms verification */
typedef enum {
    ITEE_IMG_ID = 0,
    DTB_IMG_ID,
    ZIMAGE_ID,
    FS_IMG_ID,
    SD_PEK_DTB_IMG_ID,
    SD_IMG_ID,
    PEK_IMG_ID,
    DP_IMG_ID,
    ROOTFS_IMG_ID,
    APP_IMG_ID,
    DTB_DP_PEK_IMG_ID,
    DTB_SD_PEK_IMG_ID,
    DP_PEK_IMG_ID,
    SD_PEK_IMG_ID,
    DP_CORE_IMG_ID,
    IMAGE_ID_MAX
} HAL_IMG_ID;//lint !e116 !e17

typedef enum {
    HAL_VMNGD_EVENT_CREATE_VF = 100,
    HAL_VMNGD_EVENT_DESTROY_VF,
    HAL_VMNGD_EVENT_MAX
}HAL_VMNGD_SUBEVENT_ID;//lint !e116 !e17

struct drvVmngdEventMsg {
    uint32_t dev_id;
    uint32_t vfid;
    uint32_t core_num;
    uint32_t total_core_num;
};

#define HAL_VERIFY_MODE_COVER_WITH_HEAD_OFF (1<<0) /* cover file with head off */

/**
* @ingroup driver
* @brief Bind Device custom-process to aicpu-process
* @attention Must have a paired hostpid and a paired aicpupid
* @param [in] info  See struct drvBindHostpidInfo
              host_pid: hops pid
              chip_id:  chip id
              mode:     mode, online:0, offline:1
              cp_type:  type of custom-process
              len:      lenth of sign
              sign:     sign of hostpid
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_XXX : bind fail
*/
DLLEXPORT drvError_t drvBindHostPid(struct drvBindHostpidInfo info);
/**
* @ingroup driver
* @brief Unbind Device custom-process to aicpu-process
* @attention The hostpid and aicpuid must be bound through the drvBindHostPid interface.
* @param [in] info  See struct drvBindHostpidInfo
              host_pid: hops pid
              chip_id:  chip id
              mode:     mode, online:0, offline:1
              cp_type:  type of custom-process
              len:      lenth of sign
              sign:     sign of hostpid
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_XXX : unbind fail
*/
DLLEXPORT drvError_t drvUnbindHostPid(struct drvBindHostpidInfo info);
/**
* @ingroup driver
* @brief Initialize Device Memory
* @attention Must have a paired hostpid
* @param [in] hostpid Paired host side pid
* @param [in] vfid Paired device virtual function id
* @param [in] dev_id device id
* @return DRV_ERROR_NONE : success
* @return DV_ERROR_XXX : init fail
*/
DV_ONLINE DVresult drvMemInitSvmDevice(int hostpid, unsigned int vfid, unsigned int dev_id);
/**
* @ingroup driver
* @brief Bind Device custom-process to aicpu-process
* @attention Must have a paired hostpid and a paired aicpupid
* @param [in] hostpid Paired host side pid
* @param [in] aicpupid Paired aicpu process pid
* @param [in] vfid Paired device virtual function id
* @param [in] dev_id device id
* @return DRV_ERROR_NONE : success
* @return DRV_ERROR_XXX : bind fail
*/
DV_ONLINE DVresult halMemBindSibling(int hostPid, int aicpuPid, unsigned int vfid, unsigned int dev_id);
/**
 * @ingroup driver
 * @brief get borad id
 * @attention This function is only can be called by components in driver of device,
 *  if the components is not in driver of device, don't use this function.
 * @param [in] dev_id device id
 * @param [out] board_id  board id number
 * @return   0   success
 * @return   -1  fail
 */
int devdrv_get_board_id(unsigned int dev_id, unsigned int *board_id);
/**
 * @ingroup driver
 * @brief get vnic ip
 * @attention This function is only can be called by components in driver of device,
 *  if the components is not in driver of device, don't use this function.
 * @param [in] dev_id phy_id in host
 * @param [out] ip_addr vnic ip address
 * @return   0   success
 * @return   -1  fail
 */
int devdrv_get_vnic_ip(unsigned int dev_id, unsigned int *ip_addr);
/**
 * @ingroup driver
 * @brief get eth_id by device index
 * @attention This function is only can be called by components in driver of device,
 *  if the components is not in driver of device, don't use this function.
 * @param [in] dev_id device id
 * @param [in] port_id port id in device
 * @param [out] eth_id ethnet id in device
 * @return   0   success
 * @return   -1  fail
 */
int drvDeviceGetEthIdByIndex(uint32_t dev_id, uint32_t port_id, uint32_t *eth_id);

/**
 * @ingroup driver
 * @brief verify image, including soc and cms verify operation
 * @attention This function is only can be called by components in driver of device,
 *  if the components is not in driver of device, don't use this function.
 * @param [in] verify_type  choose soc or cms
 * @param [in] image_id  image id, only use in cms verification.
  * @param [in] img_path  verify image path.
 * @param [in] mode choose cover file soc head or not.
 * @return  0  success, return others fail
 */
int halVerifyImg(HAL_VERIFY_TYPE verify_type, HAL_IMG_ID image_id, const char *img_path, int mode);

/**
 * @ingroup driver
 * @brief only for tsd register virtmng client
 * @attention This function is only can be called by components in driver of device,
 *  if the components is not in driver of device, don't use this function.
 * @param [in] verify_type  choose soc or cms
 * @param [in] image_id  image id, only use in cms verification.
  * @param [in] img_path  verify image path.
 * @param [in] mode choose cover file soc head or not.
 * @return  0  success, return others fail
 */
int halRegisterVmngClient(void);

/**
* @ingroup driver
* @brief record wait event or notify
* @attention only called by cp process
* @param [in] devId  device id
* @param [in] record_type  event or notify
* @param [in] record_Id
* @return 0  success, return others fail
*/
int tsDevRecord(unsigned int devId, unsigned int tsId, unsigned int record_type, unsigned int record_Id);

/**
* @ingroup driver
* @brief record wait event or notify
* @attention only called by cp process
* @param [in] devId  device id
* @param [out] vf_max_num  maximum number of segmentation
* @return 0  success, return others fail
*/
int halGetDeviceVfMax(unsigned int devId, unsigned int *vf_max_num);

/**
* @ingroup driver
* @brief record wait event or notify
* @attention only called by cp process
* @param [in] devId  device id
* @param [out] vf_list  list of vfid
* @param [in] list_len  length of vf_list
* @param [out] vf_num  number of vf
* @return 0  success, return others fail
*/
int halGetDeviceVfList(unsigned int devId, unsigned int *vf_list, unsigned int list_len, unsigned int *vf_num);

#ifdef __cplusplus
}
#endif
#endif

