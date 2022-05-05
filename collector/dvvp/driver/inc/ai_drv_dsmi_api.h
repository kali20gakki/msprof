/**
* @file ai_drv_dsmi_api.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_DEVICE_AI_DRV_DSMI_API_H
#define ANALYSIS_DVVP_DEVICE_AI_DRV_DSMI_API_H

#include <vector>
#include <map>
#include <string>
#include "ascend_hal.h"
#include "message/prof_params.h"


namespace Analysis {
namespace Dvvp {
namespace Driver {
int DrvGetAicoreInfo(int deviceId, int64_t &freq);
std::string DrvGeAicFrq(int deviceId);
}  // namespace driver
}  // namespace dvvp
}  // namespace analysis

#endif
