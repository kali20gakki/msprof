/**
 * @file adx_hdc_device.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_hdc_device.h"
#include <memory>
#include "log/adx_log.h"
#include "ascend_hal.h"
#include "adx_dsmi.h"
#include "adx_comm_opt.h"
#include "adx_comm_opt_manager.h"
using AdxDrvStateInfoPt = devdrv_state_info_t *;
namespace Adx {
void AdxHdcDevice::GetAllEnableDevices(std::vector<std::string> &devices)
{
    devices.clear();
    std::lock_guard<std::mutex> lk(this->deviceNotifyMtx_);
    if (AdxHdcDevice::NoDevice()) {
        uint32_t devLogIds[DEVICE_NUM_MAX] = {0};
        uint32_t nums = 0;
        if (IdeGetPhyDevList(&nums, devLogIds, DEVICE_NUM_MAX) == IDE_DAEMON_ERROR) {
            return;
        }

        for (uint32_t i = 0; i < nums && i < DEVICE_NUM_MAX; i++) {
            AdxHdcDevice::InitDevice(std::to_string(devLogIds[i]));
        }
    }
    AdxHdcDevice::GetEnableDevices(devices);
}
}

/**
 * @brief device startup notify callback function.
 * @param [in]  num:    number of devices ids in devIds.
 * @param [in]  devIds: device id list.
 *
 * @return DRVDEV_CALL_BACK_SUCCESS on success otherwise DRVDEV_CALL_BACK_FAILED
 */
extern "C" int32_t AdxDevStartupNotifyCb(uint32_t num, IdeU32Pt devIds)
{
    IDE_CTRL_VALUE_FAILED(devIds != nullptr, return DRVDEV_CALL_BACK_FAILED, "Startup Notify input error");

    if (num > DEVICE_NUM_MAX) {
        IDE_LOGW("device startup notify %u devices extra the limit %u.", num, DEVICE_NUM_MAX);
    }

    // [attention] callback return devIds may not useful device, should check by drvGetDevNum
    for (uint32_t i = 0; i < num && i < DEVICE_NUM_MAX; i++) {
        if (devIds[i] < DEVICE_NUM_MAX) {
            IDE_LOGI("device %u notify startup state.", devIds[i]);
            std::shared_ptr<Adx::AdxDevice> device =
                Adx::AdxCommOptManager::Instance().GetDevice(Adx::OptType::COMM_HDC);
            IDE_CTRL_VALUE_WARN(device != nullptr, return DRVDEV_CALL_BACK_FAILED,
                "hdc get device failed");
            device->EnableNotify(std::to_string(devIds[i]));
        } else {
            IDE_LOGW("device startup notify skip bad device id %u.", devIds[i]);
        }
    }

    return DRVDEV_CALL_BACK_SUCCESS;
}

/**
 * @brief device state change notify callback function.
 * @param [in]  stat:   devStatInfo struct.
 *
 * @return DRVDEV_CALL_BACK_SUCCESS on success otherwise DRVDEV_CALL_BACK_FAILED
 */
extern "C" int32_t AdxDevPwStateNotifyCb(AdxDrvStateInfoPt stat)
{
    uint32_t devId;
    IDE_CTRL_VALUE_FAILED(stat != nullptr, return DRVDEV_CALL_BACK_FAILED, "cb input error");
    std::shared_ptr<Adx::AdxDevice> device = nullptr;
    devId = (uint32_t)stat->devId;
    if (devId < DEVICE_NUM_MAX) {
        IDE_LOGI("device %u notify state change %u.", devId, stat->state);
        switch (stat->state) {
            case GO_TO_SO:
            case GO_TO_ENABLE_DEV:
                device  = Adx::AdxCommOptManager::Instance().GetDevice(Adx::OptType::COMM_HDC);
                IDE_CTRL_VALUE_FAILED(device != nullptr, return DRVDEV_CALL_BACK_FAILED,
                    "Startup Notify input error");
                device->EnableNotify(std::to_string(devId));
                break;
            case GO_TO_S3:
            case GO_TO_S4:
            case GO_TO_SUSPEND:
            case GO_TO_DISABLE_DEV:
                device = Adx::AdxCommOptManager::Instance().GetDevice(Adx::OptType::COMM_HDC);
                IDE_CTRL_VALUE_FAILED(device != nullptr, return DRVDEV_CALL_BACK_FAILED,
                    "hdc get device failed");
                device->DisableNotify(std::to_string(devId));
                break;

            default:
                IDE_LOGI("unprocess power state(%d).", stat->state);
                break;
        }
    } else {
        IDE_LOGW("device state notify skip bad device id %u.", devId);
    }

    return DRVDEV_CALL_BACK_SUCCESS;
}
