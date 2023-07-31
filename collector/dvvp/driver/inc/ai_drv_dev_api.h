/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_AI_DRV_DEV_API_H
#define ANALYSIS_DVVP_DEVICE_AI_DRV_DEV_API_H

#include <vector>
#include <map>
#include <string>
#include "ascend_hal.h"
#include "message/prof_params.h"

#define MSPROF_HELPER_HOST DRV_ERROR_NOT_SUPPORT
namespace analysis {
namespace dvvp {
namespace driver {
constexpr char NOT_SUPPORT_FREQUENCY[] = "";
constexpr uint32_t FREQUENCY_KHZ_TO_MHZ = 1000; // KHz to MHz
constexpr uint32_t SUPPORT_OSC_FREQ_API_VERSION = 0x071905;
int DrvGetDevNum();
int DrvGetHostPhyIdByDeviceIndex(int index);
int DrvGetDevIds(int numDevices, std::vector<int> &devIds);
int DrvGetEnvType(uint32_t deviceId, int64_t &envType);
int DrvGetCtrlCpuId(uint32_t deviceId, int64_t &ctrlCpuId);
int DrvGetCtrlCpuCoreNum(uint32_t deviceId, int64_t &ctrlCpuCoreNum);
int DrvGetCtrlCpuEndianLittle(uint32_t deviceId, int64_t &ctrlCpuEndianLittle);
int DrvGetAiCpuCoreNum(uint32_t deviceId, int64_t &aiCpuCoreNum);
int DrvGetAiCpuCoreId(uint32_t deviceId, int64_t &aiCpuCoreId);
int DrvGetAiCpuOccupyBitmap(uint32_t deviceId, int64_t &aiCpuOccupyBitmap);
int DrvGetTsCpuCoreNum(uint32_t deviceId, int64_t &tsCpuCoreNum);
int DrvGetAiCoreId(uint32_t deviceId, int64_t &aiCoreId);
int DrvGetAiCoreNum(uint32_t deviceId, int64_t &aiCoreNum);
int DrvGetAivNum(uint32_t deviceId, int64_t &aivNum);
int DrvGetPlatformInfo(uint32_t &platformInfo);
int DrvGetDeviceTime(uint32_t deviceId, unsigned long long &startMono, unsigned long long &cntvct);
std::string DrvGetDevIdsStr();
bool DrvCheckIfHelperHost();
bool DrvGetHostFreq(std::string &freq);
bool DrvGetDeviceFreq(uint32_t deviceId, std::string &freq);
uint32_t DrvGetApiVersion();
}  // namespace driver
}  // namespace dvvp
}  // namespace analysis

#endif