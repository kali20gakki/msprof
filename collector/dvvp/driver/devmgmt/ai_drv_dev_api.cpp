/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "ai_drv_dev_api.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "driver_plugin.h"

namespace analysis {
namespace dvvp {
namespace driver {
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Plugin;

int DrvGetDevNum(void)
{
    uint32_t numDev = 0;
    drvError_t ret = DriverPlugin::instance()->MsprofDrvGetDevNum(&numDev);
    if (ret != DRV_ERROR_NONE || numDev > DEV_NUM) {
        MSPROF_LOGE("Failed to drvGetDevNum, ret=%d, num=%u", static_cast<int>(ret), numDev);
        MSPROF_CALL_ERROR("EK9999", "Failed to drvGetDevNum, ret=%d, num=%u", static_cast<int>(ret), numDev);
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to drvGetDevNum, numDev=%u", numDev);
    return static_cast<int>(numDev);
}

int DrvGetHostPhyIdByDeviceIndex(int index)
{
    uint32_t phyId = 0;
    drvError_t ret = DriverPlugin::instance()->MsprofDrvGetDevIDByLocalDevID(static_cast<uint32_t>(index), &phyId);
    if (ret != DRV_ERROR_NONE || phyId >= DEV_NUM) {
        MSPROF_LOGW("Failed to get phyId by device index: %d, use device index directly", index);
        phyId = static_cast<uint32_t>(index);
    } else {
        MSPROF_LOGI("Succeeded to get phyId: %u by index: %d", phyId, index);
    }
    return static_cast<int>(phyId);
}

int DrvGetDevIds(int numDevices,
                 std::vector<int> &devIds)
{
    devIds.clear();

    if (numDevices <= 0 || numDevices > DEV_NUM) {
        return PROFILING_FAILED;
    }

    uint32_t devices[DEV_NUM] = { 0 };
    drvError_t ret = DriverPlugin::instance()->MsprofDrvGetDevIDs(devices, (uint32_t)numDevices);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to drvGetDevIDs, ret=%d", static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to drvGetDevIDs, ret=%d", static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to drvGetDevIDs, numDevices=%d", numDevices);
    for (int ii = 0; ii < numDevices; ++ii) {
        devIds.push_back(static_cast<int>(devices[ii]));
    }

    return PROFILING_SUCCESS;
}

int DrvGetPlatformInfo(uint32_t &platformInfo)
{
    drvError_t ret = DriverPlugin::instance()->MsprofDrvGetPlatformInfo(&platformInfo);
    if (ret != DRV_ERROR_NONE) {
        if (ret != MSPROF_HELPER_HOST) {
            MSPROF_LOGE("Failed to drvGetPlatformInfo, ret=%d", static_cast<int>(ret));
            MSPROF_CALL_ERROR("EK9999", "Failed to drvGetPlatformInfo, ret=%d", static_cast<int>(ret));
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

int DrvGetEnvType(uint32_t deviceId, int64_t &envType)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_SYSTEM, INFO_TYPE_ENV, &envType);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetEnvType, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetEnvType, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetEnvType, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetCtrlCpuId(uint32_t deviceId, int64_t &ctrlCpuId)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_CCPU, INFO_TYPE_ID, &ctrlCpuId);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuId, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetCtrlCpuId, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetCtrlCpuId, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetCtrlCpuCoreNum(uint32_t deviceId, int64_t &ctrlCpuCoreNum)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_CCPU, INFO_TYPE_CORE_NUM, &ctrlCpuCoreNum);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuCoreNum, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetCtrlCpuCoreNum, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetCtrlCpuCoreNum, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetCtrlCpuEndianLittle(uint32_t deviceId, int64_t &ctrlCpuEndianLittle)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_CCPU, INFO_TYPE_ENDIAN, &ctrlCpuEndianLittle);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuEndianLittle, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetCtrlCpuEndianLittle, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetCtrlCpuEndianLittle, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetAiCpuCoreNum(uint32_t deviceId, int64_t &aiCpuCoreNum)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_AICPU, INFO_TYPE_CORE_NUM, &aiCpuCoreNum);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetAiCpuCoreNum, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetAiCpuCoreNum, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetAiCpuCoreNum, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetAiCpuCoreId(uint32_t deviceId, int64_t &aiCpuCoreId)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_AICPU, INFO_TYPE_ID, &aiCpuCoreId);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetAiCpuCoreId, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetAiCpuCoreId, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetAiCpuCoreId, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetAiCpuOccupyBitmap(uint32_t deviceId, int64_t &aiCpuOccupyBitmap)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_AICPU, INFO_TYPE_OCCUPY, &aiCpuOccupyBitmap);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetAiCpuOccupyBitmap, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetAiCpuOccupyBitmap, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetAiCpuOccupyBitmap, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetTsCpuCoreNum(uint32_t deviceId, int64_t &tsCpuCoreNum)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_TSCPU, INFO_TYPE_CORE_NUM, &tsCpuCoreNum);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetTsCpuCoreNum, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetTsCpuCoreNum, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetTsCpuCoreNum, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetAiCoreId(uint32_t deviceId, int64_t &aiCoreId)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_AICORE, INFO_TYPE_ID, &aiCoreId);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetAiCoreId, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetAiCoreId, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetAiCoreId, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetAiCoreNum(uint32_t deviceId, int64_t &aiCoreNum)
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_AICORE, INFO_TYPE_CORE_NUM, &aiCoreNum);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetAiCoreNum, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetAiCoreNum, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Succeeded to DrvGetAiCoreNum, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetAivNum(uint32_t deviceId, int64_t &aivNum)
{
    aivNum = 8; // 8: aiv core num have to wait drv ready,just pass 8 at now
    MSPROF_LOGI("Succeeded to DrvGetAivNum, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int DrvGetDeviceTime(uint32_t deviceId, unsigned long long &startMono, unsigned long long &cntvct)
{
    int64_t time = 0;
    drvError_t ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId,
        MODULE_TYPE_SYSTEM, INFO_TYPE_MONOTONIC_RAW, &time);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetDeviceTime startMono, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetDeviceTime startMono, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }
    startMono = static_cast<unsigned long long>(time);

    ret = DriverPlugin::instance()->MsprofHalGetDeviceInfo(deviceId, MODULE_TYPE_SYSTEM, INFO_TYPE_SYS_COUNT, &time);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Failed to DrvGetDeviceTime cntvct, deviceId=%d, ret=%d", deviceId, static_cast<int>(ret));
        MSPROF_CALL_ERROR("EK9999", "Failed to DrvGetDeviceTime cntvct, deviceId=%d, ret=%d",
            deviceId, static_cast<int>(ret));
        return PROFILING_FAILED;
    }
    cntvct = static_cast<unsigned long long>(time);

    MSPROF_LOGI("Succeeded to DrvGetDeviceTime, devId=%d, startMono=%llu ns, cntvct=%llu",
        deviceId, startMono, cntvct);
    return PROFILING_SUCCESS;
}

std::string DrvGetDevIdsStr()
{
    int devNum = DrvGetDevNum();
    std::vector<int> devIds;
    int ret = DrvGetDevIds(devNum, devIds);
    if (ret != PROFILING_SUCCESS || devIds.size() == 0) {
        return "";
    } else {
        std::stringstream result;
        for (auto devId : devIds) {
            result << devId << ",";
        }
        return result.str().substr(0, result.str().size() - 1);
    }
}

bool DrvCheckIfHelperHost()
{
    uint32_t numDev = 0;
    drvError_t ret = DriverPlugin::instance()->MsprofDrvGetDevNum(&numDev);
    if (ret == MSPROF_HELPER_HOST) {
        MSPROF_LOGI("Msprof work in Helper!");
        return true;
    }
    return false;
}
}  // namespace driver
}  // namespace dvvp
}  // namespace analysis
