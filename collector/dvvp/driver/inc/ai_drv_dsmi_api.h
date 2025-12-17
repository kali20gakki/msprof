/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
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
